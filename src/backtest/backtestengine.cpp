#include "backtestengine.h"

BacktestResult BacktestEngine::run(const BacktestRequest& request, const std::vector<Candle>& candles) const {
    BacktestResult res;
    if (candles.empty()) {
        res.state = BacktestState::Failed;
        res.errorText = "No candles for backtest";
        return res;
    }
    if (request.strategyName.trimmed().isEmpty() || request.strategyName == "None") {
        res.state = BacktestState::Failed;
        res.errorText = "Strategy name is not selected";
        return res;
    }

    double peakEquity = request.initialbalance;

    for (const auto& a : candles) {
        EquityPoint p;
        p.time = QDateTime::fromMSecsSinceEpoch(a.timestamp_);
        p.equity = request.initialbalance;
        if (p.equity > peakEquity) {
            peakEquity = p.equity;
        }
        p.drawdown = peakEquity - p.equity;
        p.cumulativePnl = 0.0;
        res.equityCurve.push_back(std::move(p));
    }
    res.state = BacktestState::Completed;

    return res;
}
