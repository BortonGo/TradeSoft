#pragma once
#include "domain/strategy/istrategy.h"
#include "indicators/ema.h"
#include <QtGlobal>
#include <cmath>

// EMA Scalper:
// вход по направлению fast>slow / fast<slow (не обязательно ждать кросса)
// выход по TP/SL/таймауту, плюс optional flip (закрыть и сразу открыть в другую сторону)
class EmaScalpStrategy final : public IStrategy {
    int fast_ = 5;
    int slow_ = 13;

    // bps: 10 bps = 0.10%
    int tpBps_ = 12;
    int slBps_ = 10;

    int maxBarsInTrade_ = 5;
    int minSpreadBps_ = 2;     // фильтр пилы: насколько fast и slow должны разойтись (в bps от цены)
    bool flipOnBiasChange_ = true;

    bool allowLong_ = true;
    bool allowShort_ = true;

    // локальное состояние (MVP)
    bool inLong_ = false;
    bool inShort_ = false;
    double entryPrice_ = 0.0;
    int barsInTrade_ = 0;

public:
    EmaScalpStrategy(int fast, int slow,
                     int tpBps, int slBps,
                     int maxBarsInTrade,
                     int minSpreadBps,
                     bool flipOnBiasChange,
                     bool allowLong, bool allowShort)
        : fast_(fast), slow_(slow),
          tpBps_(tpBps), slBps_(slBps),
          maxBarsInTrade_(maxBarsInTrade),
          minSpreadBps_(minSpreadBps),
          flipOnBiasChange_(flipOnBiasChange),
          allowLong_(allowLong), allowShort_(allowShort) {}

    void onStart(const StrategyContext& ctx) override {
        Q_UNUSED(ctx);
        inLong_ = false;
        inShort_ = false;
        entryPrice_ = 0.0;
        barsInTrade_ = 0;
    }

    std::vector<StrategySignal> onCandleClosed(const StrategyContext& ctx, const Candle& closed) override {
        std::vector<StrategySignal> out;
        if (!ctx.series) return out;

        const auto& candles = ctx.series->getCandles();
        if (candles.size() < static_cast<size_t>(slow_ + 2)) return out;

        const std::vector<double> emaFast = EMA::calculate(candles, fast_);
        const std::vector<double> emaSlow = EMA::calculate(candles, slow_);
        if (emaFast.size() < 1 || emaSlow.size() < 1) return out;

        const double fNow = emaFast.back();
        const double sNow = emaSlow.back();

        const double px = closed.close_;
        if (px <= 0.0) return out;

        auto mk = [&](StrategySignalType type, const char* reason) {
            StrategySignal s;
            s.type = type;
            s.symbolId = ctx.symbolId;
            s.tf = ctx.tf;
            s.timestamp = closed.timestamp_;
            s.reason = reason;
            return s;
        };

        // spread filter (anti-chop)
        const double spread = std::abs(fNow - sNow);
        const double spreadBps = (spread / px) * 10000.0;
        const bool spreadOk = spreadBps >= static_cast<double>(minSpreadBps_);

        const bool biasLong = (fNow > sNow);
        const bool biasShort = (fNow < sNow);

        // ---- if we have a position: manage it ----
        if (inLong_ || inShort_) {
            barsInTrade_++;

            // gross return (fraction)
            double ret = 0.0;
            if (entryPrice_ > 0.0) {
                if (inLong_)  ret = (px - entryPrice_) / entryPrice_;
                if (inShort_) ret = (entryPrice_ - px) / entryPrice_;
            }

            const double tp = static_cast<double>(tpBps_) / 10000.0;
            const double sl = static_cast<double>(slBps_) / 10000.0;

            const bool hitTp = (ret >= tp);
            const bool hitSl = (ret <= -sl);
            const bool timeout = (barsInTrade_ >= maxBarsInTrade_);

            // optional flip on bias change
            const bool biasFlipToShort = inLong_  && flipOnBiasChange_ && biasShort;
            const bool biasFlipToLong  = inShort_ && flipOnBiasChange_ && biasLong;

            if (inLong_ && (hitTp || hitSl || timeout || biasFlipToShort)) {
                out.push_back(mk(StrategySignalType::ExitLong,
                                 hitTp ? "Scalp TP -> exit long" :
                                 hitSl ? "Scalp SL -> exit long" :
                                 timeout ? "Scalp timeout -> exit long" :
                                 "Bias flip -> exit long"));

                inLong_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;

                // immediate reverse if bias says so + spread ok
                if (biasFlipToShort && allowShort_ && spreadOk) {
                    out.push_back(mk(StrategySignalType::EnterShort, "Bias flip -> enter short"));
                    inShort_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;
                }
                return out; // one decision per candle
            }

            if (inShort_ && (hitTp || hitSl || timeout || biasFlipToLong)) {
                out.push_back(mk(StrategySignalType::ExitShort,
                                 hitTp ? "Scalp TP -> exit short" :
                                 hitSl ? "Scalp SL -> exit short" :
                                 timeout ? "Scalp timeout -> exit short" :
                                 "Bias flip -> exit short"));

                inShort_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;

                if (biasFlipToLong && allowLong_ && spreadOk) {
                    out.push_back(mk(StrategySignalType::EnterLong, "Bias flip -> enter long"));
                    inLong_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;
                }
                return out;
            }

            // still holding
            return out;
        }

        // ---- no position: open if bias + spread ok ----
        if (!spreadOk) return out;

        if (biasLong && allowLong_) {
            out.push_back(mk(StrategySignalType::EnterLong, "EMA bias long + spread -> enter long"));
            inLong_ = true;
            entryPrice_ = px;
            barsInTrade_ = 0;
            return out;
        }

        if (biasShort && allowShort_) {
            out.push_back(mk(StrategySignalType::EnterShort, "EMA bias short + spread -> enter short"));
            inShort_ = true;
            entryPrice_ = px;
            barsInTrade_ = 0;
            return out;
        }

        return out;
    }
};
