#include "donchian.h"
#include <limits>
#include <algorithm>

Donchian::DonchianResult Donchian::calculate(const std::vector<Candle>& candles, int period)
{
    const int n = candles.size();

    DonchianResult r;
    r.upper  = std::vector<double>(n, std::numeric_limits<double>::quiet_NaN());
    r.lower  = std::vector<double>(n, std::numeric_limits<double>::quiet_NaN());
    r.middle = std::vector<double>(n, std::numeric_limits<double>::quiet_NaN());

    if (n <= 0 || period <= 0 || n < period) {
        return r;
    }

    for (int i = period - 1; i < n; ++i) {

        double hi = candles[i].high_;
        double lo = candles[i].low_;

        const int j0 = i - period + 1;
        for (int j = j0; j <= i; ++j) {
            hi = std::max(hi, candles[j].high_);
            lo = std::min(lo, candles[j].low_);
        }

        r.upper[i]  = hi;
        r.lower[i]  = lo;
        r.middle[i] = 0.5 * (hi + lo);
    }

    return r;
}
