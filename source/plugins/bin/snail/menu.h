/// @file plugins/bin/snail/menu.h

#ifndef PLUGIN_SNAIL_MENU_H
#define PLUGIN_SNAIL_MENU_H

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>

#include "plugins/bin/snail/mainwindow.h"

namespace SnailNvimQt {

class MainWindow;

class GkideMenu: public QObject
{
    Q_OBJECT

public:
    ~GkideMenu();
    static GkideMenu *getGkideMenuInstance(MainWindow *mw, bool redo=false);

private:
    GkideMenu(MainWindow *mw);
    static GkideMenu *m_instance;

    MainWindow *m_mainwin;

    QMenuBar *m_menuBar;

    QMenu *m_fileMenu;
    QMenu *m_helpMenu;

private slots:
    void trigerHelpAboutGKIDE(bool act);
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_MENU_H
