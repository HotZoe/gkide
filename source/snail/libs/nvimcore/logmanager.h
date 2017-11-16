/// @file snail/libs/nvimcore/logmanager.h

#ifndef SNAIL_LIBS_NVIMCORE_LOGMANAGER_H
#define SNAIL_LIBS_NVIMCORE_LOGMANAGER_H

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
    void logging_nothing(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
#else
    void logging_handler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
#endif

#ifdef Q_OS_MAC
    bool getLoginEnvironment(const QString &path);
#endif

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_LOGMANAGER_H
