#pragma once

#include <QObject>
#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <memory>
#include "core\candleseries.h"
#include "core\candle.h"
#include "core\timeframe.h"
#include "indicators\indicatortypes.h"

class ChartWidget : public QWidget {
    Q_OBJECT

    std::shared_ptr<CandleSeries> series_;
    Timeframe currentTf_ = Timeframe::M1;

    int firstVisible_ = 0;
    int visibleCount_ = 500;

    int candleWidth_ = 6;
    int minCandleWidth_ = 2;
    int maxCandleWidth_ = 20;
    int candleGap_ = 2;


    int leftPadding_ = 10;
    int rightPadding_ = 60;
    int topPadding_ = 10;
    int bottomPadding_ = 20;

    bool followRight_ = true;

    bool isPanning_ = false;
    int panLastX_ = 0;
    double panRemainder_ = 0.0;
    double candleWidthAcc_ = 6.0;

    std::vector<IndicatorLine> indicatorLines_;

public:
    explicit ChartWidget(QWidget* parent = nullptr);
    void setTimeframe(Timeframe tf);
    void setIndicatorLines(const std::vector<IndicatorLine>& lines);

public slots:
    void slot_setSeries(std::shared_ptr<CandleSeries> series);
    void slot_onCandleUpdate(Candle c);
    void slot_onCandleClosed(Candle c);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

private:
    int maxVisibleByWidth() const;
    int lastVisible() const;
    void normalizeViewport();
    bool isNearRightEdgeByData(int snapCandles) const;
    QRectF plotRect() const;
    int stepPx() const;
    int clampCandleWidth(int width) const;

};


