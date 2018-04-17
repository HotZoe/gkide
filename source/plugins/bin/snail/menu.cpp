/// @file plugins/bin/snail/menu.cpp

#include "plugins/bin/snail/menu.h"
#include "plugins/bin/snail/version.h"
#include "plugins/bin/snail/attributes.h"

namespace SnailNvimQt {

GkideMenu* GkideMenu::m_instance = NULL;

GkideMenu::GkideMenu(MainWindow *mw)
    : m_mainwin(mw)
{
    m_menuBar = new QMenuBar(m_mainwin); // menubar
    m_toolBar = new QToolBar(m_mainwin); // toolbar
    m_toolBar->setFixedHeight(30);
    m_toolBar->setMovable(false);
    m_toolBar->setVisible(false);

    // File
    m_file = new QMenu(m_menuBar);
    m_file->setTitle(QObject::tr("File"));

    // Edit
    m_edit = new QMenu(m_menuBar);
    m_edit->setTitle(QObject::tr("Edit"));

    // Search
    m_search = new QMenu(m_menuBar);
    m_search->setTitle(QObject::tr("Search"));

    // Project
    m_project = new QMenu(m_menuBar);
    m_project->setTitle(QObject::tr("Project"));

    // Options
    m_options = new QMenu(m_menuBar);
    m_options->setTitle(QObject::tr("Options"));

    // Tools
    m_tools = new QMenu(m_menuBar);
    m_tools->setTitle(QObject::tr("Tools"));

    // View
    m_view = new QMenu(m_menuBar);
    m_view->setTitle(QObject::tr("View"));

    // Window
    m_window = new QMenu(m_menuBar);
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
                                QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_file_OpenFileProject = new QAction(m_mainwin);
    m_file_OpenFileProject->setText("Open File or Project");
    m_file_OpenFileProject->setShortcut(Qt::CTRL | Qt::Key_O);
    m_file_OpenFileProject->setIcon(iconOpenFileProject);

    // File->Save
    QIcon iconSaveFile;
    iconSaveFile.addFile(":/menu/ico/file_save.png",
                         QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_file_SaveFile = new QAction(m_mainwin);
    m_file_SaveFile->setText("Save");
    m_file_SaveFile->setShortcut(Qt::CTRL | Qt::Key_S);
    m_file_SaveFile->setIcon(iconSaveFile);

    // Edit->Cut
    QIcon iconEditCut;
    iconEditCut.addFile(":/menu/ico/edit_cut.png",
                        QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_edit_Cut = new QAction(m_mainwin);
    m_edit_Cut->setText("Cut");
    m_edit_Cut->setShortcut(Qt::CTRL | Qt::Key_X);
    m_edit_Cut->setIcon(iconEditCut);

    // Edit->Copy
    QIcon iconEditCopy;
    iconEditCopy.addFile(":/menu/ico/edit_copy.png",
                         QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_edit_Copy = new QAction(m_mainwin);
    m_edit_Copy->setText("Copy");
    m_edit_Copy->setShortcut(Qt::CTRL | Qt::Key_C);
    m_edit_Copy->setIcon(iconEditCopy);

    // Edit->Paste
    QIcon iconEditPaste;
    iconEditPaste.addFile(":/menu/ico/edit_paste.png",
                          QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_edit_Paste = new QAction(m_mainwin);
    m_edit_Paste->setText("Paste");
    m_edit_Paste->setShortcut(Qt::CTRL | Qt::Key_P);
    m_edit_Paste->setIcon(iconEditPaste);

    // Edit->Undo
    QIcon iconEditUndo;
    iconEditUndo.addFile(":/menu/ico/edit_undo.png",
                         QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_edit_Undo = new QAction(m_mainwin);
    m_edit_Undo->setText("Undo");
    //m_edit_Undo->setShortcut(Qt::CTRL | Qt::Key_P);
    m_edit_Undo->setIcon(iconEditUndo);

    // Edit->Redo
    QIcon iconEditRedo;
    iconEditRedo.addFile(":/menu/ico/edit_redo.png",
                         QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_edit_Redo = new QAction(m_mainwin);
    m_edit_Redo->setText("Redo");
    //m_edit_Redo->setShortcut(Qt::CTRL | Qt::Key_P);
    m_edit_Redo->setIcon(iconEditRedo);

    // Search->Search
    QIcon iconSearchSearch;
    iconSearchSearch.addFile(":/menu/ico/search.png",
                             QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_search_Search = new QAction(m_mainwin);
    m_search_Search->setText("Search");
    //m_search_Search->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Search->setIcon(iconSearchSearch);

    // Search->Prev
    QIcon iconSearchPrev;
    iconSearchPrev.addFile(":/menu/ico/search_prev.png",
                           QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_search_Prev = new QAction(m_mainwin);
    m_search_Prev->setText("Prev");
    //m_search_Prev->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Prev->setIcon(iconSearchPrev);

    // Search->Next
    QIcon iconSearchNext;
    iconSearchNext.addFile(":/menu/ico/search_next.png",
                           QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_search_Next = new QAction(m_mainwin);
    m_search_Next->setText("Next");
    //m_search_Next->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Next->setIcon(iconSearchNext);

    // Search->Bookmark
    QIcon iconSearchBookmark;
    iconSearchBookmark.addFile(":/menu/ico/search_bookmark.png",
                               QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_search_Bookmark = new QAction(m_mainwin);
    m_search_Bookmark->setText("Bookmark");
    //m_search_Bookmark->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_Bookmark->setIcon(iconSearchBookmark);

    // Search->Prev Result
    QIcon iconSearchPrevResult;
    iconSearchPrevResult.addFile(":/menu/ico/search_prev_result.png",
                                 QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_search_PrevResult = new QAction(m_mainwin);
    m_search_PrevResult->setText("Prev Result");
    //m_search_PrevResult->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_PrevResult->setIcon(iconSearchPrevResult);

    // Search->Next Result
    QIcon iconSearchNextResult;
    iconSearchNextResult.addFile(":/menu/ico/search_next_result.png",
                                 QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_search_NextResult = new QAction(m_mainwin);
    m_search_NextResult->setText("Next Result");
    //m_search_NextResult->setShortcut(Qt::CTRL | Qt::Key_P);
    m_search_NextResult->setIcon(iconSearchNextResult);

    // Project->Build
    QIcon iconProjectBuild;
    iconProjectBuild.addFile(":/menu/ico/project_build.png",
                             QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_project_Build = new QAction(m_mainwin);
    m_project_Build->setText("Build");
    //m_project_Build->setShortcut(Qt::CTRL | Qt::Key_P);
    m_project_Build->setIcon(iconProjectBuild);

    // Options->Remote
    QIcon iconOptionsRemote;
    iconOptionsRemote.addFile(":/menu/ico/options_remote.png",
                              QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_options_Remote = new QAction(m_mainwin);
    m_options_Remote->setText("Remote");
    //m_options_Remote->setShortcut(Qt::CTRL | Qt::Key_P);
    m_options_Remote->setIcon(iconOptionsRemote);

    // Tools

    // View->Toolbar Show Hide
    QAction *a_view_ToolbarShowHide = m_toolBar->toggleViewAction();
    a_view_ToolbarShowHide->setCheckable(true);
    a_view_ToolbarShowHide->setChecked(false);
    a_view_ToolbarShowHide->setText("Toolbar Show/Hide");

    // Window

    // Help->About GKIDE
    m_help_AboutGKIDE = new QAction(m_mainwin);
    m_help_AboutGKIDE->setText("About GKIDE");
    QObject::connect(m_help_AboutGKIDE, &QAction::triggered,
                     this, &GkideMenu::trigerHelpAboutGKIDE);

    // Run GKIDE nvim language script
    QIcon iconRunNvl;
    iconRunNvl.addFile(":/menu/ico/run_nvl.png",
                       QSize(8, 8), QIcon::Normal, QIcon::Off);
    m_run_nvl = new QAction(m_mainwin);
    m_run_nvl->setText("Run NVL");
    m_run_nvl->setIcon(iconRunNvl);

    m_file->addAction(m_file_NewFileProject);
    m_file->addAction(m_file_OpenFileProject);
    m_file->addSeparator();
    m_file->addAction(m_file_SaveFile);
    m_edit->addAction(m_edit_Cut);
    m_edit->addAction(m_edit_Copy);
    m_edit->addAction(m_edit_Paste);
    m_edit->addAction(m_edit_Undo);
    m_edit->addAction(m_edit_Redo);
    m_search->addAction(m_search_Search);
    m_search->addAction(m_search_Prev);
    m_search->addAction(m_search_Next);
    m_search->addAction(m_search_Bookmark);
    m_search->addAction(m_search_PrevResult);
    m_search->addAction(m_search_NextResult);
    m_project->addAction(m_project_Build);
    m_options->addAction(m_options_Remote);
    //m_tools
    m_view->addAction(a_view_ToolbarShowHide);
    //m_window
    m_help->addAction(m_help_AboutGKIDE);

    m_toolBar->addAction(m_file_OpenFileProject);
    m_toolBar->addAction(m_file_SaveFile);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_edit_Cut);
    m_toolBar->addAction(m_edit_Copy);
    m_toolBar->addAction(m_edit_Paste);
    m_toolBar->addAction(m_edit_Undo);
    m_toolBar->addAction(m_edit_Redo);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_search_Prev);
    m_toolBar->addAction(m_search_Search);
    m_toolBar->addAction(m_search_Next);
    m_toolBar->addAction(m_search_PrevResult);
    m_toolBar->addAction(m_search_Bookmark);
    m_toolBar->addAction(m_search_NextResult);
    m_toolBar->addSeparator();
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

void GkideMenu::trigerHelpAboutGKIDE(bool VATTR_UNUSED_MATCH(act))
{
    GkideVersionInfo *info = new GkideVersionInfo(m_mainwin);
    info->show();
}

#define WIN_HIGHT   260
#define WIN_WIDTH   620

#define LOGO_X      20
#define LOGO_Y      50
#define LOGO_HIGHT  160
#define LOGO_WIDTH  160
GkideVersionInfo::GkideVersionInfo(MainWindow *mw)
{
    this->setParent(mw);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setWindowTitle("About GKIDE");
    this->setFixedSize(WIN_WIDTH, WIN_HIGHT);

    QImage gkideLogoImg(":/logo/gkide.png");
    QPixmap gkideLogoPixmap;
    gkideLogoPixmap.convertFromImage(gkideLogoImg);
    gkideLogoPixmap = gkideLogoPixmap.scaled(160, 160, Qt::KeepAspectRatio);
    m_imageLabel = new QLabel(this);
    m_imageLabel->setGeometry(QRect(LOGO_X, LOGO_Y, LOGO_WIDTH, LOGO_HIGHT));
    m_imageLabel->setPixmap(gkideLogoPixmap);

    #define LABEL_H     15  // hight
    #define LABEL_W     420 // width
    #define LABEL_X     180
    #define DELTA_Y     (9 + LABEL_H)

    int label_y = 20;

    QFont titleFont;
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleFont.setWeight(75);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H + 5));
    m_titleLabel->setAlignment(Qt::AlignCenter);//AlignLeft,
    m_titleLabel->setText("GKIDE Version v" GKIDE_RELEASE_VERSION);
    label_y += DELTA_Y + 7;

    QFont txtFont;
    txtFont.setPointSize(10);
    QFont progFont;
    progFont.setPointSize(12);
    progFont.setBold(true);
    progFont.setWeight(75);

    m_snailLabel = new QLabel(this);
    m_snailLabel->setFont(progFont);
    m_snailLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_snailLabel->setAlignment(Qt::AlignCenter);
    m_snailLabel->setText("snail v" SNAIL_VERSION_BASIC "-" SNAIL_RELEASE_TYPE);
    label_y += DELTA_Y;

    m_snailHashLabel = new QLabel(this);
    m_snailHashLabel->setFont(txtFont);
    m_snailHashLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_snailHashLabel->setAlignment(Qt::AlignCenter);
    m_snailHashLabel->setText("Reversion @" GIT_COMMIT_HASH);
    label_y += DELTA_Y;

    m_snailTimeLabel = new QLabel(this);
    m_snailTimeLabel->setFont(txtFont);
    m_snailTimeLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_snailTimeLabel->setAlignment(Qt::AlignCenter);
    // Build at 2017-12-02 12:32:54
    m_snailTimeLabel->setText("Build at " BUILD_TIMESTAMP);
    label_y += DELTA_Y;

    m_snailBuilderLabel = new QLabel(this);
    m_snailBuilderLabel->setFont(txtFont);
    m_snailBuilderLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_snailBuilderLabel->setAlignment(Qt::AlignCenter);
    //By XXXX on XXXX, windows7, x64, v16.04
    m_snailBuilderLabel->setText("By " BUILD_BY_USER
                                 " on " BUILD_ON_HOST
                                 ", " BUILD_OS_NAME
                                 ", " BUILD_OS_ARCH
                                 ", v" BUILD_OS_VERSION);
    label_y += DELTA_Y;

    NvimVersion *nvimVerObj = mw->getNvimConnector()->getNvimVersionObj();

    m_nvimLabel = new QLabel(this);
    m_nvimLabel->setFont(progFont);
    m_nvimLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_nvimLabel->setAlignment(Qt::AlignCenter);
    m_nvimLabel->setText(QString("nvim v")
                         + QString::number(nvimVerObj->nvimVersionMajor(), 10)
                         + QString(".")
                         + QString::number(nvimVerObj->nvimVersionMinor(), 10)
                         + QString(".")
                         + QString::number(nvimVerObj->nvimVersionPatch(), 10)
                         + QString("-")
                         + nvimVerObj->getBuildReleaseType()
                         + QString(", API(v")
                         + QString::number(nvimVerObj->nvimApiLevel(), 10)
                         + QString(")"));
    label_y += DELTA_Y;

    m_nvimHashLabel = new QLabel(this);
    m_nvimHashLabel->setFont(txtFont);
    m_nvimHashLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_nvimHashLabel->setAlignment(Qt::AlignCenter);
    m_nvimHashLabel->setText(QString("Reversion @")
                             + nvimVerObj->getBuildReversion());
    label_y += DELTA_Y;

    m_nvimTimeLabel = new QLabel(this);
    m_nvimTimeLabel->setFont(txtFont);
    m_nvimTimeLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_nvimTimeLabel->setAlignment(Qt::AlignCenter);
    // Build at 2017-12-02 12:32:54 +08000
    m_nvimTimeLabel->setText(QString("Build at ")
                             + nvimVerObj->getBuildTimestamp());
    label_y += DELTA_Y;

    m_nvimBuilderLabel = new QLabel(this);
    m_nvimBuilderLabel->setFont(txtFont);
    m_nvimBuilderLabel->setGeometry(QRect(LABEL_X, label_y, LABEL_W, LABEL_H));
    m_nvimBuilderLabel->setAlignment(Qt::AlignCenter);
    //By XXXX on XXXX, windows7, x64, v16.04
    m_nvimBuilderLabel->setText(QString("By ") + nvimVerObj->getBuildByUser()
                                + QString(" on ") + nvimVerObj->getBuildOnHost()
                                + QString(", ") + nvimVerObj->getBuildOsName()
                                + QString(", ") + nvimVerObj->getBuildOsArch()
                                + QString(", v") + nvimVerObj->getBuildOsVersion());
}

GkideVersionInfo::~GkideVersionInfo()
{
    delete m_imageLabel;
    delete m_titleLabel;
    delete m_snailLabel;
    delete m_snailHashLabel;
    delete m_snailTimeLabel;
    delete m_snailBuilderLabel;
    delete m_nvimLabel;
    delete m_nvimHashLabel;
    delete m_nvimTimeLabel;
    delete m_nvimBuilderLabel;
}

} // namespace::SnailNvimQt
