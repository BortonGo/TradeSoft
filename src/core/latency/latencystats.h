#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

struct LatencyStatsSnapshot final {
    std::size_t count = 0;
    std::uint64_t minNs = 0;
    std::uint64_t p50Ns = 0;
    std::uint64_t p95Ns = 0;
    std::uint64_t p99Ns = 0;
    std::uint64_t maxNs = 0;
    std::uint64_t meanNs = 0;
};

class LatencyStats final {
    std::vector<std::uint64_t> samples_;

    std::uint64_t percentile(std::size_t p) const {
        if (p < 0.0 || p > 100.0) throw std::out_of_range("p < 0 or p > 100");
        if (samples_.empty()) return 0;
        std::vector<std::uint64_t> s = samples_;
        std::sort(s.begin(), s.end());
        if (p == 0.0) return s.front();
        auto idx = static_cast<std::size_t>(std::ceil((p / 100.0) * s.size()) - 1);
        if (idx > s.size() - 1) {
            idx = static_cast<int>(s.size()) - 1;
        }
        return s[idx];
    }

public:
    void reserve(std::size_t capacity) {
        samples_.reserve(capacity);
    }
    void addSample(std::uint64_t latencyNs) {
        samples_.push_back(latencyNs);
    }
    void clear() {
        samples_.clear();
    }

    std::size_t count() const noexcept {
        return samples_.size();
    }
    bool empty() const noexcept {
        return samples_.empty();
    }

    LatencyStatsSnapshot snapshot() const {
        LatencyStatsSnapshot snap {};
        if (samples_.empty()) return snap;
        snap.count = samples_.size();
        snap.minNs = *min_element(samples_.begin(), samples_.end());
        snap.maxNs = *max_element(samples_.begin(), samples_.end());
        snap.p50Ns = percentile(50);
        snap.p95Ns = percentile(95);
        snap.p99Ns = percentile(99);
        auto acc = std::accumulate(samples_.begin(), samples_.end(), std::uint64_t{0});
        snap.meanNs = acc/snap.count;
        return snap;
    }
};
