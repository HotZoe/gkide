/// @file plugins/bin/snail/menu.cpp

#include "plugins/bin/snail/menu.h"
#include "plugins/bin/snail/version.h"

namespace SnailNvimQt {

GkideMenu* GkideMenu::m_instance = NULL;

GkideMenu::GkideMenu(MainWindow *mw)
    : m_mainwin(mw)
{
    m_menuBar = new QMenuBar(m_mainwin); // menubar
    m_toolBar = new QToolBar(m_mainwin); // toolbar

    // File
    m_file = new QMenu(m_menuBar);
    m_file->setTitle(QObject::tr("File"));

    // Edit
    QMenu *m_edit = new QMenu(m_menuBar);
    m_edit->setTitle(QObject::tr("Edit"));

    // Search
    QMenu *m_search = new QMenu(m_menuBar);
    m_search->setTitle(QObject::tr("Search"));

    // Project
    QMenu *m_project = new QMenu(m_menuBar);
    m_project->setTitle(QObject::tr("Project"));

    // Options
    QMenu *m_options = new QMenu(m_menuBar);
    m_options->setTitle(QObject::tr("Options"));

    // Tools
    QMenu *m_tools = new QMenu(m_menuBar);
    m_tools->setTitle(QObject::tr("Tools"));

    // View
    QMenu *m_view = new QMenu(m_menuBar);
    m_view->setTitle(QObject::tr("View"));

    // Window
    QMenu *m_window = new QMenu(m_menuBar);
    m_window->setTitle(QObject::tr("Window"));

    // Help
    m_help = new QMenu(QObject::tr("Help"));
    m_help->setTitle(QObject::tr("Help"));

    // File->New File or Project ...
    m_file_NewFileProject = new QAction(m_mainwin);
    m_file_NewFileProject->setText("New File or Project");
    m_file_NewFileProject->setShortcut(Qt::CTRL | Qt::Key_N);

    // File->Open File or Project ...
    QIcon iconOpenFileProject;
    iconOpenFileProject.addFile(":/menu/ico/file_open.png",
                                QSize(16, 16), QIcon::Normal, QIcon::Off);
    m_file_OpenFileProject = new QAction(m_mainwin);
    m_file_OpenFileProject->setText("Open File or Project");
    m_file_OpenFileProject->setShortcut(Qt::CTRL | Qt::Key_O);
    m_file_OpenFileProject->setIcon(iconOpenFileProject);

    // File->Save
    QIcon iconSaveFile;
    iconSaveFile.addFile(":/menu/ico/file_save.png",
                         QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_file_SaveFile = new QAction(m_mainwin);
    m_file_SaveFile->setText("Save");
    m_file_SaveFile->setShortcut(Qt::CTRL | Qt::Key_S);
    m_file_SaveFile->setIcon(iconSaveFile);

    // Edit->Cut
    QIcon iconEditCut;
    iconEditCut.addFile(":/menu/ico/edit_cut.png",
                        QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_edit_Cut = new QAction(m_mainwin);
    m_edit_Cut->setText("Cut");
    m_edit_Cut->setShortcut(Qt::CTRL | Qt::Key_X);
    m_edit_Cut->setIcon(iconEditCut);

    // Edit->Copy
    QIcon iconEditCopy;
    iconEditCopy.addFile(":/menu/ico/edit_copy.png",
                         QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_edit_Copy = new QAction(m_mainwin);
    m_edit_Copy->setText("Copy");
    m_edit_Copy->setShortcut(Qt::CTRL | Qt::Key_C);
    m_edit_Copy->setIcon(iconEditCopy);

    // Edit->Paste
    QIcon iconEditPaste;
    iconEditPaste.addFile(":/menu/ico/edit_paste.png",
                         QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_edit_Paste = new QAction(m_mainwin);
    m_edit_Paste->setText("Paste");
    m_edit_Paste->setShortcut(Qt::CTRL | Qt::Key_P);
    m_edit_Paste->setIcon(iconEditPaste);

    // Edit->Undo
    QIcon iconEditUndo;
    iconEditUndo.addFile(":/menu/ico/edit_undo.png",
                         QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_edit_Undo = new QAction(m_mainwin);
    m_edit_Undo->setText("Undo");
    //m_edit_Undo->setShortcut(Qt::CTRL | Qt::Key_P);
    m_edit_Undo->setIcon(iconEditUndo);

    // Edit->Redo
    QIcon iconEditRedo;
    iconEditRedo.addFile(":/menu/ico/edit_redo.png",
                         QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_edit_Redo = new QAction(m_mainwin);
    m_edit_Redo->setText("Redo");
    //m_edit_Redo->setShortcut(Qt::CTRL | Qt::Key_P);
    m_edit_Redo->setIcon(iconEditRedo);

    // Search->Search
    QIcon iconSearchSearch;
    iconSearchSearch.addFile(":/menu/ico/search.png",
                             QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_search_Search = new QAction(m_mainwin);
    m_search_Search->setText("Search");
    //m_search_Search->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Search->setIcon(iconSearchSearch);

    // Search->Prev
    QIcon iconSearchPrev;
    iconSearchPrev.addFile(":/menu/ico/search_prev.png",
                             QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_search_Prev = new QAction(m_mainwin);
    m_search_Prev->setText("Prev");
    //m_search_Prev->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Prev->setIcon(iconSearchPrev);

    // Search->Next
    QIcon iconSearchNext;
    iconSearchNext.addFile(":/menu/ico/search_next.png",
                             QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_search_Next = new QAction(m_mainwin);
    m_search_Next->setText("Next");
    //m_search_Next->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Next->setIcon(iconSearchNext);

    // Search->Bookmark
    QIcon iconSearchBookmark;
    iconSearchBookmark.addFile(":/menu/ico/search_bookmark.png",
                             QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_search_Bookmark = new QAction(m_mainwin);
    m_search_Bookmark->setText("Bookmark");
    //m_search_Bookmark->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Bookmark->setIcon(iconSearchBookmark);

    // Search->Prev Result
    QIcon iconSearchPrevResult;
    iconSearchPrevResult.addFile(":/menu/ico/search_prev_result.png",
                             QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_search_PrevResult = new QAction(m_mainwin);
    m_search_PrevResult->setText("Prev Result");
    //m_search_PrevResult->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_PrevResult->setIcon(iconSearchPrevResult);

    // Search->Next Result
    QIcon iconSearchNextResult;
    iconSearchNextResult.addFile(":/menu/ico/search_next_result.png",
                                 QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_search_NextResult = new QAction(m_mainwin);
    m_search_NextResult->setText("Next Result");
    //m_search_NextResult->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_NextResult->setIcon(iconSearchNextResult);

    // Project->Build
    QIcon iconProjectBuild;
    iconProjectBuild.addFile(":/menu/ico/project_build.png",
                             QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_project_Build = new QAction(m_mainwin);
    m_project_Build->setText("Build");
    //m_project_Build->setShortcut(Qt::CTRL | Qt::Key_P);
    m_project_Build->setIcon(iconProjectBuild);

    // Options->Remote
    QIcon iconOptionsRemote;
    iconOptionsRemote.addFile(":/menu/ico/options_remote.png",
                               QSize(16, 16), QIcon::Normal, QIcon::Off);
    QAction *m_options_Remote = new QAction(m_mainwin);
    m_options_Remote->setText("Remote");
    //m_options_Remote->setShortcut(Qt::CTRL | Qt::Key_P);
    m_options_Remote->setIcon(iconOptionsRemote);

    // Help->About GKIDE
    m_help_AboutGKIDE = new QAction(m_mainwin);
    m_help_AboutGKIDE->setText("About GKIDE");
    QObject::connect(m_help_AboutGKIDE, &QAction::triggered,
                     this, &GkideMenu::trigerHelpAboutGKIDE);

     // Run GKIDE nvim language script
     QIcon iconRunNvl;
     iconRunNvl.addFile(":/menu/ico/run_nvl.png",
                        QSize(16, 16), QIcon::Normal, QIcon::Off);
     QAction *m_run_nvl = new QAction(m_mainwin);
     m_run_nvl->setText("Run NVL");
     m_run_nvl->setIcon(iconRunNvl);

    m_file->addAction(m_file_NewFileProject);
    m_file->addAction(m_file_OpenFileProject);

    m_help->addAction(m_help_AboutGKIDE);

    m_toolBar->addAction(m_file_OpenFileProject);
    m_toolBar->addAction(m_file_SaveFile);
    m_toolBar->addAction(m_edit_Cut);
    m_toolBar->addAction(m_edit_Copy);
    m_toolBar->addAction(m_edit_Paste);
    m_toolBar->addAction(m_edit_Undo);
    m_toolBar->addAction(m_edit_Redo);
    m_toolBar->addAction(m_search_Prev);
    m_toolBar->addAction(m_search_Search);
    m_toolBar->addAction(m_search_Next);
    m_toolBar->addAction(m_search_PrevResult);
    m_toolBar->addAction(m_search_Bookmark);
    m_toolBar->addAction(m_search_NextResult);
    m_toolBar->addAction(m_project_Build);
    m_toolBar->addAction(m_options_Remote);
    m_toolBar->addAction(m_run_nvl);

    m_menuBar->addAction(m_file->menuAction());
    m_menuBar->addAction(m_edit->menuAction());
    m_menuBar->addAction(m_search->menuAction());
    m_menuBar->addAction(m_project->menuAction());
    m_menuBar->addAction(m_options->menuAction());
    m_menuBar->addAction(m_tools->menuAction());
    m_menuBar->addAction(m_view->menuAction());
    m_menuBar->addAction(m_window->menuAction());
    m_menuBar->addAction(m_help->menuAction());

    m_mainwin->setMenuBar(m_menuBar);
    m_mainwin->addToolBar(Qt::TopToolBarArea, m_toolBar);
}

GkideMenu *GkideMenu::getGkideMenuInstance(MainWindow *mw, bool redo)
{
    if(redo)
    {
        delete m_instance;
        m_instance = new GkideMenu(mw);
    }

    if(m_instance == NULL)
    {
        m_instance = new GkideMenu(mw);
    }

    return m_instance;
}

GkideMenu::~GkideMenu()
{
    delete m_menuBar;
}

void GkideMenu::trigerHelpAboutGKIDE(bool act)
{
    GkideVersionInfo *info = new GkideVersionInfo(m_mainwin);
    info->show();
}

} // namespace::SnailNvimQt
