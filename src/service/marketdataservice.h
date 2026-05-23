#pragma once
#include "core/appconfig.h"

#include <exchange/iexchangeclient.h>
#include <core/candleseries.h>
#include <memory>
#include <QObject>
#include <QTimer>

class MarketDataService final : public QObject {
    Q_OBJECT

    std::shared_ptr<IExchangeClient> exchange_;
    std::shared_ptr<CandleSeries> currentSeries_;

    QTimer* rtTimer_ = nullptr;
    QString rtSymbolId_;
    Timeframe rtTimeframe_;

    bool useExchangeRealtime_ = false;
    bool useWebSocketRealtime_ = false;
    bool webSocketLiveNotified_ = false;
    bool requestInFlight_ = false;
    int realtimePollingMs_ = 1000;
    RealtimeTransport realtimeTransport_ = RealtimeTransport::Auto;
    int consecutiveRealtimeFailures_ = 0;


public:
    explicit MarketDataService(std::shared_ptr<IExchangeClient> exchange, QObject* parent = nullptr);

    void loadHistory(const QString& symbolId, Timeframe tf);
    void startRealTime();
    void stopRealTime();
    void setRealtimePollingMs(int intervalMs);
    void setRealtimeTransport(RealtimeTransport transport);

signals:

    void signal_seriesLoaded(std::shared_ptr<CandleSeries> series);
    void signal_candleUpdated(Candle c);
    void signal_candleClosed(Candle c);
    void signal_connectionStateChanged(QString state); // not now

private slots:
    void onRtTick();

private:
    void startPollingRealtime();
    void applyRealtimeCandle(const Candle& freshIn);
    void setRealtimeIntervalForFailures();
};
