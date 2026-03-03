#include "strategycontroller.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDebug>
#include <QHeaderView>

StrategyController::StrategyController(Ui::MainWindow* ui, MarketDataService* mds, QObject* parent)
    : QObject(parent), ui_(ui), marketData_(mds) {
    Q_ASSERT(ui_);
    Q_ASSERT(marketData_);

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

    runner_ = new StrategyRunner(marketData_, this);

    ensureDemoPipeline();
    runner_->setRiskManager(risk_);
    runner_->setExecutionService(demoExec_);
    runner_->setTradeJournal(journal_);

    connect(runner_, &StrategyRunner::signal_signalGenerated,this, &StrategyController::onRunnerSignal);

    ui_->btnStart->setEnabled(true);
    ui_->btnStop->setEnabled(false);
}

void StrategyController::onStart() {
    if (running_) return;

    resetDemoSession();

    cfg_ = readConfigFromUi();

    runner_->setRiskSettings(cfg_.risk);

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

    const QString symbolId = ui_->comboSymbol->currentData().toString();
    if (symbolId.isEmpty()) {
        qDebug() << "[STRATEGY] Can't start: symbol not selected";
        return;
    }
    runner_->setStrategy(std::unique_ptr<IStrategy>(
        new EmaCrossStrategy(
            9, 21,
            cfg_.risk.allowLong,
            cfg_.risk.allowShort
        )
    ));

    running_ = true;
    setParamsLocked(true);

    runner_->start(symbolId, cfg_.strategy.tf);

    qDebug() << "[STRATEGY] START"
             << cfg_.strategy.name
             << toUiString(cfg_.strategy.tf)
             << "symbol=" << symbolId
             << "accountId=" << cfg_.accountId;
}

void StrategyController::onStop()
{
    if (!running_) return;

    running_ = false;
    if (runner_) runner_->stop();

    setParamsLocked(false);
    ui_->tab_strategy->setFocus();
    qDebug() << "[STRATEGY] STOP";
}

void StrategyController::onRunnerSignal(StrategySignal s) {
    // Теперь сделки в таблицу пишет TradeJournal (через fills).
    // Здесь оставляем только лог
    qDebug() << "[UI] StrategySignal type=" << (int)s.type
             << "symbol=" << s.symbolId
             << "tf=" << toUiString(s.tf)
             << "reason=" << s.reason;
}

void StrategyController::setParamsLocked(bool locked) {
    const bool enabled = !locked;

    ui_->cbStrategy->setEnabled(enabled);
    ui_->cbStrategyTf->setEnabled(enabled);

    ui_->gbRM->setEnabled(enabled);
    ui_->gbAccount->setEnabled(enabled);

    ui_->btnStart->setEnabled(!locked);
    ui_->btnStop->setEnabled(locked);
}

StrategyConfig StrategyController::readConfigFromUi() const {
    StrategyConfig c;

    c.strategy.name = ui_->cbStrategy->currentText();
    c.strategy.tf   = timeframeFromUiString(ui_->cbStrategyTf->currentText());

    c.risk.riskPct       = ui_->spinRiskPct->value();
    c.risk.maxPosUsdt    = ui_->spinMaxPosUSDT->value();
    c.risk.leverage      = ui_->spinLeverage->value();
    c.risk.allowLong     = ui_->chkAllowLong->isChecked();
    c.risk.allowShort    = ui_->chkAllowShort->isChecked();
    c.risk.feePct        = ui_->spinFeePct->value();
    c.risk.slippageBps   = ui_->spinSlippageBps->value();

    c.accountId = ui_->cbAccount->currentData().toString();
    return c;
}

void StrategyController::ensureDemoPipeline() {
    if (!risk_) {
        risk_ = new RiskManager();
    }
    if (!demoExec_) {
        demoExec_ = new DemoExecutionService();
    }
    if (!journal_) {
        journal_ = new TradeJournal(tradesModel_, 1000.0);
    }
}

void StrategyController::resetDemoSession() {
    tradesModel_->clear();

    if (journal_) {
        delete journal_;
        journal_ = nullptr;
    }

    journal_ = new TradeJournal(tradesModel_, 1000.0);
    if (runner_) {
        runner_->setTradeJournal(journal_);
    }
}
