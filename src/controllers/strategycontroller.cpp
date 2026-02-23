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

void StrategyController::onTick()
{
    // 1) "рынок"
    static double lastPrice = 150.0;
    lastPrice += (qrand() % 21 - 10) * 0.1; // +-1.0
    const double curPrice = lastPrice;

    // 2) если сделки нет — открываем
    if (!hasOpen_) {
        TradeRecord t;
        t.time   = QDateTime::currentDateTime();
        t.symbol = ui_->comboSymbol->currentText();
        t.side   = TradeSide::Buy;
        t.qty    = 1.0;
        t.price  = curPrice;                 // open price
        t.status = TradeStatus::Open;
        t.lifetimeTicks = 0;

        tradesModel_->appendTrade(t);

        hasOpen_     = true;
        ticksAlive_  = 0;
        openRow_     = tradesModel_->rowCount() - 1;

        openPrice_   = t.price;
        openSide_    = t.side;
        openQty_     = t.qty;

        return;
    }

    // дальше будет live/close...

    // 3) если сделка открыта — обновляем "живую" строку
    ticksAlive_++;

    TradeRecord t = tradesModel_->tradeAt(openRow_); // если у тебя нет — добавим ниже
    t.lifetimeTicks = ticksAlive_;

    // простая unrealized PnL (по желанию)
    const double dir = (openSide_ == TradeSide::Buy) ? 1.0 : -1.0;
    const double unrealPnl = (curPrice - openPrice_) * openQty_ * dir;
    t.pnl = unrealPnl;           // если есть поле pnl

    tradesModel_->updateTrade(openRow_, t);

    // 4) закрываем через 5 тиков
    if (ticksAlive_ >= 5) {
        t.status = TradeStatus::Closed;
        t.closeTime = QDateTime::currentDateTime(); // если есть
        t.closePrice = curPrice;                    // если есть

        const double dir = (openSide_ == TradeSide::Buy) ? 1.0 : -1.0;
        const double pnl = (curPrice - openPrice_) * openQty_ * dir;
        t.pnl = pnl;

        tradesModel_->updateTrade(openRow_, t);

        // сброс состояния
        hasOpen_ = false;
        ticksAlive_ = 0;
        openRow_ = -1;
    }
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
