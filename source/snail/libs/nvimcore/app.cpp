#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QFileOpenEvent>
#include "logmanager.h"
#include "snail/app/envdefs.h"
#include "snail/libs/nvimcore/app.h"
#include "snail/libs/nvimcore/mainwindow.h"

namespace SnailNvimQt
{

App::App(int &argc, char **argv): QApplication(argc, argv)
{
	setWindowIcon(QIcon(":/neovim.png"));
    setApplicationDisplayName("GKIDE");

#ifdef Q_OS_MAC
	QByteArray shellPath = qgetenv("SHELL");
    if(!getLoginEnvironment(shellPath))
    {
		getLoginEnvironment("/bin/bash");
	}
#endif

    if(!qgetenv(ENV_LOG_FILE).isEmpty())
    {
        qInstallMessageHandler(logging_handler);
    }
    else
    {
        qInstallMessageHandler(logging_nothing);
    }
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
#ifdef NEOVIMQT_GUI_WIDGET
	SnailNvimQt::Shell *win = new SnailNvimQt::Shell(c);
	win->show();
    if(parser.isSet("fullscreen"))
    {
		win->showFullScreen();
    }
    else if(parser.isSet("maximized"))
    {
		win->showMaximized();
    }
    else
    {
		win->show();
	}
#else
	SnailNvimQt::MainWindow *win = new SnailNvimQt::MainWindow(c);

	QObject::connect(instance(), SIGNAL(openFilesTriggered(const QList<QUrl>)),
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
#endif
}

/// Initialize CLI parser with all the snail arguments, process the provided arguments and
/// check for errors.
///
/// When appropriate this function will call QCommandLineParser::showHelp() terminating the program.
void App::processCliOptions(QCommandLineParser &parser, const QStringList &arguments)
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
    arg_nvim.setDefaultValue(App::applicationDirPath() + "/nvim");
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
    arg_server.setValueName("geometry");// TODO,
    parser.addOption(arg_geometry);

    // --help
	parser.addHelpOption();

    arg_desc = QCoreApplication::translate("main", "Edit specified file(s).");
    parser.addPositionalArgument("file", arg_desc, "[file...]");

    arg_desc = QCoreApplication::translate("main", "Additional arguments forwarded to nvim.");
    parser.addPositionalArgument("...", arg_desc, "[-- ...]");

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
        qWarning() << "Options --server, --spawn and --embed are mutually exclusive.\n";
        ::exit(EXIT_FAILURE);
	}

    if(!parser.positionalArguments().isEmpty() && (has_embed || has_server))
    {
        qWarning() << "Options --embed and --server do not accept positional arguments.\n";
        ::exit(EXIT_FAILURE);
	}

    if(parser.positionalArguments().isEmpty() && has_spawn)
    {
        qWarning() << "Option --spawn requires at least one positional argument.\n";
        ::exit(EXIT_FAILURE);
	}
}

NvimConnector* App::createConnector(const QCommandLineParser& parser)
{
    if(parser.isSet("embed"))
    {
		return SnailNvimQt::NvimConnector::fromStdinOut();
    }
    else if(parser.isSet("server"))
    {
		QString server = parser.value("server");
		return SnailNvimQt::NvimConnector::connectToNeovim(server);
    }else if(parser.isSet("spawn") && !parser.positionalArguments().isEmpty())
    {
		const QStringList& args = parser.positionalArguments();
		return SnailNvimQt::NvimConnector::spawn(args.mid(1), args.at(0));
    }
    else
    {
		QStringList neovimArgs;
		neovimArgs << "--cmd";
		neovimArgs << "set termguicolors";
		auto path = qgetenv("NVIM_QT_RUNTIME_PATH");
        if(QFileInfo(path).isDir())
        {
			neovimArgs.insert(0, "--cmd");
			neovimArgs.insert(1, QString("let &rtp.=',%1'").arg(QString::fromLocal8Bit(path)));
		}
#ifdef NVIM_QT_RUNTIME_PATH
        else if(QFileInfo(NVIM_QT_RUNTIME_PATH).isDir())
        {
			neovimArgs.insert(0, "--cmd");
			neovimArgs.insert(1, QString("let &rtp.=',%1'").arg(NVIM_QT_RUNTIME_PATH));
        }
        else
#endif
		{
			// Look for the runtime relative to the nvim-qt binary
			QDir d = QFileInfo(QCoreApplication::applicationDirPath()).dir();
#ifdef Q_OS_MAC
			// within the bundle at ../Resources/runtime
			d.cd("Resources");
			d.cd("runtime");
#else
			// ../share/nvim-qt/runtime
			d.cd("share");
			d.cd("nvim-qt");
			d.cd("runtime");
#endif
            if(d.exists())
            {
				neovimArgs.insert(0, "--cmd");
				neovimArgs.insert(1, QString("let &rtp.=',%1'").arg(d.path()));
			}
		}

		// Pass positional file arguments to Neovim
		neovimArgs.append(parser.positionalArguments());
		return SnailNvimQt::NvimConnector::spawn(neovimArgs, parser.value("nvim"));
	}
}

} // [Namespace] SnailNvimQt
