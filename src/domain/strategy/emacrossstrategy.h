#pragma once
#include "domain/strategy/istrategy.h"
#include "indicators/ema.h"

class EmaCrossStrategy final : public IStrategy {
    int fast_ = 9;
    int slow_ = 20;
    bool allowLong_ = true;
    bool allowShort_ = true;

    // упрощённо: состояние позиции внутри стратегии (на итерацию 1)
    // позже это уйдёт в PositionService/Journal, а стратегия станет “stateless”
    bool inLong_ = false;
    bool inShort_ = false;

public:
    EmaCrossStrategy(int fast, int slow, bool allowLong, bool allowShort) : fast_(fast), slow_(slow),
        allowLong_(allowLong), allowShort_(allowShort) {}

    void onStart(const StrategyContext& ctx) {
        Q_UNUSED(ctx);
        inLong_ = false;
        inShort_ = false;
    }

    std::vector<StrategySignal> onCandleClosed(const StrategyContext& ctx, const Candle& closed) {
        std::vector<StrategySignal> out;
        if (!ctx.series) return out;

        std::vector<Candle>& candles = ctx.series->getCandles();
        if (candles.size() < slow_ + 2) return out;

        const std::vector<double> emaFast = EMA::calculate(candles, fast_);
        const std::vector<double> emaSlow = EMA::calculate(candles, slow_);
        if (emaFast.size() < 2 || emaSlow.size() < 2) return out;

        const double fPrev = emaFast[emaFast.size() - 2];
        const double fNow = emaFast[emaFast.size() - 1];
        const double sPrev = emaSlow[emaSlow.size() - 2];
        const double sNow = emaSlow[emaSlow.size() - 1];

        const bool crossUp = (fPrev <= sPrev) && (fNow > sNow);
        const bool crossDown = (fPrev >= sPrev) && (fNow < sNow);

        auto mk = [&](StrategySignalType type, const QString& reason){
            StrategySignal s;
            s.type = type;
            s.symbolId = ctx.symbolId;
            s.tf = ctx.tf;
            s.timestamp = closed.timestamp_;
            s.reason = reason;
            return s;
        };

        // Cross up
        if (crossUp) {
            if (inShort_) {
                out.push_back(makeSignal(StrategySignalType::ExitShort, "EMA cross up -> exit short"));
                inShort_ = false;
            }

            if (allowLong_ && !inLong_) {
                out.push_back(makeSignal(StrategySignalType::EnterLong, "EMA cross up -> enter long"));
                inLong_ = true;
            }
        }

        // Cross down
        if (crossDown) {
            if (inLong_) {
                out.push_back(makeSignal(StrategySignalType::ExitLong, "EMA cross down -> exit long"));
                inLong_ = false;
            }

            if (allowShort_ && !inShort_) {
                out.push_back(makeSignal(StrategySignalType::EnterShort, "EMA cross down -> enter short"));
                inShort_ = true;
            }
        }
        return out;
    }
};
