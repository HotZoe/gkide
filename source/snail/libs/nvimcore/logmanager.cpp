#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QFileOpenEvent>
#include "configsnail.h"
#include "snail/app/envdefs.h"
#include "snail/app/attributes.h"

namespace SnailNvimQt
{

/// snail log level enum definations
enum LogLevelType
{
    LOG_ALL   = 0, ///< lowest rank and turn on all logging
    LOG_TRACE = 1, ///< application trace information
    LOG_DEBUG = 2, ///< application debug information
    LOG_STATE = 3, ///< application state information
    LOG_ALERT = 4, ///< potentially harmful situations
    LOG_ERROR = 5, ///< error events occurs, but continue
    LOG_FATAL = 6, ///< core dump, abort immediately
    LOG_OFF   = 7, ///< highest rank and turn off all logging
};

void logging_nothing(QtMsgType type VARS_ATTR_UNUSED,
                     const QMessageLogContext &ctx VARS_ATTR_UNUSED,
                     const QString &msg VARS_ATTR_UNUSED)
{
    // ignore all Qt loggings when enable logging and not set environment log file
    return;
}

/// A log handler for Qt messages, all messages are dumped into the file: @def ENV_LOG_FILE.
///
/// In UNIX Qt prints messages to the console output, but in Windows this is the only way to
/// get Qt's debug/warning messages.
void logging_handler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
#ifdef DISABLE_LOGGING
    return;
#endif

    LogLevelType lglv;

    switch(type)
    {
    case QtInfoMsg: // qInfo(4) => TRACE(1)
        lglv = LOG_TRACE;
        break;
    case QtDebugMsg: // qDebug(0) => DEBUG(2), STATE(3)
        lglv = LOG_DEBUG;
        break;
    case QtWarningMsg: // qWarning(1) => ALERT(4)
        lglv = LOG_ALERT;
        break;
    case QtCriticalMsg: // qCritical(2) => ERROR(5)
        lglv = LOG_ERROR;
        break;
    case QtFatalMsg: // qFatal(3) => FATAL(6)
        ::abort();
        break;
    default:
        lglv = LOG_TRACE;
        break;
    };

    bool env_ok = false;
    int env_level = LOG_LEVEL_MIN;

    if(!qgetenv(ENV_LOG_LEVEL).isEmpty())
    {
        env_level = qgetenv(ENV_LOG_LEVEL).toInt(&env_ok, 10);
    }

    if(env_ok && lglv < env_level)
    {
        return;
    }

    if(lglv < LOG_LEVEL_MIN || lglv >= LOG_OFF)
    {
        return;
    }

    QFile logFile(qgetenv(ENV_LOG_FILE));
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

    log += QString(ctx.file) + "@" + QString::number(ctx.line) + QString("] %1\n").arg(msg);

    if(logFile.open(QIODevice::Append | QIODevice::Text))
    {
		QTextStream stream(&logFile);
        stream << log;
	}
}

#ifdef Q_OS_MAC
bool getLoginEnvironment(const QString &path)
{
	QProcess proc;
	proc.start(path, {"-l", "-c", "env", "-i"});
    if(!proc.waitForFinished())
    {
        qDebug() << "Failed to execute shell to get environemnt: " << path;
		return false;
	}

	QByteArray out = proc.readAllStandardOutput();
    foreach(const QByteArray &item, out.split('\n'))
    {
		int index = item.indexOf('=');
        if(index > 0)
        {
			qputenv(item.mid(0, index), item.mid(index+1));
			qDebug() << item.mid(0, index) << item.mid(index+1);
		}
	}

	return true;
}
#endif

} // [Namespace] SnailNvimQt
