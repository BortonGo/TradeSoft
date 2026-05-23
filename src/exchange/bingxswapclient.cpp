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

#if TRADESOFT_HAS_WEBSOCKETS
#include <QDateTime>
#include <QAbstractSocket>
#include <QWebSocketProtocol>
#endif

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#if TRADESOFT_HAS_WEBSOCKETS
#include <zlib.h>
#endif

#include <algorithm>

// --- helpers ---
static void enableRedirects(QNetworkRequest& req) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
#else
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
}

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

#if TRADESOFT_HAS_WEBSOCKETS
static QString tfToBingXWsInterval(Timeframe tf) {
    return tfToBingXInterval(tf);
}
#endif

static qint64 jsonToInt64(const QJsonValue& value) {
    bool ok = false;
    if (value.isString()) {
        const qint64 out = value.toString().toLongLong(&ok);
        return ok ? out : 0;
    }
    if (value.isDouble()) {
        return static_cast<qint64>(value.toDouble());
    }
    return 0;
}

static double jsonToDouble(const QJsonValue& value) {
    bool ok = false;
    if (value.isString()) {
        const double out = value.toString().toDouble(&ok);
        return ok ? out : 0.0;
    }
    return value.toDouble();
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

static bool parseWebSocketKline(const QJsonObject& root, Candle& c) {
    const QJsonValue dataValue = root.value("data");
    QJsonObject data;

    if (dataValue.isArray()) {
        const QJsonArray arr = dataValue.toArray();
        if (arr.isEmpty() || !arr.first().isObject()) {
            return false;
        }
        data = arr.first().toObject();
    } else if (dataValue.isObject()) {
        data = dataValue.toObject();
    }

    if (data.isEmpty()) {
        return false;
    }

    QJsonObject kline = data.value("K").toObject();
    if (kline.isEmpty()) {
        kline = data;
    }

    qint64 timestamp = jsonToInt64(kline.value("t"));
    if (timestamp <= 0) {
        timestamp = jsonToInt64(kline.value("T"));
    }
    if (timestamp <= 0) {
        return false;
    }

    c.timestamp_ = timestamp;
    c.open_ = jsonToDouble(kline.value("o"));
    c.high_ = jsonToDouble(kline.value("h"));
    c.low_ = jsonToDouble(kline.value("l"));
    c.close_ = jsonToDouble(kline.value("c"));
    c.volume_ = jsonToDouble(kline.value("v"));
    c.isFinal_ = false;
    return true;
}

#if TRADESOFT_HAS_WEBSOCKETS
static QByteArray gzipDecompress(const QByteArray& input)
{
    if (input.isEmpty()) {
        return {};
    }

    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.constData()));
    stream.avail_in = static_cast<uInt>(input.size());

    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
        return {};
    }

    QByteArray output;
    char buffer[32768];

    int ret = Z_OK;
    while (ret == Z_OK) {
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        stream.avail_out = sizeof(buffer);

        ret = inflate(&stream, Z_NO_FLUSH);
        if (output.size() < static_cast<int>(stream.total_out)) {
            output.append(buffer, static_cast<int>(stream.total_out) - output.size());
        }
    }

    inflateEnd(&stream);
    return ret == Z_STREAM_END ? output : QByteArray{};
}
#endif

BingXSwapClient::~BingXSwapClient()
{
    stopKlineStream();
}

