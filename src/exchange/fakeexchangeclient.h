#pragma once
#include "iexchangeclient.h"

class FakeExchangeClient final : public IExchangeClient
{
public:
    QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) override;

    bool fetchLastKline(const QString& symbolId, Timeframe tf, Candle& out) override;
};

