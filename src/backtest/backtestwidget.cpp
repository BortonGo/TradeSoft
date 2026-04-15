#include "backtestwidget.h"

#include <QPainter>
#include <QPolygonF>
#include <QPen>
#include <QBrush>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>

BacktestWidget::BacktestWidget(QWidget* parent)
    : QWidget(parent) {}

void BacktestWidget::setPoints(const std::vector<GraphPoint>& points) {
    points_ = points;
    update();
}

void BacktestWidget::clearPoints() {
    points_.clear();
    update();
}

void BacktestWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(22, 22, 22));

    if (points_.empty()) {
        painter.setPen(QColor(220, 220, 220));
        painter.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    const QRect plot = rect().adjusted(
        leftPadding_,
        topPadding_,
        -rightPadding_,
        -bottomPadding_
    );

    if (plot.width() <= 0 || plot.height() <= 0) {
        return;
    }

    double minX = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();

    for (const auto& p : points_) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    if (!std::isfinite(minX) || !std::isfinite(maxX) ||
        !std::isfinite(minY) || !std::isfinite(maxY)) {
        painter.setPen(QColor(220, 220, 220));
        painter.drawText(rect(), Qt::AlignCenter, "Invalid data");
        return;
    }

    if (maxX <= minX) {
        minX -= 1.0;
        maxX += 1.0;
    }

    if (maxY <= minY) {
        minY -= 1.0;
        maxY += 1.0;
    }

    const double xRange = maxX - minX;
    const double yRange = maxY - minY;

    const double xPad = xRange * 0.02;
    const double yPad = yRange * 0.05;

    minX -= xPad;
    maxX += xPad;
    minY -= yPad;
    maxY += yPad;

    const double xScale = static_cast<double>(plot.width()) / (maxX - minX);
    const double yScale = static_cast<double>(plot.height()) / (maxY - minY);

    auto mapX = [&](double x) -> double {
        return plot.left() + (x - minX) * xScale;
    };

    auto mapY = [&](double y) -> double {
        return plot.bottom() - (y - minY) * yScale;
    };

    painter.setPen(QPen(QColor(70, 70, 70), 1));
    painter.drawRect(plot);

    QPolygonF polyline;
    polyline.reserve(static_cast<int>(points_.size()));

    for (const auto& p : points_) {
        polyline << QPointF(mapX(p.x), mapY(p.y));
    }

    if (polyline.size() == 1) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 180, 255));
        painter.drawEllipse(polyline[0], 3.0, 3.0);
        return;
    }

    painter.setPen(QPen(QColor(0, 180, 255), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(polyline);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 180, 255));
    painter.drawEllipse(polyline.first(), 3.0, 3.0);
    painter.drawEllipse(polyline.last(), 3.0, 3.0);
}
