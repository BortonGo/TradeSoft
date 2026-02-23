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

void StrategyController::onStart()
{
    if (running_) return;

    cfg_ = readConfigFromUi();

    // минимальная валидация
    if (cfg_.strategy.name.isEmpty()) {
        qDebug() << "[STRATEGY] Can't start: strategy not selected";
        return;
    }
    if (!cfg_.risk.allowLong && !cfg_.risk.allowShort) {
        qDebug() << "[STRATEGY] Can't start: both long/short disabled";
        return;
    }
    if (cfg_.accountId.isEmpty()) {
        qDebug() << "[STRATEGY] Can't start: account not selected";
        return;
    }

    running_ = true;
    setParamsLocked(true);

    tick_.start();
    qDebug() << "[STRATEGY] START"
             << cfg_.strategy.name
             << toUiString(cfg_.strategy.tf)
             << "accountId=" << cfg_.accountId;
}

void StrategyController::onStop() {
    if (!running_) {
        return;
    }
    running_ = false;
    tick_.stop();

    setParamsLocked(false);
    ui_->tab_strategy->setFocus();
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

void StrategyController::setParamsLocked(bool locked)
{
    // locked = true -> параметры нельзя менять
    const bool enabled = !locked;

    ui_->cbStrategy->setEnabled(enabled);
    ui_->cbStrategyTf->setEnabled(enabled);

    ui_->gbRM->setEnabled(enabled);
    ui_->gbAccount->setEnabled(enabled);

    ui_->btnStart->setEnabled(!locked);
    ui_->btnStop->setEnabled(locked);
}

StrategyConfig StrategyController::readConfigFromUi() const
{
    StrategyConfig c;

    // Strategy
    c.strategy.name = ui_->cbStrategy->currentText();
    c.strategy.tf   = timeframeFromUiString(ui_->cbStrategyTf->currentText());

    // Risk
    c.risk.riskPct       = ui_->spinRiskPct->value();
    c.risk.maxPosUsdt    = ui_->spinMaxPosUSDT->value();
    c.risk.leverage      = ui_->spinLeverage->value();
    c.risk.allowLong     = ui_->chkAllowLong->isChecked();
    c.risk.allowShort    = ui_->chkAllowShort->isChecked();
    c.risk.feePct        = ui_->spinFeePct->value();
    c.risk.slippageBps   = ui_->spinSlippageBps->value();

    // Account (id лежит в itemData)
    c.accountId = ui_->cbAccount->currentData().toString();

    return c;
}
