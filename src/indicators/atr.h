#pragma once
#include <QVector>
#include <QList>
#include "core/candle.h"

class ATR {
public:
    static QVector<double> calculate(const QList<Candle>& candles, int period);
};
