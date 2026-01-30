#include "candleseries.h"

CandleSeries::CandleSeries(const QString& symbolId, Timeframe tf) : symbol_(symbolId), timeframe_(tf) {}

const QString& CandleSeries::symbol() const { return symbol_; }
Timeframe CandleSeries::timeframe() const { return timeframe_; }
