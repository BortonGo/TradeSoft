#include "core/appconfig.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

namespace {

QString appDataDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/.tradesoft";
    }
    QDir().mkpath(dir);
    return dir;
}

QJsonArray symbolsToJson(const QList<Symbol>& symbols)
{
    QJsonArray arr;
    for (const Symbol& symbol : symbols) {
        QJsonObject obj;
        obj["base"] = symbol.base_;
        obj["quote"] = symbol.quote_;
        arr.append(obj);
    }
    return arr;
}

QList<Symbol> symbolsFromJson(const QJsonValue& value)
{
    QList<Symbol> symbols;
    if (!value.isArray()) {
        return symbols;
    }

    const QJsonArray arr = value.toArray();
    for (const QJsonValue& item : arr) {
        if (!item.isObject()) {
            continue;
        }
        const QJsonObject obj = item.toObject();
        const QString base = obj.value("base").toString().trimmed();
        const QString quote = obj.value("quote").toString().trimmed();
        if (!base.isEmpty() && !quote.isEmpty()) {
            symbols.append(Symbol(base, quote));
        }
    }
    return symbols;
}

} // namespace

QString toConfigString(RealtimeTransport transport)
{
    switch (transport) {
    case RealtimeTransport::Auto: return "auto";
    case RealtimeTransport::WebSocket: return "websocket";
    case RealtimeTransport::Polling: return "polling";
    }
    return "auto";
}

RealtimeTransport realtimeTransportFromConfigString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == "websocket" || normalized == "ws") {
        return RealtimeTransport::WebSocket;
    }
    if (normalized == "polling" || normalized == "http") {
        return RealtimeTransport::Polling;
    }
    return RealtimeTransport::Auto;
}

QString AppConfig::configFilePath()
{
    return appDataDir() + "/config.json";
}

AppConfig AppConfig::load()
{
    AppConfig config;
    QFile file(configFilePath());

    if (!file.exists()) {
        if (!config.save()) {
            qWarning() << "[AppConfig] Failed to write default config:" << configFilePath();
        }
        return config;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[AppConfig] Failed to open config:" << file.errorString();
        return config;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[AppConfig] Invalid config JSON:" << error.errorString();
        return config;
    }

    const QJsonObject root = doc.object();
    config.defaultSymbolId = root.value("defaultSymbol").toString(config.defaultSymbolId).trimmed();
    config.defaultTimeframe = timeframeFromUiString(root.value("defaultTimeframe").toString(toUiString(config.defaultTimeframe)));
    config.realtimePollingMs = root.value("realtimePollingMs").toInt(config.realtimePollingMs);
    config.realtimePollingMs = qBound(250, config.realtimePollingMs, 60000);
    config.realtimeTransport = realtimeTransportFromConfigString(
        root.value("realtimeTransport").toString(toConfigString(config.realtimeTransport)));

    const QList<Symbol> parsedSymbols = symbolsFromJson(root.value("symbols"));
    if (!parsedSymbols.isEmpty()) {
        config.symbols = parsedSymbols;
    }

    return config;
}

bool AppConfig::save() const
{
    QJsonObject root;
    root["defaultSymbol"] = defaultSymbolId;
    root["defaultTimeframe"] = toUiString(defaultTimeframe);
    root["realtimePollingMs"] = realtimePollingMs;
    root["realtimeTransport"] = toConfigString(realtimeTransport);
    root["symbols"] = symbolsToJson(symbols);

    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}
