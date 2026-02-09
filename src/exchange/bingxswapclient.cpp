#include "bingxswapclient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QDebug>

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


