#include "backtesttradesmodel.h"
#include <QLocale>
#include <QBrush>

BacktestTradesModel::BacktestTradesModel(QObject *parent) : QAbstractTableModel(parent) {}

int BacktestTradesModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(trades_.size());
}

int BacktestTradesModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

static QString sideToString(BacktestTradeSide s) {
    return (s == BacktestTradeSide::Long) ? "Long" : "Short";
}

QVariant BacktestTradesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};
    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= (int)trades_.size()) return {};

    const BacktestTrade& t = trades_[(std::size_t)row];

    // alignment for numeric columns
    if (role == Qt::TextAlignmentRole) {
        switch (col) {
        case ColGrossPnl:
        case ColNetPnl:
        case ColFee:
        case ColBarsHeld:
        case ColWinner:
                return int(Qt::AlignCenter);
        case ColQty:
        case ColEntryPrice:
        case ColExitPrice:
        case ColSide:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    // pnl coloring
    if (role == Qt::ForegroundRole) {
        if (col == ColGrossPnl || col == ColNetPnl) {
            const double pnl = (col == ColGrossPnl) ? t.grossPnl : t.netPnl;
            if (pnl > 0.0) return QBrush(QColor(0, 180, 0));
            if (pnl < 0.0) return QBrush(QColor(200, 40, 40));
        }
        return {};
    }

    if (role != Qt::DisplayRole) return {};

    const QLocale loc;
    auto fmt6 = [&](double v) { return loc.toString(v, 'f', 6); };
    auto fmt2 = [&](double v) { return loc.toString(v, 'f', 2); };

    switch (col) {
    case ColEntryTime:      return t.entryTime.toString("yyyy-MM-dd HH:mm:ss");
    case ColExitTime:       return t.exitTime.toString("yyyy-MM-dd HH:mm:ss");
    case ColSide:           return sideToString(t.side);
    case ColQty:            return fmt6(t.quantity);
    case ColEntryPrice:     return fmt2(t.entryPrice);
    case ColExitPrice:      return fmt2(t.exitPrice);
    case ColGrossPnl:       return fmt2(t.grossPnl);
    case ColNetPnl:         return fmt2(t.netPnl);
    case ColFee:            return fmt2(t.feePaid);
    case ColBarsHeld:       return t.barsHeld;
    case ColWinner:         return (t.winner) ? "Win" : "Loss";
    default:                return {};
    }
}

QVariant BacktestTradesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return {};
    if (orientation != Qt::Horizontal) return section + 1;

    switch (section) {
    case ColEntryTime:      return "Entry Time";
    case ColExitTime:       return "Exit Time";
    case ColSide:           return "Side";
    case ColQty:            return "Qty";
    case ColEntryPrice:     return "Entry";
    case ColExitPrice:      return "Exit";
    case ColGrossPnl:       return "Gross PnL";
    case ColNetPnl:         return "Net PnL";
    case ColFee:            return "Fee";
    case ColBarsHeld:       return "Bars";
    case ColWinner:         return "Winner";
    default:                return {};
    }
}

void BacktestTradesModel::clear() {
    beginResetModel();
    trades_.clear();
    endResetModel();
}

void BacktestTradesModel::setTrades(const std::vector<BacktestTrade>& trades) {
    beginResetModel();
    trades_ = trades;
    endResetModel();
}

BacktestTrade BacktestTradesModel::tradeAt(int row) const {
    if (row < 0 || row >= (int)trades_.size()) return BacktestTrade{};
    return trades_[row];
}




