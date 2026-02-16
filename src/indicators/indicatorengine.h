#pragma once
#include "indicatortypes.h"
#include "src/core/candleseries.h"
#include "ema.h"
#include "donchian.h"
#include <QSet>

class IndicatorEngine
{
    QSet<IndicatorId> enabled_;
    QVector<IndicatorLine> lines_;

public:
    void setEnabled(IndicatorId id, bool on);
    bool isEnabled(IndicatorId Id) const;

    void rebuild(const CandleSeries& series);

    QVector<IndicatorLine> overlayLines() const;
};

