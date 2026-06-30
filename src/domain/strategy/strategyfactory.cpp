#include "strategyfactory.h"

#include <QtGlobal>
#include <QDebug>
#include <QLoggingCategory>

#include "domain/strategy/emacrossstrategy.h"
#include "domain/strategy/emapullbackstrategy.h"
#include "domain/strategy/emascalpstrategy.h"

Q_LOGGING_CATEGORY(logStrategyFactory, "tradesoft.strategy.factory")

std::unique_ptr<IStrategy> StrategyFactory::create(const StrategyConfig& cfg) {
    const QString name = cfg.strategy.name.trimmed();

    // 1) EMA Cross
    if (name.compare("EMA Cross", Qt::CaseInsensitive) == 0 ||
        name.compare("Ema Cross", Qt::CaseInsensitive) == 0)
    {
        const int fast = 9;
        const int slow = 21;

        return std::unique_ptr<IStrategy>(
            new EmaCrossStrategy(fast, slow, cfg.risk.allowLong, cfg.risk.allowShort)
        );
    }

    // 2) EMA Pullback (Cross + Pullback + Adaptive risk)
    if (name.compare("EMA Pullback", Qt::CaseInsensitive) == 0 ||
        name.compare("Ema Pullback", Qt::CaseInsensitive) == 0)
    {
        // Стартовые параметры под M1 (BTCUSDT), дальше вынесем в UI
        const int fast = 5;
        const int slow = 13;

        const int maxBarsInTrade = 5;
        const int minSpreadBps = 3;

        const int maxPullbackBars = 3;     // сколько свечей после кросса ждём откат к mid
        const double slFromRangeMul = 0.5; // SL = 50% диапазона триггер-свечи
        const double rr = 1.5;             // TP = SL * RR

        const int minSlBps = 6;
        const int maxSlBps = 25;
        const int minTpBps = 6;
        const int maxTpBps = 40;

        const bool flip = true;

        return std::unique_ptr<IStrategy>(
            new EmaPullbackStrategy(
                fast, slow,
                maxBarsInTrade,
                minSpreadBps,
                maxPullbackBars,
                slFromRangeMul,
                rr,
                minSlBps, maxSlBps,
                minTpBps, maxTpBps,
                flip,
                cfg.risk.allowLong,
                cfg.risk.allowShort
            )
        );
    }

    // 3) EMA Scalp
    if (name.compare("EMA Scalp", Qt::CaseInsensitive) == 0 ||
        name.compare("Ema Scalp", Qt::CaseInsensitive) == 0)
    {
        const int fast = 5;
        const int slow = 13;

        const bool flip = true;

        const int tpBps = 10;
        const int slBps = 8;
        const int maxBars = 6;
        const int minSpreadBps = 3;

        return std::unique_ptr<IStrategy>(
            new EmaScalpStrategy(fast, slow, tpBps, slBps, maxBars, minSpreadBps, flip, cfg.risk.allowLong, cfg.risk.allowShort)
        );
    }

    // fallback: если в UI что-то другое выбрали — не падаем
    qCWarning(logStrategyFactory) << "Unknown strategy name:" << name << " -> fallback to EMA Cross";

    return std::unique_ptr<IStrategy>(
        new EmaCrossStrategy(9, 21, cfg.risk.allowLong, cfg.risk.allowShort)
    );
}
