#pragma once
#include <QHash>
#include <QString>
#include "ui/models/tradesmodel.h"
#include "domain/order/fill.h"

struct TradeReport {
    double equity = 0.0;
    double netPnl = 0.0;
    double fees = 0.0;

    int closedTrades = 0;
    int winTrades = 0;

    double maxDrawdown = 0.0;
};

class TradeJournal final {
    TradesModel* model_ = nullptr;

    // open trade row by symbol
    QHash<QString, int> openRowBySymbol_;

    // stats
    double startEquity_ = 0.0;
    double equity_ = 0.0;
    double peakEquity_ = 0.0;

    double netPnl_ = 0.0;
    double fees_ = 0.0;

    int closedTrades_ = 0;
    int winTrades_ = 0;
    double maxDD_ = 0.0;

public:
    explicit TradeJournal(TradesModel* model, double startEquityUsdt);

    bool hasOpen(const QString& symbol) const;
    TradeSide openSide(const QString& symbol) const;
    double openQty(const QString& symbol) const;

    // Apply execution result
    void onFill(const Fill& f);

    TradeReport report() const;
    double equity() const { return equity_; }

    void onPriceUpdate(const QString& symbol, double markPrice, double feePct);
};
