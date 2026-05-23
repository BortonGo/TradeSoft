#include "marketdataservice.h"
#include "service/marketdata/candlecache.h"
#include <iostream>
#include <QDebug>
#include <QtGlobal>

MarketDataService::MarketDataService(std::shared_ptr<IExchangeClient> exchange, QObject *parent) :
    QObject(parent), exchange_(exchange)
{
    Q_ASSERT(exchange_);
}

void MarketDataService::loadHistory(const QString& symbolId, Timeframe tf) {
    if (symbolId.isEmpty()){
        qWarning() << "[MarketDataService] symbolId is empty!";
        emit signal_connectionStateChanged("Market data: symbol is empty");
        return;
    }

    if (exchange_ == nullptr){
        qWarning() << "[MarketDataService] exchange_ == nullptr";
        emit signal_connectionStateChanged("Market data: exchange is not configured");
        return;
    }

    emit signal_connectionStateChanged("Market data: loading " + symbolId + " " + toUiString(tf));

    std::vector<Candle> cachedCandles = CandleCache::load(symbolId, tf);
    std::vector<Candle> candles;

    if (CandleCache::isFresh(cachedCandles, tf)) {
        candles = cachedCandles;
        emit signal_connectionStateChanged("Market data: loaded fresh cache");
        qDebug() << "[MarketDataService] Loaded fresh candles from cache"
                 << symbolId << toUiString(tf) << candles.size();
    } else {
        candles = exchange_->fetchKlines(symbolId, tf);
        if (!candles.empty()) {
            CandleCache::save(symbolId, tf, candles);
            emit signal_connectionStateChanged("Market data: loaded from exchange");
            qDebug() << "[MarketDataService] Saved candles to cache"
                     << symbolId << toUiString(tf) << candles.size();
        } else if (!cachedCandles.empty()) {
            candles = cachedCandles;
            emit signal_connectionStateChanged("Market data: exchange failed, using stale cache");
            qWarning() << "[MarketDataService] Exchange returned no candles; using stale cache"
                       << symbolId << toUiString(tf) << candles.size();
        } else {
            emit signal_connectionStateChanged("Market data: no candles loaded");
        }
    }

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
    consecutiveRealtimeFailures_ = 0;
}

void MarketDataService::startRealTime()
{
    if (!currentSeries_) {
        qWarning() << "[MarketDataService] currentSeries_ == nullptr";
        emit signal_connectionStateChanged("Market data: realtime cannot start without series");
        return;
    }

    if (exchange_->supportsWebSocketRealtime()) {
        useWebSocketRealtime_ = true;
        exchange_->startKlineStream(rtSymbolId_, rtTimeframe_,
            [this](bool ok, const Candle& fresh) {
                if (!ok) {
                    if (!useWebSocketRealtime_) {
                        return;
                    }
                    useWebSocketRealtime_ = false;
                    emit signal_connectionStateChanged("Market data: websocket failed, using polling");
                    startPollingRealtime();
                    return;
                }

                consecutiveRealtimeFailures_ = 0;
                applyRealtimeCandle(fresh);
            });
        emit signal_connectionStateChanged("Market data: websocket starting");
        return;
    }

    startPollingRealtime();
}

void MarketDataService::startPollingRealtime()
{
    if (!rtTimer_) {
        rtTimer_ = new QTimer(this);
        connect(rtTimer_, &QTimer::timeout, this, &MarketDataService::onRtTick);
    }

    rtTimer_->setInterval(realtimePollingMs_);

    if (!rtTimer_->isActive())
        rtTimer_->start();

    emit signal_connectionStateChanged("Market data: realtime polling started");
}

void MarketDataService::setRealtimePollingMs(int intervalMs)
{
    realtimePollingMs_ = qBound(250, intervalMs, 60000);
    if (rtTimer_) {
        rtTimer_->setInterval(realtimePollingMs_);
    }
}

void MarketDataService::stopRealTime() {
    useWebSocketRealtime_ = false;
    if (exchange_) {
        exchange_->stopKlineStream();
    }
    if (rtTimer_) {
        rtTimer_->stop();
    }
    requestInFlight_ = false;
    emit signal_connectionStateChanged("Market data: realtime stopped");
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
                    ++consecutiveRealtimeFailures_;
                    setRealtimeIntervalForFailures();
                    emit signal_connectionStateChanged(
                        "Market data: realtime retry #" + QString::number(consecutiveRealtimeFailures_));
                    return;
                }

                if (consecutiveRealtimeFailures_ > 0) {
                    consecutiveRealtimeFailures_ = 0;
                    setRealtimeIntervalForFailures();
                    emit signal_connectionStateChanged("Market data: realtime recovered");
                }

                applyRealtimeCandle(freshIn);
            }
        );

        return;
    }
}

void MarketDataService::applyRealtimeCandle(const Candle& freshIn)
{
    if (!currentSeries_ || currentSeries_->getCount() == 0) {
        return;
    }

    Candle fresh = freshIn;
    Candle last = currentSeries_->last();

    // same candle -> update
    if (fresh.timestamp_ == last.timestamp_) {
        fresh.isFinal_ = false;
        currentSeries_->updateLastCandle(fresh);
        CandleCache::save(rtSymbolId_, rtTimeframe_, currentSeries_->getCandles());
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
        CandleCache::save(rtSymbolId_, rtTimeframe_, currentSeries_->getCandles());
        emit signal_candleUpdated(fresh);
        return;
    }

    // if fresh.timestamp_ < last.timestamp_ -> ignore (old data)
}

void MarketDataService::setRealtimeIntervalForFailures()
{
    if (!rtTimer_) {
        return;
    }

    const int multiplier = qBound(1, consecutiveRealtimeFailures_ + 1, 10);
    rtTimer_->setInterval(realtimePollingMs_ * multiplier);
}
