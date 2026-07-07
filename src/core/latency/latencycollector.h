#pragma once
#include "core/latency/latencystats.h"
#include "core/events/latencytimestamp.h"

struct LatencyCollectorSnapshot final {
    LatencyStatsSnapshot tickToStrategy;
    LatencyStatsSnapshot strategyToOrder;
    LatencyStatsSnapshot orderToFill;
};

class LatencyCollector final {
    LatencyStats tickToStrategy_;
    LatencyStats strategyToOrder_;
    LatencyStats orderToFill_;

public:
    void reserve(std::size_t capacity) {
        tickToStrategy_.reserve(capacity);
        strategyToOrder_.reserve(capacity);
        orderToFill_.reserve(capacity);
    }
    void clear() {
        tickToStrategy_.clear();
        strategyToOrder_.clear();
        orderToFill_.clear();
    }

    void recordTickToStrategy(const LatencyTimestamp& latency) {
        tickToStrategy_.addSample(latency.strategyDoneNs - latency.receivedNs);
    }
    void recordStrategyToOrder(const LatencyTimestamp& latency) {
        strategyToOrder_.addSample(latency.orderCreatedNs - latency.strategyDoneNs);
    }
    void recordOrderToFill(const LatencyTimestamp& latency) {
        orderToFill_.addSample(latency.fillHandledNs - latency.orderCreatedNs);
    }

    LatencyCollectorSnapshot snapshot() const {
        LatencyCollectorSnapshot snap {};
        snap.tickToStrategy = tickToStrategy_.snapshot();
        snap.strategyToOrder = strategyToOrder_.snapshot();
        snap.orderToFill = orderToFill_.snapshot();
        return snap;
    }
};
