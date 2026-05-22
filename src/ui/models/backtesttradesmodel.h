#pragma once
#include <QAbstractTableModel>
#include <vector>
#include "backtest/backtesttypes.h"

class BacktestTradesModel final : public QAbstractTableModel {
    std::vector<BacktestTrade> trades_;

public:
    enum Column {
            ColEntryTime = 0,
            ColExitTime,
            ColSide,
            ColQty,
            ColEntryPrice,
            ColExitPrice,
            ColGrossPnl,
            ColNetPnl,
            ColFee,
            ColBarsHeld,
            ColWinner,
            ColCount
        };

    explicit BacktestTradesModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,Qt::Orientation orientation ,int role = Qt::DisplayRole) const override;

    void clear();
    void setTrades(const std::vector<BacktestTrade>& trades);
    BacktestTrade tradeAt(int row) const;
};
