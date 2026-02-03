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

public:
    explicit ChartWidget(QWidget* parent = nullptr);

public slots:
    void slot_setSeries(std::shared_ptr<CandleSeries> series);
    void slot_onCandleUpdate(Candle c); // написать позже реализацию
    void slot_onCandleClosed(Candle c); // написать позже реализацию

protected:
    void paintEvent(QPaintEvent* event) override;


};
