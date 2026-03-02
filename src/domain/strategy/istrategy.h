#pragma once
#include <memory>
#include <vector>
#include "domain/strategy/strategysignal.h"
#include "core/candleseries.h"

struct StrategyContext {
    QString symbolId;
    Timeframe tf {};
    std::shared_ptr<CandleSeries> series;
};

class IStrategy {
public:
    virtual ~IStrategy() = default;
    virtual void onStart(const StrategyContext& ctx) = 0;
    virtual vector<StrategySignal> onCandleClosed(const StrategyContext& ctx, const Candle& closed) = 0;
    virtual void onCandleUpdated(const StrategyContext& ctx, const Candle& forming) {
            Q_UNUSED(ctx); Q_UNUSED(forming);
        }
};
