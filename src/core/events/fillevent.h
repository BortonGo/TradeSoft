#pragma once
#include "core/events/latencytimestamp.h"
#include "domain/order/fill.h"

struct FillEvent final {
    Fill fill {};
    LatencyTimestamp latency {};
};
