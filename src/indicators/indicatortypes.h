#pragma once
#include <QString>
#include <QtGlobal>
#include <vector>
#include <cmath>
#include <limits>
#include <type_traits>
#include <QHashFunctions>

enum class IndicatorId {
    EMA9,
    EMA20,
    EMA50,
    DON20,
    DON20UPPER,
    DON20LOWER,
    DON20MIDDLE,
    RSI14,
    ATR14
};

inline uint qHash(IndicatorId key, uint seed = 0) noexcept
{
    using U = typename std::underlying_type<IndicatorId>::type;
    return ::qHash(static_cast<U>(key), seed);
}

struct IndicatorLine {
    IndicatorId id_;
    QString name_;
    std::vector<double> values_;
};

inline double indNaN() {
    return std::numeric_limits<double>::quiet_NaN();
}
