/// @file plugins/bin/snail/app.h

#ifndef PLUGIN_SNAIL_APP_H
#define PLUGIN_SNAIL_APP_H

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
    static void appExit(QString reason, QString todo);
    static NvimConnector *createConnector(const QCommandLineParser &p);
    static void initCliArgs(QCommandLineParser &p,
                            const QStringList &arguments);

    App(int &argc, char **argv);
    bool event(QEvent *event);
    void showUi(NvimConnector *c, const QCommandLineParser &);

signals:
    void openFilesTriggered(const QList<QUrl>);
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_APP_H
