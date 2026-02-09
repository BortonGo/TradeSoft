#pragma once
#include "iexchangeclient.h"

#include <QNetworkAccessManager>

class BingXSwapClient final : public IExchangeClient
{
public:
    QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) override;

    bool supportsPollingRealtime() const override { return true; }
    void fetchLastKlineAsync(const QString& symbolId, Timeframe tf, LastKlineCallback cb) override;

private:
    QNetworkAccessManager* mgr_ = nullptr; // one per app
};
