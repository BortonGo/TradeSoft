#include "backtestengine.h"
#include "domain\strategy\strategyfactory.h"
#include <QDebug>
#include <cmath>

static StrategyConfig buildStrategyConfig(const BacktestRequest& request) {
    StrategyConfig cfg;
    cfg.strategy.name = request.strategyName;
    cfg.strategy.tf = request.timeframe;
    cfg.risk = request.backtestRisk;
    return cfg;
}

static double calcBacktestQty(const RiskSettings& risk, double markPrice, double equityUsdt) {
    if (markPrice <= 0.0) {
        return 0.0;
    }

    double notionalUsdt = 0.0;

    if (risk.mode == RiskMode::FixedUsdt) {
        notionalUsdt = risk.maxPosUsdt;
    } else {
        notionalUsdt = equityUsdt * (risk.riskPct / 100.0);
    }

    if (notionalUsdt <= 0.0) {
        return 0.0;
    }

    notionalUsdt *= std::max(1, risk.leverage);

    const double qty = notionalUsdt / markPrice;
    return (qty > 0.0) ? qty : 0.0;
}

static double calcTradeFee(double price, double feePct, double qty) {
    if (price <= 0.0 || feePct <= 0.0 || qty <= 0.0) {
        return 0.0;
    }
    return price * qty * (feePct / 100.0);
}

