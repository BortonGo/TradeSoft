#pragma once
#include "domain/strategy/istrategy.h"
#include "indicators/ema.h"
#include <QtGlobal>
#include <cmath>
#include <algorithm>

// EMA Cross + Pullback-to-50% candle strategy:
// 1) Ждём кросс fast/slow
// 2) После кросса ждём откат к 50% последней свечи (mid = (H+L)/2)
// 3) Входим, когда close дошёл до mid (на закрытии)
// 4) Выходим по TP/SL/таймауту, опционально flip при смене bias
class EmaPullbackStrategy final : public IStrategy {
    int fast_ = 5;
    int slow_ = 13;

    // ---- entry logic ----
    int maxPullbackBars_ = 3;      // сколько свечей после кросса ждём откат к mid
    int minSpreadBps_ = 2;         // анти-пила: насколько EMA должны разойтись (bps от цены)

    // ---- risk (adaptive) ----
    // SL = clamp(rangeBps * slFromRangeMul_, minSlBps_, maxSlBps_)
    // TP = clamp(SL * rr_, minTpBps_, maxTpBps_)
    double slFromRangeMul_ = 0.5;  // 0.5 = половина диапазона триггер-свечи
    double rr_ = 1.5;              // risk-reward
    int minSlBps_ = 6;
    int maxSlBps_ = 25;
    int minTpBps_ = 6;
    int maxTpBps_ = 40;

    int maxBarsInTrade_ = 5;
    bool flipOnBiasChange_ = true;

    bool allowLong_ = true;
    bool allowShort_ = true;

    // ---- local state (MVP) ----
    bool inLong_ = false;
    bool inShort_ = false;
    double entryPrice_ = 0.0;
    int barsInTrade_ = 0;

    // фиксируем TP/SL при входе (в долях, например 0.0012)
    double tpFrac_ = 0.0;
    double slFrac_ = 0.0;

    enum class PendingDir { None, Long, Short };
    PendingDir pending_ = PendingDir::None;
    double pendingLevel_ = 0.0;       // mid = (H+L)/2 триггер-свечи
    double pendingTrigRange_ = 0.0;   // (H-L) триггер-свечи
    int pendingBars_ = 0;

