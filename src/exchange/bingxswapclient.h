#pragma once
#include "iexchangeclient.h"
#include <QNetworkAccessManager>

class BingXSwapClient final : public IExchangeClient
{
public:
    BingXSwapClient() = default;
    ~BingXSwapClient() override = default;

    QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) override;

    bool fetchLastKline(const QString& symbolId, Timeframe tf, Candle& out) override;

    bool supportsPollingRealtime() const override { return true; }

private:
    QNetworkAccessManager* mgr_ = nullptr; // создадим позже, когда qApp уже есть
};
