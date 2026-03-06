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
    int tpBps_ = 25;
    int slBps_ = 10;

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
    int cooldownAfterExit_ = 1;

    // indicator state, updated only on closed candles
    bool indicatorsReady_ = false;
    double lastFastEma_ = 0.0;
    double lastSlowEma_ = 0.0;
    double lastClosedPrice_ = 0.0;
    bool biasLong_ = false;
    bool biasShort_ = false;
    bool spreadOk_ = false;

    // protect from duplicate actions on same forming candle
    qint64 lastActionBarTs_ = -1;

    static double bpsToFrac(int bps) {
        return static_cast<double>(bps) / 10000.0;
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
        lastActionBarTs_ = -1;
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

        // Считаем, что на новом закрытом баре "тик-счётчик" для действий можно сбросить
        lastActionBarTs_ = -1;

        if (cooldownBars_ > 0) {
            --cooldownBars_;
        }

        // barsInTrade_ увеличиваем только по закрытым свечам
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

        auto markAction = [&]() {
            lastActionBarTs_ = curBarTs;
        };

        const double tp = bpsToFrac(tpBps_);
        const double sl = bpsToFrac(slBps_);

        // ---------------- manage open position intrabar ----------------
        if (inLong_ || inShort_) {
            if (lastActionBarTs_ == curBarTs) {
                return out;
            }

            double ret = 0.0;
            if (entryPrice_ > 0.0) {
                if (inLong_) {
                    ret = (px - entryPrice_) / entryPrice_;
                } else if (inShort_) {
                    ret = (entryPrice_ - px) / entryPrice_;
                }
            }

            const bool hitTp = (ret >= tp);
            const bool hitSl = (ret <= -sl);
            const bool timeout = (barsInTrade_ >= maxBarsInTrade_);

            const bool flipToShort = inLong_  && flipOnBiasChange_ && biasShort_;
            const bool flipToLong  = inShort_ && flipOnBiasChange_ && biasLong_;

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
                markAction();

                if (flipToShort && allowShort_ && spreadOk_) {
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
                markAction();

                if (flipToLong && allowLong_ && spreadOk_) {
                    out.push_back(mk(StrategySignalType::EnterLong, "Flip -> enter long"));
                    inLong_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;
                    cooldownBars_ = 0;
                }

                return out;
            }

            return out;
        }

        // ---------------- flat: cooldown ----------------
        if (cooldownBars_ > 0) {
            return out;
        }

        if (lastActionBarTs_ == curBarTs) {
            return out;
        }

        // ---------------- flat: entries intrabar ----------------
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
            entryPrice_ = px;
            barsInTrade_ = 0;
            markAction();
            return out;
        }

        if (biasShort_ && allowShort_ && (px >= lastFastEma_) && pullbackOkShort) {
            out.push_back(mk(StrategySignalType::EnterShort,
                             "Scalp: bias short + touch fast EMA -> enter short"));
            inShort_ = true;
            entryPrice_ = px;
            barsInTrade_ = 0;
            markAction();
            return out;
        }

        return out;
    }
};
