#pragma once
#include <QString>
#include <QList>
#include <functional>

#include "core/candle.h"
#include "core/timeframe.h"

class IExchangeClient {
public:
    virtual ~IExchangeClient() = default;

    // history
    virtual QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) = 0;

    // realtime polling support
    virtual bool supportsPollingRealtime() const { return false; }

    // async: request last kline (non-blocking)
    using LastKlineCallback = std::function<void(bool ok, const Candle& c)>;
    virtual void fetchLastKlineAsync(const QString& symbolId, Timeframe tf, LastKlineCallback cb) = 0;
};
