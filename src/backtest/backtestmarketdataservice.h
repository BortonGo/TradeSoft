#pragma once
#include "exchange\iexchangeclient.h"
#include "historyrequest.h"
#include <memory>
#include <vector>

class BacktestMarketDataService final
{
    std::shared_ptr<IExchangeClient> exchange_;
public:
    explicit BacktestMarketDataService(std::shared_ptr<IExchangeClient> exchange);
    std::vector<Candle> loadHistory(const HistoryRequest& rec) const;
};
