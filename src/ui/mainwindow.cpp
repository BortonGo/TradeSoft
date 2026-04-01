#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core\timeframe.h"
#include "core\symbol.h"
#include "service\marketdataservice.h"
#include "exchange\bingxswapclient.h"
#include "controllers/strategycontroller.h"
#include "ui\chartwidget.h"
#include "ui\indicatordialog.h"

#include <QStandardItemModel>
#include <QHeaderView>

#include <memory>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    for(const Symbol& s : someSymbols()) {
        ui->comboSymbol->addItem(s.display(), s.id());
        ui->cbBtSymbol->addItem(s.display(), s.id());
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
    ui->btCbXAxis->addItem("Trade Index", static_cast<int>(GraphAxis::TradeIndex));
    ui->btCbXAxis->addItem("Time", static_cast<int>(GraphAxis::Time));

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
    std::shared_ptr<IExchangeClient> ex = std::make_shared<BingXSwapClient>();

    //BacktestController
    backtestController = new BacktestController(ui, ex, this);

    //MarketDataService
    MarketDataService* market = new MarketDataService(ex, this);
    marketData_ = market;

    //MarketDataStrategy
    marketDataStrategy_ = new MarketDataService(ex, this);

    // StrategyController
    strategyController_ = new StrategyController(ui, marketDataStrategy_, this);

    connect(marketData_, &MarketDataService::signal_seriesLoaded, ui->chartWidget, &ChartWidget::slot_setSeries);
    connect(marketData_, &MarketDataService::signal_candleUpdated, ui->chartWidget, &ChartWidget::slot_onCandleUpdate);
    connect(marketData_, &MarketDataService::signal_candleClosed, ui->chartWidget, &ChartWidget::slot_onCandleClosed);

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
