#pragma once
#include <memory>
#include <QObject>

#include "service/marketdataservice.h"
#include "domain/strategy/istrategy.h"
#include "domain/strategy/strategysignal.h"

class StrategyRunner final : public QObject {
    Q_OBJECT

    MarketDataService* mds_ = nullptr;
    std::unique_ptr<IStrategy> strategy_;

    StrategyContext ctx_;
    bool running_ = false;

public:
    explicit StrategyRunner(MarketDataService* mds, QObject* parent = nullptr);

    void setStrategy(std::unique_ptr<IStrategy> s);
    bool isRunning() const { return running_; }

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
};
