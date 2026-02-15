#pragma once
#include <QString>
#include <QVector>
#include <cmath>

enum class IndicatorId {
    EMA20,
    EMA50,
    DON20,
    RSI14,
    ATR14
};

struct IndicatorLine {
    IndicatorId id_;
    QString name_;
    QVector<double> values;
};

inline double indNaN() {
    std::numeric_limits<double>::quiet_NaN();
}