BacktestResult BacktestEngine::run(const BacktestRequest& request, const std::vector<Candle>& candles) const {
    BacktestResult res;
    if (candles.empty()) {
        res.state = BacktestState::Failed;
        res.errorText = "No candles for backtest";
        return res;
    }
    if (request.strategyName.trimmed().isEmpty() || request.strategyName == "None") {
        res.state = BacktestState::Failed;
        res.errorText = "Strategy is not selected";
        return res;
    }
    if (request.executionMode == BacktestExecutionMode::IntrabarLowerTf) {
        res.state = BacktestState::Failed;
        res.errorText = "Intrabar backtest is not implemented yet";
        return res;
    }

    const StrategyConfig cfg = buildStrategyConfig(request);
    double peakEquity = request.initialbalance;
    auto strategy = StrategyFactory::create(cfg);
    if (!strategy) {
        res.state = BacktestState::Failed;
        res.errorText = "Strategy is not supported";
        return res;
    }

    const int warmup = request.warmupBars;
    if (static_cast<int>(candles.size()) <= warmup) {
        res.state = BacktestState::Failed;
        res.errorText = "Not enough candles for warmup";
        return res;
    }

    std::shared_ptr<CandleSeries> series = std::make_shared<CandleSeries>(request.symbol, request.timeframe);

    for (int i = 0; i < warmup; ++i) {
        series->addCandle(candles[i]);
    }

    StrategyContext ctx;
    ctx.series = series;
    ctx.symbolId = request.symbol;
    ctx.tf = request.timeframe;

    strategy->onStart(ctx);

    BacktestTrade btTrade;
    bool inLong = false;
    bool inShort = false;
    double currentEquity = request.initialbalance;

    for (int i = warmup; i < static_cast<int>(candles.size()); ++i) {
        series->addCandle(candles[i]);

        const auto signalsCandleClosed = strategy->onCandleClosed(ctx, candles[i]);

        for (const auto& s : signalsCandleClosed) {
            if (s.type == StrategySignalType::EnterLong && !inLong && !inShort) {
                btTrade = BacktestTrade{};
                btTrade.entryPrice = candles[i].close_;
                btTrade.entryTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.side = BacktestTradeSide::Long;
                btTrade.quantity = calcBacktestQty(request.backtestRisk, btTrade.entryPrice, currentEquity);
                inLong = true;
            }
            if (s.type == StrategySignalType::EnterShort && !inLong && !inShort) {
                btTrade = BacktestTrade{};
                btTrade.entryPrice = candles[i].close_;
                btTrade.entryTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.side = BacktestTradeSide::Short;
                btTrade.quantity = calcBacktestQty(request.backtestRisk, btTrade.entryPrice, currentEquity);
                inShort = true;
            }
            if (s.type == StrategySignalType::ExitLong && inLong && !inShort) {
                btTrade.exitPrice = candles[i].close_;
                btTrade.exitTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.grossPnl = (btTrade.exitPrice - btTrade.entryPrice) * btTrade.quantity;
                const double entryFee = calcTradeFee(btTrade.entryPrice, request.backtestRisk.feePct, btTrade.quantity);
                const double exitFee = calcTradeFee(btTrade.exitPrice, request.backtestRisk.feePct, btTrade.quantity);
                btTrade.feePaid = entryFee + exitFee;
                btTrade.netPnl = btTrade.grossPnl - btTrade.feePaid;
                currentEquity += btTrade.netPnl;
                btTrade.winner = btTrade.netPnl > 0.0;
                res.trades.push_back(std::move(btTrade));
                btTrade = BacktestTrade {};
                inLong = false;
                inShort = false;

            }
            if (s.type == StrategySignalType::ExitShort && !inLong && inShort) {
                btTrade.exitPrice = candles[i].close_;
                btTrade.exitTime = QDateTime::fromMSecsSinceEpoch(candles[i].timestamp_);
                btTrade.grossPnl = (btTrade.entryPrice - btTrade.exitPrice) * btTrade.quantity;
                const double entryFee = calcTradeFee(btTrade.entryPrice,request.backtestRisk.feePct, btTrade.quantity);
                const double exitFee = calcTradeFee(btTrade.exitPrice, request.backtestRisk.feePct, btTrade.quantity);

                btTrade.feePaid = entryFee + exitFee;
                btTrade.netPnl = btTrade.grossPnl - btTrade.feePaid;
                currentEquity += btTrade.netPnl;
                btTrade.winner = btTrade.netPnl > 0.0;
                res.trades.push_back(std::move(btTrade));
                btTrade = BacktestTrade {};
                inLong = false;
                inShort = false;
            }
        }
    }
    res.stats.trades =  static_cast<int>(res.trades.size());

    //stats
    int winCount = 0;
    int lossCount = 0;
    double winSum = 0;
    double lossSum = 0;
    for (const auto& t : res.trades) {
        if (t.winner) {
            ++winCount;
            winSum += t.netPnl;
        } else {
            ++lossCount;
            lossSum += t.netPnl;
        }
    }
    if (res.stats.trades > 0) {
        res.stats.winratePct = 100.0 * static_cast<double>(winCount) / res.stats.trades;
        res.stats.avgWin = (winCount > 0) ? winSum / winCount : 0.0;
        res.stats.avgLoss = (lossCount > 0) ? lossSum / lossCount : 0.0;
        if (lossSum < 0.0) {
            res.stats.profitFactor = winSum / std::abs(lossSum);
        }
        res.stats.expectancy = (winSum + lossSum) / static_cast<double>(res.stats.trades);
    }

    qDebug() << "[BACKTEST ENGINE]  Total trades = " << res.stats.trades <<
                ", strategy = " << cfg.strategy.name <<
                ", Wins = " << winCount <<
                ", Losses = " << lossCount <<
                ", WinSum = " << winSum <<
                ", LossSum = " << lossSum <<
                ", Winrate = " << res.stats.winratePct;

    double equityCurveEquity = request.initialbalance;
    for (const auto& a : res.trades) {
        equityCurveEquity += a.netPnl;
        EquityPoint p;
        p.time = a.exitTime;
        p.equity = equityCurveEquity;
        peakEquity = std::max(peakEquity, p.equity);
        p.drawdown = peakEquity - p.equity;
        p.cumulativePnl = equityCurveEquity - request.initialbalance;
        res.equityCurve.push_back(p);
        double ddPct = 0.0;
        if (peakEquity > 0.0) {
            ddPct = 100.0 * (peakEquity - p.equity) / peakEquity;
        }
        res.stats.MaxDDPct = std::max(res.stats.MaxDDPct, ddPct);
    }

    res.stats.netPnl = currentEquity - request.initialbalance;

    res.state = BacktestState::Completed;

    return res;
}

