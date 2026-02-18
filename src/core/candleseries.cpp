#include "candleseries.h"
#include <math.h>

CandleSeries::CandleSeries(const QString& symbolId, Timeframe tf) : symbol_(symbolId), timeframe_(tf) {}

const QString& CandleSeries::symbol() const { return symbol_; }
Timeframe CandleSeries::timeframe() const { return timeframe_; }
int CandleSeries::candleCount() const { return candles_.size(); }

void CandleSeries::addCandle(const Candle& c) {
    candles_.push_back(c);
}

void CandleSeries::updateLastCandle(const Candle& c) {
    if(candles_.empty()){
        return;
    }

    Candle& last = candles_.back();

    last.high_ = std::max(last.high_, c.high_);
    last.low_ = std::min(last.low_, c.low_);
    last.close_ = c.close_;
    last.volume_ = c.volume_;
    last.isFinal_ = c.isFinal_;

}

const Candle& CandleSeries::last() const {
    return candles_.back();
}

int CandleSeries::getCount() const {
    return candles_.size();
}

const std::vector<Candle>& CandleSeries::getCandles() const {
    return candles_;
}
