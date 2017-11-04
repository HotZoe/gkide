/// @file snail/libs/nvimcore/logmanager.cpp

#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QFileOpenEvent>
#include "envdefs.h"
#include "configsnail.h"
#include "snail/app/attributes.h"
#include "snail/libs/nvimcore/logmanager.h"

namespace SnailNvimQt {

/// snail log level enum definations
enum LogLevelType
{
    LOG_TRACE = 0, ///< application trace information
    LOG_DEBUG = 1, ///< application debug information
    LOG_STATE = 2, ///< application state information
    LOG_ALERT = 3, ///< potentially harmful situations
    LOG_ERROR = 4, ///< error events occurs, but continue
    LOG_FATAL = 5, ///< core dump, abort immediately
    LOG_OFF   = 6, ///< highest rank and turn off all logging
};

void logging_nothing(QtMsgType FUNC_ATTR_ARGS_UNUSED_REALY(type),
                     const QMessageLogContext &FUNC_ATTR_ARGS_UNUSED_REALY(ctx),
                     const QString &FUNC_ATTR_ARGS_UNUSED_REALY(msg))
{
    return; // ignore all Qt loggings when enable logging and not set environment log file
}

/// A log handler for Qt messages, all messages are dumped into the file: @def ENV_GKIDE_SNAIL_LOGGINGS.
///
/// In UNIX Qt prints messages to the console output, but in Windows this is the only way to
/// get Qt's debug/warning messages.
void logging_handler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
#ifdef SNAIL_LOGGING_DISABLE
    return;
#else
    LogLevelType lglv;
    switch(type)
    {
        case QtInfoMsg:     // qInfo(4) => TRACE(0)
            lglv = LOG_TRACE;
            break;
        case QtDebugMsg:    // qDebug(0) => DEBUG(1), STATE(2)
            lglv = LOG_DEBUG;
            break;
        case QtWarningMsg:  // qWarning(1) => ALERT(3)
            lglv = LOG_ALERT;
            break;
        case QtCriticalMsg: // qCritical(2) => ERROR(4)
            lglv = LOG_ERROR;
            break;
        case QtFatalMsg:    // qFatal(3) => FATAL(5)
            ::abort();
            break;
        default:
            lglv = LOG_TRACE;
            break;
    };

    bool env_ok = false;
    int env_level = SNAIL_LOG_LEVEL_MIN;
    if(!qgetenv(ENV_GKIDE_SNAIL_LOGLEVEL).isEmpty())
    {
        env_level = qgetenv(ENV_GKIDE_SNAIL_LOGLEVEL).toInt(&env_ok, 10);
    }

    if(env_ok && lglv < env_level)
    {
        return;
    }

    if(lglv < SNAIL_LOG_LEVEL_MIN || lglv >= LOG_OFF)
    {
        return;
    }

    QFile logFile(qgetenv(ENV_GKIDE_SNAIL_LOGGINGS));
    QString log = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");

    // cast to avoid gcc '-Wswitch'
    switch(static_cast<int>(lglv))
    {
        case LOG_TRACE:
            log += QString("TRACE [");
            break;
        case LOG_DEBUG:
            log += QString("DEBUG [");
            break;
        case LOG_ALERT:
            log += QString("ALERT [");
            break;
        case LOG_ERROR:
            log += QString("ERROR [");
            break;
    };

    QString file_name = QString(ctx.file); // full file path
    file_name = QString(file_name.constData() + file_name.lastIndexOf("/") + 1);

    log += file_name + "@" + QString::number(ctx.line) + QString("] %1\n").arg(msg);
    if(logFile.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&logFile);
        stream << log;
    }
#endif
}

#ifdef Q_OS_MAC
bool getLoginEnvironment(const QString &path)
{
    QProcess proc;
    proc.start(path, {"-l", "-c", "env", "-i"});
    if(!proc.waitForFinished())
    {
        //qDebug() << "Failed to execute shell to get environemnt: " << path;
        return false;
    }

    QByteArray out = proc.readAllStandardOutput();
    foreach(const QByteArray &item, out.split('\n'))
    {
        int index = item.indexOf('=');
        if(index > 0)
        {
            qputenv(item.mid(0, index), item.mid(index+1));
            //qDebug() << item.mid(0, index) << item.mid(index+1);
        }
    }
    return true;
}
#endif

} // [Namespace] SnailNvimQt
