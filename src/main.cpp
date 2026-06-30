#include "ui/mainwindow.h"
#include "core/candleseries.h"
#include "core/logging.h"
#include <QApplication>
#include <QIcon>
#include <QMetaType>
#include <QSslSocket>
#include <QLoggingCategory>
#include <memory>

#include <QFileInfo>

Q_LOGGING_CATEGORY(logApp, "tradesoft.app")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("BortonGo");
    QCoreApplication::setApplicationName("TradeSoft");
    AppLogging::install();

    QIcon icon(":/icons/resources/TradeSoftLogo.ico");
    a.setWindowIcon(icon);
    qRegisterMetaType<std::shared_ptr<CandleSeries>>("std::shared_ptr<CandleSeries>");
    MainWindow w;
    w.show();

    qCInfo(logApp) << "SSL supported:" << QSslSocket::supportsSsl();
    qCInfo(logApp) << "SSL build:" << QSslSocket::sslLibraryBuildVersionString();
    qCInfo(logApp) << "SSL runtime:" << QSslSocket::sslLibraryVersionString();
    qCInfo(logApp) << "Log file:" << AppLogging::logFilePath();

    return a.exec();
}
