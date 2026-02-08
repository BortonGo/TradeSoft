#include "ui\chartwidget.h"
#include <limits>
#include <algorithm>
#include <cmath>
#include <QPainter>
#include <QDebug>

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {
    candleWidthAcc_ = static_cast<double>(candleWidth_);
}

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

//==================================================== PaintEvent ====================================================
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

    const int count = series_->getCount();
    if (count <= 0) {
        return;
    }

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

    // Y axis (price scale)

    // Сколько подписей хотим(1 text on 60px)
    const int desiredTicks = qBound(5, plot.height() / 60, 10);

    const double priceRange = (maxHigh - minLow);
    if (priceRange > 0.0 && desiredTicks > 0) {

        // rawStep
        const double rawStep = priceRange / static_cast<double>(desiredTicks);

        // nice step: 1/2/5 * 10^n
        const double pow10 = std::pow(10, std::floor(std::log10(rawStep)));
        const double m = rawStep / pow10;

        double niceM = 1.0;
        if (m <= 1.0) {
            niceM = 1.0;
        } else if (m <= 2.0) {
            niceM = 2.0;
        } else if (m <= 5.0) {
            niceM = 5.0;
        } else {
            niceM = 10.0;
        }

        const double stepPrice = niceM * pow10;

        // decimals for beautiful numbers
        int decimals = 2;
        if (stepPrice < 1.0) {
            decimals = static_cast<int>(std::ceil(-std::log10(stepPrice))) + 1;
            decimals = qBound(2, decimals, 8);
        }

        // first tick
        const double firstTick = std::ceil(minLow / stepPrice) * stepPrice;

        // draw text and ticks in right padding
        painter.setPen(QColor(180,180,180));

        int lastDrawnY = std::numeric_limits<int>::min();

        for (double p = firstTick; p <= maxHigh + stepPrice * 0.5; p += stepPrice) {
            const int y = priceToY(p);

            if (std::abs(y - lastDrawnY) < 14) {
                continue;
            }
            lastDrawnY = y;

            painter.drawLine(plot.right() - 3, y, plot.right() + 3, y);

            // right text
            const QString label = QString::number(p, 'f', decimals);

            // text rectangle
            QRect textRect(plot.right() + 6, y - 8, rightPadding_ - 8, 16);
            painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, label);

        }

    }

    // frame
    painter.setPen(QColor(80,80,80));
    painter.drawRect(plot);

}


//==================================================== ResizeEvent ====================================================
void ChartWidget::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    QWidget::resizeEvent(event);

    normalizeViewport();
    update();
}


//==================================================== MouseEvent ====================================================
void ChartWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isPanning_ = true;
        panLastX_ = event->pos().x();
        panRemainder_ = 0.0;

        followRight_ = false;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ChartWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!isPanning_ || !series_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const int dx = event->pos().x() - panLastX_;
    panLastX_ = event->pos().x();

    const int step = candleWidth_ + candleGap_;

    if (step <= 0) {
        return;
    }

    // pixels to candles
    double deltaBars = static_cast<double>(dx) / step;
    panRemainder_ += deltaBars;

    const int shift = static_cast<int>(panRemainder_);
    if (shift != 0) {

        const int prevFirst = firstVisible_;

        firstVisible_ -= shift;
        panRemainder_ -= shift;

        normalizeViewport();

        const int count = series_->getCount();
        const int vis = std::min(visibleCount_, maxVisibleByWidth());
        const int maxFirst = std::max(0, count - vis);

        const int SNAP_CANDLES = 5;

        const bool movedTowardRightEdge = (shift < 0);
        const bool wasInHistory = (prevFirst < maxFirst - SNAP_CANDLES);
        const bool nowNearEdge = (firstVisible_ >= maxFirst - SNAP_CANDLES);

        if(movedTowardRightEdge && wasInHistory && nowNearEdge) {
            followRight_ = true;
            firstVisible_ = maxFirst;
            panRemainder_ = 0.0;
            normalizeViewport();
        }

        update();
    }

    event->accept();

}

void ChartWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isPanning_ = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}


