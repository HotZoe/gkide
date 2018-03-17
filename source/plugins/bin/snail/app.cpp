/// @file plugins/bin/snail/app.cpp

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QFileOpenEvent>
#include "generated/config/gkideenvs.h"
#include "plugins/bin/snail/app.h"
#include "plugins/bin/snail/logmanager.h"
#include "plugins/bin/snail/mainwindow.h"

#ifdef SNAIL_ENABLE_WINPOS_INFO
    #include "plugins/bin/snail/winpos_info.h"
#endif

namespace SnailNvimQt {

App::App(int &argc, char **argv): QApplication(argc, argv)
{
    setWindowIcon(QIcon(":/logo/snail.png"));
    setApplicationDisplayName("GKIDE");

#ifdef Q_OS_MAC
    QByteArray shellPath = qgetenv("SHELL");

    if(!getLoginEnvironment(shellPath))
    {
        getLoginEnvironment("/bin/bash");
    }
#endif

#ifdef SNAIL_LOGGING_DISABLE
    qInstallMessageHandler(logging_nothing);
#else
    qInstallMessageHandler(logging_handler);
#endif
}

bool App::event(QEvent *event)
{
    if(event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *fileOpenEvent = static_cast<QFileOpenEvent *>(event);

        if(fileOpenEvent)
        {
            emit openFilesTriggered({fileOpenEvent->url()});
        }
    }

    return QApplication::event(event);
}

void App::showUi(NvimConnector *c, const QCommandLineParser &parser)
{
    SnailNvimQt::MainWindow *win = new SnailNvimQt::MainWindow(c);

#ifdef SNAIL_ENABLE_WINPOS_INFO
    WinPosInfo *winpos_info = new WinPosInfo(win);
    winpos_info->show();
#endif

    connect(instance(), SIGNAL(openFilesTriggered(const QList<QUrl>)),
            win->shell(), SLOT(openFiles(const QList<QUrl>)));

    if(parser.isSet("fullscreen"))
    {
        win->delayedShow(SnailNvimQt::MainWindow::DelayedShow::FullScreen);
    }
    else if(parser.isSet("maximized"))
    {
        win->delayedShow(SnailNvimQt::MainWindow::DelayedShow::Maximized);
    }
    else
    {
        win->delayedShow();
    }
}

/// Initialize CLI parser with all the snail arguments, process the
/// provided arguments and check for errors.
///
/// When appropriate this function will call
/// QCommandLineParser::showHelp() terminating the program.
void App::initCliArgs(QCommandLineParser &parser,
                      const QStringList &arguments)
{
    QString arg_desc;

#ifdef Q_OS_UNIX
    // --nofork
    QCommandLineOption arg_nofork("nofork");
    arg_desc = QCoreApplication::translate("main", "Run snail in foreground.");
    arg_nofork.setDescription(arg_desc);
    parser.addOption(arg_nofork);
#endif

    // --nvim <nvim_path>
    QCommandLineOption arg_nvim("nvim");
    arg_desc = QCoreApplication::translate("main", "nvim executable path.");
    arg_nvim.setDescription(arg_desc);
    arg_nvim.setValueName("nvim_exec");

    #ifdef Q_OS_WIN
    arg_nvim.setDefaultValue(App::applicationDirPath() + "/nvim.exe");
    #else
    arg_nvim.setDefaultValue(App::applicationDirPath() + "/nvim");
    #endif

    parser.addOption(arg_nvim);

    // --server <address>
    QCommandLineOption arg_server("server");
    arg_desc = QCoreApplication::translate("main", "Connect to existing nvim instance.");
    arg_server.setDescription(arg_desc);
    arg_server.setValueName("server_addr");
    parser.addOption(arg_server);

    // --embed
    QCommandLineOption arg_embed("embed");
    arg_desc = QCoreApplication::translate("main", "Communicate with nvim over stdin/stdout/stderr.");
    arg_embed.setDescription(arg_desc);
    parser.addOption(arg_embed);

    // --spawn
    QCommandLineOption arg_spawn("spawn");
    arg_desc = QCoreApplication::translate("main", "Treat positional arguments as the nvim argv.");
    arg_spawn.setDescription(arg_desc);
    parser.addOption(arg_spawn);

    // --maximized
    QCommandLineOption arg_maximized("maximized");
    arg_desc = QCoreApplication::translate("main", "Maximize the window on startup");
    arg_maximized.setDescription(arg_desc);
    parser.addOption(arg_maximized);

    // --fullscreen
    QCommandLineOption arg_fullscreen("fullscreen");
    arg_desc = QCoreApplication::translate("main", "Fullscreen the window on startup.");
    arg_fullscreen.setDescription(arg_desc);
    parser.addOption(arg_fullscreen);

    // --geometry
    QCommandLineOption arg_geometry("geometry");
    arg_desc = QCoreApplication::translate("main", "Initial the window geometry.");
    arg_geometry.setDescription(arg_desc);
    arg_server.setValueName("geometry");
    parser.addOption(arg_geometry);

    // positional arguments
    arg_desc = QCoreApplication::translate("main", "Edit specified file(s).");
    parser.addPositionalArgument("file", arg_desc, "[file...]");
    arg_desc = QCoreApplication::translate("main", "Additional arguments forwarded to nvim.");
    parser.addPositionalArgument("...", arg_desc, "[-- ...]");

    // --version
    parser.addVersionOption();
    // --help
    parser.addHelpOption();

    // processes the real command line arguments
    parser.process(arguments);

    if(parser.isSet("help"))
    {
        parser.showHelp();
    }

    bool has_embed = parser.isSet("embed");
    bool has_spawn = parser.isSet("spawn");
    bool has_server = parser.isSet("server");
    int exclusive = has_server + has_embed + has_spawn;

    if(exclusive > 1)
    {
        qWarning() << "Options --server, --spawn and --embed "
                      "are mutually exclusive.\n";
        ::exit(EXIT_FAILURE);
    }

    if(!parser.positionalArguments().isEmpty() && (has_embed || has_server))
    {
        qWarning() << "Options --embed and --server "
                      "do not accept positional arguments.\n";
        ::exit(EXIT_FAILURE);
    }

    if(parser.positionalArguments().isEmpty() && has_spawn)
    {
        qWarning() << "Option --spawn requires at least "
                      "one positional argument.\n";
        ::exit(EXIT_FAILURE);
    }
}

NvimConnector *App::createConnector(const QCommandLineParser &parser)
{
    if(parser.isSet("embed"))
    {
        return SnailNvimQt::NvimConnector::connectToStdInOut();
    }
    else if(parser.isSet("server"))
    {
        QString server = parser.value("server");

        qDebug() << "serverAddr=" << server;
        Q_ASSERT(server.isEmpty() == false);

        return SnailNvimQt::NvimConnector::connectToNvim(server);
    }
    else if(parser.isSet("spawn") && !parser.positionalArguments().isEmpty())
    {
        const QStringList &args = parser.positionalArguments();
        return SnailNvimQt::NvimConnector::startEmbedNvim(args.mid(1), args.at(0));
    }
    else
    {
        QStringList nvimArgs;
        nvimArgs << "--cmd";
        nvimArgs << "set termguicolors";

        // nvim program
        QString nvimProg;

        // environment nvim program check: $GKIDE_SNAIL_NVIMEXEC
        if(qEnvironmentVariableIsSet(ENV_GKIDE_SNAIL_NVIMEXEC))
        {
            QString nvim_bin = qgetenv(ENV_GKIDE_SNAIL_NVIMEXEC);

            qDebug() << "nvimProgEnv=" << nvim_bin;

            if(QFileInfo(nvim_bin).isExecutable())
            {
                nvimProg = nvim_bin;
            }
        }

        // fall back to the gkide-nvim default path: gkide/bin/nvim
        if(!QFileInfo(nvimProg).isExecutable())
        {
            // default nvim program: gkide/bin/nvim
            nvimProg = parser.value("nvim");
        }

        qDebug() << "nvimProg: " << nvimProg;

        // get the GKIDE install directory
        QDir gkideDir = QFileInfo(QCoreApplication::applicationDirPath()).dir();

        #ifdef Q_OS_MAC
        // within the bundle at: gkide/Resources/plg
        gkideDir.cd("Resources");
        #endif

        gkideDir.cd("plg");

        // default plugin directory check: gkide/plg
        if(gkideDir.exists())
        {
            QString plgDir = QString("let &rtp.=',%1'").arg(gkideDir.path());
            nvimArgs.insert(1, plgDir);
        }

        // environment plugin directory check: $GKIDE_NVIM_RTMPLG
        if(qEnvironmentVariableIsSet(ENV_GKIDE_NVIM_RTMPLG))
        {
            QString envPlgDir = qgetenv(ENV_GKIDE_NVIM_RTMPLG);

            qDebug() << "plgDirEnv=" << envPlgDir;

            if(QFileInfo(envPlgDir).isDir())
            {
                QString plgDir = QString("let &rtp.=',%1'").arg(envPlgDir);
                nvimArgs.insert(1, plgDir);
            }
        }

        // Pass positional file arguments to nvim
        nvimArgs.append(parser.positionalArguments());

        return SnailNvimQt::NvimConnector::startEmbedNvim(nvimArgs, nvimProg);
    }
}

} // namespace::SnailNvimQt
