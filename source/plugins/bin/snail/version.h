/// @file plugins/bin/snail/version.h

#ifndef PLUGIN_SNAIL_VERSION_H
#define PLUGIN_SNAIL_VERSION_H

#include <QVariant>
#include "generated/config/miscinfo.h"
#include "generated/config/gkideversion.h"

namespace SnailNvimQt {

class NvimVersion
{
public:
    NvimVersion(void);
    ~NvimVersion();

    bool checkNvimVersion(void);
    bool checkNvimApiVersion(void);
    bool setNvimVersionInfo(QVariantMap info);

    int nvimVersionMajor(void)
    {
        return m_major;
    }

    int nvimVersionMinor(void)
    {
        return m_minor;
    }

    int nvimVersionPatch(void)
    {
        return m_patch;
    }

    int nvimApiLevel(void)
    {
        return m_api_level;
    }

    int nvimApiCompatible(void)
    {
        return m_api_compatible;
    }

    bool nvimApiPrerelease(void)
    {
        return m_api_prerelease;
    }

    int bindNvimVersionMajor(void)
    {
        return NVIM_VERSION_MAJOR;
    }

    int bindNvimVersionMinor(void)
    {
        return NVIM_VERSION_MINOR;
    }

    int bindNvimVersionPatch(void)
    {
        return NVIM_VERSION_PATCH;
    }

    const char *bindNvimVersionString(void)
    {
        return NVIM_VERSION_BASIC;
    }

    int bindNvimVersionInt32(void)
    {
        return NVIM_VERSION_INT32;
    }

    int bindNvimApiLevel(void)
    {
        return NVIM_API_VERSION;
    }

    int bindNvimApiCompatible(void)
    {
        return NVIM_API_COMPATIBLE;
    }

    bool bindNvimApiPrerelease(void)
    {
        return NVIM_API_PRERELEASE;
    }

private:
    int m_major; ///< nvim runtime major version
    int m_minor; ///< nvim runtime minor version
    int m_patch; ///< nvim runtime patch version

    int m_api_level;       ///< nvim runtime API level
    int m_api_compatible;  ///< nvim runtime API compatible level
    bool m_api_prerelease; ///< nvim runtime API prerelease
};

class SnailVersion
{
public:
    SnailVersion(void);
    ~SnailVersion();

    int getVersionMajor(void)
    {
        return SNAIL_VERSION_MAJOR;
    }

    int getVersionMinor(void)
    {
        return SNAIL_VERSION_MINOR;
    }

    int getVersionPatch(void)
    {
        return SNAIL_VERSION_PATCH;
    }

    const char *getVersionString(void)
    {
        return SNAIL_VERSION_BASIC;
    }

    int getVersionInt32(void)
    {
        return SNAIL_VERSION_INT32;
    }
};

class GkideVersion
{
public:
    GkideVersion(void);
    ~GkideVersion();

    int getVersionMajor(void)
    {
        return GKIDE_VERSION_MAJOR;
    }

    int getVersionMinor(void)
    {
        return GKIDE_VERSION_MINOR;
    }

    int getVersionPatch(void)
    {
        return GKIDE_VERSION_PATCH;
    }

    const char *getVersionString(void)
    {
        return GKIDE_RELEASE_VERSION;
    }

    int getVersionInt32(void)
    {
        return GKIDE_VERSION_INT32;
    }

    const char *getReleaseHash(void)
    {
        return GKIDE_RELEASE_HASH;
    }

    const char *getReleaseTime(void)
    {
        return GKIDE_RELEASE_TIME;
    }

    const char *getReleaseType(void)
    {
        return GKIDE_RELEASE_TYPE;
    }

    const char *getPackageName(void)
    {
        return GKIDE_PACKAGE_NAME;
    }
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_VERSION_H
