#include "fakeexchangeclient.h"
#include "core\timeframe.h"
#include <QDateTime>
#include <QtGlobal>

#include <random>
#include <algorithm>
#include <cstdlib>

QList<Candle> FakeExchangeClient::fetchKlines(const QString& symbolId, Timeframe tf) {
    Q_UNUSED(symbolId);

    const int N = 500;
    const int64_t stepsMs = timeframeToMs(tf);

    int64_t startPlotTime = QDateTime::currentMSecsSinceEpoch() - static_cast<int64_t>(N) * stepsMs;

    double startPlotPrice = 20000.0; // условно на данном этапе

    std::mt19937 rng{std::random_device{}()};

    std::normal_distribution<double> deltaDist(0.0, 15.0);

    std::uniform_real_distribution<double> wickDist(0.0, 8.0);

    std::uniform_real_distribution<double> volDist(10.0, 200.0);

    QList<Candle> out;
    out.reserve(N);

    startPlotTime = (startPlotTime / stepsMs) * stepsMs;

    for(int i = 0; i < N; ++i){
        const double open = startPlotPrice;
        const double close = startPlotPrice + deltaDist(rng);

        const double hiBase = std::max(open, close);
        const double loBase = std::min(open, close);

        const double high = hiBase + wickDist(rng);
        const double low = loBase - wickDist(rng);

        Candle c;
        c.timestamp_ = startPlotTime + static_cast<int64_t>(i) * stepsMs;
        c.open_ = open;
        c.close_ = close;
        c.high_ = high;
        c.low_ = low;
        c.volume_ = volDist(rng);
        c.isFinal_ = true;

        out.push_back(c);

        startPlotPrice = close;
    }

    return out;
}

bool FakeExchangeClient::fetchLastKline(const QString& symbolId, Timeframe tf, Candle& out) {
    auto list = fetchKlines(symbolId, tf);
    if (list.isEmpty()) return false;
    out = list.last();
    return true;
}


