#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/core/timeframe.h"
#include "src/core/symbol.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Timeframes
    ui->comboTimeframe->clear();

    for(Timeframe tf : allTimeframes()) {
        ui->comboTimeframe->addItem(toUiString(tf), static_cast<int>(tf));
    };

    // Some symbols
    ui->comboSymbol->clear();

    for(Symbol s : someSymbols()) {
        ui->comboSymbol->addItem(s.display(), s.id());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
