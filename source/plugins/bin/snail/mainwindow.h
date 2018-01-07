/// @file plugins/bin/snail/mainwindow.h

#ifndef SNAIL_LIBS_NVIMCORE_MAINWINDOW_H
#define SNAIL_LIBS_NVIMCORE_MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "plugins/bin/snail/nvimconnector.h"
#include "plugins/bin/snail/errorwidget.h"
#include "plugins/bin/snail/shell.h"

namespace SnailNvimQt {

class MainWindow: public QMainWindow
{
    Q_OBJECT
public:
    enum DelayedShow
    {
        Disabled,
        Normal,
        Maximized,
        FullScreen,
    };

    MainWindow(NvimConnector *, QWidget *parent=0);
    bool neovimAttached() const;
    Shell *shell();

public slots:
    void delayedShow(DelayedShow type=DelayedShow::Normal);

signals:
    void neovimAttached(bool);

protected:
    virtual void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
    virtual void changeEvent(QEvent *ev) Q_DECL_OVERRIDE;

private slots:
    void neovimSetTitle(const QString &title);
    void neovimWidgetResized();
    void neovimMaximized(bool);
    void neovimFullScreen(bool);
    void neovimGuiCloseRequest();
    void neovimExited(int status);
    void neovimError(NvimConnector::NeovimError);
    void reconnectNeovim();
    void showIfDelayed();
    void neovimAttachmentChanged(bool);

private:
    void init(NvimConnector *);
    NvimConnector *m_nvim;
    ErrorWidget *m_errorWidget;
    Shell *m_shell;
    DelayedShow m_delayedShow;
    QStackedWidget m_stack;
};

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_MAINWINDOW_H
