/// @file plugins/bin/snail/mainwindow.h

#ifndef PLUGIN_SNAIL_MAINWINDOW_H
#define PLUGIN_SNAIL_MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

#include "plugins/bin/snail/menu.h"
#include "plugins/bin/snail/shell.h"
#include "plugins/bin/snail/errorwidget.h"
#include "plugins/bin/snail/nvimconnector.h"

namespace SnailNvimQt {

class GkideMenu;

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

    MainWindow(NvimConnector *c, QWidget *parent=0);
    bool neovimAttached(void) const;
    Shell *shell(void);

public slots:
    void delayedShow(DelayedShow type=DelayedShow::Normal);

signals:
    void neovimAttached(bool);

protected:
    virtual void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
    virtual void changeEvent(QEvent *ev) Q_DECL_OVERRIDE;

private slots:
    void neovimSetTitle(const QString &title);
    void neovimWidgetResized(void);
    void neovimMaximized(bool);
    void neovimFullScreen(bool);
    void neovimGuiCloseRequest(void);
    void neovimExited(int status);
    void neovimError(NvimConnector::NvimError);
    void reconnectNeovim(void);
    void showIfDelayed(void);
    void neovimAttachmentChanged(bool);

private:
    void init(NvimConnector *);

private:
    NvimConnector *m_nvimCon;
    ErrorWidget *m_errorWidget;
    Shell *m_shell;
    DelayedShow m_delayedShow;
    QStackedWidget m_stack;
    GkideMenu *m_menu;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_MAINWINDOW_H
