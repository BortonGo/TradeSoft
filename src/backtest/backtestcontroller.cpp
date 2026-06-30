#include "backtestcontroller.h"
#include "backtest/backtestreportexporter.h"
#include "ui_mainwindow.h"
#include <QDesktopServices>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QUrl>
#include <utility>
#include <memory>

Q_LOGGING_CATEGORY(logBacktestUi, "tradesoft.backtest.ui")

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

    setupReportUi();

    tradesModel_ = new BacktestTradesModel(this);
    ui_->btTableTrades->setModel(tradesModel_);
    ui_->btTableTrades->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui_->btTableTrades->setSelectionMode(QAbstractItemView::SingleSelection);
    ui_->btTableTrades->setAlternatingRowColors(true);

    auto* header = ui_->btTableTrades->horizontalHeader();
    header->setVisible(true);
    header->setSectionResizeMode(QHeaderView::Interactive);

    header->setSectionResizeMode(BacktestTradesModel::ColEntryTime, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColExitTime, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColSide, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColQty, QHeaderView::ResizeToContents);

    header->setSectionResizeMode(BacktestTradesModel::ColEntryPrice, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColExitPrice, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColGrossPnl, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColNetPnl, QHeaderView::Stretch);
    header->setSectionResizeMode(BacktestTradesModel::ColFee, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColBarsHeld, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(BacktestTradesModel::ColWinner, QHeaderView::ResizeToContents);

    graphWidget_ = new BacktestWidget(ui_->backtestWidget);
    auto* layout = qobject_cast<QVBoxLayout*>(ui_->backtestWidget->layout());
    if (!layout) {
        layout = new QVBoxLayout(ui_->backtestWidget);
        layout->setContentsMargins(0, 0, 0, 0);
    }

    layout->addWidget(graphWidget_);
}

static QLineEdit* makeReadonlyMetric(QWidget* parent)
{
    auto* edit = new QLineEdit(parent);
    edit->setReadOnly(true);
    edit->setFixedHeight(26);
    edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    edit->setStyleSheet("background-color: rgb(35, 35, 35); color: white;");
    return edit;
}

static QLabel* makeMetricLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setFixedHeight(26);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    label->setStyleSheet("color: #9A9A9A;");
    return label;
}

