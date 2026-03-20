#include "backtestmarketdataservice.h"
#include <utility>

BacktestMarketDataService::BacktestMarketDataService(std::shared_ptr<IExchangeClient> exchange) :
    exchange_(std::move(exchange)) {}

std::vector<Candle> BacktestMarketDataService::loadHistory(const HistoryRequest& rec) const {
    if (!exchange_) return {};
    return exchange_->fetchHistory(rec);
}
