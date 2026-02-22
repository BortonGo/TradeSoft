#include "strategycontroller.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDebug>
#include <QHeaderView>

StrategyController::StrategyController(Ui::MainWindow* ui, QObject* parent) : QObject(parent), ui_(ui) {
    Q_ASSERT(ui_);
    connect(ui_->btnStart, &QPushButton::clicked, this, &StrategyController::onStart);
    connect(ui_->btnStop,  &QPushButton::clicked, this, &StrategyController::onStop);

    tradesModel_ = new TradesModel(this);

    ui_->tableTrades->setModel(tradesModel_);
    ui_->tableTrades->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui_->tableTrades->setSelectionMode(QAbstractItemView::SingleSelection);
    if (tradesModel_->columnCount() > 0) {
        ui_->tableTrades->horizontalHeader()->setStretchLastSection(true);
    }
    ui_->tableTrades->setAlternatingRowColors(true);

    tick_.setInterval(1000);
    connect(&tick_, &QTimer::timeout, this, &StrategyController::onTick);

    ui_->btnStart->setEnabled(true);
    ui_->btnStop->setEnabled(false);
}

void StrategyController::onStart() {
    if (running_) {
        return;
    }
    running_ = true;
    tick_.start();

    ui_->btnStart->setEnabled(false);
    ui_->btnStop->setEnabled(true);
    qDebug() << "[STRATEGY] START";
}

void StrategyController::onStop() {
    if (!running_) {
        return;
    }
    running_ = false;
    tick_.stop();

    ui_->btnStart->setEnabled(true);
    ui_->btnStop->setEnabled(false);
    qDebug() << "[STRATEGY] STOP";
}

void StrategyController::onTick() {
    TradeRecord t;
    t.time = QDateTime::currentDateTime();
    t.symbol = ui_->comboSymbol->currentText();
    t.side = (qrand() % 2 == 0) ? TradeSide::Buy : TradeSide::Sell;
    t.qty = 1.0;
    t.price = 100.0 + (qrand() % 100);
    t.fee = 0.01;
    t.status = TradeStatus::Open;

    tradesModel_->appendTrade(t);
}


