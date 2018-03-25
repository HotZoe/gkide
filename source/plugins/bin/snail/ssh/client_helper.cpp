/// @file plugins/bin/snail/ssh/client_helper.h

#include "plugins/bin/snail/ssh/client_helper.h"

namespace SnailNvimQt {

SshClientHelper::SshClientHelper(SshClient *client)
    : m_client(client)
{
    /* nothing to do */
}

void SshClientHelper::timeoutNoData(quint64 req_id)
{
    qDebug() << "SshClientHelper::timeoutNoData: " << req_id;
}

} // namespace::SnailNvimQt
