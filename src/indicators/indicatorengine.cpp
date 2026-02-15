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

