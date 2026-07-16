#include "strategycontroller.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDebug>
#include <QLoggingCategory>
#include <QHeaderView>
#include <cmath>

Q_LOGGING_CATEGORY(logStrategyUi, "tradesoft.ui.strategy")

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
    clearChartLevels();

    cfg_ = readConfigFromUi();

    runner_->setRiskSettings(cfg_.risk);
    runner_->setConfig(cfg_);

    // минимальная валидация
    if (cfg_.strategy.name.isEmpty()) {
        qCWarning(logStrategyUi) << "Can't start: strategy not selected";
        return;
    }
    if (!cfg_.risk.allowLong && !cfg_.risk.allowShort) {
        qCWarning(logStrategyUi) << "Can't start: both long/short disabled";
        return;
    }
    if (cfg_.accountId.isEmpty()) {
        qCWarning(logStrategyUi) << "Can't start: account not selected";
        return;
    }

    const QString symbolId = ui_->comboSymbol->currentData().toString();
    if (symbolId.isEmpty()) {
        qCWarning(logStrategyUi) << "Can't start: symbol not selected";
        return;
    }
    runner_->setStrategy(StrategyFactory::create(cfg_));

    running_ = true;
    setParamsLocked(true);

    runner_->start(symbolId, cfg_.strategy.tf);

    qCInfo(logStrategyUi) << "Start"
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
    qCInfo(logStrategyUi) << "Stop";

    clearChartLevels();
}

void StrategyController::onRunnerSignal(StrategySignal s) {
    qCDebug(logStrategyUi) << "StrategySignal type=" << (int)s.type
                           << "symbol=" << s.symbolId
                           << "tf=" << toUiString(s.tf)
                           << "reason=" << s.reason;

    switch (s.type) {
    case StrategySignalType::EnterLong:
    case StrategySignalType::EnterShort:
        showLevelsForSignal(s);
        break;

    case StrategySignalType::ExitLong:
    case StrategySignalType::ExitShort:
        clearChartLevels();
        break;

    default:
        break;
    }
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

    // visual defaults
    c.visual.showEntryLine = true;
    c.visual.showTpLine = true;
    c.visual.showSlLine = true;

    // fixed exits defaults
    c.fixedExit.enabled = false;
    c.fixedExit.tpBps = 0;
    c.fixedExit.slBps = 0;

    // Привязываем fixed TP/SL к стратегиям, где они действительно фиксированные
    const QString name = c.strategy.name.trimmed();

    if (name.compare("EMA Scalp", Qt::CaseInsensitive) == 0 ||
        name.compare("Ema Scalp", Qt::CaseInsensitive) == 0) {
        c.fixedExit.enabled = true;
        c.fixedExit.tpBps = 28;
        c.fixedExit.slBps = 12;
    }

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
        journal_ = new TradeJournal(1000.0);
        connectTradeJournalToUi();
    }
}

void StrategyController::resetDemoSession() {
    ++tradeUiSessionId_;
    tradesModel_->clear();

    delete journal_;
    journal_ = new TradeJournal(1000.0);

    connectTradeJournalToUi();
    runner_->setTradeJournal(journal_);

    clearChartLevels();
}

void StrategyController::clearChartLevels() {
    if (!ui_ || !ui_->chartWidget) return;
    ui_->chartWidget->clearTradeLevels();
}

void StrategyController::showLevelsForSignal(const StrategySignal& s) {
    if (!ui_ || !ui_->chartWidget || !tradesModel_) {
        qCWarning(logStrategyUi) << "showLevelsForSignal aborted (null pointers)";
        return;
    }

    qCDebug(logStrategyUi) << "showLevelsForSignal called"
                           << "symbol=" << s.symbolId
                           << "rows=" << tradesModel_->rowCount();

    for (int row = tradesModel_->rowCount() - 1; row >= 0; --row) {
        const TradeRecord t = tradesModel_->tradeAt(row);

        qCDebug(logStrategyUi) << "checking row" << row
                               << "symbol=" << t.symbol
                               << "status=" << static_cast<int>(t.status)
                               << "tp=" << t.tpPrice
                               << "sl=" << t.slPrice;

        if (t.symbol == s.symbolId && t.status == TradeStatus::Open) {

            qCDebug(logStrategyUi) << "Found open trade"
                                   << "tp=" << t.tpPrice
                                   << "sl=" << t.slPrice;

            ui_->chartWidget->setTradeLevels(t.tpPrice, t.slPrice);
            return;
        }
    }
    qCDebug(logStrategyUi) << "No open trade found for symbol" << s.symbolId;
}

void StrategyController::connectTradeJournalToUi() {
    const std::uint64_t sessionId = tradeUiSessionId_;
    journal_->setTradeEventCallback(
        [this, sessionId](const TradeEvent& event) {
            QMetaObject::invokeMethod(this, [this, event, sessionId]{
                applyTradeEvent(event, sessionId);
            }, Qt::QueuedConnection);
        });
}

void StrategyController::applyTradeEvent(const TradeEvent& event, std::uint64_t sessionId) {
    if (sessionId != tradeUiSessionId_ || !tradesModel_) return;

    switch(event.type) {
    case TradeEventType::Added:
        tradesModel_->appendTrade(event.trade);
        break;
    case TradeEventType::Updated:
    case TradeEventType::Closed:
        tradesModel_->updateTrade(event.row, event.trade);
        break;
    }
}

