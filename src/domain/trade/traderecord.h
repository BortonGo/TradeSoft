#pragma once
#include <QString>
#include <QDateTime>

enum class TradeSide {
    Buy,
    Sell
};

enum class TradeStatus {
    Open,
    Closed,
    PartiallyFilled,
    Rejected
};

struct TradeRecord {
    QDateTime time;
    QString symbol;
    TradeSide side = TradeSide::Buy;
    double qty = 0.0;
    double price = 0.0;
    double fee = 0.0;
    TradeStatus status = TradeStatus::Open;
};