//==================================================== WheelEvent ====================================================
void ChartWidget::wheelEvent(QWheelEvent* event) {
    if (!series_ || series_->getCount() == 0) {
        event->ignore();
        return;
    }

    int delta = event->angleDelta().y(); // delta > 0 -> wheel up, zoom in // delta < 0 -> wheel down, zoom out
    if (delta == 0) {
        delta = event->pixelDelta().y();
    }
    if(delta == 0) {
        event->ignore();
        return;
    }

    const QRectF plotArea = plotRect();
    double cursorXInPlot = event->posF().x() - plotArea.left();

    if (cursorXInPlot < 0.0) {
        cursorXInPlot = 0;
    } else if (cursorXInPlot > plotArea.width()) {
        cursorXInPlot = plotArea.width();
    }

    int oldStep = stepPx();

    double anchor = static_cast<double>(firstVisible_) + cursorXInPlot / static_cast<double>(oldStep); //anchor - ind candle under cursor now

    // New scale

    double zoomSteps = static_cast<double>(delta) / 120.0;
    double zoomBase = 1.12; // percent of change scale by one zoomStep
    double zoomKoef = std::pow(zoomBase, zoomSteps);
    candleWidthAcc_ *= zoomKoef;

    if (candleWidthAcc_ < minCandleWidth_) {
        candleWidthAcc_ = minCandleWidth_;
    }

    if (candleWidthAcc_ > maxCandleWidth_) {
        candleWidthAcc_ = maxCandleWidth_;
    }

    int newWidth = clampCandleWidth(static_cast<int>(std::lround(candleWidthAcc_)));
    newWidth = clampCandleWidth(newWidth);

    if (newWidth == candleWidth_) {
        event->accept();
        return;
    }

    // FollowRight (like TradingView)

    double distToRight = plotArea.width() - cursorXInPlot;
    bool nearRightEdge = distToRight < 30.0;

    if(!nearRightEdge) {
        followRight_ = false;
    }

    candleWidth_ = newWidth;
    int newStep = stepPx();

    double newFirst = anchor - cursorXInPlot / static_cast<double>(newStep);

    double floored = std::floor(newFirst);
    double frac = newFirst - floored;

    firstVisible_ = static_cast<int>(floored);
    panRemainder_ = frac + static_cast<double>(newStep);

    normalizeViewport();
    update();
    event->accept();
}


//========================================================== SLOTS ==========================================================

void ChartWidget::slot_setSeries(std::shared_ptr<CandleSeries> series) {
    series_ = series;
    followRight_ = true;
    normalizeViewport();
    update();
}

void ChartWidget::slot_onCandleUpdate(Candle c) {
    if (!series_) {
        return;
    }

    series_->updateLastCandle(c);
    normalizeViewport();
    update();
}

void ChartWidget::slot_onCandleClosed(Candle c) {
    if (!series_) {
        return;
    }
    series_->updateLastCandle(c);
    normalizeViewport();
    update();
}

//========================================================== Helpers ==========================================================
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
    int maxFirstAllowed = count - 1;
    if (followRight_) {
        firstVisible_ = maxFirst;
    } else {
        firstVisible_ = qBound(0, firstVisible_, maxFirstAllowed);
    }

    if (candleWidth_ < minCandleWidth_) {
        candleWidth_ = minCandleWidth_;
    } else if (candleWidth_ > maxCandleWidth_) {
        candleWidth_ = maxCandleWidth_;
    }
}

//========================================================== Helpers to WheelEvent ==========================================================
QRectF ChartWidget::plotRect() const {
    return QRectF(leftPadding_,
                  topPadding_,
                  width() - leftPadding_ - rightPadding_,
                  height() - topPadding_ - bottomPadding_
                  );
}

int ChartWidget::stepPx() const {
    return candleWidth_ + candleGap_;
}

int ChartWidget::clampCandleWidth(int width) const {
    if (width < minCandleWidth_) {
        return minCandleWidth_;
    }
    if (width > maxCandleWidth_) {
        return maxCandleWidth_;
    }
    return width;
}


