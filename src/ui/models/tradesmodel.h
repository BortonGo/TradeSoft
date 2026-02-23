#pragma once
#include <QAbstractTableModel>
#include <vector>
#include "domain/trade/traderecord.h"

class TradesModel final : public QAbstractTableModel
{
    std::vector<TradeRecord> trades_;

public:
    enum Column {
            ColTime = 0,
            ColSymbol,
            ColSide,
            ColQty,
            ColOpenPrice,
            ColClosePrice,
            ColPnL,
            ColFee,
            ColTicks,
            ColStatus,
            ColCount
        };

    explicit TradesModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,Qt::Orientation orientation ,int role = Qt::DisplayRole) const override;

    void clear();

    TradeRecord tradeAt(int row) const;

    void appendTrade(const TradeRecord& t);
    void updateTrade(int row, const TradeRecord& t);

};
