#pragma once
#include <memory>
#include <QObject>
#include <optional>

#include "service/marketdataservice.h"
#include "domain/strategy/istrategy.h"
#include "domain/strategy/strategysignal.h"
#include "domain/strategy/strategyconfig.h"
#include "domain/risk/riskmanager.h"
#include "service/execution/demoexecutionservice.h"
#include "service/trade/tradejournal.h"

#include "core/events/latencytimestamp.h"
#include "core/latency/latencyclock.h"
#include "core/latency/latencycollector.h"

#include "core/events/marketevent.h"
#include "core/events/signalevent.h"
#include "core/events/orderevent.h"
#include "core/events/fillevent.h"

class StrategyRunner final : public QObject {
    Q_OBJECT

    MarketDataService* mds_ = nullptr;
    std::unique_ptr<IStrategy> strategy_;

    RiskManager* risk_ = nullptr;
    DemoExecutionService* exec_ = nullptr;
    TradeJournal* journal_ = nullptr;
    RiskSettings riskSettings_;
    StrategyConfig cfg_;

    LatencyCollector latencyCollector_;

    StrategyContext ctx_;
    bool running_ = false;

public:
    explicit StrategyRunner(MarketDataService* mds, QObject* parent = nullptr);

    void setStrategy(std::unique_ptr<IStrategy> s);
    bool isRunning() const { return running_; }

    void setRiskManager(RiskManager* r) { risk_ = r; }
    void setExecutionService(DemoExecutionService* e) { exec_ = e; }
    void setTradeJournal(TradeJournal* j) { journal_ = j; }
    void setRiskSettings(const RiskSettings& r) { riskSettings_ = r; }
    void setConfig(const StrategyConfig& c) { cfg_ = c; }

    LatencyCollectorSnapshot latencySnapshot() const {
        return latencyCollector_.snapshot();
    }

public slots:
    void start(const QString& symbolId, Timeframe tf);
    void stop();

signals:
    void signal_started();
    void signal_stopped();
    void signal_signalGenerated(StrategySignal s);

private slots:
    void onSeriesLoaded(std::shared_ptr<CandleSeries> series);
    void onCandleClosed(Candle c);
    void onCandleUpdated(Candle c);

private:
    void handleSignal(SignalEvent& signalEvent, const Candle& closed);
    void handleMarketEvent(MarketEvent& event);
    MarketEvent makeMarketEvent(MarketEventType type, const Candle& candle) const;
    std::optional<OrderEvent> buildOrderEvent(const SignalEvent& signalEvent, const Candle& candle, LatencyTimestamp& latency);
    FillEvent executeOrderEvent(const OrderEvent& orderEvent, double markPrice);
    void handleFillEvent(FillEvent& fillEvent);

};
