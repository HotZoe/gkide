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
    QToolBar *m_toolBar;

    QMenu *m_file;
    QMenu *m_help;

    QAction *m_file_NewFileProject;
    QAction *m_file_OpenFileProject;
    QAction *m_help_AboutGKIDE;

private slots:
    void trigerHelpAboutGKIDE(bool act);
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_MENU_H
