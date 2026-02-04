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
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.fillRect(rect(), QColor(22,22,22)); // фон

    if (!series_ || series_->getCount() == 0) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    const auto& candles = series_->getCandles();

    const int maxBars = 120;
    const int total = candles.size();
    const int first = std::max(0, total - maxBars);
    const int bars = total - first;
    if (bars <= 0) {
        return;
    }

    // рабочая область с отступами
    const int leftPad = 40, rightPad = 40, topPad = 10, bottomPad =20;
    QRect plot = rect().adjusted(leftPad, topPad, -rightPad, -bottomPad);

    // min/max по видимому диапазону
    double minLow = candles[first].low_;
    double maxHigh = candles[first].high_;
    for (int i = first; i < total; ++i) {
        minLow = std::min(minLow, candles[i].low_);
        maxHigh = std::max(maxHigh, candles[i].high_);
    }
    if (maxHigh <= minLow) {
        return;
    }

    // scale Y
    const double yScale = plot.height() / (maxHigh - minLow);

    auto priceToY = [&](double price) -> int {
        const double y = plot.top() + (maxHigh - price) * yScale;
        return static_cast<int>(std::round(y));
    };

    // step X
    const double dx = (bars > 1) ? (static_cast<double>(plot.width()) / bars) : plot.width();
    const double bodyW = std::max(1.0, dx * 0.6);

    for (int k = 0; k < bars; ++k) {
        const Candle& c = candles[first + k];
        const int xCenter = static_cast<int>(std::round(plot.left() + (k + 0.5) * dx));

        const int yOpen = priceToY(c.open_);

        const int yClose = priceToY(c.close_);
        const int yHigh = priceToY(c.high_);
        const int yLow = priceToY(c.low_);

        const bool bull = (c.close_ >= c.open_);

        // tail
        painter.setPen(bull ? QColor(0,200,0) : QColor(200,0,0));
        painter.drawLine(xCenter, yHigh, xCenter, yLow);

        // body
        const int topY = std::min(yOpen, yClose);
        const int botY = std::max(yOpen, yClose);

        QRect body(static_cast<int>(std::round(xCenter - bodyW / 2.0)), topY, static_cast<int>(std::round(bodyW)), std::max(1, botY - topY));

        painter.fillRect(body, bull ? QColor(0,200,0) : QColor(200,0,0));
    }

    // frame
    painter.setPen(QColor(80,80,80));
    painter.drawRect(plot);

}

///
/// Реализовать позже
///

void ChartWidget::slot_onCandleUpdate(Candle c) {
    if (!series_) {
        return;
    }

    series_->updateLastCandle(c);
    update();
}

void ChartWidget::slot_onCandleClosed(Candle c) {
    if (!series_) {
        return;
    }
    series_->updateLastCandle(c);
    update();
}

