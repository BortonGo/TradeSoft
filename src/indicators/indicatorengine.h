#pragma once
#include "indicatortypes.h"
#include "src/core/candleseries.h"
#include "ema.h"
#include "donchian.h"
#include "rsi.h"
#include "atr.h"
#include <QSet>

class IndicatorEngine final {
    QSet<IndicatorId> enabled_;
    std::vector<IndicatorLine> lines_;

public:
    void setEnabled(IndicatorId id, bool on);
    bool isEnabled(IndicatorId Id) const;

    void rebuild(const CandleSeries& series);

    std::vector<IndicatorLine> overlayLines() const;
};

