#pragma once
#include <QObject>
#include <QTimer>
#include "ui/models/tradesmodel.h"
#include "domain/strategy/strategyconfig.h"

namespace Ui { class MainWindow; }

class StrategyController final : public QObject
{
    Q_OBJECT

    Ui::MainWindow* ui_ = nullptr;
    TradesModel* tradesModel_ = nullptr;
    bool running_ = false;
    QTimer tick_;
    StrategyConfig cfg_;

    bool hasOpen_ = false;
    int  ticksAlive_ = 0;
    int  openRow_ = -1;

    double openPrice_ = 0.0;
    TradeSide openSide_ = TradeSide::Buy;
    double openQty_ = 0.0;

public:
    explicit StrategyController(Ui::MainWindow* ui, QObject* parent = nullptr);

public slots:
    void onStart();
    void onStop();
    void onTick();

private:
    void setParamsLocked(bool locked);
    StrategyConfig readConfigFromUi() const;

};
