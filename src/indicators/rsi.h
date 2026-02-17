#pragma once
#include <QVector>
#include <QList>
#include "core/candle.h"

class RSI
{
public:
    static QVector<double> calculate(const QList<Candle>& candles, int period);
};

