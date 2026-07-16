#include "backtest/backtestengine.h"
#include "backtest/backtestreportexporter.h"
#include "core/appconfig.h"
#include "core/candle.h"
#include "core/timeframe.h"
#include "domain/order/order.h"
#include "domain/risk/riskconfig.h"
#include "domain/risk/riskmanager.h"
#include "domain/strategy/strategysignal.h"
#include "exchange/bingxswapclient.h"
#include "indicators/atr.h"
#include "indicators/donchian.h"
#include "indicators/ema.h"
#include "indicators/rsi.h"
#include "service/marketdata/candlecache.h"
#include "service/trade/tradejournal.h"
#include "core/events/marketevent.h"
#include "core/events/orderevent.h"
#include "core/events/tradeevent.h"
#include "core/latency/latencystats.h"
#include "core/latency/latencycollector.h"
#include "core/latency/latencyclock.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <QDateTime>
#include <QString>

QString formatLatencyStats(const LatencyStatsSnapshot& snap);

namespace {

    Candle candle(double close, double high = 0.0, double low = 0.0) {
        Candle c{};
        c.open_ = close;
        c.close_ = close;
        c.high_ = (high > 0.0) ? high : close;
        c.low_ = (low > 0.0) ? low : close;
        c.volume_ = 1.0;
        c.isFinal_ = true;
        return c;
    }

