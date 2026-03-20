#include "bingxswapclient.h"

#include <QNetworkAccessManager>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QDebug>
#include <QTimer>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <algorithm>

// --- helpers ---
static QString tfToBingXInterval(Timeframe tf) {
    switch (tf) {
    case Timeframe::M1:  return "1m";
    case Timeframe::M5:  return "5m";
    case Timeframe::M15: return "15m";
    case Timeframe::H1:  return "1h";
    case Timeframe::H4:  return "4h";
    case Timeframe::D1:  return "1d";
    default: return "1m";
    }
}

static QString toBingXSymbol(const QString& neutralId) {
    if (neutralId.endsWith("USDT")) {
        return neutralId.left(neutralId.size() - 4) + "-USDT";
    }
    return neutralId;
}

static bool parseSingleKlineObj(const QJsonObject& o, Candle& c) {
    if (!o.contains("time")) return false;

    c.timestamp_ = static_cast<qint64>(o.value("time").toVariant().toLongLong());
    c.open_   = o.value("open").toString().toDouble();
    c.close_  = o.value("close").toString().toDouble();
    c.high_   = o.value("high").toString().toDouble();
    c.low_    = o.value("low").toString().toDouble();
    c.volume_ = o.value("volume").toString().toDouble();
    c.isFinal_ = false;
    return true;
}

std::vector<Candle> BingXSwapClient::fetchKlines(const QString& symbolId, Timeframe tf)
{
    std::vector<Candle> out;

    // создаём менеджер только когда приложение уже существует
    if (!mgr_) {
        QCoreApplication* app = QCoreApplication::instance();
        if (!app) {
            qWarning() << "[BingXSwapClient] No QCoreApplication instance yet!";
            return out;
        }
        mgr_ = new QNetworkAccessManager(app);
    }

    const QString symbol   = toBingXSymbol(symbolId);      // "ETH-USDT"
    const QString interval = tfToBingXInterval(tf);        // "1m"
    const int limit = 500;

    QUrl url("https://open-api.bingx.com/openApi/swap/v3/quote/klines");
    QUrlQuery q;
    q.addQueryItem("symbol", symbol);
    q.addQueryItem("interval", interval);
    q.addQueryItem("limit", QString::number(limit));
    url.setQuery(q);

    qDebug() << "[BingXSwapClient] GET" << url.toString();

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "TradeSoft-MVP/1.0");
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Connection", "close");
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkReply* reply = mgr_->get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start(12000);
    loop.exec();

    if (!timer.isActive()) {
        qWarning() << "[BingXSwapClient] TIMEOUT";
        reply->deleteLater();
        return out;
    }
    timer.stop();

    const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug() << "[BingXSwapClient] HTTP status:" << statusCode;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[BingXSwapClient] Network error:" << reply->errorString();
        reply->deleteLater();
        return out;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    // ---- JSON parse ----
    QJsonParseError jerr;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &jerr);
    if (jerr.error != QJsonParseError::NoError || doc.isNull() || !doc.isObject()) {
        qWarning() << "[BingXSwapClient] JSON parse error:" << jerr.errorString();
        qDebug().noquote() << "[BingXSwapClient] Body head:" << QString::fromUtf8(body.left(200));
        return out;
    }

    const QJsonObject root = doc.object();
    const int code = root.value("code").toInt(-1);
    if (code != 0) {
        qWarning() << "[BingXSwapClient] API error code:" << code
                   << "msg:" << root.value("msg").toString();
        return out;
    }

    const QJsonValue dataVal = root.value("data");
    if (!dataVal.isArray()) {
        qWarning() << "[BingXSwapClient] 'data' is not array";
        return out;
    }

    const QJsonArray arr = dataVal.toArray();
    out.reserve(arr.size());

    for (const QJsonValue& v : arr) {
        if (!v.isObject())
            continue;

        const QJsonObject o = v.toObject();

        Candle c;
        c.timestamp_ = static_cast<int64_t>(o.value("time").toVariant().toLongLong());

        c.open_   = o.value("open").toString().toDouble();
        c.close_  = o.value("close").toString().toDouble();
        c.high_   = o.value("high").toString().toDouble();
        c.low_    = o.value("low").toString().toDouble();
        c.volume_ = o.value("volume").toString().toDouble();

        c.isFinal_ = true;

        out.push_back(c);
    }

    // BingX часто отдаёт от новых к старым — разворачиваем к виду "старые -> новые"
    std::reverse(out.begin(), out.end());

    if (!out.empty()) {
        qDebug() << "[BingXSwapClient] Parsed candles:" << out.size()
                 << "first ts:" << out.front().timestamp_
                 << "last ts:" << out.back().timestamp_;
    } else {
        qWarning() << "[BingXSwapClient] Parsed 0 candles";
    }

    return out;
}


