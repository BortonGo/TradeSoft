#include "ui\mainwindow.h"
#include "core\candleseries.h"
#include <QApplication>
#include <QMetaType>
#include <memory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<std::shared_ptr<CandleSeries>>("std::shared_ptr<CandleSeries>");
    MainWindow w;
    w.show();

    return a.exec();
}
