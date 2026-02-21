#include "strategycontroller.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDebug>

StrategyController::StrategyController(Ui::MainWindow* ui, QObject* parent) : QObject(parent), ui_(ui) {
    Q_ASSERT(ui_);
    connect(ui_->btnStart, &QPushButton::clicked, this, &StrategyController::onStart);
    connect(ui_->btnStop,  &QPushButton::clicked, this, &StrategyController::onStop);
}

void StrategyController::onStart() {
    if (running_) {
        return;
    }
    running_ = true;
    qDebug() << "[STRATEGY] START";
}

void StrategyController::onStop() {
    if (!running_) {
        return;
    }
    running_ = false;
    qDebug() << "[STRATEGY] STOP";
}


