#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core/timeframe.h"
#include "core/symbol.h"
#include "service/marketdataservice.h"
#include "exchange/bingxswapclient.h"
#include "controllers/strategycontroller.h"
#include "ui/chartwidget.h"
#include "ui/indicatordialog.h"

#include <QStandardItemModel>
#include <QHeaderView>
#include <QDebug>
#include <QStatusBar>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>

#include <memory>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    config_ = AppConfig::load();
    qDebug() << "[AppConfig] Loaded from" << AppConfig::configFilePath()
             << "defaultSymbol=" << config_.defaultSymbolId
             << "defaultTimeframe=" << toUiString(config_.defaultTimeframe)
             << "pollingMs=" << config_.realtimePollingMs;

    const auto setFixedComboSize = [](QComboBox* combo, int width, int height) {
        if (!combo) {
            return;
        }
        combo->setFixedSize(width, height);
        combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    };

    setFixedComboSize(ui->comboSymbol, 120, 30);
    setFixedComboSize(ui->comboTimeframe, 72, 30);
    setFixedComboSize(ui->cbStrategy, 120, 30);
    setFixedComboSize(ui->cbStrategyTf, 72, 30);

    if (auto* strategyLayout = qobject_cast<QHBoxLayout*>(ui->SetStrategyFrame->layout())) {
        strategyLayout->setContentsMargins(10, 7, 10, 7);
        strategyLayout->setSpacing(8);
    }

    if (auto* strategyBlocksLayout = qobject_cast<QHBoxLayout*>(ui->RmFrame->layout())) {
        strategyBlocksLayout->setContentsMargins(10, 10, 10, 10);
        strategyBlocksLayout->setSpacing(10);
        strategyBlocksLayout->setStretch(0, 1);
        strategyBlocksLayout->setStretch(1, 1);
    }

    ui->gbRM->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->gbAccount->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->gbRM->setMinimumWidth(0);
    ui->gbAccount->setMinimumWidth(0);

    if (auto* rmGrid = qobject_cast<QGridLayout*>(ui->gbRM->layout())) {
        rmGrid->setVerticalSpacing(8);
        rmGrid->setHorizontalSpacing(10);
        rmGrid->setColumnStretch(0, 0);
        rmGrid->setColumnStretch(1, 1);
        for (int row = 0; row < 7; ++row) {
            rmGrid->setRowMinimumHeight(row, 34);
        }
    }

    const QList<QWidget*> rmControls = {
        ui->cbRiskMode,
        ui->spinRiskPct,
        ui->spinMaxPosUSDT,
        ui->spinLeverage,
        ui->spinFeePct,
        ui->spinSlippageBps
    };
    for (QWidget* widget : rmControls) {
        if (widget) {
            widget->setFixedHeight(34);
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    const QList<QCheckBox*> rmCheckBoxes = {
        ui->chkAllowLong,
        ui->chkAllowShort
    };
    for (QCheckBox* checkbox : rmCheckBoxes) {
        if (checkbox) {
            checkbox->setFixedHeight(34);
            checkbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
    }

    if (auto* rmGrid = qobject_cast<QGridLayout*>(ui->gbRM->layout())) {
        auto* checkBoxRow = new QWidget(ui->gbRM);
        auto* checkBoxLayout = new QHBoxLayout(checkBoxRow);
        checkBoxLayout->setContentsMargins(0, 0, 0, 0);
        checkBoxLayout->setSpacing(28);
        checkBoxLayout->addStretch(1);
        checkBoxLayout->addWidget(ui->chkAllowLong);
        checkBoxLayout->addWidget(ui->chkAllowShort);
        checkBoxLayout->addStretch(1);
        rmGrid->addWidget(checkBoxRow, 4, 0, 1, 2);
    }

    const QList<QWidget*> rmLabels = {
        ui->label,
        ui->label_2,
        ui->label_3,
        ui->label_4,
        ui->label_7,
        ui->label_8
    };
    for (QWidget* widget : rmLabels) {
        if (widget) {
            widget->setFixedHeight(34);
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    if (auto* accountGrid = qobject_cast<QGridLayout*>(ui->gbAccount->layout())) {
        accountGrid->setVerticalSpacing(8);
        accountGrid->setHorizontalSpacing(10);
        accountGrid->setColumnStretch(0, 0);
        accountGrid->setColumnStretch(1, 1);
        for (int row = 0; row < 6; ++row) {
            accountGrid->setRowMinimumHeight(row, 34);
        }
    }

    const QList<QWidget*> accountControls = {
        ui->cbAccountType,
        ui->cbAccount,
        ui->lblBalanceValue,
        ui->lblEquityValue,
        ui->lblFreeValue,
        ui->lblUsedValue,
        ui->lblUPnlValie
    };
    for (QWidget* widget : accountControls) {
        if (widget) {
            widget->setFixedHeight(34);
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    // table model, to header dont crash (idk why crash)
    ui->tableTrades->setModel(new QStandardItemModel(0, 7, ui->tableTrades));

    auto header = ui->tableTrades->horizontalHeader();

    header->setVisible(true);
    header->setSectionResizeMode(QHeaderView::Interactive); // Based mode

    header->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Time
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Symbol
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Side
    header->setSectionResizeMode(3, QHeaderView::Stretch);          // Qty
    header->setSectionResizeMode(4, QHeaderView::Stretch);          // Price
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents); // Fee
    header->setSectionResizeMode(6, QHeaderView::Stretch);          // Status

    // block signals comboSymbol/Timeframne
    // make for correct work om_combo...IndexChanged
    ui->comboSymbol->blockSignals(true);
    ui->comboTimeframe->blockSignals(true);

    // Timeframes
    ui->comboTimeframe->clear();
    ui->cbStrategyTf->clear();
    ui->cbBtTimeframe->clear();

    for(const Timeframe& tf : allTimeframes()) {
        ui->comboTimeframe->addItem(toUiString(tf), static_cast<int>(tf));
        ui->cbStrategyTf->addItem(toUiString(tf), static_cast<int>(tf));
        ui->cbBtTimeframe->addItem(toUiString(tf), static_cast<int>(tf));
    };

    // Some symbols
    ui->comboSymbol->clear();
    ui->cbBtSymbol->clear();

    for(const Symbol& s : config_.symbols) {
        ui->comboSymbol->addItem(s.display(), s.id());
        ui->cbBtSymbol->addItem(s.display(), s.id());
    }

    const int defaultSymbolIdx = ui->comboSymbol->findData(config_.defaultSymbolId);
    if (defaultSymbolIdx >= 0) {
        ui->comboSymbol->setCurrentIndex(defaultSymbolIdx);
        ui->cbBtSymbol->setCurrentIndex(defaultSymbolIdx);
    }

    const int defaultTfValue = static_cast<int>(config_.defaultTimeframe);
    const int defaultChartTfIdx = ui->comboTimeframe->findData(defaultTfValue);
    if (defaultChartTfIdx >= 0) {
        ui->comboTimeframe->setCurrentIndex(defaultChartTfIdx);
    }
    const int defaultStrategyTfIdx = ui->cbStrategyTf->findData(defaultTfValue);
    if (defaultStrategyTfIdx >= 0) {
        ui->cbStrategyTf->setCurrentIndex(defaultStrategyTfIdx);
    }
    const int defaultBacktestTfIdx = ui->cbBtTimeframe->findData(defaultTfValue);
    if (defaultBacktestTfIdx >= 0) {
        ui->cbBtTimeframe->setCurrentIndex(defaultBacktestTfIdx);
    }

    // Account
    AccountConfig demo;
    demo.id = "demo";
    demo.name = "Demo";
    demo.type = AccountType::Demo;
    demo.makerFeePct = 0.02;
    demo.takerFeePct = 0.06;
    demo.maxLeverage = 50;

    AccountState demoSt;
    demoSt.accountId = demo.id;
    demoSt.balance = 1000;
    demoSt.equity = 1000;

    accounts_.add(demo, demoSt);

    AccountConfig real;
    real.id = "real";
    real.name = "Real";
    real.type = AccountType::Real;
    real.makerFeePct = 0.02;
    real.takerFeePct = 0.06;
    real.maxLeverage = 50;

    AccountState realSt;
    realSt.accountId = real.id;
    realSt.balance = 0;
    realSt.equity = 0;

    accounts_.add(real, realSt);

    ui->cbAccount->clear();
    for (const auto& cfg : accounts_.configs()) {
        ui->cbAccount->addItem(cfg.name, cfg.id);
    }

    // Backtest params
    ui->cbRiskMode->clear();
    ui->cbRiskMode->addItem("Fixed USDT", static_cast<int>(RiskMode::FixedUsdt));
    ui->cbRiskMode->addItem("Percent Of Equity", static_cast<int>(RiskMode::PercentOfEquity));

    ui->btCbGraphType->clear();
    ui->btCbGraphType->addItem("Equity Curve", static_cast<int>(GraphType::EquityCurve));
    ui->btCbGraphType->addItem("Drawdown Curve", static_cast<int>(GraphType::DrawdownCurve));
    ui->btCbGraphType->addItem("PnL By Trade", static_cast<int>(GraphType::PnlByTrade));
    ui->btCbGraphType->addItem("Scatter", static_cast<int>(GraphType::Scatter));
    ui->btCbGraphType->addItem("Custom", static_cast<int>(GraphType::Custom));

    ui->btCbXAxis->clear();
    ui->btCbXAxis->addItem("Time", static_cast<int>(GraphAxis::Time));
    ui->btCbXAxis->addItem("Trade Index", static_cast<int>(GraphAxis::TradeIndex));

    ui->btCbYAxis->clear();
    ui->btCbYAxis->addItem("Equity", static_cast<int>(GraphAxis::Equity));
    ui->btCbYAxis->addItem("Net PnL", static_cast<int>(GraphAxis::NetPnl));
    ui->btCbYAxis->addItem("Gross PnL", static_cast<int>(GraphAxis::GrossPnl));
    ui->btCbYAxis->addItem("Drawdown", static_cast<int>(GraphAxis::Drawdown));
    ui->btCbYAxis->addItem("Entry Price", static_cast<int>(GraphAxis::EntryPrice));
    ui->btCbYAxis->addItem("Exit Price", static_cast<int>(GraphAxis::ExitPrice));
    ui->btCbYAxis->addItem("Quantity", static_cast<int>(GraphAxis::Quantity));
    ui->btCbYAxis->addItem("Bars Held", static_cast<int>(GraphAxis::BarsHeld));

    // unlock signals comboSymbol/Timeframne
    ui->comboSymbol->blockSignals(false);
    ui->comboTimeframe->blockSignals(false);

    //IExchangeClient
    std::shared_ptr<IExchangeClient> chartExchange = std::make_shared<BingXSwapClient>();
    std::shared_ptr<IExchangeClient> strategyExchange = std::make_shared<BingXSwapClient>();
    std::shared_ptr<IExchangeClient> backtestExchange = std::make_shared<BingXSwapClient>();

    //BacktestController
    backtestController = new BacktestController(ui, backtestExchange, this);

    //MarketDataService
    MarketDataService* market = new MarketDataService(chartExchange, this);
    market->setRealtimePollingMs(config_.realtimePollingMs);
    marketData_ = market;

    //MarketDataStrategy
    marketDataStrategy_ = new MarketDataService(strategyExchange, this);
    marketDataStrategy_->setRealtimePollingMs(config_.realtimePollingMs);

    // StrategyController
    strategyController_ = new StrategyController(ui, marketDataStrategy_, this);

    connect(marketData_, &MarketDataService::signal_seriesLoaded, ui->chartWidget, &ChartWidget::slot_setSeries);
    connect(marketData_, &MarketDataService::signal_candleUpdated, ui->chartWidget, &ChartWidget::slot_onCandleUpdate);
    connect(marketData_, &MarketDataService::signal_candleClosed, ui->chartWidget, &ChartWidget::slot_onCandleClosed);
    connect(marketData_, &MarketDataService::signal_connectionStateChanged, this, [this](const QString& state) {
        statusBar()->showMessage(state);
        qDebug() << "[MarketDataStatus]" << state;
    });

    indicatorService_ = new IndicatorService(marketData_, this);

    connect(indicatorService_, &IndicatorService::signal_overlayLinesUpdated, ui->chartWidget, &ChartWidget::setIndicatorLines);

    reloadAndStart();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_comboSymbol_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    reloadAndStart();
}

void MainWindow::on_comboTimeframe_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    reloadAndStart();
}

void MainWindow::reloadAndStart() {
    if (!marketData_) {
        return;
    }

    const QString currentSymbol = ui->comboSymbol->currentData().toString();
    const Timeframe tf = static_cast<Timeframe>(ui->comboTimeframe->currentData().toInt());

    marketData_->stopRealTime();
    marketData_->loadHistory(currentSymbol, tf);
    marketData_->startRealTime();
}

void MainWindow::on_btnIndicators_clicked()
{
    IndicatorDialog dlg(this);

        // восстановление состояния чекбоксов
        dlg.setConfig(indicatorService_->config());

        if (dlg.exec() == QDialog::Accepted) {
            indicatorService_->applyConfig(dlg.config());
        }
}
