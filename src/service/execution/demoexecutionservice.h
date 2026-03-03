#pragma once
#include "domain/order/order.h"
#include "domain/order/fill.h"
#include "domain/strategy/strategyconfig.h"

class DemoExecutionService final
{
public:
    DemoExecutionService() = default;

    // Market fill по markPrice (+slippage), fee по risk.feePct
    Fill executeMarket(const Order& o, double markPrice, const RiskSettings& risk) const;
};
