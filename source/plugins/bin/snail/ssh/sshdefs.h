/// @file plugins/bin/snail/ssh/sshdefs.h

#ifndef PLUGIN_SNAIL_SSH_SSHDEFS_H
#define PLUGIN_SNAIL_SSH_SSHDEFS_H

#include <QDebug>
#include <QString>

namespace SnailNvimQt {

struct SshAuthInfo
{
    QString m_host; ///< host address
    quint32 m_port; ///< host port
    QString m_user; ///< host login name
    QString m_pass; ///< host login pass

    bool m_verbose;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_SSH_SSHDEFS_H
