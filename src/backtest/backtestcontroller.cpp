#include "backtestcontroller.h"
#include <utility>
#include <memory>

BacktestController::BacktestController(std::shared_ptr<IExchangeClient> exchange) : exchange_(std::move(exchange)),
    marketDataService_(std::unique_ptr<BacktestMarketDataService>(new BacktestMarketDataService(exchange_))) {}

std::vector<Candle> BacktestController::loadHistory(const HistoryRequest& rec) const {
    if (!marketDataService_) return {};

    return marketDataService_->loadHistory(rec);
}

