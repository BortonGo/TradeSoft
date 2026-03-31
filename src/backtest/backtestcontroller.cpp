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
    return r;
}

/*std::vector<Candle> BacktestController::loadHistoryFromUi() const {
    return loadHistory(buildHistoryRequest());
}*/

void BacktestController::onStart() {
    HistoryRequest req = buildHistoryRequest();
    const std::vector<Candle> candles = loadHistory(req);
    BacktestRequest BtReq = buildBacktestRequest(req);
    BacktestResult res = engine_.run(BtReq, candles);
    qDebug() << "[BACKTEST] START  symbol =" << req.symbolId << ", tf =" << toUiString(req.timeframe)
           << ", begin =" << req.begin.toString() << ", end =" << req.end.toString() << ", size =" << candles.size();
    qDebug() << "[BACKTEST RESULT]   state =" << toString(res.state) << ", err =" << res.errorText;
}

void BacktestController::onBuildGraph() {

}
