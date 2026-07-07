#pragma once
#include "core/candle.h"
#include "core/events/latencytimestamp.h"
#include "core/timeframe.h"
#include <QString>

enum class MarketEventType {
    CandleUpdated,
    CandleClosed,
    SnapshotLoaded,
    ConnectionStateChanged
};

struct MarketEvent final {
    MarketEventType type = MarketEventType::CandleUpdated;
    QString symbolId;
    Timeframe timeframe {};
    Candle candle {};
    QString message;
    LatencyTimestamp latency {};
};
