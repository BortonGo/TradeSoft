#pragma once
#include "iexchangeclient.h"
#include <vector>
#include <QNetworkAccessManager>

#if TRADESOFT_HAS_WEBSOCKETS
#include <QWebSocket>
#endif

class BingXSwapClient final : public IExchangeClient {
public:
    ~BingXSwapClient() override;

    std::vector<Candle> fetchKlines(const QString& symbolId, Timeframe tf) override;
    std::vector<Candle> fetchHistory(const HistoryRequest& request) override;

    bool supportsPollingRealtime() const override { return true; }
    void fetchLastKlineAsync(const QString& symbolId, Timeframe tf, LastKlineCallback cb) override;

    bool supportsWebSocketRealtime() const override { return true; }
    void startKlineStream(const QString& symbolId, Timeframe tf, RealtimeKlineCallback cb) override;
    void stopKlineStream() override;

private:
    QNetworkAccessManager* mgr_ = nullptr; // one per app

#if TRADESOFT_HAS_WEBSOCKETS
    QWebSocket* ws_ = nullptr;
    QString wsDataType_;
    RealtimeKlineCallback wsCallback_;

    void ensureWebSocket();
    void sendKlineSubscription();
    void handleWebSocketPayload(const QByteArray& payload);
#endif
};
