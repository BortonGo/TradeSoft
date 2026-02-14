#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core\timeframe.h"
#include "core\symbol.h"
#include "service\marketdataservice.h"
#include "exchange\bingxswapclient.h"
#include "ui\chartwidget.h"
#include <memory>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // block signals comboSymbol/Timeframne
    // make for correct work om_combo...IndexChanged
    ui->comboSymbol->blockSignals(true);
    ui->comboTimeframe->blockSignals(true);

    // Timeframes
    ui->comboTimeframe->clear();

    for(const Timeframe& tf : allTimeframes()) {
        ui->comboTimeframe->addItem(toUiString(tf), static_cast<int>(tf));
    };

    // Some symbols
    ui->comboSymbol->clear();

    for(const Symbol& s : someSymbols()) {
        ui->comboSymbol->addItem(s.display(), s.id());
    }

    // unlock signals comboSymbol/Timeframne
    ui->comboSymbol->blockSignals(false);
    ui->comboTimeframe->blockSignals(false);

    //IExchangeClient
    std::shared_ptr<IExchangeClient> ex = std::make_shared<BingXSwapClient>();

    //MarketDataService
    MarketDataService* market = new MarketDataService(ex, this);
    marketData_ = market;

    QObject::connect(marketData_, &MarketDataService::signal_seriesLoaded, ui->chartWidget, &ChartWidget::slot_setSeries);
    QObject::connect(marketData_, &MarketDataService::signal_candleUpdated, ui->chartWidget, &ChartWidget::slot_onCandleUpdate);
    QObject::connect(marketData_, &MarketDataService::signal_candleClosed, ui->chartWidget, &ChartWidget::slot_onCandleClosed);

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
    // Реализовать позже
}
