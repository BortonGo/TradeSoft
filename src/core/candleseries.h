#pragma once
#include "timeframe.h"
#include "candle.h"

class CandleSeries
{
    QString symbol_;
    Timeframe timeframe_;
    QList<Candle> candles_;

public:
    CandleSeries(const QString& symbolId, Timeframe tf);

    const QString& symbol() const;
    Timeframe timeframe() const;

    void addCandle(const Candle& c);
    void updateLastCandle(const Candle& c);
    const Candle& last() const;
};

