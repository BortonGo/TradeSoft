#pragma once
#include <QString>
#include <QList>
#include "core\candle.h"
#include "core\timeframe.h"

class IExchangeClient {
public:
    virtual ~IExchangeClient() {}

    virtual QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) = 0;

    // for polling
    virtual bool fetchLastKline(const QString& symbolId, Timeframe tf, Candle& out) = 0;

    virtual bool supportsPollingRealtime() const { return false; }

};
