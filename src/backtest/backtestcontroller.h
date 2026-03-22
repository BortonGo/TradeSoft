#include "exchange\iexchangeclient.h"
#include "backtestmarketdataservice.h"
#include <memory>
#include <vector>

class BacktestController
{
    std::shared_ptr<IExchangeClient> exchange_;
    std::unique_ptr<BacktestMarketDataService> marketDataService_;
public:
    explicit BacktestController(std::shared_ptr<IExchangeClient> exchange);
    std::vector<Candle> loadHistory(const HistoryRequest& rec) const;
};

