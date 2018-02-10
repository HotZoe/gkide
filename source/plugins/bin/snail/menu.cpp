/// @file plugins/bin/snail/menu.cpp

#include "plugins/bin/snail/menu.h"

namespace SnailNvimQt {

GkideMenu* GkideMenu::m_instance = NULL;

GkideMenu::GkideMenu(void)
{}

GkideMenu::GkideMenu(MainWindow *mw)
{
    m_menuBar = new QMenuBar(mw);

    m_fileMenu = new QMenu(QObject::tr("File"));
    m_fileMenu->addAction(QObject::tr("Open File"));
    m_fileMenu->addAction(QObject::tr("Open Project"));
    //m_fileMenu->addSeparator();
    //m_fileMenu->addSection("Section");
    m_menuBar->addMenu(m_fileMenu);

    m_helpMenu = new QMenu(QObject::tr("Help"));
    m_helpMenu->addAction(QObject::tr("About GKIDE"));
    m_menuBar->addMenu(m_helpMenu);
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

    delete m_fileMenu;
    delete m_helpMenu;
}

} // namespace::SnailNvimQt
