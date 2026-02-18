#pragma once
#include <vector>
#include "core/candle.h"

class RSI
{
public:
    static std::vector<double> calculate(const std::vector<Candle>& candles, int period);
};

