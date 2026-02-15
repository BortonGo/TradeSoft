#pragma once
#include <QMainWindow>
#include "service\marketdataservice.h"
#include "indicators\indicatorengine.h"
#include "service\indicatorservice.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow *ui;

    IndicatorEngine indicators_;
    IndicatorService* indicatorService_ = nullptr;


private slots:
    void on_comboSymbol_currentIndexChanged(int index);

    void on_comboTimeframe_currentIndexChanged(int index);

    void on_btnIndicators_clicked();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    MarketDataService* marketData_ = nullptr;

private:
    void reloadAndStart();

};


