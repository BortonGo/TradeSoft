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
