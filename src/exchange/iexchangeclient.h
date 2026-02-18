#pragma once
#include <QString>
#include <vector>
#include <functional>

#include "core/candle.h"
#include "core/timeframe.h"

class IExchangeClient {
public:
    // history
    virtual std::vector<Candle> fetchKlines(const QString& symbolId, Timeframe tf) = 0;

    // realtime polling support
    virtual bool supportsPollingRealtime() const { return false; }

    // async: request last kline (non-blocking)
    using LastKlineCallback = std::function<void(bool ok, const Candle& c)>;
    virtual void fetchLastKlineAsync(const QString& symbolId, Timeframe tf, LastKlineCallback cb) = 0;

    virtual ~IExchangeClient() = default;
};