void BacktestController::setupReportUi()
{
    auto* parent = ui_->BtGbBacktestResults;
    auto* grid = ui_->gridLayout_8;

    ui_->BtResultsFrame->setFixedHeight(126);
    parent->setFixedHeight(104);
    grid->setContentsMargins(10, 18, 10, 8);
    grid->setVerticalSpacing(6);
    grid->setHorizontalSpacing(8);

    const auto normalizeMetricWidget = [](QWidget* widget) {
        if (!widget) return;
        widget->setFixedHeight(24);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    };

    const QList<QWidget*> existingMetricWidgets = {
        ui_->btLabelTradesValue, ui_->btPf, ui_->btWR, ui_->btNetPnl,
        ui_->btAvgWin, ui_->btMaxDD, ui_->btAvgLoss, ui_->btEcpecrancy,
        ui_->label_27, ui_->label_28, ui_->label_29, ui_->label_30,
        ui_->label_31, ui_->label_32, ui_->label_33, ui_->label_34
    };
    for (QWidget* widget : existingMetricWidgets) {
        normalizeMetricWidget(widget);
    }

    for (int row = 0; row < 2; ++row) {
        grid->setRowMinimumHeight(row, 24);
        grid->setRowStretch(row, 0);
    }
    for (int col = 0; col < 14; ++col) {
        grid->setColumnStretch(col, (col % 2 == 0) ? 0 : 1);
    }

    grossPnlEdit_ = makeReadonlyMetric(parent);
    feesEdit_ = makeReadonlyMetric(parent);
    bestTradeEdit_ = makeReadonlyMetric(parent);
    worstTradeEdit_ = makeReadonlyMetric(parent);
    avgBarsHeldEdit_ = makeReadonlyMetric(parent);

    grid->addWidget(ui_->label_27, 0, 0);
    grid->addWidget(ui_->btLabelTradesValue, 0, 1);
    grid->addWidget(ui_->label_29, 0, 2);
    grid->addWidget(ui_->btWR, 0, 3);
    grid->addWidget(ui_->label_28, 0, 4);
    grid->addWidget(ui_->btPf, 0, 5);
    grid->addWidget(ui_->label_30, 0, 6);
    grid->addWidget(ui_->btNetPnl, 0, 7);
    grid->addWidget(makeMetricLabel("Gross:", parent), 0, 8);
    grid->addWidget(grossPnlEdit_, 0, 9);
    grid->addWidget(makeMetricLabel("Fees:", parent), 0, 10);
    grid->addWidget(feesEdit_, 0, 11);
    grid->addWidget(ui_->label_32, 0, 12);
    grid->addWidget(ui_->btMaxDD, 0, 13);

    grid->addWidget(ui_->label_31, 1, 0);
    grid->addWidget(ui_->btAvgWin, 1, 1);
    grid->addWidget(ui_->label_33, 1, 2);
    grid->addWidget(ui_->btAvgLoss, 1, 3);
    grid->addWidget(ui_->label_34, 1, 4);
    grid->addWidget(ui_->btEcpecrancy, 1, 5);
    grid->addWidget(makeMetricLabel("Best:", parent), 1, 6);
    grid->addWidget(bestTradeEdit_, 1, 7);
    grid->addWidget(makeMetricLabel("Worst:", parent), 1, 8);
    grid->addWidget(worstTradeEdit_, 1, 9);
    grid->addWidget(makeMetricLabel("Avg bars:", parent), 1, 10);
    grid->addWidget(avgBarsHeldEdit_, 1, 11);

    openReportsButton_ = new QPushButton("Reports", parent);
    openReportsButton_->setFixedHeight(24);
    openReportsButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    openReportsButton_->setStyleSheet(
        "QPushButton { background-color: #2b2b2b; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #333333; }");
    grid->addWidget(makeMetricLabel("Reports:", parent), 1, 12);
    grid->addWidget(openReportsButton_, 1, 13);
    connect(openReportsButton_, &QPushButton::clicked, this, &BacktestController::openReportsFolder);
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
    if (r.strategyName == "EMA Scalp") {
        r.executionMode = BacktestExecutionMode::IntrabarLowerTf;
    } else {
        r.executionMode = BacktestExecutionMode::BarClose;
    }

    return r;
}

std::vector<GraphPoint> BacktestController::buildEquityGraph(const BacktestResult& res, const GraphRequest& gr) const {
    if (res.equityCurve.empty()) return {};
    std::vector<GraphPoint> points;
    std::vector<BacktestTrade> filteredTrades = filterTrades(res, gr);
    std::vector<EquityPoint> filteredCurve = buildEquityCurveFromTrades(filteredTrades, lastRequest_.initialbalance);
    points.reserve(filteredCurve.size());
    for (int i = 0; i < static_cast<int>(filteredCurve.size()); ++i) {
        GraphPoint p;
        if (gr.xAxis == GraphAxis::TradeIndex) {
            p.x = static_cast<double>(i);
        } else {
            p.x = static_cast<double>(filteredCurve[i].time.toMSecsSinceEpoch());
        }
        p.y = filteredCurve[i].equity;
        points.push_back(p);
    }
    return points;
}

std::vector<GraphPoint> BacktestController::buildDrawdownGraph(const BacktestResult& res, const GraphRequest& gr) const {
    if (res.equityCurve.empty()) return {};
    std::vector<GraphPoint> points;
    std::vector<BacktestTrade> filteredTrades = filterTrades(res, gr);
    std::vector<EquityPoint> filteredCurve = buildEquityCurveFromTrades(filteredTrades, lastRequest_.initialbalance);
    points.reserve(filteredCurve.size());
    for (int i = 0; i < static_cast<int>(filteredCurve.size()); ++i) {
        GraphPoint p;
        if (gr.xAxis == GraphAxis::TradeIndex) {
            p.x = static_cast<double>(i);
        } else {
            p.x = static_cast<double>(filteredCurve[i].time.toMSecsSinceEpoch());
        }
        p.y = filteredCurve[i].drawdown;
        points.push_back(p);
    }
    return points;
}

