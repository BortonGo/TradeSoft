#pragma once
#include <QString>
#include <QDateTime>
#include <vector>

#include "domain/risk/riskconfig.h"
#include "core/timeframe.h"

enum class BacktestState {
    Idle,
    LoadingHistory,
    Running,
    Completed,
    Cancelled,
    Failed
};

inline QString toString(BacktestState state) {
    switch (state){
        case BacktestState::Cancelled : return "Cancelled";
        case BacktestState::Completed : return "Completed";
        case BacktestState::Failed : return "Failed";
        case BacktestState::Idle : return "Idle";
        case BacktestState::LoadingHistory : return "LoadingHistory";
        case BacktestState::Running : return "Running";
    }
    return "Idle";
}

enum class GraphType {
    None,
    EquityCurve,
    DrawdownCurve,
    PnlByTrade,
    Scatter,
    Custom
};

enum class GraphAxis {
    None,
    TradeIndex,
    Time,
    Equity,
    NetPnl,
    GrossPnl,
    Drawdown,
    EntryPrice,
    ExitPrice,
    Quantity,
    BarsHeld
};

enum class BacktestTradeSide {
    Long,
    Short
};

enum class CandlePriceType {
    Last,
    Mark
};

enum class BacktestExecutionMode {
    BarClose,
    IntrabarLowerTf
};

struct BacktestRequest final {
    QString symbol;
    Timeframe timeframe = Timeframe::M1;

    BacktestExecutionMode executionMode = BacktestExecutionMode::BarClose;
    Timeframe executionTimeframe = Timeframe::M1;

    QDateTime begin;
    QDateTime end;

    QString strategyName;

    double initialbalance = 1000.0;

    RiskSettings backtestRisk = [](){
        RiskSettings r;
        r.mode = RiskMode::FixedUsdt;
        r.riskPct = 1.0;
        r.maxPosUsdt = 100.0;
        r.leverage = 1;
        r.allowLong = true;
        r.allowShort = true;
        r.feePct = 0.0;
        r.slippageBps = 0;
        return r;
    }();

    CandlePriceType candlePriceType = CandlePriceType::Last;
    int warmupBars = 200;
};

struct BacktestTrade final {
    QDateTime entryTime;
    QDateTime exitTime;

    BacktestTradeSide side = BacktestTradeSide::Long;

    double entryPrice = 0.0;
    double exitPrice = 0.0;
    double quantity = 0.0;

    double grossPnl = 0.0;
    double netPnl = 0.0;
    double feePaid = 0.0;

    int barsHeld = 0;
    bool winner = false;
};

struct EquityPoint final {
    QDateTime time;
    double equity = 0.0;
    double drawdown = 0.0;
    double cumulativePnl = 0.0;
};

struct BacktestStats final {
    int trades = 0;
    double winratePct = 0.0;
    double profitFactor = 0.0;
    double grossPnl = 0.0;
    double netPnl = 0.0;
    double totalFees = 0.0;
    double avgWin = 0.0;
    double avgLoss = 0.0;
    double bestTrade = 0.0;
    double worstTrade = 0.0;
    double avgBarsHeld = 0.0;
    double maxDrawdown = 0.0;
    double MaxDDPct = 0.0;
    double expectancy = 0.0;
};

struct BacktestResult final {
    BacktestState state = BacktestState::Idle;
    QString errorText;

    BacktestStats stats;
    std::vector<BacktestTrade> trades;
    std::vector<EquityPoint> equityCurve;
};

struct GraphRequest final {
    GraphType type = GraphType::EquityCurve;
    GraphAxis xAxis = GraphAxis::TradeIndex;
    GraphAxis yAxis = GraphAxis::NetPnl;

    bool longOnly = false;
    bool shortOnly = false;
    bool winnersOnly = false;
    bool losersOnly = false;
};

struct GraphPoint final {
    double x = 0.0;
    double y = 0.0;
    QString label;
};
