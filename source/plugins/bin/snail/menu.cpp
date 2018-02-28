/// @file plugins/bin/snail/menu.cpp

#include "plugins/bin/snail/menu.h"
#include "plugins/bin/snail/version.h"

namespace SnailNvimQt {

GkideMenu* GkideMenu::m_instance = NULL;

GkideMenu::GkideMenu(MainWindow *mw)
    : m_mainwin(mw)
{
    // menu bar
    m_menuBar = new QMenuBar(mw);

    QAction *ptr = NULL;

    // File
    m_fileMenu = new QMenu(QObject::tr("File"));
    m_fileMenu->setParent(m_menuBar);
    m_menuBar->addMenu(m_fileMenu);
    // file->New File or Project ...
    ptr = new QAction("New File or Project ...", m_fileMenu);
    ptr->setShortcut(Qt::CTRL | Qt::Key_N);
    m_fileMenu->addAction(ptr);
    // file->Open File or Project ...
    ptr = new QAction("Open File or Project ...", m_fileMenu);
    ptr->setShortcut(Qt::CTRL | Qt::Key_O);
    m_fileMenu->addAction(ptr);
    //m_fileMenu->addSeparator();
    //m_fileMenu->addSection("Section");

    // Help
    m_helpMenu = new QMenu(QObject::tr("Help"));
    m_helpMenu->setParent(m_menuBar);
    m_menuBar->addMenu(m_helpMenu);
    // Help->About GKIDE
    ptr = new QAction("About GKIDE", m_helpMenu);
    m_helpMenu->addAction(ptr);
    QObject::connect(ptr, &QAction::triggered,
                     this, &GkideMenu::trigerHelpAboutGKIDE);
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
