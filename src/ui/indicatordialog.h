#pragma once
#include <QDialog>
#include "indicators/indicatorconfig.h"

namespace Ui {
class IndicatorDialog;
}

class IndicatorDialog final : public QDialog
{
    Q_OBJECT

public:

    explicit IndicatorDialog(QWidget *parent = nullptr);
    ~IndicatorDialog();

    void setConfig(const IndicatorConfig& cfg);
    IndicatorConfig config() const;

private:
    Ui::IndicatorDialog *ui;
};

