#include "core/logging.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageLogContext>
#include <QMutex>
#include <QStandardPaths>
#include <QTextStream>

namespace {

QMutex logMutex;
QString currentLogFilePath;

QString levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "DEBUG";
    case QtInfoMsg: return "INFO";
    case QtWarningMsg: return "WARN";
    case QtCriticalMsg: return "ERROR";
    case QtFatalMsg: return "FATAL";
    }
    return "LOG";
}

QString appDataDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/.tradesoft";
    }
    QDir().mkpath(dir);
    return dir;
}

void tradeSoftMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    const QString line = QString("[%1] [%2] %3 (%4:%5)")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                                  levelName(type),
                                  msg,
                                  QString::fromUtf8(context.file ? context.file : "unknown"),
                                  QString::number(context.line));

    {
        QMutexLocker lock(&logMutex);
        QFile file(currentLogFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << line << '\n';
        }
    }

    QTextStream err(stderr);
    err << line << '\n';

    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace

namespace AppLogging {

void install()
{
    currentLogFilePath = appDataDir() + "/tradesoft.log";
    qInstallMessageHandler(tradeSoftMessageHandler);
}

QString logFilePath()
{
    if (currentLogFilePath.isEmpty()) {
        currentLogFilePath = appDataDir() + "/tradesoft.log";
    }
    return currentLogFilePath;
}

}
