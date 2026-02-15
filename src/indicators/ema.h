#pragma once
#include <QVector>
#include <QList>
#include "core/candle.h"

class EMA
{
public:
    static QVector<double> calculate(const QList<Candle>& candles, int period);
};
