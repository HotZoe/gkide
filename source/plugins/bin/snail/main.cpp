/// @file plugins/bin/snail/main.cpp

#include <QFile>
#include <QtGlobal>
#include <QProcess>
#include <QApplication>
#include <QCommandLineParser>

#include "plugins/bin/snail/app.h"
#include "plugins/bin/snail/nvimconnector.h"
#include "generated/config/gkideversion.h"

#define SNAIL_MODIFY_TIME \
    GIT_COMMIT_DATE " " GIT_COMMIT_TIME " " GIT_COMMIT_ZONE

#define BUILD_OS_INFO \
    BUILD_ON_HOST "(" BUILD_OS_NAME ", v" BUILD_OS_VERSION ", " BUILD_OS_ARCH ")"

#define SNAIL_VERSION_INFO                              \
    "v" SNAIL_VERSION_BASIC "-" SNAIL_RELEASE_TYPE "\n" \
    "build at " BUILD_TIMESTAMP "\n"                    \
    "modified at " SNAIL_MODIFY_TIME "\n"               \
    "compiled by " BUILD_BY_USER "@" BUILD_OS_INFO "\n" \
    GKIDE_PACKAGE_NAME

/// GUI Interface
int gui_main(int argc, char **argv)
{
    SnailNvimQt::App app(argc, argv);
    QCommandLineParser parser;

    SnailNvimQt::App::initCliArgs(parser, app.arguments());
    auto c = SnailNvimQt::App::createConnector(parser);
    app.showUi(c, parser);

    return app.exec();
}

/// Command Line Interface, parsing command line arguments then start GUI
int cli_main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    SnailNvimQt::App::initCliArgs(parser, app.arguments());
    QStringList new_args = app.arguments().mid(1);
    new_args.insert(0, "--nofork");

    // detached from the command line, re-run snail, get a new GUI process
    if(QProcess::startDetached(app.applicationFilePath(), new_args))
    {
        return 0; // snail command line exit
    }
    else
    {
        qWarning() << "Unable to fork into background";
        return -1;
    }
}

int main(int argc, char **argv)
{
    QCoreApplication::setApplicationVersion(SNAIL_VERSION_INFO);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Do an early check for --nofork before creating a QApplication,
    // thus we have chance to parse the command line arguments
    // by QCommandLineParser
    bool nofork = false;

    for(int i=1; i<argc; i++)
    {
        if(QString::compare("--nofork", argv[i]) == 0)
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
