#include "ui\chartwidget.h"
#include <limits>
#include <cmath>
#include <QPainter>

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {}

static bool calcVisibleMinMax(const CandleSeries& s, int first, int last, double& outMinLow, double& outMaxHigh) {
    if (first < 0 || (last < first)) {
        return false;
    }
    outMinLow = std::numeric_limits<double>::infinity();
    outMaxHigh = -std::numeric_limits<double>::infinity();

    const QList<Candle>& candles = s.getCandles();

    for (int i = first; i <= last; ++i) {
        const Candle& c = candles[i];
        outMinLow = std::min(outMinLow, c.low_);
        outMaxHigh = std::max(outMaxHigh, c.high_);
    }

    if (!std::isfinite(outMinLow) || !std::isfinite(outMaxHigh)) {
        return false;
    }
    if (outMaxHigh <= outMinLow) {
        const double mid = outMinLow;
        outMinLow = mid - 1.0;
        outMaxHigh = mid + 1.0;
    }
    return true;
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

    // plot-area
    QRect plot = rect().adjusted(leftPadding_, topPadding_, -rightPadding_, -bottomPadding_);
    if (plot.width() <= 0 || plot.height() <= 0) {
        return;
    }

    // if window dont set yet, visibleCount_ going to width
    const int maxVis = maxVisibleByWidth();
    if (maxVis <= 0) {
        return;
    }
    const int vis = std::min(visibleCount_, maxVis);

    const int count = series_->getCount();
    if (count <= 0) {
        return;
    }

    // stand right (while dont have pan/zoom)
    firstVisible_ = std::max(0, count - vis);

    const int first = firstVisible_;
    const int last = lastVisible();
    if (last < first) {
        return;
    }

    // min/max
    double minLow = 0.0;
    double maxHigh = 0.0;
    if (!calcVisibleMinMax(*series_, first, last, minLow, maxHigh)) {
        return;
    }

    // padding by Y
    const double range = maxHigh - minLow;
    minLow -= range * 0.05;
    maxHigh += range * 0.05;

    // scale Y
    const double yScale = plot.height() / (maxHigh - minLow);

    auto priceToY = [&](double price) -> int {
        const double y = plot.top() + (maxHigh - price) * yScale;
        return static_cast<int>(std::round(y));
    };

    // step X
    const int step = candleWidth_ + candleGap_;

    for (int i = first; i <= last; ++i) {
        const Candle& c = candles[i];

        const int k = i - first;
        const int x = plot.left() + k * step;
        const int xCenter = x + candleWidth_ / 2;

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

        QRect body(static_cast<int>(std::round(xCenter - candleWidth_ / 2.0)), topY, static_cast<int>(std::round(candleWidth_)), std::max(1, botY - topY));

        painter.fillRect(body, bull ? QColor(0,200,0) : QColor(200,0,0));
    }

    // frame
    painter.setPen(QColor(80,80,80));
    painter.drawRect(plot);

}

int ChartWidget::maxVisibleByWidth() const {
    const int plotW = width() - leftPadding_ - rightPadding_;
    if (plotW <= 0) {
        return 0;
    }

    const int step = candleWidth_ + candleGap_;
    if (step <= 0) {
        return 0;
    }

    return std::max(1, plotW / step);
}

int ChartWidget::lastVisible() const {
    if (!series_ || series_->getCount() <= 0) {
        return -1;
    }

    const int count = series_->getCount();
    const int maxVis = maxVisibleByWidth();
    const int vis = std::min(visibleCount_, maxVis);

    const int last = firstVisible_ + vis - 1;
    return qBound(0, last, count - 1);
}

void ChartWidget::normalizeViewport() {
    if (!series_) {
        return;
    }
    const int count = series_->getCount();
    if (count == 0) {
        return;
    }

    int vis = std::min(visibleCount_, maxVisibleByWidth());
    if (vis <= 0) {
        return;
    }

    int maxFirst = std::max(0, count - vis);
    if (followRight_ == true) {
        firstVisible_ = maxFirst;
    } else if (firstVisible_ < 0){
        firstVisible_ = 0;
    } else if (firstVisible_ > maxFirst) {
        firstVisible_ = maxFirst;
    }
}



//========================================================== SLOTS ==========================================================

void ChartWidget::slot_setSeries(std::shared_ptr<CandleSeries> series) {
    series_ = series;
    update();
}

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

