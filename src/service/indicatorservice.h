#pragma once
#include <QObject>
#include <memory>

#include "service/marketdataservice.h"
#include "indicators/indicatorengine.h"
#include "indicators/indicatorconfig.h"

class IndicatorDialog; // forward

class IndicatorService : public QObject
{
    Q_OBJECT

    IndicatorConfig cfg_;

public:
    explicit IndicatorService(MarketDataService* mds, QObject* parent = nullptr);

    void applyDialogResultFlags(bool ema20, bool ema50, bool donchian20, bool rsi14, bool atr14);

    IndicatorConfig config() const;
    void applyConfig(const IndicatorConfig& cfg);

signals:
    void signal_overlayLinesUpdated(QVector<IndicatorLine> lines);

private:
    MarketDataService* mds_ = nullptr; // не владеем
    std::shared_ptr<CandleSeries> series_; // храним тут, MainWindow не хранит
    IndicatorEngine engine_;

private slots:
    void onSeriesLoaded(std::shared_ptr<CandleSeries> s);
    void onCandleUpdated(Candle c);
    void onCandleClosed(Candle c);

private:
    void rebuildAndEmit();
};
