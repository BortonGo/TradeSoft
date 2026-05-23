#pragma once

#include "core/symbol.h"
#include "core/timeframe.h"

#include <QList>
#include <QString>

enum class RealtimeTransport {
    Auto,
    WebSocket,
    Polling
};

QString toConfigString(RealtimeTransport transport);
RealtimeTransport realtimeTransportFromConfigString(const QString& value);

struct AppConfig final {
    QString defaultSymbolId = "ETHUSDT";
    Timeframe defaultTimeframe = Timeframe::M1;
    int realtimePollingMs = 1000;
    RealtimeTransport realtimeTransport = RealtimeTransport::Auto;
    QList<Symbol> symbols = someSymbols();

    static AppConfig load();
    static QString configFilePath();

    bool save() const;
};
