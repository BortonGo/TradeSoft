#pragma once
#include <cstdint>

struct Candle {
    int64_t timestamp;

    double open;
    double high;
    double low;
    double close;

    double volume;
    bool isFinal;
};
