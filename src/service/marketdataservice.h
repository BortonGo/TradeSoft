#pragma once
#include <exchange\iexchangeclient.h>
#include <core\candleseries.h>
#include <memory>
#include <QObject>
#include <QTimer>

class MarketDataService : public QObject
{
    Q_OBJECT

    std::shared_ptr<IExchangeClient> exchange_;
    std::shared_ptr<CandleSeries> currentSeries_;

    QTimer* rtTimer_ = nullptr;
    QString rtSymbolId_;
    Timeframe rtTimeframe_;
    int tickInCandle_ = 0;
    std::mt19937 rng_;

    bool useExchangeRealtime_ = false;

public:
    explicit MarketDataService(std::shared_ptr<IExchangeClient> exchange, QObject* parent = nullptr);

    void loadHistory(const QString& symbolId, Timeframe tf);
    int ticksPerCandle(Timeframe tf) const;
    Candle makeNextCandle(const Candle& closed) const;
    void startRealTime();
    void stopRealTime();

signals:

    void signal_seriesLoaded(std::shared_ptr<CandleSeries> series);
    void signal_candleUpdated(Candle c);
    void signal_candleClosed(Candle c);
    void signal_connectionStateChanged(QString state); // not now

private slots:
    void onRtTick();
};

