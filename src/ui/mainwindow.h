#pragma once
#include <QMainWindow>
#include "service\marketdataservice.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow *ui;

private slots:
    void on_btnLoad_clicked();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    MarketDataService* marketData_ = nullptr;

};


