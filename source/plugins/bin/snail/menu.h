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

class GkideMenu
{
public:
    ~GkideMenu();
    static GkideMenu *getGkideMenuInstance(MainWindow *mw, bool redo=false);

private:
    GkideMenu(void);
    GkideMenu(MainWindow *mw);
    static GkideMenu *m_instance;

    QMenuBar *m_menuBar;

    QMenu *m_fileMenu;
    QMenu *m_helpMenu;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_MENU_H
