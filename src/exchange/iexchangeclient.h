#pragma once
#include <QString>
#include <QList>
#include "core\candle.h"
#include "core\timeframe.h"

class IExchangeClient {
public:
    virtual QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) = 0;
};
