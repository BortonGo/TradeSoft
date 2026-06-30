#include "service/marketdata/candlecache.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logCandleCache, "tradesoft.marketdata.cache")

namespace {

QString cacheDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/.cache/tradesoft";
    }
    dir += "/marketdata";
    QDir().mkpath(dir);
    return dir;
}

QString safePart(QString value)
{
    value.replace(QRegularExpression("[^A-Za-z0-9_-]"), "_");
    return value;
}

QJsonObject candleToJson(const Candle& c)
{
    QJsonObject obj;
    obj["timestamp"] = QString::number(c.timestamp_);
    obj["open"] = c.open_;
    obj["high"] = c.high_;
    obj["low"] = c.low_;
    obj["close"] = c.close_;
    obj["volume"] = c.volume_;
    obj["isFinal"] = c.isFinal_;
    return obj;
}

bool candleFromJson(const QJsonValue& value, Candle& c)
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject obj = value.toObject();
    bool ok = false;
    const qint64 timestamp = obj.value("timestamp").toString().toLongLong(&ok);
    if (!ok || timestamp <= 0) {
        return false;
    }

    c.timestamp_ = timestamp;
    c.open_ = obj.value("open").toDouble();
    c.high_ = obj.value("high").toDouble();
    c.low_ = obj.value("low").toDouble();
    c.close_ = obj.value("close").toDouble();
    c.volume_ = obj.value("volume").toDouble();
    c.isFinal_ = obj.value("isFinal").toBool(true);
    return true;
}

} // namespace

QString CandleCache::filePath(const QString& symbolId, Timeframe tf)
{
    return cacheDir() + "/" + safePart(symbolId) + "_" + safePart(toUiString(tf)) + ".json";
}

std::vector<Candle> CandleCache::load(const QString& symbolId, Timeframe tf)
{
    std::vector<Candle> candles;
    QFile file(filePath(symbolId, tf));
    if (!file.exists()) {
        return candles;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(logCandleCache) << "Failed to open cache:" << file.errorString();
        return candles;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(logCandleCache) << "Invalid cache JSON:" << error.errorString();
        return candles;
    }

    const QJsonArray arr = doc.object().value("candles").toArray();
    candles.reserve(arr.size());
    for (const QJsonValue& value : arr) {
        Candle c{};
        if (candleFromJson(value, c)) {
            candles.push_back(c);
        }
    }

    return candles;
}

bool CandleCache::save(const QString& symbolId, Timeframe tf, const std::vector<Candle>& candles)
{
    if (symbolId.isEmpty() || candles.empty()) {
        return false;
    }

    QJsonObject root;
    root["symbol"] = symbolId;
    root["timeframe"] = toUiString(tf);
    root["savedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QJsonArray arr;
    for (const Candle& c : candles) {
        arr.append(candleToJson(c));
    }
    root["candles"] = arr;

    QFile file(filePath(symbolId, tf));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCWarning(logCandleCache) << "Failed to write cache:" << file.errorString();
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return true;
}

bool CandleCache::isFresh(const std::vector<Candle>& candles, Timeframe tf)
{
    if (candles.empty()) {
        return false;
    }

    const qint64 nowMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    const qint64 tfMs = timeframeToMs(tf);
    const qint64 lastTs = candles.back().timestamp_;

    return lastTs > 0 && (nowMs - lastTs) <= (tfMs * 2);
}
