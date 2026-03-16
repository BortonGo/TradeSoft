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

};

struct BacktestRequest {

};

struct BacktestTrade {

};

struct EquityPoint {

};

struct BacktestStats {

};

struct BacktestResult {

};

struct GraphRequest {

};

struct GraphPoint {

};
