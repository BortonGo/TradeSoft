#pragma once
#include "domain/strategy/istrategy.h"
#include "indicators/ema.h"
#include <QtGlobal>
#include <cmath>
#include <algorithm>

class EmaScalpStrategy final : public IStrategy {
    int fast_ = 5;
    int slow_ = 13;

    // bps: 30 = 0.30%
    int tpBps_ = 13;
    int slBps_ = 8;

    int maxBarsInTrade_ = 6;

    int minSpreadBps_ = 3;
    int enterBandBps_ = 2;

    bool requirePullbackTick_ = true;

    bool flipOnBiasChange_ = true;
    bool allowLong_ = true;
    bool allowShort_ = true;

    // position state
    bool inLong_ = false;
    bool inShort_ = false;
    double entryPrice_ = 0.0;
    int barsInTrade_ = 0;

    int cooldownBars_ = 0;
    int cooldownAfterExit_ = 0;

    // indicator state, updated only on closed candles
    bool indicatorsReady_ = false;
    double lastFastEma_ = 0.0;
    double lastSlowEma_ = 0.0;
    double lastClosedPrice_ = 0.0;
    bool biasLong_ = false;
    bool biasShort_ = false;
    bool spreadOk_ = false;

    // protect from duplicate actions on same forming candle
    qint64 lastEntryBarTs_ = -1;
    qint64 lastExitBarTs_  = -1;

    // track only the movement AFTER entry on the entry bar
    qint64 entryBarTs_ = -1;
    double postEntryHigh_ = 0.0;
    double postEntryLow_ = 0.0;
    bool postEntryRangeValid_ = false;

    static double bpsToFrac(int bps) {
        return static_cast<double>(bps) / 10000.0;
    }

    bool enteredThisBar(qint64 ts) const {
        return lastEntryBarTs_ == ts;
    }

    bool exitedThisBar(qint64 ts) const {
        return lastExitBarTs_ == ts;
    }

    bool canEnterThisBar(qint64 ts) const {
        return !enteredThisBar(ts) && !exitedThisBar(ts);
    }

    bool canExitThisBar(qint64 ts) const {
        return !exitedThisBar(ts);
    }

    void markEntryThisBar(qint64 ts) {
        lastEntryBarTs_ = ts;
    }

    void markExitThisBar(qint64 ts) {
        lastExitBarTs_ = ts;
    }

    void initPostEntryRange(qint64 barTs, double px) {
        entryBarTs_ = barTs;
        postEntryHigh_ = px;
        postEntryLow_ = px;
        postEntryRangeValid_ = true;
    }

    void resetPostEntryRange() {
        entryBarTs_ = -1;
        postEntryHigh_ = 0.0;
        postEntryLow_ = 0.0;
        postEntryRangeValid_ = false;
    }

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

        indicatorsReady_ = false;
        lastFastEma_ = 0.0;
        lastSlowEma_ = 0.0;
        lastClosedPrice_ = 0.0;
        biasLong_ = false;
        biasShort_ = false;
        spreadOk_ = false;

        lastEntryBarTs_ = -1;
        lastExitBarTs_  = -1;