    void require(bool ok, const std::string& message) {
        if (!ok) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }
    }

    void requireNear(double actual, double expected, double eps, const std::string& message) {
        require(std::abs(actual - expected) <= eps,
                message + " (actual=" + std::to_string(actual) + ", expected=" + std::to_string(expected) + ")");
    }

    void testEma() {
        const std::vector<Candle> candles = {
            candle(1.0), candle(2.0), candle(3.0), candle(4.0), candle(5.0)
        };

        const auto ema = EMA::calculate(candles, 3);

        require(ema.size() == candles.size(), "EMA keeps output size aligned with candles");
        require(std::isnan(ema[0]) && std::isnan(ema[1]), "EMA leaves warmup values as NaN");
        requireNear(ema[2], 2.0, 1e-9, "EMA first value is SMA over period");
        requireNear(ema[3], 3.0, 1e-9, "EMA smooths next value");
        requireNear(ema[4], 4.0, 1e-9, "EMA continues smoothing");
    }

    void testRsi() {
        const std::vector<Candle> rising = {
            candle(1.0), candle(2.0), candle(3.0), candle(4.0), candle(5.0)
        };

        const auto rsi = RSI::calculate(rising, 3);

        require(rsi.size() == rising.size(), "RSI keeps output size aligned with candles");
        require(std::isnan(rsi[0]) && std::isnan(rsi[1]) && std::isnan(rsi[2]), "RSI leaves warmup values as NaN");
        requireNear(rsi[3], 100.0, 1e-9, "RSI is 100 when there are no losses");
        requireNear(rsi[4], 100.0, 1e-9, "RSI remains 100 on continued gains");
    }

    void testAtr() {
        const std::vector<Candle> candles = {
            candle(10.0, 11.0, 9.0),
            candle(12.0, 13.0, 10.0),
            candle(11.0, 14.0, 10.0),
            candle(15.0, 16.0, 12.0),
        };

        const auto atr = ATR::calculate(candles, 2);

        require(atr.size() == candles.size(), "ATR keeps output size aligned with candles");
        require(std::isnan(atr[0]) && std::isnan(atr[1]), "ATR leaves warmup values as NaN");
        requireNear(atr[2], 3.5, 1e-9, "ATR first value averages true ranges");
        requireNear(atr[3], 4.25, 1e-9, "ATR applies Wilder smoothing");
    }

    void testDonchian() {
        const std::vector<Candle> candles = {
            candle(10.0, 11.0, 9.0),
            candle(12.0, 13.0, 10.0),
            candle(11.0, 12.0, 8.0),
        };

        const auto d = Donchian::calculate(candles, 2);

        require(d.upper.size() == candles.size(), "Donchian keeps output size aligned with candles");
        require(std::isnan(d.upper[0]) && std::isnan(d.lower[0]) && std::isnan(d.middle[0]),
                "Donchian leaves warmup values as NaN");
        requireNear(d.upper[1], 13.0, 1e-9, "Donchian upper uses period high");
        requireNear(d.lower[1], 9.0, 1e-9, "Donchian lower uses period low");
        requireNear(d.middle[1], 11.0, 1e-9, "Donchian middle is channel midpoint");
        requireNear(d.upper[2], 13.0, 1e-9, "Donchian window advances upper");
        requireNear(d.lower[2], 8.0, 1e-9, "Donchian window advances lower");
    }

    void testRiskManager() {
        RiskSettings risk;
        risk.mode = RiskMode::FixedUsdt;
        risk.maxPosUsdt = 100.0;
        risk.leverage = 2;
        risk.allowLong = true;
        risk.allowShort = false;

        StrategySignal enterLong;
        enterLong.type = StrategySignalType::EnterLong;
        enterLong.symbolId = "ETHUSDT";

        const RiskManager rm;
        const Order longOrder = rm.buildOrder(enterLong, risk, 2000.0, 1000.0, false, TradeSide::Buy);

        require(longOrder.symbol == "ETHUSDT", "RiskManager copies symbol to entry order");
        require(longOrder.side == TradeSide::Buy, "RiskManager maps EnterLong to buy order");
        require(!longOrder.reduceOnly, "RiskManager entry is not reduce-only");
        requireNear(longOrder.qty, 0.1, 1e-9, "RiskManager applies fixed USDT and leverage to qty");

        StrategySignal enterShort;
        enterShort.type = StrategySignalType::EnterShort;
        enterShort.symbolId = "ETHUSDT";
        const Order blockedShort = rm.buildOrder(enterShort, risk, 2000.0, 1000.0, false, TradeSide::Buy);
        require(blockedShort.qty == 0.0, "RiskManager blocks short entries when allowShort is false");

        StrategySignal exitLong;
        exitLong.type = StrategySignalType::ExitLong;
        exitLong.symbolId = "ETHUSDT";
        const Order exitOrder = rm.buildOrder(exitLong, risk, 2000.0, 1000.0, true, TradeSide::Buy);

        require(exitOrder.reduceOnly, "RiskManager exit is reduce-only");
        require(exitOrder.side == TradeSide::Sell, "RiskManager closes long with sell order");
    }

    void testBacktestValidation() {
        BacktestEngine engine;
        BacktestRequest request;
        request.strategyName = "EMA Cross";
        request.symbol = "ETHUSDT";
        request.timeframe = Timeframe::M1;

        const BacktestResult noCandles = engine.run(request, {});
        require(noCandles.state == BacktestState::Failed, "Backtest fails without candles");

        request.strategyName = "None";
        const std::vector<Candle> candles = { candle(1.0), candle(2.0), candle(3.0), candle(4.0) };
        const BacktestResult noStrategy = engine.run(request, candles);
        require(noStrategy.state == BacktestState::Failed, "Backtest fails without selected strategy");
    }

    void testBacktestStatsDefaults() {
        BacktestStats stats;
        require(stats.grossPnl == 0.0, "BacktestStats gross PnL defaults to zero");
        require(stats.totalFees == 0.0, "BacktestStats total fees default to zero");
        require(stats.bestTrade == 0.0, "BacktestStats best trade defaults to zero");
        require(stats.worstTrade == 0.0, "BacktestStats worst trade defaults to zero");
        require(stats.avgBarsHeld == 0.0, "BacktestStats avg bars held defaults to zero");
        require(stats.maxDrawdown == 0.0, "BacktestStats max drawdown defaults to zero");
    }

    void testBacktestReportPaths() {
        BacktestRequest request;
        request.symbol = "ETH/USDT";
        request.timeframe = Timeframe::M1;
        request.strategyName = "EMA Cross";

        require(BacktestReportExporter::summaryPath().endsWith("latest_backtest_summary.json"),
                "BacktestReportExporter exposes summary path");
        require(BacktestReportExporter::tradesPath().endsWith("latest_backtest_trades.csv"),
                "BacktestReportExporter exposes trades path");
        require(BacktestReportExporter::snapshotBaseName(request).contains("ETH_USDT_M1_EMA_Cross"),
                "BacktestReportExporter creates sanitized snapshot names");
    }

    void testCandleCacheFreshness() {
        std::vector<Candle> candles;
        require(!CandleCache::isFresh(candles, Timeframe::M1), "CandleCache treats empty candles as stale");

        Candle fresh = candle(100.0);
        fresh.timestamp_ = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
        candles.push_back(fresh);
        require(CandleCache::isFresh(candles, Timeframe::M1), "CandleCache treats current candle as fresh");

        Candle stale = candle(100.0);
        stale.timestamp_ = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - timeframeToMs(Timeframe::M1) * 10;
        candles[0] = stale;
        require(!CandleCache::isFresh(candles, Timeframe::M1), "CandleCache treats old candle as stale");

        require(CandleCache::filePath("ETH/USDT", Timeframe::M1).contains("ETH_USDT_M1"),
                "CandleCache sanitizes symbol path parts");
    }

    void testRealtimeTransportConfig() {
        require(toConfigString(RealtimeTransport::Auto) == "auto",
                "RealtimeTransport serializes auto mode");
        require(toConfigString(RealtimeTransport::WebSocket) == "websocket",
                "RealtimeTransport serializes websocket mode");
        require(toConfigString(RealtimeTransport::Polling) == "polling",
                "RealtimeTransport serializes polling mode");

        require(realtimeTransportFromConfigString("ws") == RealtimeTransport::WebSocket,
                "RealtimeTransport accepts ws alias");
        require(realtimeTransportFromConfigString("HTTP") == RealtimeTransport::Polling,
                "RealtimeTransport accepts http alias");
        require(realtimeTransportFromConfigString("unknown") == RealtimeTransport::Auto,
                "RealtimeTransport falls back to auto");
    }

    void testBingXKlineStreamParser() {
        const QByteArray payload = R"({
            "code": 0,
            "dataType": "ETH-USDT@kline_1m",
            "s": "ETH-USDT",
            "data": [{
                "o": "2070.10",
                "h": "2074.50",
                "l": "2069.80",
                "c": "2072.25",
                "v": "123.456",
                "T": 1716469200000
            }]
        })";

        Candle parsed{};
        require(BingXSwapClient::parseKlineStreamPayload(payload, "ETH-USDT@kline_1m", parsed),
                "BingX websocket parser accepts matching kline payload");
        require(parsed.timestamp_ == 1716469200000,
                "BingX websocket parser reads timestamp");
        requireNear(parsed.open_, 2070.10, 1e-9,
                    "BingX websocket parser reads open");
        requireNear(parsed.high_, 2074.50, 1e-9,
                    "BingX websocket parser reads high");
        requireNear(parsed.low_, 2069.80, 1e-9,
                    "BingX websocket parser reads low");
        requireNear(parsed.close_, 2072.25, 1e-9,
                    "BingX websocket parser reads close");
        requireNear(parsed.volume_, 123.456, 1e-9,
                    "BingX websocket parser reads volume");
        require(!parsed.isFinal_,
                "BingX websocket parser treats stream candle as non-final");

        require(!BingXSwapClient::parseKlineStreamPayload(payload, "BTC-USDT@kline_1m", parsed),
                "BingX websocket parser rejects mismatched stream");
    }

    void testTradeJournalOwnsState() {
        TradeJournal journal(1000.0);

        int eventCount = 0;
        TradeEvent lastEvent;

        journal.setTradeEventCallback([&](const TradeEvent& event) {
            ++eventCount;
            lastEvent = event;
        });

        Fill open;
        open.time = QDateTime::fromMSecsSinceEpoch(1000);
        open.symbol = "ETHUSDT";
        open.side = TradeSide::Buy;
        open.qty = 0.5;
        open.price = 2000.0;
        open.fee = 1.0;
        open.reduceOnly = false;

        journal.onFill(open);

        require(eventCount == 1, "TradeJournal emits one event on open fill");
        require(lastEvent.type == TradeEventType::Added, "TradeJournal emits Added event on open fill");
        require(lastEvent.row == 0, "TradeJournal emits opened trade row");
        require(journal.hasOpen("ETHUSDT"), "TradeJournal tracks open position");
        require(journal.openSide("ETHUSDT") == TradeSide::Buy, "TradeJournal returns open side from owned state");
        requireNear(journal.openQty("ETHUSDT"), 0.5, 1e-9, "TradeJournal returns open qty from owned state");
        require(lastEvent.trade.symbol == "ETHUSDT", "TradeJournal event contains opened trade");

        journal.onPriceUpdate("ETHUSDT", 2010.0, 0.06);

        require(eventCount == 2, "TradeJournal emits second event on price update");
        require(lastEvent.type == TradeEventType::Updated, "TradeJournal emits Updated event on price update");
        require(lastEvent.row == 0, "TradeJournal emits updated trade row");
        require(lastEvent.trade.status == TradeStatus::Open, "TradeJournal keeps trade open after price update");

        Fill close;
        close.time = QDateTime::fromMSecsSinceEpoch(2000);
        close.symbol = "ETHUSDT";
        close.side = TradeSide::Sell;
        close.qty = 0.5;
        close.price = 2020.0;
        close.fee = 1.0;
        close.reduceOnly = true;

        journal.onFill(close);

        require(!journal.hasOpen("ETHUSDT"), "TradeJournal clears open position after close fill");
        require(eventCount == 3, "TradeJournal emits third event on close fill");
        require(lastEvent.type == TradeEventType::Closed, "TradeJournal emits Closed event on close fill");
        require(lastEvent.row == 0, "TradeJournal emits closed trade row");
        require(lastEvent.trade.status == TradeStatus::Closed, "TradeJournal closes trade in owned state");

        const TradeReport report = journal.report();
        require(report.closedTrades == 1, "TradeJournal reports one closed trade");
        require(report.winTrades == 1, "TradeJournal reports winning trade");
    }

    void testCoreEventsDefaults() {
        MarketEvent market;
        market.symbolId = "ETHUSDT";
        market.timeframe = Timeframe::M1;
        market.candle.close_ = 2000.0;
        market.latency.receivedNs = 10;

        require(market.type == MarketEventType::CandleUpdated, "MarketEvent defaults to candle updated");
        require(market.symbolId == "ETHUSDT", "MarketEvent stores symbol");
        requireNear(market.candle.close_, 2000.0, 1e-9, "MarketEvent stores candle");
        require(market.latency.receivedNs == 10, "MarketEvent stores latency timestamps");

        OrderEvent order;
        order.type = OrderEventType::RiskRejected;
        order.reason = "qty <= 0";
        require(order.type == OrderEventType::RiskRejected, "OrderEvent stores type");
        require(order.reason == "qty <= 0", "OrderEvent stores rejection reason");

        TradeEvent trade;
        trade.type = TradeEventType::Added;
        trade.row = 3;
        require(trade.row == 3, "TradeEvent stores row");
    }

    void testLatencyStatsSnapshot() {
        LatencyStats stats;
        stats.addSample(10);
        stats.addSample(20);
        stats.addSample(30);

        const LatencyStatsSnapshot s = stats.snapshot();
        require(s.count == 3, "LatencyStats stores sample count");
        require(s.minNs == 10, "LatencyStats stores min");
        require(s.p50Ns == 20, "LatencyStats computes p50");
        require(s.maxNs == 30, "LatencyStats stores max");
        require(s.meanNs == 20, "LatencyStats stores mean");
    }

    void testEmptyLatencyStatsSnapshot() {
        LatencyStats stats;

        const LatencyStatsSnapshot s = stats.snapshot();
        require(s.count == 0, "LatencyStats stores sample count");
        require(s.minNs == 0, "LatencyStats stores min");
        require(s.p50Ns == 0, "LatencyStats computes p50");
        require(s.maxNs == 0, "LatencyStats stores max");
        require(s.meanNs == 0, "LatencyStats stores mean");
    }

    void testLatencyCollector() {
        LatencyTimestamp ts;
        ts.receivedNs = 100;
        ts.strategyDoneNs = 160;
        ts.orderCreatedNs = 210;
        ts.fillHandledNs = 300;

        LatencyCollector collector;
        collector.recordTickToStrategy(ts);
        collector.recordStrategyToOrder(ts);
        collector.recordOrderToFill(ts);

        const LatencyCollectorSnapshot s = collector.snapshot();
        require(s.tickToStrategy.p50Ns == 60, "LatencyCollector records tick-to-strategy");
        require(s.strategyToOrder.p50Ns == 50, "LatencyCollector records strategy-to-order");
        require(s.orderToFill.p50Ns == 90, "LatencyCollector records order-to-fill");
    }

    void testLatencyCollectorReverseTimestamp() {
        LatencyTimestamp ts;
        ts.receivedNs = 100;
        ts.strategyDoneNs = 90;
        ts.orderCreatedNs = 300;
        ts.fillHandledNs = 210;

        LatencyCollector collector;
        collector.recordTickToStrategy(ts);
        collector.recordStrategyToOrder(ts);
        collector.recordOrderToFill(ts);

        const LatencyCollectorSnapshot s = collector.snapshot();
        require(s.tickToStrategy.p50Ns == 0, "LatencyCollector records tick-to-strategy");
        require(s.strategyToOrder.p50Ns == 210, "LatencyCollector records strategy-to-order");
        require(s.orderToFill.p50Ns == 0, "LatencyCollector records order-to-fill");
    }

    void testLatencyCollectorZeroTimestamp() {
        LatencyTimestamp ts;
        ts.receivedNs = 0;
        ts.strategyDoneNs = 160;
        ts.orderCreatedNs = 0;
        ts.fillHandledNs = 300;

        LatencyCollector collector;
        collector.recordTickToStrategy(ts);
        collector.recordStrategyToOrder(ts);
        collector.recordOrderToFill(ts);

        const LatencyCollectorSnapshot s = collector.snapshot();
        require(s.tickToStrategy.count == 0, "LatencyCollector ignores zero received timestamp");
        require(s.strategyToOrder.count == 0, "LatencyCollector ignores zero order timestamp");
        require(s.orderToFill.count == 0, "LatencyCollector ignores zero order timestamp for fill latency");
    }

    void testLatencyClock() {
        const std::uint64_t first = LatencyClock::nowNs();
        const std::uint64_t second = LatencyClock::nowNs();
        require(first > 0, "LatencyClock returns non-zero timestamp");
        require(second >= first, "LatencyClock is monotonic for consecutive reads");
    }

    void testFormatter() {
        LatencyStatsSnapshot snap;
        snap.count = 2;
        snap.minNs = 1'000;
        snap.p50Ns = 2'000;
        snap.p95Ns = 3'000;
        snap.p99Ns = 4'000;
        snap.maxNs = 5'000;
        snap.meanNs = 2'500;

        const QString formatted = formatLatencyStats(snap);
        require(formatted.contains("count=2"), "Latency formatter prints sample count");
        require(formatted.contains("min=1us"), "Latency formatter converts min ns to us");
        require(formatted.contains("mean=2.5us"), "Latency formatter converts mean ns to us");
        require(formatted.contains("max=5us"), "Latency formatter converts max ns to us");
        require(formatted.contains("p50=2us"), "Latency formatter converts p50 ns to us");
        require(formatted.contains("p95=3us"), "Latency formatter converts p95 ns to us");
        require(formatted.contains("p99=4us"), "Latency formatter converts p99 ns to us");

        LatencyStatsSnapshot empty;
        require(formatLatencyStats(empty) == "count=0", "Latency formatter keeps empty snapshot compact");
    }

    void testLatencyStatsCapacity() {
        LatencyStats stats{2};
        stats.addSample(10);
        stats.addSample(20);
        stats.addSample(30);
        require(stats.snapshot().maxNs == 20, "LatencyStats max is 20, not 30");
        require(stats.snapshot().count == 2, "LatencyStats count is 2, like a capacity");
        stats.clear();
        require(stats.snapshot().count == 0, "LatencyStats is empty");
        stats.addSample(20);
        stats.addSample(40);
        stats.addSample(50);
        require(stats.snapshot().maxNs == 40, "LatencyStats max is 40, not 50");
        require(stats.snapshot().count == 2, "LatencyStats count is 2, like a capacity");
        require(stats.snapshot().minNs == 20, "LatencyStats min is 20");
    }
}

int main() {
    testEma();
    testRsi();
    testAtr();
    testDonchian();
    testRiskManager();
    testBacktestValidation();
    testBacktestStatsDefaults();
    testBacktestReportPaths();
    testCandleCacheFreshness();
    testRealtimeTransportConfig();
    testBingXKlineStreamParser();
    testTradeJournalOwnsState();
    testCoreEventsDefaults();
    testLatencyStatsSnapshot();
    testEmptyLatencyStatsSnapshot();
    testLatencyCollector();
    testLatencyCollectorReverseTimestamp();
    testLatencyCollectorZeroTimestamp();
    testLatencyClock();
    testFormatter();
    testLatencyStatsCapacity();

    std::cout << "All TradeSoft tests passed\n";
    return 0;
}
