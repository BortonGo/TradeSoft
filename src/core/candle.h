#pragma once
#include <cstdint>

struct Candle {
    int64_t timestamp_;

    double open_;
    double high_;
    double low_;
    double close_;

    double volume_;
    bool isFinal_;
};
