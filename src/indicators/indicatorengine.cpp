#include "indicatorengine.h"

void IndicatorEngine::setEnabled(IndicatorId id, bool on){
    if (on) {
        enabled_.insert(id);
    }
    else {
        enabled_.remove(id);
    }
}

bool IndicatorEngine::isEnabled(IndicatorId Id) const {
    return enabled_.contains(Id);
}

void IndicatorEngine::rebuild(const CandleSeries& series) {
    lines_.clear();

    const QList<Candle> c = series.getCandles();
    const int n = c.size();
    if (n <= 0) {
        return;
    }

    if (isEnabled(IndicatorId::EMA20)) {
        IndicatorLine line;
        line.id_ = IndicatorId::EMA20;
        line.name_ = "EMA 20";
        line.values_ = EMA::calculate(c, 20);
        lines_.push_back(std::move(line));
    }

    if (isEnabled(IndicatorId::EMA50)) {
        IndicatorLine line;
        line.id_ = IndicatorId::EMA50;
        line.name_ = "EMA 50";
        line.values_ = EMA::calculate(c, 50);
        lines_.push_back(std::move(line));
    }
}

QVector<IndicatorLine> IndicatorEngine::overlayLines() const {
    return lines_;
}

QVector<double> IndicatorEngine::calcEma(const QList<Candle>& candles, int period) {
    const int n = candles.size();

    QVector<double> out(n, std::numeric_limits<double>::quiet_NaN());
    if (n <=0 || period <= 0) {
        return out;
    }

    if (n < period) {
        return out;
    }

    // start SMA
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += candles[i].close_;
    }

    double ema =sum / period;
    out[period - 1] = ema;

    // EMA
    const double k = 2.0 / (period + 1);
    for (int i = period; i < n; ++i) {
        const double price = candles[i].close_;
        ema = ema + k * (price - ema);
        out[i] = ema;
    }

    return out;
}
