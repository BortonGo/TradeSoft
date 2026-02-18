#include "rsi.h"
#include <limits>
#include <cmath>

std::vector<double> RSI::calculate(const std::vector<Candle>& candles, int period) {
    const int n = candles.size();
    std::vector<double> out(n, std::numeric_limits<double>::quiet_NaN());
    if (n < period || period <= 0) return out;

    // Wilder RSI
    double gainSum = 0.0;
    double lossSum = 0.0;

    // initial average over first period
    for (int i = 1; i <= period; ++i) {
        const double ch = candles[i].close_ - candles[i - 1].close_;
        if (ch > 0) {
            gainSum += ch;
        }
        else {
            lossSum -= ch;
        }
    }

    double avgGain = gainSum / period;
    double avgLoss = lossSum / period;

    auto rsiFrom = [](double g, double l) -> double {
        if (l <= 0.0) {
            return 100.0;
        }
        const double rs = g / l;
        return 100.0 - (100 / (1 + rs));
    };

    out[period] = rsiFrom(avgGain, avgLoss);

    // Wilder smoothing
    for (int i = period + 1; i < n; ++i) {
        const double ch = candles[i].close_ - candles[i - 1].close_;
        const double gain = (ch > 0) ? ch : 0.0;
        const double loss = (ch < 0) ? -ch : 0.0;

        avgGain = (avgGain * (period - 1) + gain) / period;
        avgLoss = (avgLoss * (period - 1) + loss) / period;

        out[i] = rsiFrom(avgGain, avgLoss);
    }

    return out;
}

