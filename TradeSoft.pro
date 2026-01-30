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
        src\mainwindow.cpp \
    src\chartwidget.cpp \
    src\candleseries.cpp \
    src\marketdataservice.cpp \
    src\bingxswapclient.cpp \
    src\fakeexchangeclient.cpp

HEADERS  += src\mainwindow.h \
    src\chartwidget.h \
    src\candle.h \
    src\candleseries.h \
    src\marketdataservice.h \
    src\iexchangeclient.h \
    src\bingxswapclient.h \
    src\fakeexchangeclient.h

FORMS    += mainwindow.ui \
    chartwidget.ui