std::vector<GraphPoint> BacktestController::buildPnlByTradeGraph(const BacktestResult& res, const GraphRequest& gr) const {
    if (res.equityCurve.empty()) return {};
    std::vector<GraphPoint> points;
    std::vector<BacktestTrade> filteredTrades = filterTrades(res, gr);
    points.reserve(filteredTrades.size());
    for (int i = 0; i < static_cast<int>(filteredTrades.size()); ++i) {
        GraphPoint p;
        p.x = static_cast<double>(i);
        p.y = filteredTrades[i].netPnl;
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

void BacktestController::onStart() {
    HistoryRequest req = buildHistoryRequest();
    const std::vector<Candle> candles = loadHistory(req);
    BacktestRequest BtReq = buildBacktestRequest(req);
    lastRequest_ = BtReq;
    lastResult_ = engine_.run(BtReq, candles);
    hasResult_ = true;
    qCInfo(logBacktestUi) << "Start symbol =" << req.symbolId << ", tf =" << toUiString(req.timeframe)
                          << ", begin =" << req.begin.toString("yyyy-MM-dd HH:mm:ss")
                          << ", end =" << req.end.toString("yyyy-MM-dd HH:mm:ss")
                          << ", size =" << candles.size();
    qCDebug(logBacktestUi) << "Result state =" << toString(lastResult_.state) << ", err =" << lastResult_.errorText;
    qCDebug(logBacktestUi) << "Equity curve size =" << lastResult_.equityCurve.size();
    if (!lastResult_.equityCurve.empty()) {
        qCDebug(logBacktestUi) << "Equity curve start time ="
                               << lastResult_.equityCurve.front().time.toString("yyyy-MM-dd HH:mm:ss")
                               << ", end time =" << lastResult_.equityCurve.back().time.toString("yyyy-MM-dd HH:mm:ss")
                               << ", start equity =" << lastResult_.equityCurve.front().equity
                               << ", end equity =" << lastResult_.equityCurve.back().equity;
    }
    setResultsToUi();
    setTradesToUi();
    ui_->editBtCandles->setText(QString::number(candles.size()));

    QString exportError;
    BacktestReportExporter::ReportPaths reportPaths;
    if (BacktestReportExporter::exportLatest(lastRequest_, lastResult_, &exportError, &reportPaths)) {
        qCInfo(logBacktestUi) << "Exported latest summary =" << reportPaths.latestSummary
                              << ", latest trades =" << reportPaths.latestTrades
                              << ", snapshot summary =" << reportPaths.snapshotSummary
                              << ", snapshot trades =" << reportPaths.snapshotTrades;
    } else {
        qCWarning(logBacktestUi) << "Export failed:" << exportError;
    }
}

void BacktestController::onBuildGraph() {
    if (!hasResult_) {
        qCDebug(logBacktestUi) << "Build graph skipped: no BacktestResult";
        return;
    }
    GraphRequest gr = buildGraphRequest();

    switch (gr.type) {
        case GraphType::EquityCurve : {
            std::vector<GraphPoint> points = buildEquityGraph(lastResult_, gr);
            qCDebug(logBacktestUi) << "Build equity graph size =" << points.size();
            if (!points.empty()) {
                qCDebug(logBacktestUi) << "Build equity graph fst x =" << points.front().x << ", fst y =" << points.front().y
                                       << ", last x =" << points.back().x << ", last y =" << points.back().y;
            }
            graphWidget_->setPoints(points);
            break;
        }
        case GraphType::DrawdownCurve : {
            std::vector<GraphPoint> points = buildDrawdownGraph(lastResult_, gr);
            qCDebug(logBacktestUi) << "Build drawdown graph size =" << points.size();
            if (!points.empty()) {
                qCDebug(logBacktestUi) << "Build drawdown graph fst x =" << points.front().x << ", fst y =" << points.front().y
                                       << ", last x =" << points.back().x << ", last y =" << points.back().y;
            }
            graphWidget_->setPoints(points);
            break;
        }
    case GraphType::PnlByTrade : {
        std::vector<GraphPoint> points = buildPnlByTradeGraph(lastResult_, gr);
        qCDebug(logBacktestUi) << "Build PnL graph size =" << points.size();
        if (!points.empty()) {
            qCDebug(logBacktestUi) << "Build PnL graph fst x =" << points.front().x << ", fst y =" << points.front().y
                                   << ", last x =" << points.back().x << ", last y =" << points.back().y;
        }
        graphWidget_->setPoints(points);
        break;
    }
        default : {
            graphWidget_->clearPoints();
            qCDebug(logBacktestUi) << "Graph type not implemented";
            break;
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

void BacktestController::setResultsToUi() {
    ui_->btLabelTradesValue->setText(QString::number(lastResult_.stats.trades));
    ui_->btWR->setText(QString::number(lastResult_.stats.winratePct, 'f', 2));
    ui_->btAvgWin->setText(QString::number(lastResult_.stats.avgWin, 'f', 2));
    ui_->btAvgLoss->setText(QString::number(lastResult_.stats.avgLoss, 'f', 2));
    ui_->btPf->setText(QString::number(lastResult_.stats.profitFactor, 'f', 2));
    ui_->btNetPnl->setText(QString::number(lastResult_.stats.netPnl, 'f', 2));
    ui_->btMaxDD->setText(QString("%1 / %2%")
                              .arg(lastResult_.stats.maxDrawdown, 0, 'f', 2)
                              .arg(lastResult_.stats.MaxDDPct, 0, 'f', 2));
    ui_->btEcpecrancy->setText(QString::number(lastResult_.stats.expectancy, 'f', 2));

    if (grossPnlEdit_) grossPnlEdit_->setText(QString::number(lastResult_.stats.grossPnl, 'f', 2));
    if (feesEdit_) feesEdit_->setText(QString::number(lastResult_.stats.totalFees, 'f', 2));
    if (bestTradeEdit_) bestTradeEdit_->setText(QString::number(lastResult_.stats.bestTrade, 'f', 2));
    if (worstTradeEdit_) worstTradeEdit_->setText(QString::number(lastResult_.stats.worstTrade, 'f', 2));
    if (avgBarsHeldEdit_) avgBarsHeldEdit_->setText(QString::number(lastResult_.stats.avgBarsHeld, 'f', 2));
}

void BacktestController::setTradesToUi() {
    tradesModel_->setTrades(lastResult_.trades);
}

void BacktestController::openReportsFolder() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(BacktestReportExporter::reportsDir()));
}

std::vector<BacktestTrade> BacktestController::filterTrades(const BacktestResult& res, const GraphRequest& gr) const {
    std::vector<BacktestTrade> result;
    for (const auto& a : res.trades) {
        if (gr.longOnly && a.side != BacktestTradeSide::Long) continue;
        if (gr.shortOnly && a.side != BacktestTradeSide::Short) continue;
        if (gr.winnersOnly && !a.winner) continue;
        if (gr.losersOnly && a.winner) continue;
        result.push_back(a);
    }
    return result;
}

std::vector<EquityPoint> BacktestController::buildEquityCurveFromTrades(const std::vector<BacktestTrade>& trades, double initialbalance) const {
    std::vector<EquityPoint> result;
    result.reserve(trades.size());
    double peakEquity = initialbalance;
    double currentEquity = initialbalance;
    for (const auto& a : trades) {
        currentEquity += a.netPnl;
        EquityPoint p;
        p.time = a.exitTime;
        p.equity = currentEquity;
        peakEquity = std::max(peakEquity, p.equity);
        p.drawdown = peakEquity - p.equity;
        p.cumulativePnl = currentEquity - initialbalance;
        result.push_back(p);
    }
    return result;
}
