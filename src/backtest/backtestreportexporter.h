#pragma once

#include "backtest/backtesttypes.h"

#include <QString>

class BacktestReportExporter final {
public:
    struct ReportPaths {
        QString latestSummary;
        QString latestTrades;
        QString snapshotSummary;
        QString snapshotTrades;
    };

    static bool exportLatest(const BacktestRequest& request, const BacktestResult& result,
                             QString* errorText = nullptr, ReportPaths* paths = nullptr);
    static QString reportsDir();
    static QString summaryPath();
    static QString tradesPath();
    static QString snapshotBaseName(const BacktestRequest& request);
};
