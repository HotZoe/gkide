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
        else if(it.key() == "build_timestamp")
        {
            m_buildTimestamp = it.value().toString();
        }
        else if(it.key() == "build_reversion")
        {
            m_buildReversion = it.value().toString();
        }
        else if(it.key() == "build_by_user")
        {
            m_buildByUser = it.value().toString();
        }
        else if(it.key() == "build_on_host")
        {
            m_buildOnHost = it.value().toString();
        }
        else if(it.key() == "build_os_name")
        {
            m_buildOsName = it.value().toString();
        }
        else if(it.key() == "build_os_arch")
        {
            m_buildOsArch = it.value().toString();
        }
        else if(it.key() == "build_os_version")
        {
            m_buildOsVersion = it.value().toString();
        }
        else if(it.key() == "build_release_type")
        {
            m_buildReleaseType = it.value().toString();
        }
        else
        {
            return false;
        }
    }

    return true;
}

} // namespace::SnailNvimQt
