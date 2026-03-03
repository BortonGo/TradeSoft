#include "riskmanager.h"
#include <QtGlobal>
#include <cmath>

static double calcQty(const RiskSettings& risk, double markPrice, double equityUsdt) {
    if (markPrice <= 0.0) return 0.0;

    double notionalUsdt = 0.0;

    if (risk.mode == RiskMode::FixedUSDT) {
        notionalUsdt = risk.maxPosUsdt;
    } else {
        // PercentEquity
        notionalUsdt = equityUsdt * (risk.riskPct / 100.0);
    }

    if (notionalUsdt <= 0.0) return 0.0;

    // leverage
    notionalUsdt *= std::max(1, risk.leverage);

    const double qty = notionalUsdt / markPrice;
    return (qty > 0.0) ? qty : 0.0;
}

Order RiskManager::buildOrder(const StrategySignal& s, const RiskSettings& risk, double markPrice,
                              double equityUsdt, bool hasOpenPos, TradeSide openSide) const {
    Order o;
    o.symbol = s.symbolId;
    o.type = OrderType::Market;

    auto isEnter = [&](StrategySignalType t) {
        return t == StrategySignalType::EnterLong || t == StrategySignalType::EnterShort;
    };
    auto isExit = [&](StrategySignalType t) {
        return t == StrategySignalType::ExitLong || t == StrategySignalType::ExitShort;
    };

    if (isEnter(s.type)) {
        // фильтры allowLong/allowShort
        if (s.type == StrategySignalType::EnterLong && !risk.allowLong) return Order{};
        if (s.type == StrategySignalType::EnterShort && !risk.allowShort) return Order{};

        // если уже есть позиция — на итерации 2 мы НЕ открываем вторую.
        // reverse делается сигналами exit + enter, поэтому сюда попадём уже после exit.
        if (hasOpenPos) {
            return Order{};
        }

        o.reduceOnly = false;
        o.side = (s.type == StrategySignalType::EnterLong) ? TradeSide::Buy : TradeSide::Sell;
        o.qty = calcQty(risk, markPrice, equityUsdt);
        return o;
    }

    if (isExit(s.type)) {
        if (!hasOpenPos) return Order{};

        // закрываем только если сторона совпадает с позицией
        // ExitLong закрывает long, ExitShort закрывает short
        if (s.type == StrategySignalType::ExitLong && openSide != TradeSide::Buy) return Order{};
        if (s.type == StrategySignalType::ExitShort && openSide != TradeSide::Sell) return Order{};

        o.reduceOnly = true;
        // чтобы закрыть long — продаём, чтобы закрыть short — покупаем
        o.side = (openSide == TradeSide::Buy) ? TradeSide::Sell : TradeSide::Buy;

        // qty закрытия = вся позиция.
        // qty=0 и заполним его в Runner перед execution.
        o.qty = 0.0;
        return o;
    }

    return Order{};
}
