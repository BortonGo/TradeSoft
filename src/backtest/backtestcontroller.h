#pragma once
#include "exchange\iexchangeclient.h"
#include "backtestmarketdataservice.h"
#include <memory>
#include <vector>

namespace Ui {class MainWindow;}

class BacktestController final
{
    std::shared_ptr<IExchangeClient> exchange_;
    std::unique_ptr<BacktestMarketDataService> marketDataService_;
    Ui::MainWindow* ui_ = nullptr;
public:
    explicit BacktestController(Ui::MainWindow* ui, std::shared_ptr<IExchangeClient> exchange);
    std::vector<Candle> loadHistory(const HistoryRequest& rec) const;

private:
    HistoryRequest buildHistoryRequest() const;
};

