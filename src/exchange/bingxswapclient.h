#pragma once
#include "iexchangeclient.h"
#include <vector>
#include <QNetworkAccessManager>
#include <QTimer>

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

    bool supportsWebSocketRealtime() const override
    {
#if TRADESOFT_HAS_WEBSOCKETS
        return true;
#else
        return false;
#endif
    }
    void startKlineStream(const QString& symbolId, Timeframe tf, RealtimeKlineCallback cb) override;
    void stopKlineStream() override;

    static bool parseKlineStreamPayload(const QByteArray& payload, const QString& expectedDataType, Candle& candle);

private:
    QNetworkAccessManager* mgr_ = nullptr; // one per app

#if TRADESOFT_HAS_WEBSOCKETS
    QWebSocket* ws_ = nullptr;
    QTimer* wsReconnectTimer_ = nullptr;
    QString wsDataType_;
    RealtimeKlineCallback wsCallback_;
    int wsReconnectAttempts_ = 0;
    int wsDebugPayloadsLeft_ = 3;
    bool wsStopRequested_ = false;

    void ensureWebSocket();
    void sendKlineSubscription();
    void handleWebSocketPayload(const QByteArray& payload);
    void scheduleReconnect();
#endif
};
