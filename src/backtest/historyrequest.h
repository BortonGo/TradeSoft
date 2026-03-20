#pragma once
#include <QString>
#include <QDateTime>

#include "core/timeframe.h"

struct HistoryRequest {
    QString symbol;
    Timeframe timeframe = Timeframe::M1;
    QDateTime begin;
    QDateTime end;

};

