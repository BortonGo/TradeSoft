#pragma once
#include <vector>
#include "core/candle.h"

class Donchian
{
public:
    struct DonchianResult {
        std::vector<double> upper;
        std::vector<double> lower;
        std::vector<double> middle;
    };

    static DonchianResult calculate(const std::vector<Candle>& candles, int period);
};

