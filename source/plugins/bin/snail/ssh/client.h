/// @file plugins/bin/snail/ssh/client.h

#ifndef PLUGIN_SNAIL_SSH_CLIENT_H
#define PLUGIN_SNAIL_SSH_CLIENT_H

#include <QObject>

#include <libssh/libssh.h>

#include "plugins/bin/snail/ssh/sshdefs.h"
#include "plugins/bin/snail/ssh/request.h"
#include "plugins/bin/snail/ssh/client_helper.h"

namespace SnailNvimQt {

class SshClientHelper;

class SshClient: public QObject
{
    Q_OBJECT
public:
    explicit SshClient(SshAuthInfo auth);
    ~SshClient(void);

    bool login(void);

    ssh_channel newChannel(void);
    SshAuthInfo getAuthInfo(void) const;

    SshRequest *remoteExecute(QString cmd, QString args);
    SshRequest *remoteExecute(ssh_channel chl, QString cmd, QString args);

signals:
    void dataAvailable(quint64 req_id, QByteArray &buffer);

protected:
    quint64 newRequestId(void);
    ssh_session sshSessionInit(void) const;
    bool verifyKnownhost(ssh_session session) const;
    ssh_auth_e authenticateConsole(ssh_session session) const;
    ssh_auth_e authKeyboardInteractive(ssh_session session,
                                       const char *password) const;
private:
    quint64 m_req_id;
    SshAuthInfo m_auth;
    ssh_session m_session;
    ssh_channel m_channel;
    SshClientHelper *m_helper;
    QHash<quint64, SshRequest *> m_requests;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_SSH_CLIENT_H
