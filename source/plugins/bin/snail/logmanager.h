/// @file plugins/bin/snail/logmanager.h

#ifndef PLUGIN_SNAIL_LOGMANAGER_H
#define PLUGIN_SNAIL_LOGMANAGER_H

#include <QUrl>
#include <QList>
#include <QEvent>
#include <QApplication>
#include <QCommandLineParser>

#ifdef Q_OS_MAC
    #include <QProcess>
#endif

namespace SnailNvimQt {

#ifdef SNAIL_LOGGING_DISABLE
    void logging_nothing(QtMsgType type,
                         const QMessageLogContext &ctx,
                         const QString &msg);
#else
    void logging_handler(QtMsgType type,
                         const QMessageLogContext &ctx,
                         const QString &msg);
#endif

#ifdef Q_OS_MAC
    bool getLoginEnvironment(const QString &path);
#endif

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_LOGMANAGER_H
