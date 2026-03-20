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

enum class GraphType {
    None,
    EquityCurve,
    DrawdownCurve,
    PnlByTrade,
    Scatter
};

enum class GraphAxis {
    None,
    TradeIndex,
    Time,
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

struct BacktestRequest {
    QString symbol;
    Timeframe timeframe = Timeframe::M1;

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
    };

    CandlePriceType candlePriceType = CandlePriceType::Last;
    int warmupBars = 200;
};

struct BacktestTrade {
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

struct EquityPoint {
    QDateTime time;
    double equity = 0.0;
    double drawdown = 0.0;
    double cumulativePnl = 0.0;
};

struct BacktestStats {
    int trades = 0;
    double winratePct = 0.0;
    double profitFactor = 0.0;
    double netPnl = 0.0;
    double avgWin = 0.0;
    double avgLoss = 0.0;
    double MaxDDPct = 0.0;
    double expectancy = 0.0;
};

struct BacktestResult {
    BacktestState state = BacktestState::Idle;
    QString errorText;

    BacktestStats stats;
    std::vector<BacktestTrade> trades;
    std::vector<EquityPoint> equityCurve;
};

struct GraphRequest {
    GraphType type = GraphType::EquityCurve;
    GraphAxis xAxis = GraphAxis::TradeIndex;
    GraphAxis yAxis = GraphAxis::NetPnl;

    bool longOnly = false;
    bool shortOnly = false;
    bool winnersOnly = false;
    bool losersOnly = false;
};

struct GraphPoint {
    double x = 0.0;
    double y = 0.0;
    QString label;
};
