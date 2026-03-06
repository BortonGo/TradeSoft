#include "strategyrunner.h"
#include <utility>
#include <QDebug>

StrategyRunner::StrategyRunner(MarketDataService* mds, QObject* parent) : QObject(parent), mds_(mds) {
    Q_ASSERT(mds_);
    connect(mds_, &MarketDataService::signal_seriesLoaded, this, &StrategyRunner::onSeriesLoaded);
    connect(mds_, &MarketDataService::signal_candleClosed,  this, &StrategyRunner::onCandleClosed);
    connect(mds_, &MarketDataService::signal_candleUpdated, this, &StrategyRunner::onCandleUpdated);
}

void StrategyRunner::setStrategy(std::unique_ptr<IStrategy> s) {
    strategy_ = std::move(s);
}

void StrategyRunner::start(const QString& symbolId, Timeframe tf) {
    if (running_) return;

    if (!strategy_) {
        qWarning() << "[RUNNER] Can't start: strategy == nullptr";
        return;
    }
    if (!risk_ || !exec_ || !journal_) {
        qWarning() << "[RUNNER] Can't start: pipeline services not set (risk/exec/journal)";
        return;
    }

    ctx_.symbolId = symbolId;
    ctx_.tf = tf;
    ctx_.series.reset();

    running_ = true;

    mds_->stopRealTime();
    mds_->loadHistory(symbolId, tf);
    mds_->startRealTime();

    emit signal_started();
    qDebug() << "[RUNNER] START symbol =" << symbolId << ", tf =" << toUiString(tf);
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
        qDebug() << "[RUNNER] REPORT netPnl=" << r.netPnl
                 << " equity=" << r.equity
                 << " fees=" << r.fees
                 << " trades=" << r.closedTrades
                 << " winrate=" << winRate
                 << " maxDD=" << r.maxDrawdown;
    }

    qDebug() << "[RUNNER] STOP";
}

void StrategyRunner::onSeriesLoaded(std::shared_ptr<CandleSeries> series) {
    if (!running_ || !strategy_) return;
    ctx_.series = series;

    strategy_->onStart(ctx_);
    qDebug() << "[RUNNER] series loaded, candles =" << series->getCount();
}

void StrategyRunner::onCandleClosed(Candle c)
{
    if (!running_ || !strategy_ || !ctx_.series) return;

    const auto signal = strategy_->onCandleClosed(ctx_, c);

    for (const auto& s : signal) {
        if (s.type == StrategySignalType::None) continue;

        qDebug() << "[Runner] CLOSED SIGNAL type=" << static_cast<int>(s.type)
                 << " symbol=" << s.symbolId
                 << " tf=" << toUiString(s.tf)
                 << " reason=" << s.reason;

        handleSignal(s, c);
        emit signal_signalGenerated(s);
    }

    // Можно оставить и тут тоже, чтобы на закрытии свечи журнал точно обновился
    if (journal_) {
        journal_->onPriceUpdate(ctx_.symbolId, c.close_, riskSettings_.feePct);
    }
}

void StrategyRunner::onCandleUpdated(Candle c)
{
    if (!running_ || !strategy_ || !ctx_.series) return;

    const auto signal = strategy_->onCandleUpdated(ctx_, c);

    for (const auto& s : signal) {
        if (s.type == StrategySignalType::None) continue;

            qDebug() << "[Runner] UPDATE SIGNAL type=" << static_cast<int>(s.type)
                 << " symbol=" << s.symbolId
                 << " tf=" << toUiString(s.tf)
                 << " reason=" << s.reason;

        handleSignal(s, c);
        emit signal_signalGenerated(s);
    }

    if (journal_) {
        journal_->onPriceUpdate(ctx_.symbolId, c.close_, riskSettings_.feePct);
    }
}

void StrategyRunner::handleSignal(const StrategySignal& s, const Candle& closed)
{
    if (!risk_ || !exec_ || !journal_) return;

    const double markPrice = closed.close_;
    if (markPrice <= 0.0) return;

    const bool hasOpen = journal_->hasOpen(s.symbolId);
    const TradeSide openSide = hasOpen ? journal_->openSide(s.symbolId) : TradeSide::Buy;

    // Build order from signal
    Order o = risk_->buildOrder(
        s,
        riskSettings_,
        markPrice,
        journal_->equity(),
        hasOpen,
        openSide
    );

    // RiskManager can return empty order
    if (o.symbol.isEmpty()) return;

    // For Exit: close full qty
    if (o.reduceOnly) {
        o.qty = journal_->openQty(o.symbol);
    }

    if (o.qty <= 0.0) return;

    // Demo fill
    Fill f = exec_->executeMarket(o, markPrice, riskSettings_);

    // For OPEN fills, calculate fixed TP/SL levels from config
    if (!f.reduceOnly && cfg_.fixedExit.enabled) {
        const double tpFrac = static_cast<double>(cfg_.fixedExit.tpBps) / 10000.0;
        const double slFrac = static_cast<double>(cfg_.fixedExit.slBps) / 10000.0;

        if (f.side == TradeSide::Buy) {
            f.tpPrice = f.price * (1.0 + tpFrac);
            f.slPrice = f.price * (1.0 - slFrac);
        } else {
            f.tpPrice = f.price * (1.0 - tpFrac);
            f.slPrice = f.price * (1.0 + slFrac);
        }
    }

    // Journal updates TradesModel + stats
    journal_->onFill(f);

    // Optional debug each fill
    qDebug() << "[RUNNER] FILL sym=" << f.symbol
             << " side=" << (f.side == TradeSide::Buy ? "Buy" : "Sell")
             << " qty=" << f.qty
             << " price=" << f.price
             << " fee=" << f.fee
             << " reduceOnly=" << f.reduceOnly
             << " tp=" << f.tpPrice
             << " sl=" << f.slPrice
             << " equity=" << journal_->equity();
}
