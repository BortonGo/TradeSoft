#pragma once
#include <QString>
#include "core/timeframe.h"
#include "domain/risk/riskconfig.h"

struct StrategySelection final {
    QString name;
    Timeframe tf;
};

struct StrategyVisualConfig final {
    bool showEntryLine = true;
    bool showTpLine = true;
    bool showSlLine = true;
};

struct FixedExitConfig final {
    bool enabled = false;   // true для fixed TP/SL стратегий
    int tpBps = 0;
    int slBps = 0;
};

struct StrategyConfig final {
    StrategySelection strategy;
    RiskSettings risk;
    StrategyVisualConfig visual;
    FixedExitConfig fixedExit;
    QString accountId;
    QString account;
};

