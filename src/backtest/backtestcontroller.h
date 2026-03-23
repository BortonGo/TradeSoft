#pragma once
#include <QObject>
#include "exchange\iexchangeclient.h"
#include "backtestmarketdataservice.h"
#include <memory>
#include <vector>

namespace Ui {class MainWindow;}

class BacktestController final : public QObject
{
    Q_OBJECT

    Ui::MainWindow* ui_ = nullptr;
    std::shared_ptr<IExchangeClient> exchange_;
    std::unique_ptr<BacktestMarketDataService> marketDataService_;
public:
    explicit BacktestController(Ui::MainWindow* ui, std::shared_ptr<IExchangeClient> exchange, QObject* parent = nullptr);
    std::vector<Candle> loadHistory(const HistoryRequest& rec) const;
    std::vector<Candle> loadHistoryFromUi() const;

public slots:
    void onStart();
    void onBuildGraph();

private:
    HistoryRequest buildHistoryRequest() const;
};

