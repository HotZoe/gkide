#ifndef SNAIL_LIBS_NVIMCORE_LOGMANAGER_H
#define SNAIL_LIBS_NVIMCORE_LOGMANAGER_H

#include <QApplication>
#include <QEvent>
#include <QUrl>
#include <QList>
#include <QCommandLineParser>

namespace SnailNvimQt
{

void logging_handler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
void logging_nothing(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);

#ifdef Q_OS_MAC
bool getLoginEnvironment(const QString& path);
#endif

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_LOGMANAGER_H
