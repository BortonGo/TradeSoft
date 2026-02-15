#include "indicatordialog.h"
#include "ui_indicatordialog.h"

IndicatorDialog::IndicatorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IndicatorDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

}

IndicatorDialog::~IndicatorDialog()
{
    delete ui;
}

IndicatorDialog::IndicatorDialogResult IndicatorDialog::result() const{
    IndicatorDialogResult r;
    r.ema20 = ui->chkEma50->isChecked();
    r.ema50 = ui->chkEma50->isChecked();
    r.donchain20 = ui->chkDonchian20->isChecked();
    r.rsi14 = ui->chkRsi14->isChecked();
    r.atr14 = ui->chkAtr14->isChecked();
    return r;
}
