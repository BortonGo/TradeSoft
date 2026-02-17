#include "ui\chartwidget.h"
#include <limits>
#include <algorithm>
#include <cmath>
#include <QPainter>
#include <QDebug>
#include <QDateTime>
#include <QFontMetrics>

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

void ChartWidget::setTimeframe(Timeframe tf) {
    currentTf_ = tf;
    update();
}

void ChartWidget::setIndicatorLines(const QVector<IndicatorLine>& lines) {
    indicatorLines_ = lines;
    update();
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

    // plot-area base
    QRect plot = rect().adjusted(leftPadding_, topPadding_, -rightPadding_, -bottomPadding_);
    if (plot.width() <= 0 || plot.height() <= 0) {
        return;
    }

    // Detect if we need bottom indicator panel (RSI/ATR)
    bool needBottomPanel = false;
    for (const auto& line : indicatorLines_) {
        if (line.id_ == IndicatorId::RSI14 || line.id_ == IndicatorId::ATR14) {
            needBottomPanel = true;
            break;
        }
    }

    // Split plot only if needed
    QRect plotPrice = plot;
    QRect plotInd;

    if (needBottomPanel) {
        const int gap = 6;
        const int indH = std::max(80, plot.height() / 4);  // 25%

        plotInd = QRect(plot.left(), plot.bottom() - indH, plot.width(), indH);
        plotPrice = QRect(plot.left(), plot.top(), plot.width(), plot.height() - indH - gap);
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
    const double yScale = plotPrice.height() / (maxHigh - minLow);

    auto priceToY = [&](double price) -> int {
        const double y = plotPrice.top() + (maxHigh - price) * yScale;
        return static_cast<int>(std::round(y));
    };

    // step X
    const int step = candleWidth_ + candleGap_;

    for (int i = first; i <= last; ++i) {
        const Candle& c = candles[i];

        const int k = i - first;
        const int x = plotPrice.left() + k * step;
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

    // Indicators overlay
    if (!indicatorLines_.isEmpty()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 1) найти Donchian upper/lower
        const IndicatorLine* donUpper = nullptr;
        const IndicatorLine* donLower = nullptr;

        for (const auto& line : indicatorLines_) {
            if (line.id_ == IndicatorId::DON20UPPER) {
                donUpper = &line;
            } else if (line.id_ == IndicatorId::DON20LOWER) {
                donLower = &line;
            }
        }

        // 2) fill Donchian channel
        if (donUpper && donLower) {
            const int nU = donUpper->values_.size();
            const int nL = donLower->values_.size();
            const int n = std::min(nU, nL);

            QVector<QPointF> upperPts;
            QVector<QPointF> lowerPts;
            upperPts.reserve(last - first + 1);
            lowerPts.reserve(last - first + 1);

            for (int i = first; i <= last; ++i) {
                if (i < 0 || i >= n) {
                    continue;
                }

                const double up = donUpper->values_[i];
                const double lo = donLower->values_[i];
                if (std::isnan(up) || std::isnan(lo)) {
                    continue;
                }

                const int k = i - first;
                const double x = plotPrice.left() + k * step + candleWidth_ / 2.0;
                const double yUp = priceToY(up);
                const double yLo = priceToY(lo);

                upperPts.push_back(QPointF(x, yUp));
                lowerPts.push_back(QPointF(x, yLo));
            }

            if (upperPts.size() >= 2 && lowerPts.size() >= 2) {
                QPainterPath area;
                area.moveTo(upperPts[0]);
                for (int i = 1; i < upperPts.size(); ++i)
                    area.lineTo(upperPts[i]);

                // закрываем контур по нижней линии в обратном порядке
                for (int i = lowerPts.size() - 1; i >= 0; --i)
                    area.lineTo(lowerPts[i]);

                area.closeSubpath();

                // кисть (полупрозрачная)
                QColor fillColor(0, 180, 255, 30);  // alpha = 40 можно менять
                painter.fillPath(area, QBrush(fillColor));
            }
        }

        // 3) рисуем линии поверх
        for (const auto& line : indicatorLines_) {
            QPen pen;
            switch (line.id_) {
            case IndicatorId::EMA9:   pen = QPen(QColor(139, 0, 255), 1); break;
            case IndicatorId::EMA20:  pen = QPen(QColor(255, 200, 0), 1); break;
            case IndicatorId::EMA50:  pen = QPen(QColor(0, 180, 255), 1); break;

            case IndicatorId::DON20UPPER:
            case IndicatorId::DON20LOWER:
                pen = QPen(QColor(0, 180, 255), 1);
                break;

            case IndicatorId::DON20MIDDLE:
                pen = QPen(QColor(0, 180, 255), 1, Qt::DashLine);
                break;

            default:
                continue;
            }

            painter.setPen(pen);

            QPolygonF poly;
            poly.reserve(last - first + 1);

            const int n = line.values_.size();
            for (int i = first; i <= last; ++i) {
                if (i < 0 || i >= n) {
                    continue;
                }
                const double v = line.values_[i];
                if (std::isnan(v)) {
                    continue;
                }

                const int k = i - first;
                const int x = plotPrice.left() + k * step + candleWidth_ / 2;
                const int y = priceToY(v);
                poly << QPointF(x, y);
            }

            if (poly.size() >= 2) painter.drawPolyline(poly);
        }

        painter.restore();


    // RSI and ATR

        const IndicatorLine* rsi = nullptr;
        const IndicatorLine* atr = nullptr;

        // 1) find pointers
        for (const auto& line : indicatorLines_) {
            if (line.id_ == IndicatorId::RSI14) {
                rsi = &line;
            } else if (line.id_ == IndicatorId::ATR14) {
                atr = &line;
            }
        }

        // если панель пустая — вообще не рисуем
        if (!rsi && !atr) {
        } else {
            painter.save();

            // background frame
            painter.setPen(QColor(60, 60, 60));
            painter.drawRect(plotInd);

            if (rsi) {
                auto rsiToY = [&](double v) -> int {
                    v = std::max(0.0, std::min(100.0, v));
                    const double t = (100.0 - v) / 100.0;
                    return static_cast<int>(std::round(plotInd.top() + t * plotInd.height()));
                };

                // 30/70 lines
                painter.setPen(QColor(80, 80, 80));
                painter.drawLine(plotInd.left(), rsiToY(70), plotInd.right(), rsiToY(70));
                painter.drawLine(plotInd.left(), rsiToY(30), plotInd.right(), rsiToY(30));

                painter.setPen(QPen(QColor(180, 120, 255), 1));

                QPolygonF poly;
                const int n = rsi->values_.size();

                for (int i = first; i <= last; ++i) {
                    if (i < 0 || i >= n) continue;

                    const double v = rsi->values_[i];
                    if (std::isnan(v)) continue;

                    const int k = i - first;
                    const double x = plotPrice.left() + k * step + candleWidth_ / 2.0;
                    const double y = rsiToY(v);
                    poly << QPointF(x, y);
                }

                if (poly.size() >= 2) {
                    painter.drawPolyline(poly);
                }
            }

            if (atr) {
                double mn = std::numeric_limits<double>::infinity();
                double mx = -std::numeric_limits<double>::infinity();

                const int n = atr->values_.size();
                for (int i = first; i <= last; ++i) {
                    if (i < 0 || i >= n) continue;

                    const double v = atr->values_[i];
                    if (std::isnan(v)) continue;

                    mn = std::min(mn, v);
                    mx = std::max(mx, v);
                }

                if (std::isfinite(mn) && std::isfinite(mx) && mx > mn) {
                    painter.setPen(QPen(QColor(0, 180, 255), 1));

                    auto atrToY = [&](double v) -> int {
                        const double t = (mx - v) / (mx - mn);
                        return static_cast<int>(std::round(plotInd.top() + t * plotInd.height()));
                    };

                    QPolygonF poly;
                    for (int i = first; i <= last; ++i) {
                        if (i < 0 || i >= n) continue;

                        const double v = atr->values_[i];
                        if (std::isnan(v)) continue;

                        const int k = i - first;
                        const double x = plotPrice.left() + k * step + candleWidth_ / 2.0;
                        const double y = atrToY(v);
                        poly << QPointF(x, y);
                    }

                    if (poly.size() >= 2) {
                        painter.drawPolyline(poly);
                    }
                }
            }

            painter.restore();
        }
    }

    // Y axis (price scale)
    {
        // Сколько подписей хотим(1 text on 60px)
        const int desiredTicks = qBound(5, plotPrice.height() / 60, 10);

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

                painter.drawLine(plotPrice.right() - 3, y, plotPrice.right() + 3, y);

                // right text
                const QString label = QString::number(p, 'f', decimals);

                // text rectangle
                QRect textRect(plotPrice.right() + 6, y - 8, rightPadding_ - 8, 16);
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, label);

            }

        }
    }

    // Current price line
    if (!candles.isEmpty()) {
        const double lastPrice = candles.last().close_;
        const int y = priceToY(lastPrice);

        QPen pen(QColor(220, 220, 220));
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawLine(plotPrice.left(), y, plotPrice.right(), y);
    }

    // Current price label
        if (!candles.isEmpty()) {
            const double lastPrice = candles.last().close_;
            const int y = priceToY(lastPrice);

            const QString text = QString::number(lastPrice, 'f', 2);

            const int padX = 3;
            const int h = 18;

            const QFontMetrics fm(painter.font());
            const int w = fm.boundingRect(text).width() + padX * 2;

            int x = plotPrice.right() + 6;
            int yTop = y - h / 2;

            yTop = qBound(plotPrice.top(), yTop, plotPrice.bottom() - h);

            QRect r(x, yTop, w, h);

            painter.save();

            // background , frame
            painter.setPen(QColor(60, 60, 60));
            painter.setBrush(QColor(35, 35, 35));
            painter.drawRect(r);

            // text
            painter.setPen(QColor(230,230,230));
            painter.drawText(r, Qt::AlignCenter, text);

            painter.restore();
    }

    // X axis (Time scale)
    {
        const int stepPx = candleWidth_ + candleGap_;

        const int minMinorLabelSpacingPx = 140;

        int stepBars = static_cast<int>(std::ceil(static_cast<double>(minMinorLabelSpacingPx) / stepPx));
        if (stepBars < 1) {
            stepBars = 1;
        }

        painter.save();
        painter.setPen(QColor(180,180,180));

        int lastMinorRight = std::numeric_limits<int>::min();
        int lastMajorRight = std::numeric_limits<int>::min();

        auto toDt = [](qint64 ms){
            return QDateTime::fromMSecsSinceEpoch(ms, Qt::UTC);
        };

        const QFontMetrics fm(painter.font());

        QDateTime prevDt;

        // Pass 1: Major
        for (int i = first; i <= last; ++i) {

            const qint64 ts = static_cast<qint64>(candles[i].timestamp_);

            const QDateTime dt = toDt(ts);

            bool majorYear = false;
            bool majorMonth = false;
            bool majorDay = false;

            if (i > first) {
                const QDate pd = prevDt.date();
                const QDate cd = dt.date();

                if (currentTf_ == Timeframe::D1) {
                    majorYear = (pd.year() != cd.year());
                    majorMonth = (!majorYear && (pd.month() != cd.month()));
                } else if (timeframeIsIntraday(currentTf_)) {
                    majorDay = (pd != cd);
                }
            }

            prevDt = dt;

            if (!(majorYear || majorMonth || majorDay)) {
                continue;
            }

            const int k = i - first;
            const int x = plotPrice.left() + k * step;
            const int xCenter = x + candleWidth_ / 2;

            QString label;
            if (majorYear) {
                label = dt.toString("yyyy");
            } else if (majorMonth) {
                label = dt.toString("MMM yyyy");
            } else {
                label = dt.toString("dd MMM");
            }

            const int textW = fm.width(label);
            const int textH = fm.height();

            QRect textRect(xCenter - textW / 2, plotPrice.bottom() + 6, textW + 2, textH);

            if (textRect.left() < plotPrice.left()) {
                textRect.moveLeft(plotPrice.left());
            }
            if (textRect.right() > plotPrice.right()) {
                textRect.moveRight(plotPrice.right());
            }

            if (textRect.left() <= lastMajorRight + 6) {
                continue;
            }
            lastMajorRight = textRect.right();

            const int tickLen = majorYear ? 12 : (majorMonth ? 10 : 8);

            painter.drawLine(xCenter, plotPrice.bottom(), xCenter, plotPrice.bottom() + tickLen);
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
        }

        // Pass 2: Minor
        for (int i = first; i <= last; i += stepBars) {

            const int k = i - first;
            const int x = plotPrice.left() + k * step;
            const int xCenter = x + candleWidth_ / 2;

            const qint64 ts = static_cast<qint64>(candles[i].timestamp_);
            const QDateTime dt = toDt(ts);

            QString label;
            if (currentTf_ == Timeframe::D1) {
                label = dt.toString("dd");
            } else {
                label = dt.toString("HH:mm");
            }

            const int textW = fm.width(label);
            const int textH = fm.height();

            QRect textRect(xCenter - textW / 2, plotPrice.bottom() + 6, textW + 2, textH);

            if (textRect.left() < plotPrice.left())  {
                textRect.moveLeft(plotPrice.left());
            }
            if (textRect.right() > plotPrice.right()) {
                textRect.moveRight(plotPrice.right());
            }

            // 1) не рисуем, если налезает на предыдущий minor
            if (textRect.left() <= lastMinorRight + 6) {
                continue;
            }

            // Для M1..H1: хотим видеть HH:mm по всем дням, major не должен "глушить" время.
            // Для H4 и D1: оставляем как сейчас — major может подавлять minor рядом с ним.
            if (timeframeShowTimeAcrossDays(currentTf_)) {
                if (textRect.left() <= lastMajorRight + 6) {
                    continue;
                }
            }

            lastMinorRight = textRect.right();

            painter.drawLine(xCenter, plotPrice.bottom(), xCenter, plotPrice.bottom() + 4);
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
        }

        painter.restore();
    }

    // frame
    painter.setPen(QColor(80,80,80));
    painter.drawRect(plotPrice);

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
    if (count <= 0) {
        return;
    }

    const int maxVis = maxVisibleByWidth();
    const int vis = std::max(1, std::min(visibleCount_, maxVis));

    const int maxFirstData = std::max(0, count - vis);
    const int maxFirst = maxFirstData + kFutureBars;

    if (followRight_) {
        firstVisible_ = maxFirstData;   // followRight = строго к последним данным, без пустоты
        panRemainder_ = 0.0;
    } else {
        firstVisible_ = qBound(0, firstVisible_, maxFirst);
    }


    // clamp candle width
    if (candleWidth_ < minCandleWidth_) {
        candleWidth_ = minCandleWidth_;
    }
    else if (candleWidth_ > maxCandleWidth_) {
        candleWidth_ = maxCandleWidth_;
    }
}

//========================================================== Helpers to WheelEvent ==========================================================
QRectF ChartWidget::plotRect() const {
    return QRectF(leftPadding_, topPadding_, width() - leftPadding_ - rightPadding_,
                  height() - topPadding_ - bottomPadding_ );
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