// For backtest
std::vector<Candle> BingXSwapClient::fetchHistory(const HistoryRequest& request) {
    std::vector<Candle> out;

    // создаём менеджер только когда приложение уже существует
    if (!mgr_) {
        QCoreApplication* app = QCoreApplication::instance();
        if (!app) {
            qWarning() << "[BingXSwapClient fetchHistory] No QCoreApplication instance yet!";
            return out;
        }
        mgr_ = new QNetworkAccessManager(app);
    }

    if (request.symbolId.isEmpty()) {
        qWarning() << "[BingXSwapClient fetchHistory] symbolId is empty!";
        return out;
    }

    if (!request.begin.isValid() || !request.end.isValid()) {
        qWarning() << "[BingXSwapClient fetchHistory] degin or end - invalid!";
        return out;
    }

    const QString symbol   = toBingXSymbol(request.symbolId);      // "ETH-USDT"
    const QString interval = tfToBingXInterval(request.timeframe);        // "1m"

    const qint64 beginMs = request.begin.toMSecsSinceEpoch();
    const qint64 endMs = request.end.toMSecsSinceEpoch();

    if (beginMs >= endMs) {
        qWarning() << "[BingXSwapClient fetchHistory] begin >= end!";
        return out;
    }

    const int limit = 500;

    qint64 cursorMs = beginMs;
    while (cursorMs < endMs) {
        QUrl url("https://open-api.bingx.com/openApi/swap/v3/quote/klines");
        QUrlQuery q;
        q.addQueryItem("symbol", symbol);
        q.addQueryItem("interval", interval);
        q.addQueryItem("startTime", QString::number(cursorMs));
        q.addQueryItem("endTime", QString::number(endMs));
        q.addQueryItem("limit", QString::number(limit));
        url.setQuery(q);

        qDebug() << "[BingXSwapClient fetchHistory] GET" << url.toString();

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, "TradeSoft-MVP/1.0");
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Connection", "close");
        req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

        QNetworkReply* reply = mgr_->get(req);

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timer.start(12000);
        loop.exec();

        if (!timer.isActive()) {
            qWarning() << "[BingXSwapClient fetchHistory] TIMEOUT";
            reply->deleteLater();
            break;
        }
        timer.stop();

        const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        qDebug() << "[BingXSwapClient fetchHistory] HTTP status:" << statusCode;

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[BingXSwapClient fetchHistory] Network error:" << reply->errorString();
            reply->deleteLater();
            break;
        }

        const QByteArray body = reply->readAll();
        reply->deleteLater();

        // ---- JSON parse ----
        QJsonParseError jerr;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &jerr);
        if (jerr.error != QJsonParseError::NoError || doc.isNull() || !doc.isObject()) {
            qWarning() << "[BingXSwapClient fetchHistory] JSON parse error:" << jerr.errorString();
            qDebug().noquote() << "[BingXSwapClient fetchHistory] Body head:" << QString::fromUtf8(body.left(200));
            break;
        }

        const QJsonObject root = doc.object();
        const int code = root.value("code").toInt(-1);
        if (code != 0) {
            qWarning() << "[BingXSwapClient fetchHistory] API error code:" << code
                       << "msg:" << root.value("msg").toString();
            break;
        }

        const QJsonValue dataVal = root.value("data");
        if (!dataVal.isArray()) {
            qWarning() << "[BingXSwapClient fetchHistory] 'data' is not array";
            break;
        }

        const QJsonArray arr = dataVal.toArray();

        std::vector<Candle> batch;
        batch.reserve(arr.size());

        for (const QJsonValue& v : arr) {
            if (!v.isObject())
                continue;

            const QJsonObject o = v.toObject();

            Candle c;
            c.timestamp_ = static_cast<int64_t>(o.value("time").toVariant().toLongLong());

            if (c.timestamp_ < beginMs || c.timestamp_ > endMs) {
                continue;
            }

            c.open_   = o.value("open").toString().toDouble();
            c.close_  = o.value("close").toString().toDouble();
            c.high_   = o.value("high").toString().toDouble();
            c.low_    = o.value("low").toString().toDouble();
            c.volume_ = o.value("volume").toString().toDouble();

            c.isFinal_ = true;

            batch.push_back(c);
        }
        if (batch.empty()) {
            break;
        }

        std::reverse(batch.begin(), batch.end());

        for (const Candle& c : batch) {
            if (!out.empty() && out.back().timestamp_ == c.timestamp_) {
                continue;
            }
            out.push_back(c);
        }

        const qint64 lastTs = batch.back().timestamp_;
        const qint64 tfMs = timeframeToMs(request.timeframe);

        const qint64 nextCursorMs = lastTs + tfMs;

        if (nextCursorMs <= cursorMs) {
            qWarning() << "[BingXSwapClient fetchHistory] cursor did not advance";
            break;
        }

        cursorMs = nextCursorMs;

    }

    if (!out.empty()) {
        qDebug() << "[BingXSwapClient fetchHistory] Parsed candles:" << out.size()
                 << "first ts:" << out.front().timestamp_
                 << "last ts:" << out.back().timestamp_;
    } else {
        qWarning() << "[BingXSwapClient fetchHistory] Parsed 0 candles";
    }

    return out;
}


