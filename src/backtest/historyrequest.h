#pragma once
#include <QString>
#include <QDateTime>

#include "core/timeframe.h"

struct HistoryRequest final {
    QString symbolId;
    Timeframe timeframe = Timeframe::M1;
    QDateTime begin;
    QDateTime end;

};