    static double clampd(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

public:
    EmaPullbackStrategy(int fast, int slow,
                        int maxBarsInTrade,
                        int minSpreadBps,
                        int maxPullbackBars,
                        double slFromRangeMul,
                        double rr,
                        int minSlBps, int maxSlBps,
                        int minTpBps, int maxTpBps,
                        bool flipOnBiasChange,
                        bool allowLong, bool allowShort)
        : fast_(fast),
          slow_(slow),

          maxPullbackBars_(maxPullbackBars),
          minSpreadBps_(minSpreadBps),

          slFromRangeMul_(slFromRangeMul),
          rr_(rr),
          minSlBps_(minSlBps),
          maxSlBps_(maxSlBps),
          minTpBps_(minTpBps),
          maxTpBps_(maxTpBps),

          maxBarsInTrade_(maxBarsInTrade),
          flipOnBiasChange_(flipOnBiasChange),
          allowLong_(allowLong),
          allowShort_(allowShort)
    {}

    void onStart(const StrategyContext& ctx) override {
        Q_UNUSED(ctx);
        inLong_ = false;
        inShort_ = false;
        entryPrice_ = 0.0;
        barsInTrade_ = 0;
        tpFrac_ = 0.0;
        slFrac_ = 0.0;

        pending_ = PendingDir::None;
        pendingLevel_ = 0.0;
        pendingTrigRange_ = 0.0;
        pendingBars_ = 0;
    }

    std::vector<StrategySignal> onCandleClosed(const StrategyContext& ctx, const Candle& closed) override {
        std::vector<StrategySignal> out;
        if (!ctx.series) return out;

        const auto& candles = ctx.series->getCandles();
        if (candles.size() < static_cast<size_t>(slow_ + 3)) return out;

        const std::vector<double> emaFast = EMA::calculate(candles, fast_);
        const std::vector<double> emaSlow = EMA::calculate(candles, slow_);
        if (emaFast.size() < 2 || emaSlow.size() < 2) return out;

        const double fNow  = emaFast.back();
        const double sNow  = emaSlow.back();
        const double fPrev = emaFast[emaFast.size() - 2];
        const double sPrev = emaSlow[emaSlow.size() - 2];

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

        // bias / cross
        const bool biasLong  = (fNow > sNow);
        const bool biasShort = (fNow < sNow);

        const bool crossUp = (fPrev <= sPrev) && (fNow > sNow);
        const bool crossDn = (fPrev >= sPrev) && (fNow < sNow);

        // ---------------- manage open position ----------------
        if (inLong_ || inShort_) {
            barsInTrade_++;

            double ret = 0.0;
            if (entryPrice_ > 0.0) {
                if (inLong_)  ret = (px - entryPrice_) / entryPrice_;
                if (inShort_) ret = (entryPrice_ - px) / entryPrice_;
            }

            const bool hitTp = (tpFrac_ > 0.0) && (ret >= tpFrac_);
            const bool hitSl = (slFrac_ > 0.0) && (ret <= -slFrac_);
            const bool timeout = (barsInTrade_ >= maxBarsInTrade_);

            const bool biasFlipToShort = inLong_  && flipOnBiasChange_ && biasShort;
            const bool biasFlipToLong  = inShort_ && flipOnBiasChange_ && biasLong;

            if (inLong_ && (hitTp || hitSl || timeout || biasFlipToShort)) {
                out.push_back(mk(StrategySignalType::ExitLong,
                                 hitTp ? "TP -> exit long" :
                                 hitSl ? "SL -> exit long" :
                                 timeout ? "Timeout -> exit long" :
                                 "Bias flip -> exit long"));

                inLong_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;
                tpFrac_ = 0.0;
                slFrac_ = 0.0;

                // optional immediate reverse (по bias + spreadOk)
                if (biasFlipToShort && allowShort_ && spreadOk) {
                    out.push_back(mk(StrategySignalType::EnterShort, "Flip -> enter short"));
                    inShort_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;

                    // риск при флипе: берём текущую свечу как триггер (быстро и стабильно)
                    const double trigRange = std::max(0.0, closed.high_ - closed.low_);
                    const double rangeBps2 = (trigRange / px) * 10000.0;
                    const double slBps = clampd(rangeBps2 * slFromRangeMul_, minSlBps_, maxSlBps_);
                    const double tpBps = clampd(slBps * rr_, minTpBps_, maxTpBps_);
                    slFrac_ = slBps / 10000.0;
                    tpFrac_ = tpBps / 10000.0;
                }
                return out;
            }

            if (inShort_ && (hitTp || hitSl || timeout || biasFlipToLong)) {
                out.push_back(mk(StrategySignalType::ExitShort,
                                 hitTp ? "TP -> exit short" :
                                 hitSl ? "SL -> exit short" :
                                 timeout ? "Timeout -> exit short" :
                                 "Bias flip -> exit short"));

                inShort_ = false;
                entryPrice_ = 0.0;
                barsInTrade_ = 0;
                tpFrac_ = 0.0;
                slFrac_ = 0.0;

                if (biasFlipToLong && allowLong_ && spreadOk) {
                    out.push_back(mk(StrategySignalType::EnterLong, "Flip -> enter long"));
                    inLong_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;

                    const double trigRange = std::max(0.0, closed.high_ - closed.low_);
                    const double rangeBps2 = (trigRange / px) * 10000.0;
                    const double slBps = clampd(rangeBps2 * slFromRangeMul_, minSlBps_, maxSlBps_);
                    const double tpBps = clampd(slBps * rr_, minTpBps_, maxTpBps_);
                    slFrac_ = slBps / 10000.0;
                    tpFrac_ = tpBps / 10000.0;
                }
                return out;
            }

            return out; // still holding
        }

        // ---------------- no position: pending pullback entry ----------------
        if (pending_ != PendingDir::None) {
            pendingBars_++;

            if (pendingBars_ > maxPullbackBars_) {
                // протух сигнал
                pending_ = PendingDir::None;
                pendingLevel_ = 0.0;
                pendingTrigRange_ = 0.0;
                pendingBars_ = 0;
                return out;
            }

            if (!spreadOk) return out;

            // long: ждём откат вниз к mid
            if (pending_ == PendingDir::Long && allowLong_) {
                if (px <= pendingLevel_) {
                    out.push_back(mk(StrategySignalType::EnterLong, "Cross + pullback to 50% -> enter long"));
                    inLong_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;

                    const double trigRange = std::max(0.0, pendingTrigRange_);
                    const double rangeBps = (trigRange / px) * 10000.0;
                    const double slBps = clampd(rangeBps * slFromRangeMul_, minSlBps_, maxSlBps_);
                    const double tpBps = clampd(slBps * rr_, minTpBps_, maxTpBps_);
                    slFrac_ = slBps / 10000.0;
                    tpFrac_ = tpBps / 10000.0;

                    pending_ = PendingDir::None;
                    pendingLevel_ = 0.0;
                    pendingTrigRange_ = 0.0;
                    pendingBars_ = 0;
                    return out;
                }
            }

            // short: ждём откат вверх к mid
            if (pending_ == PendingDir::Short && allowShort_) {
                if (px >= pendingLevel_) {
                    out.push_back(mk(StrategySignalType::EnterShort, "Cross + pullback to 50% -> enter short"));
                    inShort_ = true;
                    entryPrice_ = px;
                    barsInTrade_ = 0;

                    const double trigRange = std::max(0.0, pendingTrigRange_);
                    const double rangeBps = (trigRange / px) * 10000.0;
                    const double slBps = clampd(rangeBps * slFromRangeMul_, minSlBps_, maxSlBps_);
                    const double tpBps = clampd(slBps * rr_, minTpBps_, maxTpBps_);
                    slFrac_ = slBps / 10000.0;
                    tpFrac_ = tpBps / 10000.0;

                    pending_ = PendingDir::None;
                    pendingLevel_ = 0.0;
                    pendingTrigRange_ = 0.0;
                    pendingBars_ = 0;
                    return out;
                }
            }

            return out; // ждём дальше
        }

        // ---------------- no position: arm signal on CROSS ----------------
        if (!spreadOk) return out;

        const double mid = 0.5 * (closed.high_ + closed.low_);
        const double trigRange = std::max(0.0, closed.high_ - closed.low_);

        if (crossUp && allowLong_) {
            pending_ = PendingDir::Long;
            pendingLevel_ = mid;
            pendingTrigRange_ = trigRange;
            pendingBars_ = 0;
            return out;
        }

        if (crossDn && allowShort_) {
            pending_ = PendingDir::Short;
            pendingLevel_ = mid;
            pendingTrigRange_ = trigRange;
            pendingBars_ = 0;
            return out;
        }

        return out;
    }
};
