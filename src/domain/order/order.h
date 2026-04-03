#pragma once
#include <QString>
#include "domain/trade/traderecord.h"
#include <cstdint>

enum class OrderType {
    Market,
    Limit
};

struct Order final {
    QString symbol;
    TradeSide side = TradeSide::Buy;
    OrderType type = OrderType::Market;

    double qty = 0.0;
    double limitPrice = 0.0;
    bool reduceOnly = false;

    int64_t clientOrderId = 0;
};
