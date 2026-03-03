#pragma once
#include <QObject>
#include "ui/models/tradesmodel.h"
#include "domain/strategy/strategyconfig.h"
#include "service/marketdataservice.h"
#include "service/strategy/strategyrunner.h"
#include "domain/strategy/emacrossstrategy.h"

#include "domain/risk/riskmanager.h"
#include "service/execution/demoexecutionservice.h"
#include "service/trade/tradejournal.h"

namespace Ui { class MainWindow; }

class StrategyController final : public QObject
{
    Q_OBJECT

    Ui::MainWindow* ui_ = nullptr;
    TradesModel* tradesModel_ = nullptr;

    MarketDataService* marketData_ = nullptr;
    StrategyRunner* runner_ = nullptr;

    RiskManager* risk_ = nullptr;
    DemoExecutionService* demoExec_ = nullptr;
    TradeJournal* journal_ = nullptr;

    bool running_ = false;
    StrategyConfig cfg_;

public:
    explicit StrategyController(Ui::MainWindow* ui, MarketDataService* mds, QObject* parent = nullptr);

public slots:
    void onStart();
    void onStop();

private slots:
    void onRunnerSignal(StrategySignal s);

private:
    void setParamsLocked(bool locked);
    StrategyConfig readConfigFromUi() const;

    void ensureDemoPipeline();
    void resetDemoSession();
};
