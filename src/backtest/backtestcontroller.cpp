#include "backtestcontroller.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <utility>
#include <memory>

BacktestController::BacktestController(Ui::MainWindow* ui, std::shared_ptr<IExchangeClient> exchange, QObject* parent) :
    QObject(parent), ui_(ui), exchange_(std::move(exchange)),
    marketDataService_(std::unique_ptr<BacktestMarketDataService>(new BacktestMarketDataService(exchange_)))
{
    Q_ASSERT(ui_);
    Q_ASSERT(exchange_);
    Q_ASSERT(marketDataService_);

    connect(ui_->btBtnStart, &QPushButton::clicked, this, &BacktestController::onStart);
    connect(ui_->btBtnBuildGraph, &QPushButton::clicked, this, &BacktestController::onBuildGraph);
}

std::vector<Candle> BacktestController::loadHistory(const HistoryRequest& rec) const {
    if (!marketDataService_) return {};

    return marketDataService_->loadHistory(rec);
}

HistoryRequest BacktestController::buildHistoryRequest() const {
    if (!ui_) return {};
    HistoryRequest r;

    r.symbolId = ui_->cbBtSymbol->currentData().toString();
    r.timeframe = static_cast<Timeframe>(ui_->cbBtTimeframe->currentData().toInt());
    r.begin = ui_->dateBtBegin->dateTime();
    r.end = ui_->dateBtEnd->dateTime();
    return r;
}

BacktestRequest BacktestController::buildBacktestRequest(const HistoryRequest& hr) const {
    if (!ui_) return {};
    BacktestRequest r;
    r.symbol = hr.symbolId;
    r.timeframe = hr.timeframe;
    r.begin = hr.begin;
    r.end = hr.end;
    r.initialbalance = ui_->btSbStartBalance->value();
    r.backtestRisk.leverage = ui_->btSbLeverage->value();
    r.backtestRisk.feePct = ui_->btSbFee->value();
    r.backtestRisk.slippageBps = ui_->btSbSlippage->value();
    r.backtestRisk.riskPct = ui_->btSbRiskPerTrade->value();
    r.backtestRisk.maxPosUsdt = ui_->btSbMaxPosUsdt->value();
    if (ui_->cbRiskMode->currentText() == "Fixed USDT") {
        r.backtestRisk.mode = RiskMode::FixedUsdt;
    } else {
        r.backtestRisk.mode = RiskMode::PercentOfEquity;
    }
    r.strategyName = ui_->btCbStrategy->currentText();
    return r;
}

std::vector<GraphPoint> BacktestController::buildEquityGraph(const BacktestResult& res) const {
    if (res.equityCurve.size() == 0) return {};
    std::vector<GraphPoint> points;
    points.reserve(res.equityCurve.size());
    for (int i = 0; i < static_cast<int>(res.equityCurve.size()); ++i) {
        GraphPoint p;
        p.x = static_cast<double>(i);
        p.y = res.equityCurve[i].equity;
        points.push_back(p);
    }
    return points;
}

/*std::vector<Candle> BacktestController::loadHistoryFromUi() const {
    return loadHistory(buildHistoryRequest());
}*/

void BacktestController::onStart() {
    HistoryRequest req = buildHistoryRequest();
    const std::vector<Candle> candles = loadHistory(req);
    BacktestRequest BtReq = buildBacktestRequest(req);
    lastResult_ = engine_.run(BtReq, candles);
    hasResult = true;
    qDebug() << "[BACKTEST] START  symbol =" << req.symbolId << ", tf =" << toUiString(req.timeframe)
           << ", begin =" << req.begin.toString() << ", end =" << req.end.toString() << ", size =" << candles.size();
    qDebug() << "[BACKTEST RESULT]   state =" << toString(lastResult_.state) << ", err =" << lastResult_.errorText;
    qDebug() << "[BACKTEST RESULT EQUITY CURVE]   size =" << lastResult_.equityCurve.size();
    if (!lastResult_.equityCurve.empty()) {
        qDebug() << "[BACKTEST RESULT EQCURVE]   start time ="
                 << lastResult_.equityCurve.front().time.toString("yyyy-MM-dd HH:mm:ss") <<
                    ", end time =" << lastResult_.equityCurve.back().time.toString("yyyy-MM-dd HH:mm:ss") <<
                    ", start equity =" << lastResult_.equityCurve.front().equity <<
                    ", end equity =" << lastResult_.equityCurve.back().equity;
    }
}

void BacktestController::onBuildGraph() {
    if (!hasResult) {
        qDebug() << "[BUILD BT GRAPH] Don't have BacktestResult";
        return;
    }
    std::vector<GraphPoint> points = buildEquityGraph(lastResult_);
    qDebug() << "[BUILD BT GRAPH]   size =" << points.size();
    if (!points.empty()) {
        qDebug() << "[BUILD BT GRAPH]   fst x =" << points.front().x << ", fst y =" << points.front().y <<
                    ", last x =" << points.back().x << ", last y =" << points.back().y;
    }
}