// Async realtime polling
void BingXSwapClient::fetchLastKlineAsync(const QString& symbolId, Timeframe tf, LastKlineCallback cb)
{
    // ensure manager exists
    if (!mgr_) {
        QCoreApplication* app = QCoreApplication::instance();
        if (!app) {
            qWarning() << "[BingXSwapClient] No QCoreApplication instance!";
            if (cb) cb(false, Candle{});
            return;
        }
        mgr_ = new QNetworkAccessManager(app);
    }

    const QString symbol   = toBingXSymbol(symbolId);
    const QString interval = tfToBingXInterval(tf);

    QUrl url("https://open-api.bingx.com/openApi/swap/v3/quote/klines");
    QUrlQuery q;
    q.addQueryItem("symbol", symbol);
    q.addQueryItem("interval", interval);
    q.addQueryItem("limit", "2");
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "TradeSoft-MVP/1.0");
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Connection", "close");
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkReply* reply = mgr_->get(req);

    // timeout per-reply
    QTimer* t = new QTimer(reply);
    t->setSingleShot(true);
    QObject::connect(t, &QTimer::timeout, reply, &QNetworkReply::abort);
    t->start(5000);

    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, cb]() {
        // finished will run even after abort() -> treat as failure if error != NoError
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            if (cb) cb(false, Candle{});
            return;
        }

        const QByteArray body = reply->readAll();
        reply->deleteLater();

        QJsonParseError jerr;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &jerr);
        if (jerr.error != QJsonParseError::NoError || !doc.isObject()) {
            if (cb) cb(false, Candle{});
            return;
        }

        const QJsonObject root = doc.object();
        if (root.value("code").toInt(-1) != 0) {
            if (cb) cb(false, Candle{});
            return;
        }

        const QJsonValue dataVal = root.value("data");
        if (!dataVal.isArray()) {
            if (cb) cb(false, Candle{});
            return;
        }

        const QJsonArray data = dataVal.toArray();
        if (data.isEmpty()) {
            if (cb) cb(false, Candle{});
            return;
        }

        Candle best{};
        bool ok = false;
        qint64 bestTs = -1;

        for (int i = 0; i < data.size(); ++i) {
            if (!data[i].isObject()) continue;
            Candle tmp{};
            if (!parseSingleKlineObj(data[i].toObject(), tmp)) continue;

            if (tmp.timestamp_ > bestTs) {
                bestTs = tmp.timestamp_;
                best = tmp;
                ok = true;
            }
        }

        if (!ok) {
            if (cb) cb(false, Candle{});
            return;
        }

        if (cb) cb(true, best);
    });
}

