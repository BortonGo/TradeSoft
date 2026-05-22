#pragma once

#include "core/symbol.h"
#include "core/timeframe.h"

#include <QList>
#include <QString>

struct AppConfig final {
    QString defaultSymbolId = "ETHUSDT";
    Timeframe defaultTimeframe = Timeframe::M1;
    int realtimePollingMs = 1000;
    QList<Symbol> symbols = someSymbols();

    static AppConfig load();
    static QString configFilePath();

    bool save() const;
};
