#pragma once
#include <QString>
#include <QDateTime>
#include "domain/trade/traderecord.h"

struct Fill {
    QDateTime time;
    QString symbol;
    TradeSide side = TradeSide::Buy;

    double qty = 0.0;
    double price = 0.0;

    double fee = 0.0;
    bool reduceOnly = false;
};
