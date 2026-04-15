#pragma once
#include <QObject>
#include "exchange\iexchangeclient.h"
#include "backtestmarketdataservice.h"
#include "ui/models/backtesttradesmodel.h"
#include "backtestengine.h"
#include "backtestwidget.h"
#include <memory>
#include <vector>

namespace Ui {class MainWindow;}

class BacktestController final : public QObject {
    Q_OBJECT

    Ui::MainWindow* ui_ = nullptr;
    BacktestTradesModel* tradesModel_ = nullptr;
    std::shared_ptr<IExchangeClient> exchange_;
    std::unique_ptr<BacktestMarketDataService> marketDataService_;
    BacktestEngine engine_;
    BacktestWidget* graphWidget_ = nullptr;

    BacktestResult lastResult_;
    bool hasResult_ = false;
public:
    explicit BacktestController(Ui::MainWindow* ui, std::shared_ptr<IExchangeClient> exchange, QObject* parent = nullptr);
    //std::vector<Candle> loadHistoryFromUi() const; // возможно понадобится в дальнейшем. На данном этапе - нет

public slots:
    void onStart();
    void onBuildGraph();

    void onGraphTypeChanged();
    void onGraphAxisChanged();

private:
    std::vector<Candle> loadHistory(const HistoryRequest& rec) const;
    HistoryRequest buildHistoryRequest() const;
    BacktestRequest buildBacktestRequest(const HistoryRequest& hr) const;
    std::vector<GraphPoint> buildEquityGraph(const BacktestResult& res, const GraphRequest& gr) const;
    std::vector<GraphPoint> buildDrawdownGraph(const BacktestResult& res, const GraphRequest& gr) const;
    GraphRequest buildGraphRequest() const;
    void setResultsToUi();
    void setTradesToUi();
};

