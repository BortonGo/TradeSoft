#include "strategyrunner.h"
#include <utility>
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logStrategy, "tradesoft.strategy")
Q_LOGGING_CATEGORY(logStrategyTicks, "tradesoft.strategy.ticks")
Q_LOGGING_CATEGORY(logLatency, "tradesoft.latency")

StrategyRunner::StrategyRunner(MarketDataService* mds, QObject* parent) : QObject(parent), mds_(mds) {
    Q_ASSERT(mds_);
    connect(mds_, &MarketDataService::signal_seriesLoaded, this, &StrategyRunner::onSeriesLoaded);
    connect(mds_, &MarketDataService::signal_candleClosed,  this, &StrategyRunner::onCandleClosed);
    connect(mds_, &MarketDataService::signal_candleUpdated, this, &StrategyRunner::onCandleUpdated);
}

void StrategyRunner::setStrategy(std::unique_ptr<IStrategy> s) {
    strategy_ = std::move(s);
}

QString formatLatencyStats(const LatencyStatsSnapshot& snap) {
    auto us = [](std::uint64_t ns) { return ns / 1000.0; };
    if (snap.count == 0) return "count=0";
    return "count=" + QString::number(snap.count) +
           " min=" + QString::number(us(snap.minNs)) +
           "us mean=" + QString::number(us(snap.meanNs)) +
           "us max=" + QString::number(us(snap.maxNs)) +
           "us p50=" + QString::number(us(snap.p50Ns)) +
           "us p95=" + QString::number(us(snap.p95Ns)) +
           "us p99=" + QString::number(us(snap.p99Ns)) + "us";
}

void StrategyRunner::start(const QString& symbolId, Timeframe tf) {
    if (running_) return;

    if (!strategy_) {
        qCWarning(logStrategy) << "Can't start: strategy == nullptr";
        return;
    }
    if (!risk_ || !exec_ || !journal_) {
        qCWarning(logStrategy) << "Can't start: pipeline services not set (risk/exec/journal)";
        return;
    }

    latencyCollector_.clear();

    ctx_.symbolId = symbolId;
    ctx_.tf = tf;
    ctx_.series.reset();

    running_ = true;

    mds_->stopRealTime();
    mds_->loadHistory(symbolId, tf);
    mds_->startRealTime();

    emit signal_started();
    qCInfo(logStrategy) << "Start symbol =" << symbolId << ", tf =" << toUiString(tf);
}

void StrategyRunner::stop() {
    if (!running_) return;
    running_ = false;

    mds_->stopRealTime();
    emit signal_stopped();

    // optional: print report
    if (journal_) {
        const TradeReport r = journal_->report();
        const double winRate = (r.closedTrades > 0) ? (100.0 * r.winTrades / r.closedTrades) : 0.0;
        qCInfo(logStrategy) << "Report netPnl=" << r.netPnl
                            << " equity=" << r.equity
                            << " fees=" << r.fees
                            << " trades=" << r.closedTrades
                            << " winrate=" << winRate
                            << " maxDD=" << r.maxDrawdown;
    }

    qCInfo(logLatency) << "Latency tick-to-strategy: " << formatLatencyStats(latencyCollector_.snapshot().tickToStrategy);
    qCInfo(logLatency) << "Latency strategy-to-order: " << formatLatencyStats(latencyCollector_.snapshot().strategyToOrder);
    qCInfo(logLatency) << "Latency order-to-fill: " << formatLatencyStats(latencyCollector_.snapshot().orderToFill);

    qCInfo(logStrategy) << "Stop";
}

void StrategyRunner::onSeriesLoaded(std::shared_ptr<CandleSeries> series) {
    if (!running_ || !strategy_) return;
    ctx_.series = series;

    strategy_->onStart(ctx_);
    qCInfo(logStrategy) << "Series loaded, candles =" << series->getCount();
}

void StrategyRunner::onCandleClosed(Candle c)
{
    if (!running_ || !strategy_ || !ctx_.series) return;

    MarketEvent marketEvent = makeMarketEvent(MarketEventType::CandleClosed, c);

    handleMarketEvent(marketEvent);

    if (journal_) {
        journal_->onPriceUpdate(ctx_.symbolId, c.close_, riskSettings_.feePct);
    }
}

void StrategyRunner::onCandleUpdated(Candle c)
{
    if (!running_ || !strategy_ || !ctx_.series) return;

    MarketEvent marketEvent = makeMarketEvent(MarketEventType::CandleUpdated, c);

    handleMarketEvent(marketEvent);

    if (journal_) {
        journal_->onPriceUpdate(ctx_.symbolId, c.close_, riskSettings_.feePct);
    }
}

void StrategyRunner::handleSignal(SignalEvent& signalEvent, const Candle& closed)
{
    if (!risk_ || !exec_ || !journal_) return;

    const StrategySignal& s = signalEvent.signal;
    LatencyTimestamp& latency = signalEvent.latency;

    std::optional<OrderEvent> orderEvent = buildOrderEvent(signalEvent, closed, latency);
    if (!orderEvent.has_value()) return;

    FillEvent fillEvent = executeOrderEvent(orderEvent.value(), closed.close_);
    handleFillEvent(fillEvent);

    signalEvent.latency = fillEvent.latency;
}

