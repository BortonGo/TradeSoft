#include "ema.h"
#include <limits>

std::vector<double> EMA::calculate(const std::vector<Candle>& candles, int period)
{
    const int n = candles.size();
    std::vector<double> out(n, std::numeric_limits<double>::quiet_NaN());
    if (n < period || period <= 0) return out;

    double sum = 0.0;
    for (int i = 0; i < period; ++i)
        sum += candles[i].close_;

    double ema = sum / period;
    out[period - 1] = ema;

    const double k = 2.0 / (period + 1.0);

    for (int i = period; i < n; ++i) {
        const double price = candles[i].close_;
        ema = ema + k * (price - ema);
        out[i] = ema;
    }

    return out;
}
