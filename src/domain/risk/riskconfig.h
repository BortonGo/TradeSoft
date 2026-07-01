#pragma once

enum class RiskMode {
    FixedUsdt,
    PercentOfEquity
};

struct RiskSettings final {
    double riskPct = 0.0;
    double maxPosUsdt = 0.0;
    double feePct = 0.0;
    RiskMode mode = RiskMode::FixedUsdt;
    int leverage = 1;
    int slippageBps = 0;
    bool allowLong = true;
    bool allowShort = true;
};
