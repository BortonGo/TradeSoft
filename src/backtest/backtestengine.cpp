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

    int signalCandleClosedCount = 0;
    for (int i = warmup; i < static_cast<int>(candles.size()); ++i) {
        series->addCandle(candles[i]);

        if (request.executionMode == BacktestExecutionMode::IntrabarLowerTf) {
            res.state = BacktestState::Failed;
            res.errorText = "Intrabar backtest is not implement yet";
            return res;
        }
        const auto signalsCandleClosed = strategy->onCandleClosed(ctx, candles[i]);

        for (const auto& s : signalsCandleClosed) {
            if (s.type != StrategySignalType::None) {
                ++signalCandleClosedCount;
            }
        }
    }

    qDebug() << "[BACKTEST ENGINE]  onCandleClosedSignals = " << signalCandleClosedCount <<
                ", strategy = " << cfg.strategy.name;

    for (const auto& a : candles) {
        EquityPoint p;
        p.time = QDateTime::fromMSecsSinceEpoch(a.timestamp_);
        p.equity = request.initialbalance;
        if (p.equity > peakEquity) {
            peakEquity = p.equity;
        }
        p.drawdown = peakEquity - p.equity;
        p.cumulativePnl = 0.0;
        res.equityCurve.push_back(p);
    }
    res.state = BacktestState::Completed;

    return res;
}
