#pragma once
#include <cstdint>

struct LatencyTimestamp final {
    std::uint64_t receivedNs = 0;
    std::uint64_t parsedNs = 0;
    std::uint64_t strategyDoneNs = 0;
    std::uint64_t riskDoneNs = 0;
    std::uint64_t orderCreatedNs = 0;
    std::uint64_t fillHandledNs = 0;
};