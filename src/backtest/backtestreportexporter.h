#pragma once

#include "backtest/backtesttypes.h"

#include <QString>

class BacktestReportExporter final {
public:
    static bool exportLatest(const BacktestRequest& request, const BacktestResult& result, QString* errorText = nullptr);
    static QString reportsDir();
    static QString summaryPath();
    static QString tradesPath();
};
