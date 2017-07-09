#ifndef NEOVIM_QT_APP
#define NEOVIM_QT_APP

#include <QApplication>
#include <QEvent>
#include <QUrl>
#include <QList>
#include <QCommandLineParser>

namespace SnailNvimQt
{

class NvimConnector;

class App: public QApplication
{
	Q_OBJECT
public:
    static NvimConnector* createConnector(const QCommandLineParser &p);
    static void processCliOptions(QCommandLineParser &p, const QStringList &arguments);

    App(int &argc, char **argv);
	bool event(QEvent *event);
    void showUi(NvimConnector *c, const QCommandLineParser &);

signals:
	void openFilesTriggered(const QList<QUrl>);
};

} // Namespace

#endif
