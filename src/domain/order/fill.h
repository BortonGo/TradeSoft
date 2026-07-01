#pragma once
#include <QString>
#include <QDateTime>
#include "domain/trade/traderecord.h"

struct Fill final {
    QDateTime time;
    QString symbol;

    double qty = 0.0;
    double price = 0.0;
    double fee = 0.0;
    double tpPrice = 0.0;
    double slPrice = 0.0;

    TradeSide side = TradeSide::Buy;
    bool reduceOnly = false;
};
