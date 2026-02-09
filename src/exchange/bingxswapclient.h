#pragma once
#include "iexchangeclient.h"
#include <QNetworkAccessManager>

class BingXSwapClient final : public IExchangeClient
{
public:
    BingXSwapClient() = default;
    ~BingXSwapClient() override = default;

    QList<Candle> fetchKlines(const QString& symbolId, Timeframe tf) override;

private:
    QNetworkAccessManager* mgr_ = nullptr; // создадим позже, когда qApp уже есть
};