MarketEvent  StrategyRunner::makeMarketEvent(MarketEventType type, const Candle& candle) const
{
    MarketEvent event;
    event.candle = candle;
    event.type = type;
    event.symbolId = ctx_.symbolId;
    event.timeframe = ctx_.tf;
    return event;
}

void StrategyRunner::handleMarketEvent(MarketEvent& event)
{
    std::vector<StrategySignal> signal;

    if (event.type == MarketEventType::CandleClosed)  {
        event.latency.receivedNs = LatencyClock::nowNs();
        signal = strategy_->onCandleClosed(ctx_, event.candle);
        event.latency.strategyDoneNs = LatencyClock::nowNs();
    } else if (event.type == MarketEventType::CandleUpdated) {
        event.latency.receivedNs = LatencyClock::nowNs();
        signal = strategy_->onCandleUpdated(ctx_, event.candle);
        event.latency.strategyDoneNs = LatencyClock::nowNs();
    } else {
        return; // заглушка
    }

    for (const auto& s : signal) {
        if (s.type == StrategySignalType::None) continue;

        SignalEvent signalEvent;
        signalEvent.signal = s;
        signalEvent.latency = event.latency;

        qCDebug(logStrategyTicks) << "Update signal type=" << static_cast<int>(s.type)
                                  << " symbol=" << s.symbolId
                                  << " tf=" << toUiString(s.tf)
                                  << " reason=" << s.reason;

        handleSignal(signalEvent, event.candle);
        event.latency = signalEvent.latency;
        emit signal_signalGenerated(signalEvent.signal);
    }

    latencyCollector_.recordTickToStrategy(event.latency);
}

std::optional<OrderEvent> StrategyRunner::buildOrderEvent(const SignalEvent& signalEvent,
                                                          const Candle& candle,
                                                          LatencyTimestamp& latency)
{
    const double markPrice = candle.close_;
    if (markPrice <= 0.0) return std::nullopt;

    const bool hasOpen = journal_->hasOpen(signalEvent.signal.symbolId);
    const TradeSide openSide = hasOpen ? journal_->openSide(signalEvent.signal.symbolId) : TradeSide::Buy;

    // Build order from signal
    Order o = risk_->buildOrder(
        signalEvent.signal,
        riskSettings_,
        markPrice,
        journal_->equity(),
        hasOpen,
        openSide
        );

    latency.orderCreatedNs = LatencyClock::nowNs();

    // RiskManager can return empty order
    if (o.symbol.isEmpty()) return std::nullopt;

    // For Exit: close full qty
    if (o.reduceOnly) {
        o.qty = journal_->openQty(o.symbol);
    }

    if (o.qty <= 0.0) return std::nullopt;

    OrderEvent orderEvent;
    orderEvent.type = OrderEventType::Created;
    orderEvent.order = o;
    orderEvent.latency = latency;

    return orderEvent;
}

FillEvent StrategyRunner::executeOrderEvent(const OrderEvent& orderEvent, double markPrice)
{
    FillEvent fillEvent;
    fillEvent.fill = exec_->executeMarket(orderEvent.order, markPrice, riskSettings_);
    fillEvent.latency = orderEvent.latency;
    return fillEvent;
}

void StrategyRunner::handleFillEvent(FillEvent& fillEvent)
{
    if (!fillEvent.fill.reduceOnly && cfg_.fixedExit.enabled) {
        const double tpFrac = static_cast<double>(cfg_.fixedExit.tpBps) / 10000.0;
        const double slFrac = static_cast<double>(cfg_.fixedExit.slBps) / 10000.0;

        if (fillEvent.fill.side == TradeSide::Buy) {
            fillEvent.fill.tpPrice = fillEvent.fill.price * (1.0 + tpFrac);
            fillEvent.fill.slPrice = fillEvent.fill.price * (1.0 - slFrac);
        } else {
            fillEvent.fill.tpPrice = fillEvent.fill.price * (1.0 - tpFrac);
            fillEvent.fill.slPrice = fillEvent.fill.price * (1.0 + slFrac);
        }
    }

    // Journal updates TradesModel + stats
    journal_->onFill(fillEvent.fill);

    fillEvent.latency.fillHandledNs = LatencyClock::nowNs();
    latencyCollector_.recordStrategyToOrder(fillEvent.latency);
    latencyCollector_.recordOrderToFill(fillEvent.latency);

    // Optional debug each fill
    qCDebug(logStrategy) << "Fill sym=" << fillEvent.fill.symbol
                         << " side=" << (fillEvent.fill.side == TradeSide::Buy ? "Buy" : "Sell")
                         << " qty=" << fillEvent.fill.qty
                         << " price=" << fillEvent.fill.price
                         << " fee=" << fillEvent.fill.fee
                         << " reduceOnly=" << fillEvent.fill.reduceOnly
                         << " tp=" << fillEvent.fill.tpPrice
                         << " sl=" << fillEvent.fill.slPrice
                         << " equity=" << journal_->equity();
}
