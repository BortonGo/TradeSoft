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
    QString symbolId;
    QString reason;
    int64_t timestamp = 0;
    StrategySignalType type = StrategySignalType::None;
    Timeframe tf {};
};
