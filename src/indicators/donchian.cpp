#include "donchian.h"
#include <limits>
#include <algorithm>

Donchian::DonchianResult Donchian::calculate(const QList<Candle>& candles, int period)
{
    const int n = candles.size();

    DonchianResult r;
    r.upper  = QVector<double>(n, std::numeric_limits<double>::quiet_NaN());
    r.lower  = QVector<double>(n, std::numeric_limits<double>::quiet_NaN());
    r.middle = QVector<double>(n, std::numeric_limits<double>::quiet_NaN());

    if (n <= 0 || period <= 0 || n < period) {
        return r;
    }

    // первая точка, где можно посчитать окно period: i = period-1
    for (int i = period - 1; i < n; ++i) {

        double hi = candles[i].high_;
        double lo = candles[i].low_;

        const int j0 = i - period + 1;   // гарантированно >= 0
        for (int j = j0; j <= i; ++j) {  // гарантированно <= n-1
            hi = std::max(hi, candles[j].high_);
            lo = std::min(lo, candles[j].low_);
            Q_ASSERT(i >= 0 && i < n);
            Q_ASSERT(i - period + 1 >= 0);
        }

        r.upper[i]  = hi;
        r.lower[i]  = lo;
        r.middle[i] = 0.5 * (hi + lo);
    }

    return r;
}
