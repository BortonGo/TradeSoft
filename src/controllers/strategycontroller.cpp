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
}

void StrategyController::onStart() {
    if (running_) {
        return;
    }
    running_ = true;
    qDebug() << "[STRATEGY] START";
    // Test deal
    TradeRecord t;
    t.time = QDateTime::currentDateTime();
    t.symbol = ui_->comboSymbol->currentText();
    t.side = TradeSide::Buy;
    t.qty = 0.01;
    t.price = 1500.0;
    t.fee = 0.06;
    t.status = TradeStatus::Open;

    tradesModel_->appendTrade(t);
}

void StrategyController::onStop() {
    if (!running_) {
        return;
    }
    running_ = false;
    qDebug() << "[STRATEGY] STOP";
}


