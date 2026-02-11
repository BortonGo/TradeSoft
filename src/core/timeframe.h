#pragma once
#include <QString>
#include <QStringList>

enum class Timeframe {
    M1,
    M5,
    M15,
    H1,
    H4,
    D1
};

inline QString toUiString(Timeframe tf) {
    switch (tf){
        case Timeframe::M1 : return "M1";
        case Timeframe::M5 : return "M5";
        case Timeframe::M15 : return "M15";
        case Timeframe::H1 : return "H1";
        case Timeframe::H4 : return "H4";
        case Timeframe::D1 : return "D1";
    }
    return "M1";
}

inline Timeframe timeframeFromUiString(QString str) {
    if (str == "M1") return Timeframe::M1;
    if (str == "M5") return Timeframe::M5;
    if (str == "M15") return Timeframe::M15;
    if (str == "H1") return Timeframe::H1;
    if (str == "H4") return Timeframe::H4;
    if (str == "D1") return Timeframe::D1;

    return Timeframe::M1;
}

inline int64_t timeframeToMs(Timeframe tf) {
    switch (tf) {
        case Timeframe::M1:  return 60LL * 1000;
        case Timeframe::M5:  return 5LL * 60 * 1000;
        case Timeframe::M15: return 15LL * 60 * 1000;
        case Timeframe::H1:  return 60LL * 60 * 1000;
        case Timeframe::H4:  return 4LL * 60 * 60 * 1000;
        case Timeframe::D1:  return 24LL * 60 * 60 * 1000;
    }
    return 60LL * 1000;
}


inline QList<Timeframe> allTimeframes() {
    return { Timeframe::M1, Timeframe::M5, Timeframe::M15,
            Timeframe::H1, Timeframe::H4, Timeframe::D1 };
}

inline bool timeframeIsIntraday(Timeframe tf) {
    return tf = Timeframe::M1
            || tf = Timeframe::M5
            || tf = Timeframe::M15
            || tf = Timeframe::H1
            || tf = Timeframe::H4;
}


