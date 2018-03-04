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
    this->setFixedSize(610, 250);

    QImage gkideLogoImg(":/logo/gkide.png");
    QPixmap gkideLogoPixmap;
    gkideLogoPixmap.convertFromImage(gkideLogoImg);
    gkideLogoPixmap = gkideLogoPixmap.scaled(160, 160, Qt::KeepAspectRatio);
    m_imageLabel = new QLabel(this);
    m_imageLabel->setGeometry(QRect(30, 40, 160, 160));
    m_imageLabel->setPixmap(gkideLogoPixmap);

    QFont titleFont;
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleFont.setWeight(75);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setGeometry(QRect(260, 20, 250, 20));
    m_titleLabel->setText("GKIDE Version v1.0.0");

    QFont txtFont;
    txtFont.setPointSize(10);
    QFont progFont;
    progFont.setPointSize(12);
    progFont.setBold(true);
    progFont.setWeight(75);

    m_packageLabel = new QLabel(this);
    m_packageLabel->setFont(txtFont);
    m_packageLabel->setGeometry(QRect(210, 50, 390, 16));
    m_packageLabel->setText(GKIDE_PACKAGE_NAME);

    m_snailLabel = new QLabel(this);
    m_snailLabel->setFont(progFont);
    m_snailLabel->setGeometry(QRect(360, 70, 45, 16));
    m_snailLabel->setText("snail");

    m_snailHashLabel = new QLabel(this);
    m_snailHashLabel->setFont(txtFont);
    m_snailHashLabel->setGeometry(QRect(330, 90, 135, 16));
    m_snailHashLabel->setText("reversion @" GIT_COMMIT_HASH);

    m_snailTimeLabel = new QLabel(this);
    m_snailTimeLabel->setFont(txtFont);
    m_snailTimeLabel->setGeometry(QRect(300, 110, 200, 16));
    // Build at 2017-12-02 12:32:54
    m_snailTimeLabel->setText("Build at " BUILD_TIMESTAMP);

    m_snailBuilderLabel = new QLabel(this);
    m_snailBuilderLabel->setFont(txtFont);
    m_snailBuilderLabel->setGeometry(QRect(250, 130, 290, 16));
    //By XXXX on XXXX, windows7, x64, v16.04
    m_snailBuilderLabel->setText("By " BUILD_BY_USER
                                 " on " BUILD_ON_HOST
                                 ", " BUILD_OS_NAME
                                 ", " BUILD_OS_ARCH
                                 ", v" BUILD_OS_VERSION);

    m_nvimLabel = new QLabel(this);
    m_nvimLabel->setFont(progFont);
    m_nvimLabel->setGeometry(QRect(363, 150, 45, 16));
    m_nvimLabel->setText("nvim");

    m_nvimHashLabel = new QLabel(this);
    m_nvimHashLabel->setFont(txtFont);
    m_nvimHashLabel->setGeometry(QRect(330, 170, 135, 16));
    m_nvimHashLabel->setText("reversion @" GIT_COMMIT_HASH);

    m_nvimTimeLabel = new QLabel(this);
    m_nvimTimeLabel->setFont(txtFont);
    m_nvimTimeLabel->setGeometry(QRect(290, 190, 200, 16));
    // Build at 2017-12-02 12:32:54
    m_nvimTimeLabel->setText("Build at " BUILD_TIMESTAMP);

    m_nvimBuilderLabel = new QLabel(this);
    m_nvimBuilderLabel->setFont(txtFont);
    m_nvimBuilderLabel->setGeometry(QRect(250, 210, 290, 16));
    //By XXXX on XXXX, windows7, x64, v16.04
    m_nvimBuilderLabel->setText("By " BUILD_BY_USER
                                " on " BUILD_ON_HOST
                                ", " BUILD_OS_NAME
                                ", " BUILD_OS_ARCH
                                ", v" BUILD_OS_VERSION);

}

GkideVersionInfo::~GkideVersionInfo()
{
    delete m_imageLabel;
    delete m_titleLabel;
    delete m_packageLabel;
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
