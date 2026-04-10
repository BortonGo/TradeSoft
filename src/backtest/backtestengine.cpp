#include "backtestengine.h"
#include "domain\strategy\strategyfactory.h"
#include <QDebug>

static StrategyConfig buildStrategyConfig(const BacktestRequest& request) {
    StrategyConfig cfg;
    cfg.strategy.name = request.strategyName;
    cfg.strategy.tf = request.timeframe;
    cfg.risk = request.backtestRisk;
    return cfg;
}

BacktestResult BacktestEngine::run(const BacktestRequest& request, const std::vector<Candle>& candles) const {
    BacktestResult res;
    if (candles.empty()) {
        res.state = BacktestState::Failed;
        res.errorText = "No candles for backtest";
        return res;
    }
    if (request.strategyName.trimmed().isEmpty() || request.strategyName == "None") {
        res.state = BacktestState::Failed;
        res.errorText = "Strategy is not selected";
        return res;
    }
    if (request.executionMode == BacktestExecutionMode::IntrabarLowerTf) {
        res.state = BacktestState::Failed;
        res.errorText = "Intrabar backtest is not implemented yet";
        return res;
    }

    const StrategyConfig cfg = buildStrategyConfig(request);
    double peakEquity = request.initialbalance;
    auto strategy = StrategyFactory::create(cfg);
    if (!strategy) {
        res.state = BacktestState::Failed;
        res.errorText = "Strategy is not supported";
        return res;
    }

    const int warmup = request.warmupBars;
    if (static_cast<int>(candles.size()) <= warmup) {
        res.state = BacktestState::Failed;
        res.errorText = "Not enough candles for warmup";
        return res;
    }

    std::shared_ptr<CandleSeries> series = std::make_shared<CandleSeries>(request.symbol, request.timeframe);

    for (int i = 0; i < warmup; ++i) {
        series->addCandle(candles[i]);
    }

    StrategyContext ctx;
    ctx.series = series;
    ctx.symbolId = request.symbol;
    ctx.tf = request.timeframe;

    strategy->onStart(ctx);

    BacktestTrade btTrade;
    bool inLong = false;
    bool inShort = false;

    for (int i = warmup; i < static_cast<int>(candles.size()); ++i) {
        series->addCandle(candles[i]);

        const auto signalsCandleClosed = strategy->onCandleClosed(ctx, candles[i]);

        for (const auto& s : signalsCandleClosed) {
            if (s.type == StrategySignalType::EnterLong && !inLong && !inShort) {
                btTrade = BacktestTrade{};
                btTrade.entryPrice = candles[i].close_;
                btTrade.entryTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.side = BacktestTradeSide::Long;
                inLong = true;
            }
            if (s.type == StrategySignalType::EnterShort && !inLong && !inShort) {
                btTrade = BacktestTrade{};
                btTrade.entryPrice = candles[i].close_;
                btTrade.entryTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.side = BacktestTradeSide::Short;
                inShort = true;
            }
            if (s.type == StrategySignalType::ExitLong && inLong && !inShort) {
                btTrade.exitPrice = candles[i].close_;
                btTrade.exitTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.quantity = 1.0;
                btTrade.grossPnl = btTrade.exitPrice - btTrade.entryPrice;
                btTrade.netPnl = btTrade.grossPnl;
                btTrade.winner = btTrade.netPnl > 0.0;
                res.trades.push_back(std::move(btTrade));
                btTrade = BacktestTrade {};
                inLong = false;
                inShort = false;
            }
            if (s.type == StrategySignalType::ExitShort && !inLong && inShort) {
                btTrade.exitPrice = candles[i].close_;
                btTrade.exitTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.quantity = 1.0;
                btTrade.grossPnl = btTrade.entryPrice - btTrade.exitPrice;
                btTrade.netPnl = btTrade.grossPnl;
                btTrade.winner = btTrade.netPnl > 0.0;
                res.trades.push_back(std::move(btTrade));
                btTrade = BacktestTrade {};
                inLong = false;
                inShort = false;
            }
        }
    }
    res.stats.trades =  static_cast<int>(res.trades.size());

    //stats
    int winCount = 0;
    int lossCount = 0;
    int grossWinSum = 0;
    int grossLossSum = 0;
    for (auto& t : res.trades) {
        if (t.winner) {
            ++winCount;
            grossWinSum += t.netPnl;
        } else {
            ++lossCount;
            grossLossSum += t.netPnl;
        }
    }

    qDebug() << "[BACKTEST ENGINE]  Total trades = " << res.stats.trades <<
                ", strategy = " << cfg.strategy.name <<
                ", Wins = " << winCount <<
                ", Losses = " << lossCount <<
                ", WinSum = " << grossWinSum <<
                ", LossSum = " << grossLossSum;

    double currentEquity = request.initialbalance;
    for (const auto& a : res.trades) {
        currentEquity += a.netPnl;
        EquityPoint p;
        p.time = a.exitTime;
        p.equity = currentEquity;
        peakEquity = std::max(peakEquity, p.equity);
        p.drawdown = peakEquity - p.equity;
        p.cumulativePnl = currentEquity - request.initialbalance;
        res.equityCurve.push_back(p);
    }
    res.state = BacktestState::Completed;

    return res;
}