bool BingXSwapClient::parseKlineStreamPayload(const QByteArray& payload, const QString& expectedDataType, Candle& candle)
{
    if (payload.isEmpty()) {
        return false;
    }

    const QString text = QString::fromUtf8(payload).trimmed();
    if (text.compare("Ping", Qt::CaseInsensitive) == 0) {
        return false;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.contains("code")) {
        const int code = root.value("code").toInt(-1);
        if (code != 0) {
            return false;
        }
    }

    const QString dataType = root.value("dataType").toString();
    if (!expectedDataType.isEmpty() && dataType != expectedDataType) {
        return false;
    }

    return parseWebSocketKline(root, candle);
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
    enableRedirects(req);

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

    const int limit = 1400;

    qint64 cursorMs = endMs;
    while (cursorMs > beginMs) {
        QUrl url("https://open-api.bingx.com/openApi/swap/v3/quote/klines");
        QUrlQuery q;
        q.addQueryItem("symbol", symbol);
        q.addQueryItem("interval", interval);
        q.addQueryItem("startTime", QString::number(beginMs));
        q.addQueryItem("endTime", QString::number(cursorMs));
        q.addQueryItem("limit", QString::number(limit));
        url.setQuery(q);

        qDebug() << "[BingXSwapClient fetchHistory] GET" << url.toString();

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, "TradeSoft-MVP/1.0");
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Connection", "close");
        enableRedirects(req);

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

        for (const Candle& c : batch) {
            if (!out.empty() && out.back().timestamp_ == c.timestamp_) {
                continue;
            }
            out.push_back(c);
        }

        const qint64 earliestTs = batch.back().timestamp_;
        const qint64 tfMs = timeframeToMs(request.timeframe);

        const qint64 nextCursorMs = earliestTs - tfMs;

        if (nextCursorMs >= cursorMs) {
            qWarning() << "[BingXSwapClient fetchHistory] cursor did not advance";
            break;
        }

        cursorMs = nextCursorMs;

    }
    std::reverse(out.begin(), out.end());

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
    enableRedirects(req);

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

void BingXSwapClient::startKlineStream(const QString& symbolId, Timeframe tf, RealtimeKlineCallback cb)
{
#if TRADESOFT_HAS_WEBSOCKETS
    if (symbolId.isEmpty()) {
        if (cb) {
            cb(false, Candle{});
        }
        return;
    }

    ensureWebSocket();
    if (!ws_) {
        if (cb) {
            cb(false, Candle{});
        }
        return;
    }

    wsCallback_ = std::move(cb);
    wsDataType_ = toBingXSymbol(symbolId) + "@kline_" + tfToBingXWsInterval(tf);
    wsStopRequested_ = false;
    wsReconnectAttempts_ = 0;

    if (ws_->state() == QAbstractSocket::ConnectedState) {
        sendKlineSubscription();
        return;
    }

    if (ws_->state() != QAbstractSocket::ConnectingState) {
        qDebug() << "[BingXSwapClient WS] Connecting to swap market stream";
        ws_->open(QUrl("wss://open-api-swap.bingx.com/swap-market"));
    }
#else
    Q_UNUSED(symbolId);
    Q_UNUSED(tf);
    if (cb) {
        cb(false, Candle{});
    }
#endif
}

void BingXSwapClient::stopKlineStream()
{
#if TRADESOFT_HAS_WEBSOCKETS
    wsStopRequested_ = true;
    wsCallback_ = nullptr;
    wsDataType_.clear();
    wsReconnectAttempts_ = 0;
    if (wsReconnectTimer_) {
        wsReconnectTimer_->stop();
    }

    if (ws_) {
        ws_->close();
        ws_->deleteLater();
        ws_ = nullptr;
    }
#endif
}

