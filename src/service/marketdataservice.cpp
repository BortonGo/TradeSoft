#include "marketdataservice.h"
#include <iostream>
#include <QDebug>

MarketDataService::MarketDataService(std::shared_ptr<IExchangeClient> exchange) : exchange_(exchange)
{
    Q_ASSERT(exchange_);
}

void MarketDataService::loadHistory(const QString& symbolId, Timeframe tf) {
    if (symbolId.isEmpty()){
        qWarning() << "symbolId is empty!";
        return;
    }

    if (exchange_ == nullptr){
        qWarning() << "exchange_ == nullptr";
        return;
    }

    QList<Candle> candles = exchange_->fetchKlines(symbolId, tf);
    std::shared_ptr<CandleSeries> series = std::make_shared<CandleSeries> (symbolId, tf);

    for(auto& c : candles) {
        series->addCandle(c);
    }

    currentSeries_ = series;

    signal_seriesLoaded(currentSeries_);

}

void MarketDataService::signal_seriesLoaded(std::shared_ptr<CandleSeries> series) {

}
