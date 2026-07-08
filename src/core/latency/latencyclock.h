#pragma once
#include <chrono>
#include <cstdint>

struct LatencyClock final {
    static std::uint64_t nowNs() {
        using clock = std::chrono::steady_clock;
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                clock::now().time_since_epoch()).count());
    }
};
