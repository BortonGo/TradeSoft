#pragma once
#include <QObject>

namespace Ui { class MainWindow; }

class StrategyController final : public QObject
{
    Q_OBJECT

    Ui::MainWindow* ui_ = nullptr;
    bool running_ = false;

public:
    explicit StrategyController(Ui::MainWindow* ui, QObject* parent = nullptr);

public slots:
    void onStart();
    void onStop();

};
