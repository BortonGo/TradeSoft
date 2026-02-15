#include "indicatorservice.h"
#include <QDebug>

IndicatorService::IndicatorService(MarketDataService* mds, QObject* parent)
    : QObject(parent), mds_(mds)
{
    Q_ASSERT(mds_);

    connect(mds_, &MarketDataService::signal_seriesLoaded,
            this, &IndicatorService::onSeriesLoaded);

    connect(mds_, &MarketDataService::signal_candleUpdated,
            this, &IndicatorService::onCandleUpdated);

    connect(mds_, &MarketDataService::signal_candleClosed,
            this, &IndicatorService::onCandleClosed);
}

IndicatorConfig IndicatorService::config() const {
    return cfg_;
}

void IndicatorService::applyConfig(const IndicatorConfig& cfg)
{
    cfg_ = cfg;
    engine_.setEnabled(IndicatorId::EMA9, cfg_.ema9);
    engine_.setEnabled(IndicatorId::EMA20, cfg_.ema20);
    engine_.setEnabled(IndicatorId::EMA50, cfg_.ema50);
    // остальные позже

    rebuildAndEmit();
}

void IndicatorService::onSeriesLoaded(std::shared_ptr<CandleSeries> s)
{
    series_ = s;
    rebuildAndEmit();
}

void IndicatorService::onCandleUpdated(Candle /*c*/)
{
    // MVP: просто пересчёт (500 свечей раз в 1с — ок)
    rebuildAndEmit();
    /* Дальше должно быть что-то типо:
     * при обновлении текущей свечи → обновляется только последняя точка индикатора
     * при закрытии свечи → добавляется новая точка индикатора
     *
     * updateLastPoint()
     * addNewPoint()  */
}

void IndicatorService::onCandleClosed(Candle /*c*/)
{
    rebuildAndEmit();
}

void IndicatorService::rebuildAndEmit()
{
    if (!series_ || series_->getCount() <= 0) {
        emit signal_overlayLinesUpdated(QVector<IndicatorLine>{});
        return;
    }

    engine_.rebuild(*series_);
    emit signal_overlayLinesUpdated(engine_.overlayLines());
}

