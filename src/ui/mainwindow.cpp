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

    for(const Timeframe& tf : allTimeframes()) {
        ui->comboTimeframe->addItem(toUiString(tf), static_cast<int>(tf));
        ui->cbStrategyTf->addItem(toUiString(tf), static_cast<int>(tf));
    };

    // Some symbols
    ui->comboSymbol->clear();

    for(const Symbol& s : someSymbols()) {
        ui->comboSymbol->addItem(s.display(), s.id());
    }

    // unlock signals comboSymbol/Timeframne
    ui->comboSymbol->blockSignals(false);
    ui->comboTimeframe->blockSignals(false);

    // StrategyController
    strategyController_ = new StrategyController(ui, this);

    //IExchangeClient
    std::shared_ptr<IExchangeClient> ex = std::make_shared<BingXSwapClient>();

    //MarketDataService
    MarketDataService* market = new MarketDataService(ex, this);
    marketData_ = market;

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
