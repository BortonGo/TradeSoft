#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/core/timeframe.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboTimeframe->clear();

    for(Timeframe tf : allTimeframes()){
        ui->comboTimeframe->addItem(toUiString(tf), static_cast<int>(tf));
    };
}

MainWindow::~MainWindow()
{
    delete ui;
}
