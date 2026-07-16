#pragma once
#include <vector>
#include <optional>
#include "core/candle.h"

class EMA final {
public:
    static std::vector<double> calculate(const std::vector<Candle>& candles, int period);
};

class EmaState final {
    int period_;
    int warmupCount_ = 0;
    double warmupSum_ = 0.0;
    std::optional<double> currentEma_;
public:
    explicit EmaState(int period);
    void reset();
    std::optional<double> update(double price);
    bool isReady() const noexcept;
    std::optional<double> value() const;
};
