#pragma once
#include "indicatortypes.h"
#include "src/core/candleseries.h"
#include <QSet>

class IndicatorEngine
{
    QSet<IndicatorId> enabled_;
    QVector<IndicatorLine> lines_;

public:
    IndicatorEngine();

    void setEnabled(IndicatorId id, bool on);
    void isEnabled(Indicator Id) const;

    void rebuild(const CandleSeries& series);

    QVector<IndicatorLine> overlayLines() const;

private:
    static QVector<double> calcEma(const QList<Candle>& candles, int period);
};

