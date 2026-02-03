#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/core/timeframe.h"
#include "src/core/symbol.h"
#include "src/service/marketdataservice.h"
#include "src/exchange/fakeexchangeclient.h"
#include <memory>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    //IExchangeClient
    std::shared_ptr<IExchangeClient> ex = std::make_shared<FakeExchangeClient>();

    //MarketDataService
    MarketDataService* market = new MarketDataService(ex, this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnLoad_clicked()
{

}
