#pragma once
#include <QMainWindow>
#include "core/appconfig.h"
#include "service/marketdataservice.h"
#include "service/indicatorservice.h"
#include "backtest/backtestcontroller.h"
#include "controllers/strategycontroller.h"
#include "domain/account/account.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

    Ui::MainWindow *ui;

    IndicatorService* indicatorService_ = nullptr;
    StrategyController* strategyController_ = nullptr;
    BacktestController* backtestController = nullptr;

    AccountStore accounts_;
    AppConfig config_;

private slots:
    void on_comboSymbol_currentIndexChanged(int index);

    void on_comboTimeframe_currentIndexChanged(int index);

    void on_btnIndicators_clicked();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    MarketDataService* marketData_ = nullptr;
    MarketDataService* marketDataStrategy_ = nullptr;

private:
    void reloadAndStart();
};

