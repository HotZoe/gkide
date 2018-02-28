/// @file plugins/bin/snail/version.cpp

#include "plugins/bin/snail/version.h"

namespace SnailNvimQt {

NvimVersion::NvimVersion(void)
    : m_major(0), m_minor(0), m_patch(0),
      m_api_level(0), m_api_compatible(0), m_api_prerelease(false)
{
    /* nothing */
}

NvimVersion::~NvimVersion(void)
{
    /* nothing */
}

bool NvimVersion::checkNvimVersion(void)
{
    return (m_major >= NVIM_VERSION_MAJOR)
           || (m_minor >= NVIM_VERSION_MINOR)
           || (m_patch >= NVIM_VERSION_PATCH);
}
bool NvimVersion::checkNvimApiVersion(void)
{
    if(m_api_level >= NVIM_API_VERSION)
    {
        return true;
    }

    if(m_api_compatible >= NVIM_API_VERSION)
    {
        return true;
    }

    return false;
}
bool NvimVersion::setNvimVersionInfo(QVariantMap info)
{
    QMapIterator<QString,QVariant> it(info);

    while(it.hasNext())
    {
        it.next();

        if(it.key() == "major")
        {
            m_major = it.value().toInt();
        }
        else if(it.key() == "minor")
        {
            m_minor = it.value().toInt();
        }
        else if(it.key() == "patch")
        {
            m_patch = it.value().toInt();
        }
        else if(it.key() == "api_level")
        {
            m_api_level = it.value().toInt();
        }
        else if(it.key() == "api_compatible")
        {
            m_api_compatible = it.value().toInt();
        }
        else if(it.key() == "api_prerelease")
        {
            m_api_prerelease = it.value().toBool();
        }
        else
        {
            return false;
        }

    }

    return true;
}

GkideVersionInfo::GkideVersionInfo(QWidget *parent)
{
    this->setParent(parent);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_DeleteOnClose, true);

    this->setWindowTitle("About GKIDE");
    this->setFixedSize(550, 260);

    m_winLayout = new QGridLayout(this);

    QImage gkideLogoImg(":/gkide.png");
    QPixmap gkideLogoPixmap;
    gkideLogoPixmap.convertFromImage(gkideLogoImg);
    gkideLogoPixmap = gkideLogoPixmap.scaled(150, 150, Qt::KeepAspectRatio);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setGeometry(10, 10, 150, 150);
    m_imageLabel->setPixmap(gkideLogoPixmap);
    // image (0, 0) - (1, 1), which takes (1 row 1 column)
    m_winLayout->addWidget(m_imageLabel, 0, 0, 1, 1);

    //long width = gkideLogoPixmap.width();
    //long height = gkideLogoPixmap.height();
    //QString info;
    //info.append("width:");
    //info.append(QString::number(width, 10));
    //info.append("height:");
    //info.append(QString::number(height, 10));

    GkideVersion gkideInfo;
    QString brief("GKIDE version ");
    brief = brief + gkideInfo.getVersionString();
    m_briefLabel = new QLabel(brief, this);
    m_winLayout->addWidget(m_briefLabel, 0, 1);
}

GkideVersionInfo::~GkideVersionInfo()
{
    delete m_briefLabel;
    delete m_imageLabel;
    delete m_winLayout;
}

} // namespace::SnailNvimQt
