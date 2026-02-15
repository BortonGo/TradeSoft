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

void IndicatorDialog::setConfig(const IndicatorConfig& cfg)
{
    ui->chkEma9->setChecked(cfg.ema9);
    ui->chkEma20->setChecked(cfg.ema20);
    ui->chkEma50->setChecked(cfg.ema50);
    ui->chkDonchian20->setChecked(cfg.donchian20);
    ui->chkRsi14->setChecked(cfg.rsi14);
    ui->chkAtr14->setChecked(cfg.atr14);
}

IndicatorConfig IndicatorDialog::config() const
{
    IndicatorConfig cfg;
    cfg.ema9 = ui->chkEma9->isChecked();
    cfg.ema20 = ui->chkEma20->isChecked();
    cfg.ema50 = ui->chkEma50->isChecked();
    cfg.donchian20 = ui->chkDonchian20->isChecked();
    cfg.rsi14 = ui->chkRsi14->isChecked();
    cfg.atr14 = ui->chkAtr14->isChecked();
    return cfg;
}

