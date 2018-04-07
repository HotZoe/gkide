/// @file plugins/bin/snail/logmanager.cpp

#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QFileOpenEvent>

#include "generated/config/gkideenvs.h"
#include "generated/config/configsnail.h"

#include "plugins/bin/snail/attributes.h"
#include "plugins/bin/snail/logmanager.h"

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

#ifdef SNAIL_LOGGING_DISABLE
void logging_nothing(QtMsgType FUNC_ATTR_ARGS_UNUSED_REALY(type),
                     const QMessageLogContext &FUNC_ATTR_ARGS_UNUSED_REALY(ctx),
                     const QString &FUNC_ATTR_ARGS_UNUSED_REALY(msg))
{
    // ignore all Qt loggings when enable
    // logging and not set environment log file
    return;
}
#else
/// A log handler for Qt messages, all messages are
/// dumped into the file: @def ENV_GKIDE_SNAIL_LOGGINGS.
///
/// In UNIX Qt prints messages to the console output,
/// but in Windows this is the only way to get Qt's debug/warning messages.
void logging_handler(QtMsgType type,
                     const QMessageLogContext &ctx,
                     const QString &msg)
{
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

    if(qEnvironmentVariableIsSet(ENV_GKIDE_SNAIL_LOGLEVEL)
       && !qEnvironmentVariableIsEmpty(ENV_GKIDE_SNAIL_LOGLEVEL))
    {
        env_level =
            qEnvironmentVariableIntValue(ENV_GKIDE_SNAIL_LOGLEVEL, &env_ok);
    }

    if(env_ok && lglv < env_level)
    {
        return;
    }

    if(lglv < SNAIL_LOG_LEVEL_MIN || lglv >= LOG_OFF)
    {
        return;
    }

    QFile logFile;

    if(qEnvironmentVariableIsSet(ENV_GKIDE_SNAIL_LOGGINGS)
       && !qEnvironmentVariableIsEmpty(ENV_GKIDE_SNAIL_LOGGINGS))
    {
        // check $GKIDE_SNAIL_LOGGINGS
        QString env_val = qgetenv(ENV_GKIDE_SNAIL_LOGGINGS);
        QFileInfo fi(env_val);

        if(fi.fileName().isEmpty())
        {
            // the env value ending in slash
            if(!fi.isDir())
            {
                // not exist, try to create it
                // with parents directories if needed.
                if(!QDir(env_val).mkpath(env_val))
                {
                    return; // can not create, skip
                }
            }

            logFile.setFileName(env_val+"snail.log");
        }
        else
        {
            // the env value not ending in slash
            QDir log_dir = fi.dir(); // get the file's parent directory

            if(!log_dir.exists())
            {
                // not exist, try to create it
                // with parents directories if needed.
                if(!log_dir.mkpath(fi.path()))
                {
                    return; // can not create, skip
                }
            }

            logFile.setFileName(env_val); // a file, just use it
        }
    }
    else if(qEnvironmentVariableIsSet(ENV_GKIDE_USR_HOME)
            && !qEnvironmentVariableIsEmpty(ENV_GKIDE_USR_HOME))
    {
        // the default, check $GKIDE_USR_HOME
        // $GKIDE_USR_HOME/snail.log
        QString env_val = qgetenv(ENV_GKIDE_USR_HOME);
        QDir home_dir = QDir(env_val);

        // not exist, try to create it with parents directories if needed.
        if(!home_dir.exists())
        {
            if(!home_dir.mkpath(env_val))
            {
                return; // can not create, this should be fixed
            }
        }

        if(QFileInfo(env_val).fileName().isEmpty())
        {
            // path ending with a slash
            logFile.setFileName(env_val+"snail.log");
        }
        else
        {
            // path not ending with a slash
            logFile.setFileName(env_val+"/snail.log");
        }
    }
    else
    {
        // $GKIDE_USR_HOME not set, use the default value $HOME value
        #ifdef Q_OS_WIN
        QString gkide_usr_home = QDir::homePath() + "/Documents/gkide";
        #else
        QString gkide_usr_home = QDir::homePath() + "/.gkide";
        #endif
        QDir home_dir = QDir(gkide_usr_home);

        if(!home_dir.exists())
        {
            // not exist, try to create it, just one level is enough
            if(!home_dir.mkdir(gkide_usr_home))
            {
                return; // can not create, this should be fixed
            }
        }

        logFile.setFileName(gkide_usr_home + "/snail.log");
    }

    QString log_msg =
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");

    // cast to avoid gcc '-Wswitch'
    switch(static_cast<int>(lglv))
    {
        case LOG_TRACE:
            log_msg += QString("TRACE [");
            break;

        case LOG_DEBUG:
            log_msg += QString("DEBUG [");
            break;

        case LOG_ALERT:
            log_msg += QString("ALERT [");
            break;

        case LOG_ERROR:
            log_msg += QString("ERROR [");
            break;
    };

    QString file_name = QString(ctx.file); // full file path

    file_name = QString(file_name.constData() + file_name.lastIndexOf("/") + 1);

    log_msg += file_name
               + "@"
               + QString::number(ctx.line)
               + QString("] %1\n").arg(msg);

    if(logFile.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&logFile);
        stream << log_msg;
    }
}
#endif

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

} // namespace::SnailNvimQt
