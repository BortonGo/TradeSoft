#include "ui\chartwidget.h"
#include <QPainter>

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {}

void ChartWidget::slot_setSeries(std::shared_ptr<CandleSeries> series) {
    series_ = series;
    update();
}

void ChartWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if(!series_) {
        painter.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    QString text;
    text += "Symbol: " + series_->symbol() + "\n";
    text += "Timeframe: " + toUiString(series_->timeframe()) + "\n";
    text += "Candle count: " + QString::number(series_->candleCount()) + "\n";

    painter.drawText(rect(), Qt::AlignLeft | Qt::AlignTop, text);
}

///
/// Реализовать позже
///

void ChartWidget::slot_onCandleUpdate(Candle c) {

}

void ChartWidget::slot_onCandleClosed(Candle c) {

}

