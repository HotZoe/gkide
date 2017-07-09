#include <QFile>
#include <QtGlobal>
#include <QProcess>
#include <QApplication>
#include <QCommandLineParser>
#include "snail/libs/nvimcore/app.h"
#include "snail/libs/nvimcore/nvimconnector.h"

// GUI Main
int gui_main(int argc, char **argv)
{
    SnailNvimQt::App app(argc, argv);

    QCommandLineParser parser;
    SnailNvimQt::App::processCliOptions(parser, app.arguments());

    auto c = app.createConnector(parser);
    app.showUi(c, parser);
    return app.exec();
}

// Command Line Interface Main
int cli_main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    SnailNvimQt::App::processCliOptions(parser, app.arguments());

    QStringList new_args = app.arguments().mid(1);
    new_args.insert(0, "--nofork");

    if(QProcess::startDetached(app.applicationFilePath(), new_args))
    {
        return 0;
    }
    else
    {
        qWarning() << "Unable to fork into background";
        return -1;
    }
}

int main(int argc, char **argv)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Do an early check for --nofork before creating a QApplication,
    // thus we have chance to parse the command line arguments by QCommandLineParser
    bool nofork = false;
    for(int i=1; i<argc; i++)
    {
        if(QString::compare("--", argv[i]) == 0)
        {
            break;
        }
        else if(QString::compare("--spawn", argv[i]) == 0)
        {
            break;
        }
        else if(QString::compare("--nofork", argv[i]) == 0)
        {
            nofork = true;
            break;
        }
    }

    if(nofork)
    {
        return gui_main(argc, argv);
    }
    else
    {
        return cli_main(argc, argv);
    }
#else
    return gui_main(argc, argv);
#endif
}
