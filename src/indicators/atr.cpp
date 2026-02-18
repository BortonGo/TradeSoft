#include "atr.h"
#include <limits>
#include <cmath>
#include <algorithm>

static inline double trueRange(const Candle& cur, const Candle& prev)
{
    const double hl = cur.high_ - cur.low_;
    const double hc = std::abs(cur.high_ - prev.close_);
    const double lc = std::abs(cur.low_  - prev.close_);
    return std::max(hl, std::max(hc, lc));
}

std::vector<double> ATR::calculate(const std::vector<Candle>& candles, int period)
{
    const int n = candles.size();
    std::vector<double> out(n, std::numeric_limits<double>::quiet_NaN());
    if (n <= period || period <= 0) return out;

    // First ATR = average TR for first period
    double trSum = 0.0;
    for (int i = 1; i <= period; ++i) {
        trSum += trueRange(candles[i], candles[i - 1]);
    }
    double atr = trSum / period;
    out[period] = atr;

    // Wilder smoothing
    for (int i = period + 1; i < n; ++i) {
        const double tr = trueRange(candles[i], candles[i - 1]);
        atr = (atr * (period - 1) + tr) / period;
        out[i] = atr;
    }

    return out;
}
