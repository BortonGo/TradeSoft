#include "backtest/backtestreportexporter.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTextStream>

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

QString sideToString(BacktestTradeSide side)
{
    return (side == BacktestTradeSide::Long) ? "Long" : "Short";
}

QString csvEscape(QString value)
{
    if (value.contains('"')) {
        value.replace("\"", "\"\"");
    }
    if (value.contains(',') || value.contains('\n') || value.contains('"')) {
        value = "\"" + value + "\"";
    }
    return value;
}

QJsonObject tradeToJson(const BacktestTrade& trade)
{
    QJsonObject obj;
    obj["entryTime"] = trade.entryTime.toString(Qt::ISODate);
    obj["exitTime"] = trade.exitTime.toString(Qt::ISODate);
    obj["side"] = sideToString(trade.side);
    obj["entryPrice"] = trade.entryPrice;
    obj["exitPrice"] = trade.exitPrice;
    obj["quantity"] = trade.quantity;
    obj["grossPnl"] = trade.grossPnl;
    obj["netPnl"] = trade.netPnl;
    obj["feePaid"] = trade.feePaid;
    obj["barsHeld"] = trade.barsHeld;
    obj["winner"] = trade.winner;
    return obj;
}

bool writeSummary(const BacktestRequest& request, const BacktestResult& result, QString* errorText)
{
    QJsonObject root;
    root["symbol"] = request.symbol;
    root["timeframe"] = toUiString(request.timeframe);
    root["strategy"] = request.strategyName;
    root["begin"] = request.begin.toString(Qt::ISODate);
    root["end"] = request.end.toString(Qt::ISODate);
    root["state"] = toString(result.state);
    root["errorText"] = result.errorText;

    QJsonObject stats;
    stats["trades"] = result.stats.trades;
    stats["winratePct"] = result.stats.winratePct;
    stats["profitFactor"] = result.stats.profitFactor;
    stats["grossPnl"] = result.stats.grossPnl;
    stats["netPnl"] = result.stats.netPnl;
    stats["totalFees"] = result.stats.totalFees;
    stats["avgWin"] = result.stats.avgWin;
    stats["avgLoss"] = result.stats.avgLoss;
    stats["bestTrade"] = result.stats.bestTrade;
    stats["worstTrade"] = result.stats.worstTrade;
    stats["avgBarsHeld"] = result.stats.avgBarsHeld;
    stats["maxDrawdown"] = result.stats.maxDrawdown;
    stats["maxDrawdownPct"] = result.stats.MaxDDPct;
    stats["expectancy"] = result.stats.expectancy;
    root["stats"] = stats;

    QJsonArray trades;
    for (const BacktestTrade& trade : result.trades) {
        trades.append(tradeToJson(trade));
    }
    root["trades"] = trades;

    QFile file(BacktestReportExporter::summaryPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (errorText) {
            *errorText = file.errorString();
        }
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool writeTradesCsv(const BacktestResult& result, QString* errorText)
{
    QFile file(BacktestReportExporter::tradesPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (errorText) {
            *errorText = file.errorString();
        }
        return false;
    }

    QTextStream out(&file);
    out << "entry_time,exit_time,side,quantity,entry_price,exit_price,gross_pnl,net_pnl,fee,bars_held,winner\n";
    for (const BacktestTrade& trade : result.trades) {
        out << csvEscape(trade.entryTime.toString(Qt::ISODate)) << ','
            << csvEscape(trade.exitTime.toString(Qt::ISODate)) << ','
            << sideToString(trade.side) << ','
            << QString::number(trade.quantity, 'f', 8) << ','
            << QString::number(trade.entryPrice, 'f', 8) << ','
            << QString::number(trade.exitPrice, 'f', 8) << ','
            << QString::number(trade.grossPnl, 'f', 8) << ','
            << QString::number(trade.netPnl, 'f', 8) << ','
            << QString::number(trade.feePaid, 'f', 8) << ','
            << trade.barsHeld << ','
            << (trade.winner ? "true" : "false") << '\n';
    }
    return true;
}

} // namespace

QString BacktestReportExporter::reportsDir()
{
    const QString dir = appDataDir() + "/reports";
    QDir().mkpath(dir);
    return dir;
}

QString BacktestReportExporter::summaryPath()
{
    return reportsDir() + "/latest_backtest_summary.json";
}

QString BacktestReportExporter::tradesPath()
{
    return reportsDir() + "/latest_backtest_trades.csv";
}

bool BacktestReportExporter::exportLatest(const BacktestRequest& request, const BacktestResult& result, QString* errorText)
{
    QString summaryError;
    if (!writeSummary(request, result, &summaryError)) {
        if (errorText) {
            *errorText = "summary: " + summaryError;
        }
        return false;
    }

    QString tradesError;
    if (!writeTradesCsv(result, &tradesError)) {
        if (errorText) {
            *errorText = "trades: " + tradesError;
        }
        return false;
    }

    return true;
}
