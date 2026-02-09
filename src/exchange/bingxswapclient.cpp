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

// only for BingX api
static QString tfToBingXInterval(Timeframe tf) {
    switch (tf) {
    case Timeframe::M1: return "1m";
    case Timeframe::M5: return "5m";
    case Timeframe::M15: return "15m";
    case Timeframe::H1: return "1h";
    case Timeframe::H4: return "4h";
    case Timeframe::D1: return "1d";
    default: return "1m";
    }
}

// only for BingX api
static QString toBingXSymbol(const QString& neutralId) {
    // "ETHUSDT" -> "ETH-USDT"
    if (neutralId.endsWith("USDT")) {
        return neutralId.left(neutralId.size() - 4) + "-USDT";
    }
    return neutralId;
}

QList<Candle> BingXSwapClient::fetchKlines(const QString& symbolId, Timeframe tf)
{
    QList<Candle> out;

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

    if (!out.isEmpty()) {
        qDebug() << "[BingXSwapClient] Parsed candles:" << out.size()
                 << "first ts:" << out.first().timestamp_
                 << "last ts:" << out.last().timestamp_;
    } else {
        qWarning() << "[BingXSwapClient] Parsed 0 candles";
    }

    return out;
}

