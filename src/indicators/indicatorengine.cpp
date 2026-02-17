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

    const QList<Candle>& c = series.getCandles();
    const int n = c.size();
    if (n <= 0) {
        return;
    }

    if (isEnabled(IndicatorId::EMA9)) {
        IndicatorLine line;
        line.id_ = IndicatorId::EMA9;
        line.name_ = "EMA 9";
        line.values_ = EMA::calculate(c, 9);
        lines_.push_back(std::move(line));
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

    if (isEnabled(IndicatorId::DON20)) {

        auto dc = Donchian::calculate(c, 20);

        IndicatorLine upper;
        upper.id_ = IndicatorId::DON20UPPER;
        upper.name_ = "Donchian 20 upper";
        upper.values_ = dc.upper;
        lines_.push_back(std::move(upper));

        IndicatorLine lower;
        lower.id_ = IndicatorId::DON20LOWER;
        lower.name_ = "Donchian 20 lower";
        lower.values_ = dc.lower;
        lines_.push_back(std::move(lower));

        IndicatorLine middle;
        middle.id_ = IndicatorId::DON20MIDDLE;
        middle.name_ = "Donchian 20 middle";
        middle.values_ = dc.middle;
        lines_.push_back(std::move(middle));
    }

    if (isEnabled(IndicatorId::RSI14)) {
        IndicatorLine line;
        line.id_ = IndicatorId::RSI14;
        line.name_ = "RSI 14";
        line.values_ = RSI::calculate(c, 14);
        lines_.push_back(std::move(line));
    }

    if (isEnabled(IndicatorId::ATR14)) {
        IndicatorLine line;
        line.id_ = IndicatorId::ATR14;
        line.name_ = "ATR 14";
        line.values_ = RSI::calculate(c, 14);
        lines_.push_back(std::move(line));
    }
}

QVector<IndicatorLine> IndicatorEngine::overlayLines() const {
    return lines_;
}

