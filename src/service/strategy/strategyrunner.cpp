#include "strategyrunner.h"
#include <QObject>
#include <utility>
#include <QDebug>

StrategyRunner::StrategyRunner(MarketDataService* mds, QObject* parent) : QObject(parent), mds_(mds) {
    Q_ASSERT(mds_);
    connect(mds_, &MarketDataService::signal_seriesLoaded, this, &StrategyRunner::onSeriesLoaded);
    connect(mds_, &MarketDataService::signal_candleClosed, this, &StrategyRunner::onCandleClosed);
    connect(mds_, &MarketDataService::signal_candleUpdated, this, &StrategyRunner::onCandleUpdated);
}

void StrategyRunner::setStrategy(std::unique_ptr<IStrategy> s) {
    strategy_ = std::move(s);
}

void StrategyRunner::start(const QString& symbolId, Timeframe tf) {
    if (running_) return;
    if (!strategy_) {
        qWarning() << "[RUNNER] Can't start : strategy == nullptr";
        return;
    }

    ctx_.symbolId = symbolId;
    ctx_.tf = tf;
    ctx_.series.reset();

    running_ = true;

    mds_->stopRealTime();
    mds_->loadHistory(symbolId, tf);
    mds_->startRealTime();

    emit signal_started();
    qDebug() << "[RUNNER] START symbol = " << symbolId << ", tf = " << toUiString(tf);
}

void StrategyRunner::stop() {
    if (!running_) return;
    running_ = false;

    mds_->stopRealTime();
    emit signal_stopped();
    qDebug() << "[RUNNER] STOP";
}

void StrategyRunner::onSeriesLoaded(std::shared_ptr<CandleSeries> series) {
    if (!running_ || !strategy_) return;
    ctx_.series = series;

    if (strategy_) {
        strategy_->onStart(ctx_);
        qDebug() << "[RUNNER] series loaded, candles = " << series->getCount();
    }
}

void StrategyRunner::onCandleClosed(Candle c)
{
    if (!running_ || !strategy_ || !ctx_.series) return;

    const auto signal = strategy_->onCandleClosed(ctx_, c);
    for (const auto& s : signal) {
        if (s.type == StrategySignalType::None) continue;
        emit signal_signalGenerated(s);
        qDebug() << "[Runner] SIGNAL type = " << (int)s.type
                 << "symbol = " << s.symbolId
                 << "tf = " << toUiString(s.tf)
                 << "reason = " << s.reason;
    }
}

void StrategyRunner::onCandleUpdated(Candle c)
{
    if (!running_ || !strategy_ || !ctx_.series) return;
    strategy_->onCandleUpdated(ctx_, c);
}
