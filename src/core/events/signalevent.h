#pragma once
#include "core/events/latencytimestamp.h"
#include "domain/strategy/strategysignal.h"

struct SignalEvent final {
    StrategySignal signal;
    LatencyTimestamp latency {};
};
