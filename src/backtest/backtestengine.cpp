#include "backtestengine.h"

BacktestResult BacktestEngine::run(const BacktestRequest& request, const std::vector<Candle>& candles) const {
    BacktestResult res;
    if (candles.empty()) {
        res.state = BacktestState::Failed;
        res.errorText = "No candles for backtest";
        return res;
    }
    res.state = BacktestState::Completed;

    for (const auto& a : candles) {
        EquityPoint p;
        p.time = QDateTime::fromMSecsSinceEpoch(a.timestamp_);
        p.equity = request.initialbalance;
        p.drawdown = 0.0;
        p.cumulativePnl = 0.0;
        res.equityCurve.push_back(std::move(p));
    }

    return res;
}
