#pragma once
#include <QString>
#include "core/timeframe.h"

enum class RiskMode {
    FixedUSDT,
    PercentEquity
};

struct RiskSettings {
    RiskMode mode = RiskMode::FixedUSDT;
    double riskPct = 0.0;
    double maxPosUsdt = 0.0;
    int leverage = 1;
    bool allowLong = true;
    bool allowShort = true;
    double feePct = 0.0;
    int slippageBps = 0;
};

struct StrategySelection {
    QString name;
    Timeframe tf;
};

struct StrategyConfig {
    StrategySelection strategy;
    RiskSettings risk;
    QString accountId;
    QString account;
};

