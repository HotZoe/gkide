/// @file plugins/bin/snail/menu.cpp

#include "plugins/bin/snail/menu.h"
#include "plugins/bin/snail/version.h"

namespace SnailNvimQt {

GkideMenu* GkideMenu::m_instance = NULL;

GkideMenu::GkideMenu(MainWindow *mw)
    : m_mainwin(mw)
{
    // menubar
    m_menuBar = new QMenuBar(mw);
    mw->setMenuBar(m_menuBar);

    // File
    m_fileMenu = new QMenu(m_menuBar);
    m_fileMenu->setTitle(QObject::tr("File"));

    // Help
    m_helpMenu = new QMenu(QObject::tr("Help"));
    m_helpMenu->setTitle(QObject::tr("Help"));

    // File->New File or Project ...
    QAction *file_NewFileProject = new QAction(mw);
    file_NewFileProject->setText("New File or Project ...");
    file_NewFileProject->setShortcut(Qt::CTRL | Qt::Key_N);

    // File->Open File or Project ...
    QAction *file_OpenFileProject = new QAction(mw);
    file_OpenFileProject->setText("Open File or Project ...");
    file_OpenFileProject->setShortcut(Qt::CTRL | Qt::Key_O);

    // Help->About GKIDE
    QAction *help_AboutGKIDE = new QAction(mw);
    help_AboutGKIDE->setText("About GKIDE");
    QObject::connect(help_AboutGKIDE, &QAction::triggered,
                     this, &GkideMenu::trigerHelpAboutGKIDE);

    m_menuBar->addAction(m_fileMenu->menuAction());
    m_fileMenu->addAction(file_NewFileProject);
    m_fileMenu->addAction(file_OpenFileProject);
    m_menuBar->addAction(m_helpMenu->menuAction());
    m_helpMenu->addAction(help_AboutGKIDE);
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
