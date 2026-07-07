#pragma once
#include "core/events/latencytimestamp.h"
#include "domain/trade/traderecord.h"

enum class TradeEventType {
    Added,
    Updated,
    Closed
};

struct TradeEvent final {
    TradeEventType type = TradeEventType::Added;
    int row = -1;
    TradeRecord trade {};
    LatencyTimestamp latency {};
};
