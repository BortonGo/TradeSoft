#pragma once
#include "domain/strategy/istrategy.h"
#include "indicators/ema.h"
#include <QtGlobal>
#include <cmath>
#include <algorithm>

// EMA Touch Scalper (real scalp):
// Bias by fast vs slow EMA.
// Enter on "touch/near" fast EMA (band in bps) in direction of bias.
// Exit by TP/SL/timeout, optional flip on bias change.
class EmaScalpStrategy final : public IStrategy {
    int fast_ = 5;
    int slow_ = 13;

    // bps: 10 bps = 0.10%
    int tpBps_ = 10;          // e.g. 8..15 bps typical for M1
    int slBps_ = 8;

    int maxBarsInTrade_ = 6;

    // Anti-chop: require EMA separation
    int minSpreadBps_ = 3;

    // Entry band around fast EMA (how close price must be to EMA fast)
    int enterBandBps_ = 2;    // 1..5 bps typical (BTC M1: start with 2-3)

    // Simple "pullback confirmation": price should move against bias on current candle close
    // We approximate it by comparing current close with previous close.
    bool requirePullbackTick_ = true;

    bool flipOnBiasChange_ = true;
    bool allowLong_ = true;
    bool allowShort_ = true;

    // local state (MVP)
    bool inLong_ = false;
    bool inShort_ = false;
    double entryPrice_ = 0.0;
    int barsInTrade_ = 0;

    int cooldownBars_ = 0;
    int cooldownAfterExit_ = 1;

    static double bpsToFrac(int bps) { return static_cast<double>(bps) / 10000.0; }

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
          allowLong_(allowLong), allowShort_(allowShort)
    {}

    void onStart(const StrategyContext& ctx) override {
        Q_UNUSED(ctx);
        inLong_ = false;
        inShort_ = false;
        entryPrice_ = 0.0;
        barsInTrade_ = 0;
        cooldownBars_ = 0;
    }

    std::vector<StrategySignal> onCandleClosed(const StrategyContext& ctx, const Candle& closed) override {
        std::vector<StrategySignal> out;
        if (!ctx.series) return out;

        const auto& candles = ctx.series->getCandles();
        if (candles.size() < static_cast<size_t>(slow_ + 3)) return out;

        const auto emaFast = EMA::calculate(candles, fast_);
        const auto emaSlow = EMA::calculate(candles, slow_);
        if (emaFast.size() < 2 || emaSlow.size() < 2) return out;

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

        // Anti-chop: EMA separation
        const double spread = std::abs(fNow - sNow);
        const double spreadBps = (spread / px) * 10000.0;
        const bool spreadOk = spreadBps >= static_cast<double>(minSpreadBps_);

        const bool biasLong = (fNow > sNow);
        const bool biasShort = (fNow < sNow);

        // Price must be near fast EMA
        const double dist = std::abs(px - fNow);
        const double distBps = (dist / px) * 10000.0;
        const bool nearFast = distBps <= static_cast<double>(enterBandBps_);

        // Pullback "tick" confirmation: for long we want current close <= prev close (small pullback),
        // for short current close >= prev close.
        const double prevClose = candles[candles.size() - 2].close_;
        const bool pullbackOkLong  = (!requirePullbackTick_) || (px <= prevClose);
        const bool pullbackOkShort = (!requirePullbackTick_) || (px >= prevClose);

        // ---------------- manage open position ----------------
        if (inLong_ || inShort_) {
            barsInTrade_++;

            double ret = 0.0;
            if (entryPrice_ > 0.0) {
                if (inLong_)  ret = (px - entryPrice_) / entryPrice_;
                if (inShort_) ret = (entryPrice_ - px) / entryPrice_;
            }

            const double tp = bpsToFrac(tpBps_);
            const double sl = bpsToFrac(slBps_);

            const bool hitTp = (ret >= tp);
            const bool hitSl = (ret <= -sl);
            const bool timeout = (barsInTrade_ >= maxBarsInTrade_);

            const bool flipToShort = inLong_  && flipOnBiasChange_ && biasShort;
            const bool flipToLong  = inShort_ && flipOnBiasChange_ && biasLong;

            if (inLong_ && (hitTp || hitSl || timeout || flipToShort)) {
                out.push_back(mk(StrategySignalType::ExitLong,
                                 hitTp ? "TP -> exit long" :
                                 hitSl ? "SL -> exit long" :
                                 timeout ? "Timeout -> exit long" :
                                 "Bias flip -> exit long"));

                inLong_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;
                cooldownBars_ = cooldownAfterExit_;

                // optional reverse (scalp flip)
                if (flipToShort && allowShort_ && spreadOk) {
                    out.push_back(mk(StrategySignalType::EnterShort, "Flip -> enter short"));
                    inShort_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;
                    cooldownBars_ = 0;
                }
                return out;
            }

            if (inShort_ && (hitTp || hitSl || timeout || flipToLong)) {
                out.push_back(mk(StrategySignalType::ExitShort,
                                 hitTp ? "TP -> exit short" :
                                 hitSl ? "SL -> exit short" :
                                 timeout ? "Timeout -> exit short" :
                                 "Bias flip -> exit short"));

                inShort_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;
                cooldownBars_ = cooldownAfterExit_;

                if (flipToLong && allowLong_ && spreadOk) {
                    out.push_back(mk(StrategySignalType::EnterLong, "Flip -> enter long"));
                    inLong_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;
                    cooldownBars_ = 0;
                }
                return out;
            }

            return out; // still holding
        }

        // ---------------- flat: cooldown ----------------
        if (cooldownBars_ > 0) {
            cooldownBars_--;
            return out;
        }

        // ---------------- flat: entries ----------------
        if (!spreadOk || !nearFast) return out;

        // For long: biasLong + price near fast EMA + price at/below fast EMA (touch from above) + pullback tick ok
        if (biasLong && allowLong_ && (px <= fNow) && pullbackOkLong) {
            out.push_back(mk(StrategySignalType::EnterLong, "Scalp: bias long + touch fast EMA -> enter long"));
            inLong_ = true;
            entryPrice_ = px;
            barsInTrade_ = 0;
            return out;
        }

        // For short: biasShort + price near fast EMA + price at/above fast EMA (touch from below) + pullback tick ok
        if (biasShort && allowShort_ && (px >= fNow) && pullbackOkShort) {
            out.push_back(mk(StrategySignalType::EnterShort, "Scalp: bias short + touch fast EMA -> enter short"));
            inShort_ = true;
            entryPrice_ = px;
            barsInTrade_ = 0;
            return out;
        }

        return out;
    }
};
