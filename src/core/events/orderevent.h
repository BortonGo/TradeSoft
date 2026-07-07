#pragma once
#include "domain/order/order.h"
#include "core/events/latencytimestamp.h"
#include <QString>

enum class OrderEventType {
    Created,
    RiskRejected,
    Sent,
    Accepted,
    Rejected,
    Cancelled
};

struct OrderEvent final {
    OrderEventType type = OrderEventType::Created;
    Order order {};
    QString reason;
    LatencyTimestamp latency {};
};
