#include "marketdataservice.h"
#include <iostream>
#include <QDebug>

MarketDataService::MarketDataService(std::shared_ptr<IExchangeClient> exchange, QObject *parent) :
    QObject(parent), exchange_(exchange), rng_(std::random_device{}())
{
    Q_ASSERT(exchange_);
}

void MarketDataService::loadHistory(const QString& symbolId, Timeframe tf) {
    if (symbolId.isEmpty()){
        qWarning() << "[MarketDataService] symbolId is empty!";
        return;
    }

    if (exchange_ == nullptr){
        qWarning() << "[MarketDataService] exchange_ == nullptr";
        return;
    }

    QList<Candle> candles = exchange_->fetchKlines(symbolId, tf);
    std::shared_ptr<CandleSeries> series = std::make_shared<CandleSeries> (symbolId, tf);

    for(const auto& c : candles) {
        series->addCandle(c);
    }

    currentSeries_ = series;

    rtTimeframe_ = tf;
    rtSymbolId_ = symbolId;


    emit signal_seriesLoaded(currentSeries_);
    useExchangeRealtime_ = exchange_->supportsPollingRealtime();

}

void MarketDataService::startRealTime()
{
    if (!currentSeries_) {
        qWarning() << "[MarketDataService] currentSeries_ == nullptr";
        return;
    }

    if (!rtTimer_) {
        rtTimer_ = new QTimer(this);
        connect(rtTimer_, &QTimer::timeout, this, &MarketDataService::onRtTick);
    }

    rtTimer_->setInterval(useExchangeRealtime_ ? 1000 : 300);


    tickInCandle_ = 0;

    if (!rtTimer_->isActive())
        rtTimer_->start();
}

void MarketDataService::stopRealTime() {
    if (!rtTimer_) {
        return;
    }
    rtTimer_->stop();
}

int MarketDataService::ticksPerCandle(Timeframe tf) const {
    switch (tf){
        case Timeframe::M1 : return 5; // 0.3sec * 200 = 60sec
        case Timeframe::M5 : return 10;
        case Timeframe::M15 : return 15;
        case Timeframe::H1 : return 20;
        case Timeframe::H4 : return 30;
        case Timeframe::D1 : return 40;
    default : return 5;
    }
}

Candle MarketDataService::makeNextCandle(const Candle& closed) const {
    Candle next;
    next.timestamp_ = closed.timestamp_ + timeframeToMs(rtTimeframe_);
    next.open_ = closed.close_;
    next.close_ = next.open_;
    next.high_ = next.open_;
    next.low_  = next.open_;
    next.volume_ = 0.0;
    next.isFinal_ = false;
    return next;
}


void MarketDataService::onRtTick()
{
    if (!currentSeries_ || currentSeries_->getCount() == 0) {
        return;
    }

    // --------- BingX realtime via polling ----------
    if (useExchangeRealtime_) {
        if (requestInFlight_) {
            return;
        }
        requestInFlight_ = true;

        exchange_->fetchLastKlineAsync(rtSymbolId_, rtTimeframe_,
            [this](bool ok, const Candle& freshIn) {

                requestInFlight_ = false;

                if (!ok || !currentSeries_ || currentSeries_->getCount() == 0) {
                    return;
                }

                Candle fresh = freshIn;
                Candle last = currentSeries_->last();

                // same candle -> update
                if (fresh.timestamp_ == last.timestamp_) {
                    fresh.isFinal_ = false;
                    currentSeries_->updateLastCandle(fresh);
                    emit signal_candleUpdated(fresh);
                    return;
                }

                // new candle -> close old + add new
                if (fresh.timestamp_ > last.timestamp_) {
                    last.isFinal_ = true;
                    currentSeries_->updateLastCandle(last);
                    emit signal_candleClosed(last);

                    fresh.isFinal_ = false;
                    currentSeries_->addCandle(fresh);
                    emit signal_candleUpdated(fresh);
                    return;
                }

                // if fresh.timestamp_ < last.timestamp_ -> ignore (old data)
            }
        );

        return;
    }

    // --------- old Fake realtime (твоя генерация) ----------
    Candle last = currentSeries_->last();

    static const double sigma = 1.5;
    static thread_local std::normal_distribution<double> dist(0.0, sigma);
    static thread_local std::uniform_real_distribution<double> volDist(10.0, 200.0);

    const double delta = dist(rng_);
    const double randomSmallVolume = volDist(rng_);

    const double newClose = last.close_ + delta;
    last.close_ = newClose;
    last.high_ = std::max(last.high_, newClose);
    last.low_  = std::min(last.low_,  newClose);
    last.volume_ += randomSmallVolume;
    last.isFinal_ = false;

    tickInCandle_++;

    const int tpc = ticksPerCandle(rtTimeframe_);
    const bool shouldClose = (tickInCandle_ >= tpc);

    if (!shouldClose) {
        currentSeries_->updateLastCandle(last);
        emit signal_candleUpdated(last);
        return;
    }

    last.isFinal_ = true;
    currentSeries_->updateLastCandle(last);
    emit signal_candleClosed(last);

    Candle next = makeNextCandle(last);
    currentSeries_->addCandle(next);
    emit signal_candleUpdated(next);

    tickInCandle_ = 0;
}


