#pragma once
#include "iexchangeclient.h"
#include <vector>
#include <QNetworkAccessManager>

class BingXSwapClient final : public IExchangeClient
{
public:
    std::vector<Candle> fetchKlines(const QString& symbolId, Timeframe tf) override;
    std::vector<Candle> fetchHistory(const HistoryRequest& request) override;

    bool supportsPollingRealtime() const override { return true; }
    void fetchLastKlineAsync(const QString& symbolId, Timeframe tf, LastKlineCallback cb) override;

private:
    QNetworkAccessManager* mgr_ = nullptr; // one per app
};
