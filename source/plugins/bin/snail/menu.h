/// @file plugins/bin/snail/menu.h

#ifndef PLUGIN_SNAIL_MENU_H
#define PLUGIN_SNAIL_MENU_H

#include <QObject>
#include <QMenu>
#include <QLabel>
#include <QDialog>
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
    QMenu *m_edit;
    QMenu *m_search;
    QMenu *m_project;
    QMenu *m_options;
    QMenu *m_tools;
    QMenu *m_view;
    QMenu *m_window;
    QMenu *m_help;

    QAction *m_file_NewFileProject;
    QAction *m_file_OpenFileProject;
    QAction *m_file_SaveFile;
    QAction *m_edit_Cut;
    QAction *m_edit_Copy;
    QAction *m_edit_Paste;
    QAction *m_edit_Undo;
    QAction *m_edit_Redo;
    QAction *m_search_Search;
    QAction *m_search_Prev;
    QAction *m_search_Next;
    QAction *m_search_Bookmark;
    QAction *m_search_PrevResult;
    QAction *m_search_NextResult;
    QAction *m_project_Build;
    QAction *m_options_Remote;
    QAction *m_help_AboutGKIDE;

    QAction *m_run_nvl;
private slots:
    void trigerHelpAboutGKIDE(bool act);
};

class GkideVersionInfo: public QDialog
{
    Q_OBJECT

public:
    GkideVersionInfo(MainWindow *mw = NULL);
    ~GkideVersionInfo();

private:
    QLabel *m_imageLabel;
    QLabel *m_titleLabel;
    QLabel *m_snailLabel;
    QLabel *m_snailHashLabel;
    QLabel *m_snailTimeLabel;
    QLabel *m_snailBuilderLabel;
    QLabel *m_nvimLabel;
    QLabel *m_nvimHashLabel;
    QLabel *m_nvimTimeLabel;
    QLabel *m_nvimBuilderLabel;

};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_MENU_H
