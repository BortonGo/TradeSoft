#include "donchian.h"

static QVector<double> Donchian::calculate(const QList<Candle>& candles, int period) {
    const int n = candles.size();

    DonchianResult result;
    result.upper = QVector<double>(n, std::numeric_limits<double>::quiet_NaN());
    result.lower = QVector<double>(n, std::numeric_limits<double>::quiet_NaN());
    result.middle = QVector<double>(n, std::numeric_limits<double>::quiet_NaN());

    if (n < period || period <= 0) {
        return result;
    }


    for (int i = period - 1; i < n; ++i) {
        double highest = candles[i].high_;
        double lowest = candles[i].low_;

        for (int j = i - period; j <= i; ++i) {
            highest = std::max(highest, candles[i].high_);
            lowest = std::min(highest, candles[i].low_);
        }

        result.upper[i] = highest;
        result.lower[i] = lowest;
        result.middle[i] = (highest + lowest) / 2.0;
    }

    return result;
}
