#include "marketdataservice.h"
#include <iostream>
#include <QDebug>

MarketDataService::MarketDataService(std::shared_ptr<IExchangeClient> exchange, QObject *parent) :
    QObject(parent), exchange_(exchange)
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

    requestInFlight_ = false;
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

    rtTimer_->setInterval(1000);

    if (!rtTimer_->isActive())
        rtTimer_->start();
}

void MarketDataService::stopRealTime() {
    if (rtTimer_) {
        rtTimer_->stop();
    }
    requestInFlight_ = false;
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
}


