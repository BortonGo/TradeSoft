#ifndef INDICATORDIALOG_H
#define INDICATORDIALOG_H

#include <QDialog>

namespace Ui {
class IndicatorDialog;
}

class IndicatorDialog : public QDialog
{
    Q_OBJECT

public:
    struct IndicatorDialogResult {
        bool ema20 = false;
        bool ema50 = false;
        bool donchain20 = false;
        bool rsi14 = false;
        bool atr14 = false;
    };

    explicit IndicatorDialog(QWidget *parent = 0);
    ~IndicatorDialog();

    IndicatorDialogResult result() const;

private:
    Ui::IndicatorDialog *ui;
};

#endif // INDICATORDIALOG_H
