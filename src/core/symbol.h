#pragma once
#include <QString>
#include <QList>

struct Symbol {
    QString base_;
    QString quote_;

    Symbol(const QString& b, const QString& q) : base_(b), quote_(q) {}

    QString id() const {
        return base_ + quote_;
    }

    QString display() const {
        return base_ + "/" + quote_;
    }
};

inline QList<Symbol> someSymbols(){
    return {
        Symbol("ETH", "USDT"),
        Symbol("BTC", "USDT"),
        Symbol("XRP", "USDT"),
        Symbol("ADA", "USDT")
    };
}
