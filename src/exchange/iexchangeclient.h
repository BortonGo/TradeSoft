#pragma once
#include <QString>
#include <vector>
#include <functional>

#include "core/candle.h"
#include "core/timeframe.h"
#include "backtest/historyrequest.h"

class IExchangeClient {
public:
    // history
    virtual std::vector<Candle> fetchKlines(const QString& symbolId, Timeframe tf) = 0;

    // history for backtest
    virtual std::vector<Candle> fetchHistory(const HistoryRequest& request) = 0;

    // realtime polling support
    virtual bool supportsPollingRealtime() const { return false; }

    // async: request last kline (non-blocking)
    using LastKlineCallback = std::function<void(bool ok, const Candle& c)>;
    virtual void fetchLastKlineAsync(const QString& symbolId, Timeframe tf, LastKlineCallback cb) = 0;

    // realtime websocket support
    virtual bool supportsWebSocketRealtime() const { return false; }

    using RealtimeKlineCallback = std::function<void(bool ok, const Candle& c)>;
    virtual void startKlineStream(const QString& symbolId, Timeframe tf, RealtimeKlineCallback cb)
    {
        Q_UNUSED(symbolId);
        Q_UNUSED(tf);
        if (cb) {
            cb(false, Candle{});
        }
    }
    virtual void stopKlineStream() {}

    virtual ~IExchangeClient() = default;
};
