#pragma once
#include <QString>
#include <QList>

struct Symbol {
    QString base;
    QString quote;

    Symbol(const QString& b, const QString& q) : base(b), quote(q) {}

    QString id() const {
        return base + quote;
    }

    QString display() const {
        return base + "/" + quote;
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
