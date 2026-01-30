#-------------------------------------------------
#
# Project created by QtCreator 2026-01-30T10:22:43
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TradeSoft
TEMPLATE = app


SOURCES += src\main.cpp\
        src\ui\mainwindow.cpp \
    src\ui\chartwidget.cpp \
    src\core\candleseries.cpp \
    src\service\marketdataservice.cpp \
    src\exchange\bingxswapclient.cpp \
    src\exchange\fakeexchangeclient.cpp

HEADERS  += src\ui\mainwindow.h \
    src\ui\chartwidget.h \
    src\core\candle.h \
    src\core\candleseries.h \
    src\service\marketdataservice.h \
    src\exchange\iexchangeclient.h \
    src\exchange\bingxswapclient.h \
    src\exchange\fakeexchangeclient.h \
    src/core/timeframe.h

FORMS    += mainwindow.ui


DISTFILES += \
    .gitignore
