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

    connect(ui_->btCbGraphType, &QComboBox::currentTextChanged, this, &BacktestController::onGraphTypeChanged);
    connect(ui_->btCbXAxis, &QComboBox::currentTextChanged, this, &BacktestController::onGraphAxisChanged);
    connect(ui_->btCbYAxis, &QComboBox::currentTextChanged, this, &BacktestController::onGraphAxisChanged);

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
    r.backtestRisk.mode = static_cast<RiskMode>(ui_->cbRiskMode->currentData().toInt());
    r.strategyName = ui_->btCbStrategy->currentText();
    return r;
}

std::vector<GraphPoint> BacktestController::buildEquityGraph(const BacktestResult& res, const GraphRequest& gr) const {
    if (res.equityCurve.empty()) return {};
    std::vector<GraphPoint> points;
    points.reserve(res.equityCurve.size());
    for (int i = 0; i < static_cast<int>(res.equityCurve.size()); ++i) {
        GraphPoint p;
        if (gr.xAxis == GraphAxis::TradeIndex) {
            p.x = static_cast<double>(i);
        } else {
            p.x = static_cast<double>(res.equityCurve[i].time.toMSecsSinceEpoch());
        }
        p.y = res.equityCurve[i].equity;
        points.push_back(p);
    }
    return points;
}

std::vector<GraphPoint> BacktestController::buildDrawdownGraph(const BacktestResult& res, const GraphRequest& gr) const {
    if (res.equityCurve.empty()) return {};
    std::vector<GraphPoint> points;
    points.reserve(res.equityCurve.size());
    for (int i = 0; i < static_cast<int>(res.equityCurve.size()); ++i) {
        GraphPoint p;
        if (gr.xAxis == GraphAxis::Time) {
            p.x = static_cast<double>(res.equityCurve[i].time.toMSecsSinceEpoch());
        } else {
            p.x = static_cast<double>(i);
        }
        p.y = res.equityCurve[i].drawdown;
        points.push_back(p);
    }
    return points;
}

GraphRequest BacktestController::buildGraphRequest() const {
    if (!ui_) return {};
    GraphRequest gr;
    gr.type = static_cast<GraphType>(ui_->btCbGraphType->currentData().toInt());
    gr.xAxis = static_cast<GraphAxis>(ui_->btCbXAxis->currentData().toInt());
    gr.yAxis = static_cast<GraphAxis>(ui_->btCbYAxis->currentData().toInt());
    gr.longOnly = ui_->btChkLongOnly->isChecked();
    gr.shortOnly = ui_->btChkShortOnly->isChecked();
    gr.winnersOnly = ui_->btChkWinnersOnly->isChecked();
    gr.losersOnly = ui_->btChkLosersOnly->isChecked();
    return gr;
}

/*std::vector<Candle> BacktestController::loadHistoryFromUi() const {
    return loadHistory(buildHistoryRequest());
}*/

void BacktestController::onStart() {
    HistoryRequest req = buildHistoryRequest();
    const std::vector<Candle> candles = loadHistory(req);
    BacktestRequest BtReq = buildBacktestRequest(req);
    lastResult_ = engine_.run(BtReq, candles);
    hasResult_ = true;
    qDebug() << "[BACKTEST] START  symbol =" << req.symbolId << ", tf =" << toUiString(req.timeframe)
           << ", begin =" << req.begin.toString("yyyy-MM-dd HH:mm:ss") << ", end =" << req.end.toString("yyyy-MM-dd HH:mm:ss") << ", size =" << candles.size();
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
    if (!hasResult_) {
        qDebug() << "[BUILD BT GRAPH] Don't have BacktestResult";
        return;
    }
    GraphRequest gr = buildGraphRequest();

    switch (gr.type) {
        case GraphType::EquityCurve : {
            std::vector<GraphPoint> points = buildEquityGraph(lastResult_, gr);
            qDebug() << "[BUILD EQ GRAPH]   size =" << points.size();
            if (!points.empty()) {
                qDebug() << "[BUILD EQ GRAPH]   fst x =" << points.front().x << ", fst y =" << points.front().y <<
                            ", last x =" << points.back().x << ", last y =" << points.back().y;
            }
            break;
        }
        case GraphType::DrawdownCurve : {
            std::vector<GraphPoint> points = buildDrawdownGraph(lastResult_, gr);
            qDebug() << "[BUILD DD GRAPH]   size =" << points.size();
            if (!points.empty()) {
                qDebug() << "[BUILD DD GRAPH]   fst x =" << points.front().x << ", fst y =" << points.front().y <<
                            ", last x =" << points.back().x << ", last y =" << points.back().y;
            }
            break;
        }
        default : {
             qDebug() << "[BUILD BT GRAPH] graph type not implemented";
        }
    }
}

