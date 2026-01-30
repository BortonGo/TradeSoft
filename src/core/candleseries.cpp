#include "candleseries.h"
#include <math.h>

CandleSeries::CandleSeries(const QString& symbolId, Timeframe tf) : symbol_(symbolId), timeframe_(tf) {}

const QString& CandleSeries::symbol() const { return symbol_; }
Timeframe CandleSeries::timeframe() const { return timeframe_; }

void CandleSeries::addCandle(const Candle& c) {
    candles_.push_back(c);
}

void CandleSeries::updateLastCandle(const Candle& c) {
    if(candles_.isEmpty()){
        return;
    }

    Candle& last = candles_.last();

    last.high = std::max(last.high, c.high);
    last.low = std::min(last.low, c.low);
    last.close = c.close;
    last.volume = c.volume;
    last.isFinal = c.isFinal;

}

const Candle& CandleSeries::last() const{
    return candles_.constLast();
}
