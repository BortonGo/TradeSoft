#include "backtestengine.h"

BacktestResult BacktestEngine::run(const BacktestRequest& request, const std::vector<Candle>& candles) const {
    BacktestResult res;
    if (candles.empty()) {
        res.state = BacktestState::Failed;
        res.errorText = "No candles for backtest";
        return res;
    }
    res.state = BacktestState::Completed;
    return res;
}
