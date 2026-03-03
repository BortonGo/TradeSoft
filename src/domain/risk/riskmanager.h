#pragma once
#include "domain/strategy/strategysignal.h"
#include "domain/strategy/strategyconfig.h"
#include "domain/order/order.h"

class RiskManager final
{
public:
    RiskManager() = default;

    // equityUsdt — текущая equity demo-счёта
    // hasOpenPos / openSide — чтобы понимать, можно ли закрывать
    Order buildOrder(const StrategySignal& s, const RiskSettings& risk, double markPrice,
                     double equityUsdt, bool hasOpenPos, TradeSide openSide) const;
};
