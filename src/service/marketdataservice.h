#pragma once
#include <src\exchange\iexchangeclient.h>
#include <src\core\candleseries.h>
#include <memory>
#include <QObject>

class MarketDataService : public QObject
{
    std::shared_ptr<IExchangeClient> exchange_;
    std::shared_ptr<CandleSeries> currentSeries_;

public:
    explicit MarketDataService(std::shared_ptr<IExchangeClient> exchange);

    void loadHistory(const QString& symbolId, Timeframe tf);

signals:

    void signal_seriesLoaded(std::shared_ptr<CandleSeries> series);
};

