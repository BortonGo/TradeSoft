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

struct TradeRecord final {
    QDateTime time;
    QDateTime closeTime;
    QString symbol;

    double qty = 0.0;
    double price = 0.0;
    double closePrice = 0.0;
    double pnl = 0.0;
    double fee = 0.0;
    double tpPrice = 0.0;
    double slPrice = 0.0;

    TradeSide side = TradeSide::Buy;
    TradeStatus status = TradeStatus::Open;
    int lifetimeTicks = 0;
};
