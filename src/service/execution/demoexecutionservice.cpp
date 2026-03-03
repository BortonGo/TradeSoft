#include "demoexecutionservice.h"
#include <QtGlobal>
#include <cmath>

static double applySlippage(double price, TradeSide side, int slippageBps)
{
    if (price <= 0.0) return price;
    const double bps = static_cast<double>(slippageBps) / 10000.0;
    const double slip = price * bps;

    // Buy хуже: дороже. Sell хуже: дешевле.
    return (side == TradeSide::Buy) ? (price + slip) : (price - slip);
}

Fill DemoExecutionService::executeMarket(const Order& o, double markPrice, const RiskSettings& risk) const
{
    Fill f;
    f.time = QDateTime::currentDateTime();
    f.symbol = o.symbol;
    f.side = o.side;
    f.qty = o.qty;
    f.reduceOnly = o.reduceOnly;

    const double fillPrice = applySlippage(markPrice, o.side, risk.slippageBps);
    f.price = fillPrice;

    const double notional = std::abs(fillPrice * o.qty);
    f.fee = notional * (risk.feePct / 100.0);
    return f;
}
