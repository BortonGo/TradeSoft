#pragma once
#include <QObject>
#include "exchange/iexchangeclient.h"
#include "backtestmarketdataservice.h"
#include "ui/models/backtesttradesmodel.h"
#include "backtestengine.h"
#include "backtestwidget.h"
#include <memory>
#include <vector>

namespace Ui {class MainWindow;}
class QLineEdit;
class QPushButton;

class BacktestController final : public QObject {
    Q_OBJECT

    Ui::MainWindow* ui_ = nullptr;
    BacktestTradesModel* tradesModel_ = nullptr;
    std::shared_ptr<IExchangeClient> exchange_;
    std::unique_ptr<BacktestMarketDataService> marketDataService_;
    BacktestEngine engine_;
    BacktestWidget* graphWidget_ = nullptr;

    BacktestRequest lastRequest_;
    BacktestResult lastResult_;
    bool hasResult_ = false;

    QLineEdit* grossPnlEdit_ = nullptr;
    QLineEdit* feesEdit_ = nullptr;
    QLineEdit* bestTradeEdit_ = nullptr;
    QLineEdit* worstTradeEdit_ = nullptr;
    QLineEdit* avgBarsHeldEdit_ = nullptr;
    QPushButton* openReportsButton_ = nullptr;
public:
    explicit BacktestController(Ui::MainWindow* ui, std::shared_ptr<IExchangeClient> exchange, QObject* parent = nullptr);

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
    std::vector<GraphPoint> buildPnlByTradeGraph(const BacktestResult& res, const GraphRequest& gr) const;
    GraphRequest buildGraphRequest() const;
    void setupReportUi();
    void setResultsToUi();
    void setTradesToUi();
    void openReportsFolder() const;
    std::vector<BacktestTrade> filterTrades(const BacktestResult& res, const GraphRequest& gr) const;
    std::vector<EquityPoint> buildEquityCurveFromTrades(const std::vector<BacktestTrade>& trades, double initialBalance) const;
};