        resetPostEntryRange();
    }

    std::vector<StrategySignal> onCandleClosed(const StrategyContext& ctx, const Candle& closed) override {
        std::vector<StrategySignal> out;
        if (!ctx.series) return out;

        const auto& candles = ctx.series->getCandles();
        if (candles.size() < static_cast<size_t>(slow_ + 3)) {
            indicatorsReady_ = false;
            return out;
        }

        const auto emaFast = EMA::calculate(candles, fast_);
        const auto emaSlow = EMA::calculate(candles, slow_);
        if (emaFast.empty() || emaSlow.empty()) {
            indicatorsReady_ = false;
            return out;
        }

        lastFastEma_ = emaFast.back();
        lastSlowEma_ = emaSlow.back();
        lastClosedPrice_ = closed.close_;

        if (lastClosedPrice_ <= 0.0) {
            indicatorsReady_ = false;
            return out;
        }

        const double spread = std::abs(lastFastEma_ - lastSlowEma_);
        const double spreadBps = (spread / lastClosedPrice_) * 10000.0;

        spreadOk_ = spreadBps >= static_cast<double>(minSpreadBps_);
        biasLong_ = (lastFastEma_ > lastSlowEma_);
        biasShort_ = (lastFastEma_ < lastSlowEma_);

        indicatorsReady_ = true;

        if (cooldownBars_ > 0) {
            --cooldownBars_;
        }

        if (inLong_ || inShort_) {
            ++barsInTrade_;
        }

        return out;
    }

    std::vector<StrategySignal> onCandleUpdated(const StrategyContext& ctx, const Candle& forming) override {
        std::vector<StrategySignal> out;
        if (!indicatorsReady_) return out;

        const double px = forming.close_;
        if (px <= 0.0) return out;

        const qint64 curBarTs = forming.timestamp_;

        auto mk = [&](StrategySignalType type, const char* reason) {
            StrategySignal s;
            s.type = type;
            s.symbolId = ctx.symbolId;
            s.tf = ctx.tf;
            s.timestamp = forming.timestamp_;
            s.reason = reason;
            return s;
        };

        const double tp = bpsToFrac(tpBps_);
        const double sl = bpsToFrac(slBps_);

        // =========================================================
        // 1) MANAGE OPEN POSITION
        // =========================================================
        if (inLong_ || inShort_) {
            bool hitTp = false;
            bool hitSl = false;

            const bool sameBarAsEntry = (entryBarTs_ == curBarTs);

            if (sameBarAsEntry && postEntryRangeValid_) {
                // On the entry bar, accumulate only prices seen AFTER entry.
                postEntryHigh_ = std::max(postEntryHigh_, px);
                postEntryLow_  = std::min(postEntryLow_,  px);
            }

            if (entryPrice_ > 0.0) {
                if (inLong_) {
                    const double tpPrice = entryPrice_ * (1.0 + tp);
                    const double slPrice = entryPrice_ * (1.0 - sl);

                    if (sameBarAsEntry && postEntryRangeValid_) {
                        hitTp = (postEntryHigh_ >= tpPrice);
                        hitSl = (postEntryLow_  <= slPrice);
                    } else {
                        hitTp = (forming.high_ >= tpPrice);
                        hitSl = (forming.low_  <= slPrice);
                    }
                } else if (inShort_) {
                    const double tpPrice = entryPrice_ * (1.0 - tp);
                    const double slPrice = entryPrice_ * (1.0 + sl);

                    if (sameBarAsEntry && postEntryRangeValid_) {
                        hitTp = (postEntryLow_  <= tpPrice);
                        hitSl = (postEntryHigh_ >= slPrice);
                    } else {
                        hitTp = (forming.low_  <= tpPrice);
                        hitSl = (forming.high_ >= slPrice);
                    }
                }
            }

            const bool timeout = (barsInTrade_ >= maxBarsInTrade_);
            const bool flipToShort = inLong_  && flipOnBiasChange_ && biasShort_;
            const bool flipToLong  = inShort_ && flipOnBiasChange_ && biasLong_;

            if (!canExitThisBar(curBarTs)) {
                return out;
            }

            if (inLong_ && (hitTp || hitSl || timeout || flipToShort)) {
                out.push_back(mk(StrategySignalType::ExitLong,
                                 hitTp ? "TP -> exit long" :
                                 hitSl ? "SL -> exit long" :
                                 timeout ? "Timeout -> exit long" :
                                           "Bias flip -> exit long"));

                inLong_ = false;
                inShort_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;
                cooldownBars_ = cooldownAfterExit_;

                markExitThisBar(curBarTs);
                resetPostEntryRange();
                return out;
            }

            if (inShort_ && (hitTp || hitSl || timeout || flipToLong)) {
                out.push_back(mk(StrategySignalType::ExitShort,
                                 hitTp ? "TP -> exit short" :
                                 hitSl ? "SL -> exit short" :
                                 timeout ? "Timeout -> exit short" :
                                           "Bias flip -> exit short"));

                inLong_ = false;
                inShort_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;
                cooldownBars_ = cooldownAfterExit_;

                markExitThisBar(curBarTs);
                resetPostEntryRange();
                return out;
            }

            return out;
        }

        // =========================================================
        // 2) FLAT: COOLDOWN
        // =========================================================
        if (cooldownBars_ > 0) {
            return out;
        }

        if (!canEnterThisBar(curBarTs)) {
            return out;
        }

        // =========================================================
        // 3) FLAT: ENTRIES INTRABAR
        // =========================================================
        if (!spreadOk_) return out;

        const double dist = std::abs(px - lastFastEma_);
        const double distBps = (dist / px) * 10000.0;
        const bool nearFast = distBps <= static_cast<double>(enterBandBps_);

        if (!nearFast) return out;

        const bool pullbackOkLong =
            (!requirePullbackTick_) || (lastClosedPrice_ > 0.0 && px <= lastClosedPrice_);

        const bool pullbackOkShort =
            (!requirePullbackTick_) || (lastClosedPrice_ > 0.0 && px >= lastClosedPrice_);

        if (biasLong_ && allowLong_ && (px <= lastFastEma_) && pullbackOkLong) {
            out.push_back(mk(StrategySignalType::EnterLong,
                             "Scalp: bias long + touch fast EMA -> enter long"));

            inLong_ = true;
            inShort_ = false;
            entryPrice_ = px;
            barsInTrade_ = 0;

            markEntryThisBar(curBarTs);
            initPostEntryRange(curBarTs, px);
            return out;
        }

        if (biasShort_ && allowShort_ && (px >= lastFastEma_) && pullbackOkShort) {
            out.push_back(mk(StrategySignalType::EnterShort,
                             "Scalp: bias short + touch fast EMA -> enter short"));

            inLong_ = false;
            inShort_ = true;
            entryPrice_ = px;
            barsInTrade_ = 0;

            markEntryThisBar(curBarTs);
            initPostEntryRange(curBarTs, px);
            return out;
        }

        return out;
    }
};
