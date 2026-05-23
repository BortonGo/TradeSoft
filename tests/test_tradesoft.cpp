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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <QDateTime>

namespace {

Candle candle(double close, double high = 0.0, double low = 0.0)
{
    Candle c{};
    c.open_ = close;
    c.close_ = close;
    c.high_ = (high > 0.0) ? high : close;
    c.low_ = (low > 0.0) ? low : close;
    c.volume_ = 1.0;
    c.isFinal_ = true;
    return c;
}

void require(bool ok, const std::string& message)
{
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void requireNear(double actual, double expected, double eps, const std::string& message)
{
    require(std::abs(actual - expected) <= eps,
            message + " (actual=" + std::to_string(actual) + ", expected=" + std::to_string(expected) + ")");
}

void testEma()
{
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

void testRsi()
{
    const std::vector<Candle> rising = {
        candle(1.0), candle(2.0), candle(3.0), candle(4.0), candle(5.0)
    };

    const auto rsi = RSI::calculate(rising, 3);

    require(rsi.size() == rising.size(), "RSI keeps output size aligned with candles");
    require(std::isnan(rsi[0]) && std::isnan(rsi[1]) && std::isnan(rsi[2]), "RSI leaves warmup values as NaN");
    requireNear(rsi[3], 100.0, 1e-9, "RSI is 100 when there are no losses");
    requireNear(rsi[4], 100.0, 1e-9, "RSI remains 100 on continued gains");
}

void testAtr()
{
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

void testDonchian()
{
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

void testRiskManager()
{
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

void testBacktestValidation()
{
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

void testBacktestStatsDefaults()
{
    BacktestStats stats;
    require(stats.grossPnl == 0.0, "BacktestStats gross PnL defaults to zero");
    require(stats.totalFees == 0.0, "BacktestStats total fees default to zero");
    require(stats.bestTrade == 0.0, "BacktestStats best trade defaults to zero");
    require(stats.worstTrade == 0.0, "BacktestStats worst trade defaults to zero");
    require(stats.avgBarsHeld == 0.0, "BacktestStats avg bars held defaults to zero");
    require(stats.maxDrawdown == 0.0, "BacktestStats max drawdown defaults to zero");
}

void testBacktestReportPaths()
{
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

void testCandleCacheFreshness()
{
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

void testRealtimeTransportConfig()
{
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

void testBingXKlineStreamParser()
{
    const QByteArray payload = R"({
        "dataType": "ETH-USDT@kline_1m",
        "data": {
            "e": "kline",
            "E": 1716469200123,
            "s": "ETH-USDT",
            "K": {
                "t": 1716469200000,
                "o": "2070.10",
                "h": "2074.50",
                "l": "2069.80",
                "c": "2072.25",
                "v": "123.456"
            }
        }
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

} // namespace

int main()
{
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

    std::cout << "All TradeSoft tests passed\n";
    return 0;
}
