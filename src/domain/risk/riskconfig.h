#pragma once

enum class RiskMode {
    FixedUsdt,
    PercentOfEquity
};

struct RiskSettings {
    RiskMode mode = RiskMode::FixedUsdt;
    double riskPct = 0.0;
    double maxPosUsdt = 0.0;
    int leverage = 1;
    bool allowLong = true;
    bool allowShort = true;
    double feePct = 0.0;
    int slippageBps = 0;
};
