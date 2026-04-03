#pragma once
#include <vector>
#include "core/candle.h"

class RSI final {
public:
    static std::vector<double> calculate(const std::vector<Candle>& candles, int period);
};

