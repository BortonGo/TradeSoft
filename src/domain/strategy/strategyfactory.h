#pragma once
#include <memory>
#include "domain/strategy/istrategy.h"
#include "domain/strategy/strategyconfig.h"

class StrategyFactory final
{
public:
    // Возвращает готовую стратегию по имени cfg.strategy.name
    // (параметры пока захардкожены)
    static std::unique_ptr<IStrategy> create(const StrategyConfig& cfg);
};
