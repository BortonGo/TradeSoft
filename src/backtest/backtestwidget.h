#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <vector>
#include "backtest/backtesttypes.h"

class BacktestWidget final : public QWidget {
    Q_OBJECT

    std::vector<GraphPoint> points_;

    int leftPadding_ = 12;
    int rightPadding_ = 12;
    int topPadding_ = 12;
    int bottomPadding_ = 12;

public:
    explicit BacktestWidget(QWidget* parent = nullptr);

    void setPoints(const std::vector<GraphPoint>& points);
    void clearPoints();

protected:
    void paintEvent(QPaintEvent* event) override;
};
