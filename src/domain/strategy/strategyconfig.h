#pragma once
#include <QString>
#include "core/timeframe.h"
#include "domain/risk/riskconfig.h"

struct StrategySelection {
    QString name;
    Timeframe tf;
};

struct StrategyVisualConfig {
    bool showEntryLine = true;
    bool showTpLine = true;
    bool showSlLine = true;
};

struct FixedExitConfig {
    bool enabled = false;   // true для fixed TP/SL стратегий
    int tpBps = 0;
    int slBps = 0;
};

struct StrategyConfig {
    StrategySelection strategy;
    RiskSettings risk;
    StrategyVisualConfig visual;
    FixedExitConfig fixedExit;
    QString accountId;
    QString account;
};