void BacktestController::onGraphTypeChanged() {
    const GraphType gt = static_cast<GraphType>(ui_->btCbGraphType->currentData().toInt());
    switch (gt) {
        case GraphType::EquityCurve : {
            ui_->btCbXAxis->blockSignals(true);
            ui_->btCbYAxis->blockSignals(true);

            const int xIndex = ui_->btCbXAxis->findData(static_cast<int>(GraphAxis::Time));
            if (xIndex >= 0) {
                ui_->btCbXAxis->setCurrentIndex(xIndex);
            }
            const int yIndex = ui_->btCbYAxis->findData(static_cast<int>(GraphAxis::Equity));
            if (yIndex >= 0) {
                ui_->btCbYAxis->setCurrentIndex(yIndex);
            }

            ui_->btCbXAxis->blockSignals(false);
            ui_->btCbYAxis->blockSignals(false);
            break;
        }

        case GraphType::DrawdownCurve : {
            ui_->btCbXAxis->blockSignals(true);
            ui_->btCbYAxis->blockSignals(true);

            const int xIndex = ui_->btCbXAxis->findData(static_cast<int>(GraphAxis::Time));
            if (xIndex >= 0) {
                ui_->btCbXAxis->setCurrentIndex(xIndex);
            }
            const int yIndex = ui_->btCbYAxis->findData(static_cast<int>(GraphAxis::Drawdown));
            if (yIndex >= 0) {
                ui_->btCbYAxis->setCurrentIndex(yIndex);
            }

            ui_->btCbXAxis->blockSignals(false);
            ui_->btCbYAxis->blockSignals(false);
            break;
        }

        case GraphType::PnlByTrade : {
            ui_->btCbXAxis->blockSignals(true);
            ui_->btCbYAxis->blockSignals(true);

            const int xIndex = ui_->btCbXAxis->findData(static_cast<int>(GraphAxis::TradeIndex));
            if (xIndex >= 0) {
                ui_->btCbXAxis->setCurrentIndex(xIndex);
            }
            const int yIndex = ui_->btCbYAxis->findData(static_cast<int>(GraphAxis::NetPnl));
            if (yIndex >= 0) {
                ui_->btCbYAxis->setCurrentIndex(yIndex);
            }

            ui_->btCbXAxis->blockSignals(false);
            ui_->btCbYAxis->blockSignals(false);
            break;
        }

        case GraphType::Scatter : {
            break;
        }

        case GraphType::Custom : {
            break;
        }

        default: {
            break;
        }

    }
}

void BacktestController::onGraphAxisChanged() {
    const GraphAxis gaX = static_cast<GraphAxis>(ui_->btCbXAxis->currentData().toInt());
    const GraphAxis gaY = static_cast<GraphAxis>(ui_->btCbYAxis->currentData().toInt());
    if (gaX == GraphAxis::Time && gaY == GraphAxis::Equity) {
        const int index = ui_->btCbGraphType->findData(static_cast<int>(GraphType::EquityCurve));
        if (index >= 0) {
            ui_->btCbGraphType->blockSignals(true);
            ui_->btCbGraphType->setCurrentIndex(index);
            ui_->btCbGraphType->blockSignals(false);
        }
    } else if (gaX == GraphAxis::Time && gaY == GraphAxis::Drawdown) {
        const int index = ui_->btCbGraphType->findData(static_cast<int>(GraphType::DrawdownCurve));
        if (index >= 0) {
            ui_->btCbGraphType->blockSignals(true);
            ui_->btCbGraphType->setCurrentIndex(index);
            ui_->btCbGraphType->blockSignals(false);
        }
    } else if (gaX == GraphAxis::TradeIndex && gaY == GraphAxis::NetPnl) {
        const int index = ui_->btCbGraphType->findData(static_cast<int>(GraphType::PnlByTrade));
        if (index >= 0) {
            ui_->btCbGraphType->blockSignals(true);
            ui_->btCbGraphType->setCurrentIndex(index);
            ui_->btCbGraphType->blockSignals(false);
        }
    } else {
        const int index = ui_->btCbGraphType->findData(static_cast<int>(GraphType::Custom));
        if (index >= 0) {
            ui_->btCbGraphType->blockSignals(true);
            ui_->btCbGraphType->setCurrentIndex(index);
            ui_->btCbGraphType->blockSignals(false);
        }
    }

}
