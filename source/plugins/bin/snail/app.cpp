/// @file plugins/bin/snail/app.cpp

#include <QDir>
#include <QDialog>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextStream>
#include <QFileOpenEvent>

#include "generated/config/gkideenvs.h"
#include "plugins/bin/snail/app.h"
#include "plugins/bin/snail/logmanager.h"
#include "plugins/bin/snail/mainwindow.h"

namespace SnailNvimQt {

void App::appExit(QString reason, QString todo)
{
    QDialog *errDialog = new QDialog;

    errDialog->setWindowTitle("GKIDE Fatal Error");
    errDialog->setFixedSize(500, 100);

    QLabel *reasonLabel = new QLabel;
    reasonLabel->setAlignment(Qt::AlignCenter);
    reasonLabel->setText(reason);

    QLabel *todoLabel = new QLabel;
    todoLabel->setAlignment(Qt::AlignCenter);
    todoLabel->setText(todo);

    QPushButton *exitButton = new QPushButton;
    exitButton->setText("Exit");
    exitButton->setFixedSize(80, 30);
    QObject::connect(exitButton, &QPushButton::clicked,
                     errDialog, &QDialog::done);

    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addWidget(exitButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(reasonLabel);
    layout->addWidget(todoLabel);
    layout->addLayout(button_layout);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    errDialog->setLayout(layout);
    errDialog->show();
    errDialog->exec();

    ::exit(EXIT_FAILURE);
}

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

    // --server <address>
    QCommandLineOption arg_server("server");
    arg_desc = QCoreApplication::translate("main", "Connect to existing nvim instance.");
    arg_server.setDescription(arg_desc);
    arg_server.setValueName("server_addr");
    parser.addOption(arg_server);

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

    bool has_spawn = parser.isSet("spawn");
    bool has_server = parser.isSet("server");
    int exclusive = has_server + has_spawn;

    if(exclusive > 1)
    {
        appExit("Options --server, --spawn are mutually exclusive.",
                "Retry with new arguments.");
    }

    if(!parser.positionalArguments().isEmpty() && has_server)
    {
        appExit("Options --server do not accept positional arguments.",
                "Retry with new arguments.");
    }

    if(parser.positionalArguments().isEmpty() && has_spawn)
    {
        appExit("Option --spawn requires at least one positional argument.",
                "Retry with new arguments.");
    }
}

NvimConnector *App::createConnector(const QCommandLineParser &parser)
{
    if(parser.isSet("server"))
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

        // GKIDE home directory
        QString gkideHome = App::applicationDirPath();

        // nvim program
        #ifdef Q_OS_WIN
        QString nvimProg = gkideHome + "/nvim.exe";
        #else
        QString nvimProg = gkideHome + "/nvim";
        #endif

        qDebug() << "nvimProg: " << nvimProg;

        if(!QFileInfo(nvimProg).isExecutable())
        {
            appExit("Program not exit: " + nvimProg,
                    "Please check and reinstall.");
        }

        // Pass positional file arguments to nvim
        nvimArgs.append(parser.positionalArguments());

        return SnailNvimQt::NvimConnector::startEmbedNvim(nvimArgs, nvimProg);
    }
}

} // namespace::SnailNvimQt
