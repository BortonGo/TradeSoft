#pragma once

#include <QObject>
#include <QWidget>
#include <QPaintEvent>
#include <memory>
#include "core\candleseries.h"
#include "core\candle.h"

class ChartWidget : public QWidget {
    Q_OBJECT

    std::shared_ptr<CandleSeries> series_;

    int firstVisible_ = 0;
    int visibleCount_ = 200;

    int candleWidth_ = 6;
    int minCandleWidth_ = 2;
    int maxCandleWidth_ = 20;
    int candleGap_ = 2;


    int leftPadding_ = 10;
    int rightPadding_ = 60;
    int topPadding_ = 10;
    int bottomPadding_ = 20;

    bool followRight_ = true;

public:
    explicit ChartWidget(QWidget* parent = nullptr);
    int maxVisibleByWidth() const;
    int lastVisible() const;
    void normalizeViewport();

public slots:
    void slot_setSeries(std::shared_ptr<CandleSeries> series);
    void slot_onCandleUpdate(Candle c); // написать позже реализацию
    void slot_onCandleClosed(Candle c); // написать позже реализацию

protected:
    void paintEvent(QPaintEvent* event) override;


};
