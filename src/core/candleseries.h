#pragma once
#include "core\timeframe.h"
#include "candle.h"
#include <vector>

class CandleSeries
{
    QString symbol_;
    Timeframe timeframe_;
    std::vector<Candle> candles_;

public:
    CandleSeries(const QString& symbolId, Timeframe tf);

    const QString& symbol() const;
    Timeframe timeframe() const;
    int candleCount() const;

    void addCandle(const Candle& c);
    void updateLastCandle(const Candle& c);
    const Candle& last() const;

    int getCount() const;
    const std::vector<Candle>& getCandles() const;
};

