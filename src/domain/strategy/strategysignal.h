#pragma once
#include <QString>
#include "core/timeframe.h"

enum StrategySignalType {
    None = 0,
    EnterLong,
    ExitLong,
    EnterShort,
    ExitShort
};

struct StrategySignal final {
    StrategySignalType type = StrategySignalType::None;
    QString symbolId;
    Timeframe tf {};
    int64_t timestamp = 0;
    QString reason;
};
