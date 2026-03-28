#pragma once
#include "backtesttypes.h"
#include "core/candle.h"
#include <vector>

class BacktestEngine
{
public:
    BacktestResult run(const BacktestRequest& request, const std::vector<Candle>& candles) const;
};
