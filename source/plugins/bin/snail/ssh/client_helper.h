/// @file plugins/bin/snail/ssh/client_helper.h

#ifndef PLUGIN_SNAIL_SSH_CLIENT_HELPER_H
#define PLUGIN_SNAIL_SSH_CLIENT_HELPER_H

#include <QObject>

#include "plugins/bin/snail/ssh/client.h"

namespace SnailNvimQt {

class SshClient;

class SshClientHelper: public QObject
{
    Q_OBJECT
public:
    SshClientHelper(SshClient *client);

public slots:
    void timeoutNoData(quint64 req_id);

private:
    SshClient *m_client;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_SSH_CLIENT_HELPER_H
