#include "tradesmodel.h"
#include <QLocale>
#include <QBrush>

TradesModel::TradesModel(QObject *parent) : QAbstractTableModel(parent) {}

int TradesModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(trades_.size());
}

int TradesModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

static QString sideToString(TradeSide s) {
    return (s == TradeSide::Buy) ? "Buy" : "Sell";
}

static QString statusToString(TradeStatus st) {
    switch (st) {
    case TradeStatus::Open: return "Open";
    case TradeStatus::Closed: return "Closed";
    case TradeStatus::PartiallyFilled: return "PartiallyFilled";
    case TradeStatus::Rejected: return "Rejected";
    }
    return {};
}

QVariant TradesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};
    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= (int)trades_.size()) return {};

    const TradeRecord& t = trades_[(std::size_t)row];

    // alignment for numeric columns
    if (role == Qt::TextAlignmentRole) {
        switch (col) {
        case ColQty:
        case ColOpenPrice:
        case ColClosePrice:
        case ColPnL:
        case ColFee:
        case ColTicks:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    // pnl coloring
    if (role == Qt::ForegroundRole) {
        if (col == ColPnL) {
            if (t.pnl > 0.0) return QBrush(QColor(0, 180, 0));
            if (t.pnl < 0.0) return QBrush(QColor(200, 40, 40));
        }
        return {};
    }

    if (role != Qt::DisplayRole) return {};

    const QLocale loc;
    auto fmt6 = [&](double v) { return loc.toString(v, 'f', 6); };
    auto fmt2 = [&](double v) { return loc.toString(v, 'f', 2); };

    switch (col) {
    case ColTime:      return t.time.toString("yyyy-MM-dd HH:mm:ss");
    case ColSymbol:    return t.symbol;
    case ColSide:      return sideToString(t.side);
    case ColQty:       return fmt6(t.qty);
    case ColOpenPrice: return fmt2(t.price);

    case ColClosePrice:
        return (t.status == TradeStatus::Closed) ? fmt2(t.closePrice) : QString();

    case ColPnL:
        // пока Open можно показывать unrealized pnl
        return fmt2(t.pnl);

    case ColFee:       return fmt2(t.fee);
    case ColTicks:     return t.lifetimeTicks;
    case ColStatus:    return statusToString(t.status);
    default:           return {};
    }
}

QVariant TradesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return {};
    if (orientation != Qt::Horizontal) return section + 1;

    switch (section) {
    case ColTime:      return "Time";
    case ColSymbol:    return "Symbol";
    case ColSide:      return "Side";
    case ColQty:       return "Qty";
    case ColOpenPrice: return "Open";
    case ColClosePrice:return "Close";
    case ColPnL:       return "PnL";
    case ColFee:       return "Fee";
    case ColTicks:     return "Ticks";
    case ColStatus:    return "Status";
    default:           return {};
    }
}

void TradesModel::clear() {
    beginResetModel();
    trades_.clear();
    endResetModel();
}

TradeRecord TradesModel::tradeAt(int row) const {
    if (row < 0 || row >= (int)trades_.size()) return TradeRecord{};
    return trades_[row];
}

void TradesModel::appendTrade(const TradeRecord& t) {
    const int row = (int)trades_.size();
    beginInsertRows(QModelIndex(), row, row);
    trades_.push_back(t);
    endInsertRows();
}

void TradesModel::updateTrade(int row, const TradeRecord& t) {
    if (row < 0 || row >= (int)trades_.size()) return;
    trades_[row] = t;

    const QModelIndex left  = index(row, 0);
    const QModelIndex right = index(row, columnCount() - 1);
    emit dataChanged(left, right);
}

std::vector<int> TradesModel::openTradeRows() const {
    std::vector<int> rows;
    rows.reserve(trades_.size());
    for (int i = 0; i < static_cast<int>(trades_.size()); ++i) {
        if (trades_[i].status == TradeStatus::Open) {
            rows.push_back(i);
        }
    }
    return rows;
}
