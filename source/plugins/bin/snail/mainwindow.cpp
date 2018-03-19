/// @file plugins/bin/snail/mainwindow.cpp

#include <QCloseEvent>

#include "plugins/bin/snail/mainwindow.h"

namespace SnailNvimQt {

MainWindow::MainWindow(NvimConnector *c, QWidget *parent)
    : QMainWindow(parent), m_nvimCon(NULL), m_errorWidget(NULL),
      m_shell(NULL), m_delayedShow(DelayedShow::Disabled)
{
    m_errorWidget = new ErrorWidget();
    m_stack.addWidget(m_errorWidget);

    connect(m_errorWidget, &ErrorWidget::reconnectNvim,
            this, &MainWindow::reconnectNvim);

    setCentralWidget(&m_stack);

    init(c);

    m_menu = GkideMenu::getGkideMenuInstance(this);
}

void MainWindow::init(NvimConnector *c)
{
    if(m_shell)
    {
        m_shell->deleteLater();
        m_stack.removeWidget(m_shell);
    }

    if(m_nvimCon)
    {
        m_nvimCon->deleteLater();
    }

    m_nvimCon = c;
    m_shell = new Shell(c);
    m_stack.insertWidget(1, m_shell);
    m_stack.setCurrentIndex(1);

    connect(m_shell, SIGNAL(neovimAttached(bool)),
            this, SLOT(neovimAttachmentChanged(bool)));

    connect(m_shell, SIGNAL(neovimTitleChanged(const QString &)),
            this, SLOT(neovimSetTitle(const QString &)));

    connect(m_shell, &Shell::neovimResized,
            this, &MainWindow::neovimWidgetResized);
    connect(m_shell, &Shell::neovimMaximized,
            this, &MainWindow::neovimMaximized);
    connect(m_shell, &Shell::neovimFullScreen,
            this, &MainWindow::neovimFullScreen);
    connect(m_shell, &Shell::neovimGuiCloseRequest,
            this, &MainWindow::neovimGuiCloseRequest);
    connect(m_nvimCon, &NvimConnector::processExited,
            this, &MainWindow::neovimExited);
    connect(m_nvimCon, &NvimConnector::error,
            this, &MainWindow::neovimError);

    m_shell->setFocus(Qt::OtherFocusReason);

    if(m_nvimCon->errorCause())
    {
        neovimError(m_nvimCon->errorCause());
    }
}

bool MainWindow::neovimAttached(void) const
{
    return (m_shell != NULL && m_shell->neovimAttached());
}

/// The Nvim process has exited
void MainWindow::neovimExited(int status)
{
    showIfDelayed();

    if(m_nvimCon->errorCause() != NvimConnector::NoError)
    {
        m_errorWidget->setText(m_nvimCon->errorString());
        m_errorWidget->showReconnect(m_nvimCon->canReconnect());
        m_stack.setCurrentIndex(0);
    }
    else if(status != 0)
    {
        m_errorWidget->setText(QString("Nvim exited with status code "
                                       "(%1)").arg(status));
        m_errorWidget->showReconnect(m_nvimCon->canReconnect());
        m_stack.setCurrentIndex(0);
    }
    else
    {
        close();
    }
}
void MainWindow::neovimError(NvimConnector::NvimError err)
{
    showIfDelayed();

    switch(err)
    {
        case NvimConnector::FailedToStart:
            m_errorWidget->setText("Unable to start nvim: "
                                   + m_nvimCon->errorString());
            break;

        default:
            m_errorWidget->setText(m_nvimCon->errorString());
    }

    m_errorWidget->showReconnect(m_nvimCon->canReconnect());
    m_stack.setCurrentIndex(0);
}

void MainWindow::neovimSetTitle(const QString &title)
{
    this->setWindowTitle(title);
}

void MainWindow::neovimWidgetResized(void)
{
    if(isMaximized() || isFullScreen())
    {
        m_shell->resizeNeovim(geometry().size());
    }
    else
    {
        m_shell->resizeNeovim(m_shell->size());
    }
}

void MainWindow::neovimMaximized(bool set)
{
    if(set)
    {
        setWindowState(windowState() | Qt::WindowMaximized);
    }
    else
    {
        setWindowState(windowState() & ~Qt::WindowMaximized);
    }
}

void MainWindow::neovimFullScreen(bool set)
{
    if(set)
    {
        setWindowState(windowState() | Qt::WindowFullScreen);
    }
    else
    {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    }
}

void MainWindow::neovimGuiCloseRequest(void)
{
    QMainWindow::close();
}

void MainWindow::reconnectNvim(void)
{
    if(m_nvimCon->canReconnect())
    {
        init(m_nvimCon->reconnect());
    }

    m_stack.setCurrentIndex(1);
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    // Never unless the Nvim shell closes too
    if(m_shell->close())
    {
        QWidget::closeEvent(ev);
    }
    else
    {
        ev->ignore();
    }
}
void MainWindow::changeEvent(QEvent *ev)
{
    if(ev->type() == QEvent::WindowStateChange && isWindow())
    {
        m_shell->updateGuiWindowState(windowState());
    }

    QWidget::changeEvent(ev);
}

/// Call show() after a 1s delay or when Nvim attachment
/// is complete, whichever comes first
void MainWindow::delayedShow(DelayedShow type)
{
    m_delayedShow = type;

    if(m_nvimCon->errorCause() != NvimConnector::NoError)
    {
        showIfDelayed();
        return;
    }

    if(type != DelayedShow::Disabled)
    {
        QTimer *t = new QTimer(this);
        t->setSingleShot(true);
        t->setInterval(1000);
        connect(m_shell, &Shell::neovimResized,
                this, &MainWindow::showIfDelayed);
        connect(t, &QTimer::timeout,
                this, &MainWindow::showIfDelayed);
        t->start();
    }
}

void MainWindow::showIfDelayed(void)
{
    if(!isVisible())
    {
        if(m_delayedShow == DelayedShow::Normal)
        {
            show();
        }
        else if(m_delayedShow == DelayedShow::Maximized)
        {
            showMaximized();
        }
        else if(m_delayedShow == DelayedShow::FullScreen)
        {
            showFullScreen();
        }
    }

    m_delayedShow = DelayedShow::Disabled;
}

void MainWindow::neovimAttachmentChanged(bool attached)
{
    emit neovimAttached(attached);

    if(isWindow() && m_shell != NULL)
    {
        m_shell->updateGuiWindowState(windowState());
    }
}

Shell *MainWindow::shell(void)
{
    return m_shell;
}

NvimConnector *MainWindow::getNvimConnector(void)
{
    return m_nvimCon;
}

} // namespace::SnailNvimQt

