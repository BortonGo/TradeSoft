#pragma once
#include <exchange\iexchangeclient.h>
#include <core\candleseries.h>
#include <memory>
#include <QObject>

class MarketDataService : public QObject
{
    Q_OBJECT

    std::shared_ptr<IExchangeClient> exchange_;
    std::shared_ptr<CandleSeries> currentSeries_;

public:
    explicit MarketDataService(std::shared_ptr<IExchangeClient> exchange, QObject* parent = nullptr);

    void loadHistory(const QString& symbolId, Timeframe tf);

signals:

    void signal_seriesLoaded(std::shared_ptr<CandleSeries> series);
};

