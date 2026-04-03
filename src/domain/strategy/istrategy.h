#pragma once
#include <memory>
#include <vector>
#include "domain/strategy/strategysignal.h"
#include "core/candleseries.h"

struct StrategyContext final {
    QString symbolId;
    Timeframe tf {};
    std::shared_ptr<CandleSeries> series;
};

class IStrategy {
public:
    virtual ~IStrategy() = default;
    virtual void onStart(const StrategyContext& ctx) = 0;
    virtual std::vector<StrategySignal> onCandleClosed(const StrategyContext& ctx, const Candle& closed) = 0;
    virtual std::vector<StrategySignal> onCandleUpdated(const StrategyContext& ctx, const Candle& forming) {
            Q_UNUSED(ctx); Q_UNUSED(forming);
            return {};
        }
};