#if TRADESOFT_HAS_WEBSOCKETS
void BingXSwapClient::ensureWebSocket()
{
    if (ws_) {
        return;
    }

    QCoreApplication* app = QCoreApplication::instance();
    if (!app) {
        qWarning() << "[BingXSwapClient WS] No QCoreApplication instance!";
        return;
    }

    ws_ = new QWebSocket(QStringLiteral("TradeSoft-MVP/1.0"), QWebSocketProtocol::VersionLatest, app);

    if (!wsReconnectTimer_) {
        wsReconnectTimer_ = new QTimer(app);
        wsReconnectTimer_->setSingleShot(true);
        QObject::connect(wsReconnectTimer_, &QTimer::timeout, wsReconnectTimer_, [this]() {
            if (!ws_ || wsStopRequested_ || wsDataType_.isEmpty()) {
                return;
            }
            qDebug() << "[BingXSwapClient WS] Reconnecting attempt" << wsReconnectAttempts_;
            ws_->open(QUrl("wss://open-api-swap.bingx.com/swap-market"));
        });
    }

    QObject::connect(ws_, &QWebSocket::connected, ws_, [this]() {
        qDebug() << "[BingXSwapClient WS] Connected";
        wsReconnectAttempts_ = 0;
        sendKlineSubscription();
    });

    QObject::connect(ws_, &QWebSocket::disconnected, ws_, [this]() {
        qWarning() << "[BingXSwapClient WS] Disconnected";
        if (!wsStopRequested_ && !wsDataType_.isEmpty()) {
            scheduleReconnect();
        }
    });

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(ws_, &QWebSocket::errorOccurred, ws_, [this](QAbstractSocket::SocketError) {
#else
    QObject::connect(ws_, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), ws_, [this](QAbstractSocket::SocketError) {
#endif
        qWarning() << "[BingXSwapClient WS] Error:" << ws_->errorString();
        if (!wsStopRequested_ && !wsDataType_.isEmpty()) {
            scheduleReconnect();
        }
    });

    QObject::connect(ws_, &QWebSocket::textMessageReceived, ws_, [this](const QString& message) {
        handleWebSocketPayload(message.toUtf8());
    });

    QObject::connect(ws_, &QWebSocket::binaryMessageReceived, ws_, [this](const QByteArray& message) {
        const QByteArray decompressed = gzipDecompress(message);
        handleWebSocketPayload(decompressed.isEmpty() ? message : decompressed);
    });
}

void BingXSwapClient::sendKlineSubscription()
{
    if (!ws_ || ws_->state() != QAbstractSocket::ConnectedState || wsDataType_.isEmpty()) {
        return;
    }

    QJsonObject request;
    request["id"] = "tradesoft-" + QString::number(QDateTime::currentMSecsSinceEpoch());
    request["reqType"] = "sub";
    request["dataType"] = wsDataType_;

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact);
    qDebug().noquote() << "[BingXSwapClient WS] Subscribe" << QString::fromUtf8(payload);
    ws_->sendTextMessage(QString::fromUtf8(payload));
}

void BingXSwapClient::handleWebSocketPayload(const QByteArray& payload)
{
    if (payload.isEmpty()) {
        return;
    }

    const QString text = QString::fromUtf8(payload).trimmed();
    if (wsDebugPayloadsLeft_ > 0) {
        qDebug().noquote() << "[BingXSwapClient WS] Payload sample:" << text.left(400);
        --wsDebugPayloadsLeft_;
    }

    if (text.compare("Ping", Qt::CaseInsensitive) == 0) {
        if (ws_) {
            ws_->sendTextMessage("Pong");
        }
        return;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning().noquote() << "[BingXSwapClient WS] Invalid payload:" << text.left(200);
        return;
    }

    const QJsonObject root = doc.object();
    if (root.contains("code")) {
        const int code = root.value("code").toInt(-1);
        if (code != 0) {
            qWarning() << "[BingXSwapClient WS] Subscription/API error:"
                       << code << root.value("msg").toString();
            if (wsCallback_) {
                wsCallback_(false, Candle{});
            }
            return;
        }

        if (root.value("dataType").toString().isEmpty() && root.value("data").isNull()) {
            return;
        }
    }

    const QString dataType = root.value("dataType").toString();
    if (dataType != wsDataType_) {
        return;
    }

    Candle candle{};
    if (!parseKlineStreamPayload(payload, wsDataType_, candle)) {
        qWarning().noquote() << "[BingXSwapClient WS] Failed to parse kline:" << text.left(200);
        return;
    }

    qDebug() << "[BingXSwapClient WS] Parsed kline"
             << candle.timestamp_ << candle.open_ << candle.high_
             << candle.low_ << candle.close_;

    if (wsCallback_) {
        wsCallback_(true, candle);
    }
}

void BingXSwapClient::scheduleReconnect()
{
    if (!wsReconnectTimer_ || wsStopRequested_ || wsDataType_.isEmpty()) {
        return;
    }

    constexpr int maxReconnectAttempts = 5;
    if (wsReconnectAttempts_ >= maxReconnectAttempts) {
        qWarning() << "[BingXSwapClient WS] Reconnect attempts exhausted";
        if (wsCallback_) {
            wsCallback_(false, Candle{});
        }
        return;
    }

    ++wsReconnectAttempts_;
    const int delayMs = std::min(30000, 1000 * wsReconnectAttempts_);
    if (!wsReconnectTimer_->isActive()) {
        qWarning() << "[BingXSwapClient WS] Scheduling reconnect in" << delayMs << "ms";
        wsReconnectTimer_->start(delayMs);
    }
}
#endif
