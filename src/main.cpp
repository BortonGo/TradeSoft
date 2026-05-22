#include "ui/mainwindow.h"
#include "core/candleseries.h"
#include <QApplication>
#include <QIcon>
#include <QMetaType>
#include <QSslSocket>
#include <QDebug>
#include <QLoggingCategory>
#include <memory>

#include <QFileInfo>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QIcon icon(":/icons/resources/TradeSoftLogo.ico");
    a.setWindowIcon(icon);
    qRegisterMetaType<std::shared_ptr<CandleSeries>>("std::shared_ptr<CandleSeries>");
    MainWindow w;
    w.show();

    QLoggingCategory::setFilterRules("qt.network.ssl.debug=true\nqt.network.access.debug=true\n");
    qDebug() << "SSL supported:" << QSslSocket::supportsSsl();
    qDebug() << "SSL build:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "SSL runtime:" << QSslSocket::sslLibraryVersionString();

    return a.exec();
}
