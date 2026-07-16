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

    static constexpr std::size_t kMaxSamplesPerMetric = 4096;

public:
    LatencyCollector() : tickToStrategy_(kMaxSamplesPerMetric),
                        strategyToOrder_(kMaxSamplesPerMetric),
                        orderToFill_(kMaxSamplesPerMetric) {}

    void clear() {
        tickToStrategy_.clear();
        strategyToOrder_.clear();
        orderToFill_.clear();
    }

    void recordTickToStrategy(const LatencyTimestamp& latency) {
        if (latency.strategyDoneNs < latency.receivedNs || latency.strategyDoneNs == 0 || latency.receivedNs == 0) return;
        tickToStrategy_.addSample(latency.strategyDoneNs - latency.receivedNs);
    }

    void recordStrategyToOrder(const LatencyTimestamp& latency) {
        if (latency.orderCreatedNs < latency.strategyDoneNs || latency.orderCreatedNs == 0 || latency.strategyDoneNs == 0) return;
        strategyToOrder_.addSample(latency.orderCreatedNs - latency.strategyDoneNs);
    }

    void recordOrderToFill(const LatencyTimestamp& latency) {
        if (latency.fillHandledNs < latency.orderCreatedNs || latency.fillHandledNs == 0 || latency.orderCreatedNs == 0) return;
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
