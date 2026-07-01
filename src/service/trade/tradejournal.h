#pragma once
#include <QHash>
#include <QString>
#include <functional>
#include <vector>
#include "domain/order/fill.h"

struct TradeReport final {
    double equity = 0.0;
    double netPnl = 0.0;
    double fees = 0.0;

    int closedTrades = 0;
    int winTrades = 0;

    double maxDrawdown = 0.0;
};

class TradeJournal final {
public:
    using TradeAddedCallback = std::function<void (const TradeRecord&)>;
    using TradeUpdatedCallback = std::function<void (int, const TradeRecord&)>;

    explicit TradeJournal(double startEquityUsdt);

    void setTradeAddedCallback(TradeAddedCallback cb);
    void setTradeUpdatedCallback(TradeUpdatedCallback cb);

    bool hasOpen(const QString& symbol) const;
    TradeSide openSide(const QString& symbol) const;
    double openQty(const QString& symbol) const;

    // Apply execution result
    void onFill(const Fill& f);

    TradeReport report() const;
    double equity() const { return equity_; }

    void onPriceUpdate(const QString& symbol, double markPrice, double feePct);

private:
    std::vector<TradeRecord> trades_;

    TradeAddedCallback onTradeAdded_;
    TradeUpdatedCallback onTradeUpdated_;

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
};
