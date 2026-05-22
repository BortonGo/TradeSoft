#pragma once

#include "core/candle.h"
#include "core/timeframe.h"

#include <QString>
#include <vector>

class CandleCache final {
public:
    static std::vector<Candle> load(const QString& symbolId, Timeframe tf);
    static bool save(const QString& symbolId, Timeframe tf, const std::vector<Candle>& candles);
    static bool isFresh(const std::vector<Candle>& candles, Timeframe tf);
    static QString filePath(const QString& symbolId, Timeframe tf);
};
