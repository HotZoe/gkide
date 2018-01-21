/// @file plugins/bin/snail/main.cpp

#include <QFile>
#include <QtGlobal>
#include <QProcess>
#include <QApplication>
#include <QCommandLineParser>
#include "versiondef.h"
#include "configsnail.h"
#include "plugins/bin/snail/app.h"
#include "plugins/bin/snail/nvimconnector.h"

#define SNAIL_APP_VERSION_STRING \
    "v" SNAIL_VERSION_BASIC "@" RELEASE_PACKAGE_NAME

/// GUI Interface
int gui_main(int argc, char **argv)
{
    SnailNvimQt::App app(argc, argv);
    QCommandLineParser parser;

#ifdef TRACE_LOG_ENABLE
    // click and run, get the arguments to this file
    QFile logFile(QCoreApplication::applicationDirPath()+"/debug.log");
    if(logFile.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&logFile);

        for(int i=1; i<argc; i++)
        {
            stream << argv[i] << "\n";
        }

        logFile.close();
    }
#endif

    SnailNvimQt::App::processCliOptions(parser, app.arguments());
    auto c = SnailNvimQt::App::createConnector(parser);
    app.showUi(c, parser);

    return app.exec();
}

/// Command Line Interface, parsing command line arguments then start GUI
int cli_main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    SnailNvimQt::App::processCliOptions(parser, app.arguments());
    QStringList new_args = app.arguments().mid(1);
    new_args.insert(0, "--nofork");

    // detached from the command line, re-run snail, get a new GUI process
    if(QProcess::startDetached(app.applicationFilePath(), new_args))
    {
        return 0; // command line snail exit
    }
    else
    {
        qWarning() << "Unable to fork into background";
        return -1;
    }
}

int main(int argc, char **argv)
{
    QCoreApplication::setApplicationVersion(SNAIL_APP_VERSION_STRING);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Do an early check for --nofork before creating a QApplication,
    // thus we have chance to parse the command line arguments
    // by QCommandLineParser
    bool nofork = false;

    for(int i=1; i<argc; i++)
    {
        if(QString::compare("--", argv[i]) == 0)
        {
            break; // have cmd-line arguments
        }
        else if(QString::compare("--spawn", argv[i]) == 0)
        {
            break; // cmd-line spawn
        }
        else if(QString::compare("--nofork", argv[i]) == 0)
        {
            nofork = true; // start GUI, no fork
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
