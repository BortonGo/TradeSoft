#include "tradesmodel.h"

TradesModel::TradesModel(QObject *parent) : QAbstractTableModel(parent) {}

int TradesModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(trades_.size());
}

int TradesModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return 7; // Time, Symbol, Side, Qty, Price, Fee, Status
}

QVariant TradesModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }
    if (role != Qt::DisplayRole) {
        return {};
    }

    const auto& t = trades_[static_cast<std::size_t>(index.row())];
    switch (index.column()) {
    case 0: return t.time.toString("yyyy-MM-dd HH:mm:ss");
    case 1: return t.symbol;
    case 2: return (t.side == TradeSide::Buy) ? "Buy" : "Sell";
    case 3: return t.qty;
    case 4: return t.price;
    case 5: return t.fee;
    case 6: {
        switch (t.status){
        case TradeStatus::Open: return "Open";
        case TradeStatus::Closed: return "Closed";
        case TradeStatus::PartiallyFilled: return "PartiallyFilled";
        case TradeStatus::Rejected: return "Rejected";
        default: return {};
        }
    }
    default: return{};
    }
}

QVariant TradesModel::headerData(int section,Qt::Orientation orientation ,int role) const {
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (orientation != Qt::Horizontal) {
        return section + 1;
    }

    switch (section) {
    case 0: return "Time";
    case 1: return "Symbol";
    case 2: return "Side";
    case 3: return "Qty";
    case 4: return "Price";
    case 5: return "Fee";
    case 6: return "Status";
    default: return {};
    }
}

void TradesModel::appendTrade(const TradeRecord& t) {
    const int row = static_cast<int>(trades_.size());
    beginInsertRows(QModelIndex(), row, row);
    trades_.push_back(t);
    endInsertRows();
}

void TradesModel::clear() {
    beginResetModel();
    trades_.clear();
    endResetModel();
}

