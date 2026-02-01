#pragma once
#include <QString>
#include <QList>
#include "src\core\candle.h"
#include "src\core\timeframe.h"
class IExchangeClient {
public:
    virtual QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) = 0;
};
