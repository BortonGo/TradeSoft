#include "tradejournal.h"
#include <QtGlobal>
#include <cmath>
#include <utility>

TradeJournal::TradeJournal(double startEquityUsdt) : startEquity_(startEquityUsdt),
    equity_(startEquityUsdt), peakEquity_(startEquityUsdt) {
}

void TradeJournal::setTradeAddedCallback(TradeAddedCallback cb) {
    onTradeAdded_ = std::move(cb);
}

void TradeJournal::setTradeUpdatedCallback(TradeUpdatedCallback cb) {
    onTradeUpdated_ = std::move(cb);
}

bool TradeJournal::hasOpen(const QString& symbol) const {
    return openRowBySymbol_.contains(symbol);
}

TradeSide TradeJournal::openSide(const QString& symbol) const {
    if (!openRowBySymbol_.contains(symbol)) return TradeSide::Buy;
    const int row = openRowBySymbol_.value(symbol);
    if (row < 0 || row >= static_cast<int>(trades_.size())) return TradeSide::Buy;
    return trades_[row].side;
}

double TradeJournal::openQty(const QString& symbol) const {
    if (!openRowBySymbol_.contains(symbol)) return 0.0;
    const int row = openRowBySymbol_.value(symbol);
    if (row < 0 || row >= static_cast<int>(trades_.size())) return 0.0;
    return trades_[row].qty;
}

void TradeJournal::onFill(const Fill& f) {
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
        t.tpPrice = f.tpPrice;
        t.slPrice = f.slPrice;

        trades_.push_back(t);
        const int row = static_cast<int>(trades_.size()) - 1;
        openRowBySymbol_.insert(f.symbol, row);
        if (onTradeAdded_) {
            onTradeAdded_(t);
        }
        return;
    }

    // CLOSE
    if (!openRowBySymbol_.contains(f.symbol)) {
        return;
    }

    const int row = openRowBySymbol_.value(f.symbol);
    if (row < 0 || row >= static_cast<int>(trades_.size())) {
        openRowBySymbol_.remove(f.symbol);
        return;
    }

    TradeRecord t = trades_[row];
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

    trades_[row] = t;
    if (onTradeUpdated_) {
        onTradeUpdated_(row, t);
    }
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

void TradeJournal::onPriceUpdate(const QString& symbol, double markPrice, double feePct) {
    if (markPrice <= 0.0) return;

    auto it = openRowBySymbol_.find(symbol);
    if (it == openRowBySymbol_.end()) return;

    const int row = it.value();
    if (row < 0 || row >= static_cast<int>(trades_.size())) {
        openRowBySymbol_.remove(symbol);
        return;
    }
    TradeRecord t = trades_[row];
    if (t.status != TradeStatus::Open) return;

    // ticks: считаем обновлениями realtime (раз в секунду)
    t.lifetimeTicks += 1;

    // gross PnL at current price
    const double qty = t.qty;
    double gross = 0.0;
    if (t.side == TradeSide::Buy) gross = (markPrice - t.price) * qty;
    else                         gross = (t.price - markPrice) * qty;

    // показываем "если закрыть сейчас": учитываем entry fee + оценку exit fee
    const double estExitFee = std::abs(markPrice * qty) * (feePct / 100.0);
    const double unrealizedNet = gross - t.fee - estExitFee;

    t.pnl = unrealizedNet;

    trades_[row] = t;
    if (onTradeUpdated_) {
        onTradeUpdated_(row, t);
    }
}
