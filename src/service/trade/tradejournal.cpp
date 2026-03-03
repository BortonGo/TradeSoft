#include "tradejournal.h"
#include <QtGlobal>
#include <cmath>

TradeJournal::TradeJournal(TradesModel* model, double startEquityUsdt) : model_(model), startEquity_(startEquityUsdt),
    equity_(startEquityUsdt), peakEquity_(startEquityUsdt) {
    Q_ASSERT(model_);
}

bool TradeJournal::hasOpen(const QString& symbol) const {
    return openRowBySymbol_.contains(symbol);
}

TradeSide TradeJournal::openSide(const QString& symbol) const {
    if (!openRowBySymbol_.contains(symbol)) return TradeSide::Buy;
    const int row = openRowBySymbol_.value(symbol);
    const TradeRecord t = model_->tradeAt(row);
    return t.side;
}

double TradeJournal::openQty(const QString& symbol) const {
    if (!openRowBySymbol_.contains(symbol)) return 0.0;
    const int row = openRowBySymbol_.value(symbol);
    const TradeRecord t = model_->tradeAt(row);
    return t.qty;
}

void TradeJournal::onFill(const Fill& f) {
    if (!model_) return;
    if (f.qty <= 0.0 || f.price <= 0.0) return;

    fees_ += f.fee;
    equity_ -= f.fee; // fee списываем сразу

    // OPEN
    if (!f.reduceOnly) {
        if (openRowBySymbol_.contains(f.symbol)) {
            // пока не поддерживаем пирамидинг/мульти-ордера
            return;
        }

        TradeRecord t;
        t.time = f.time;
        t.symbol = f.symbol;
        t.side = f.side;
        t.qty = f.qty;
        t.price = f.price;
        t.fee = f.fee;
        t.pnl = 0.0;
        t.status = TradeStatus::Open;

        model_->appendTrade(t);
        const int row = model_->rowCount() - 1;
        openRowBySymbol_.insert(f.symbol, row);
        return;
    }

    // CLOSE
    if (!openRowBySymbol_.contains(f.symbol)) {
        return;
    }

    const int row = openRowBySymbol_.value(f.symbol);
    TradeRecord t = model_->tradeAt(row);
    if (t.status != TradeStatus::Open) {
        openRowBySymbol_.remove(f.symbol);
        return;
    }

    // qty close = full position
    const double qty = t.qty;

    // gross pnl (WITHOUT fees)
    double grossPnl = 0.0;
    if (t.side == TradeSide::Buy) {
        // long
        grossPnl = (f.price - t.price) * qty;
    } else {
        // short
        grossPnl = (t.price - f.price) * qty;
    }

    // net pnl for this trade (WITH fees)
    const double totalFees = t.fee + f.fee;
    const double netPnl = grossPnl - totalFees;

    t.closeTime = f.time;
    t.closePrice = f.price;
    t.pnl = netPnl;            // показываем net pnl
    t.fee = totalFees;         // показываем суммарные комиссии
    t.status = TradeStatus::Closed;

    model_->updateTrade(row, t);
    openRowBySymbol_.remove(f.symbol);

    // stats
    netPnl_ += netPnl;
    closedTrades_++;
    if (netPnl > 0.0) winTrades_++;

    // equity: fees уже списались в начале onFill(), поэтому добавляем только gross pnl
    equity_ += grossPnl;

    if (equity_ > peakEquity_) peakEquity_ = equity_;
    const double dd = peakEquity_ - equity_;
    if (dd > maxDD_) maxDD_ = dd;
}

TradeReport TradeJournal::report() const {
    TradeReport r;
    r.equity = equity_;
    r.netPnl = netPnl_;
    r.fees = fees_;
    r.closedTrades = closedTrades_;
    r.winTrades = winTrades_;
    r.maxDrawdown = maxDD_;
    return r;
}
