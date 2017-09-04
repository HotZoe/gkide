/// @file snail/libs/nvimcore/app.h

#ifndef SNAIL_LIBS_NVIMCORE_APP_H
#define SNAIL_LIBS_NVIMCORE_APP_H

#include <QUrl>
#include <QList>
#include <QEvent>
#include <QApplication>
#include <QCommandLineParser>

namespace SnailNvimQt {

class NvimConnector;

class App: public QApplication
{
    Q_OBJECT
public:
    static NvimConnector *createConnector(const QCommandLineParser &p);
    static void processCliOptions(QCommandLineParser &p, const QStringList &arguments);

    App(int &argc, char **argv);
    bool event(QEvent *event);
    void showUi(NvimConnector *c, const QCommandLineParser &);

signals:
    void openFilesTriggered(const QList<QUrl>);
};

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_APP_H
