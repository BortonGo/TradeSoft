#pragma once
#include <QVector>
#include <QList>
#include "core/candle.h"

class Donchian
{
public:
    struct DonchianResult {
        QVector<double> upper;
        QVector<double> lower;
        QVector<double> middle;
    };

    static DonchianResult calculate(const QList<Candle>& candles, int period);
};

