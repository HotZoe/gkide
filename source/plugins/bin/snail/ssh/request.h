/// @file plugins/bin/snail/ssh/request.h

#ifndef PLUGIN_SNAIL_SSH_REQUEST_H
#define PLUGIN_SNAIL_SSH_REQUEST_H

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QByteArray>

#include <libssh/libssh.h>

namespace SnailNvimQt {

class SshRequest: public QObject
{
    Q_OBJECT
public:
    SshRequest(quint64 req_id, ssh_session session, ssh_channel channel);

    quint64 getReqId(void) const;
    ssh_session getSession(void) const;
    ssh_channel getChannel(void) const;

    void setTimeoutStart(int msec);

signals:
    void timeoutNoData(quint64 req_id);
    void dataAvailable(quint64 req_id, QByteArray &buffer);

private slots:
    void requestTimeout(void);

private:
    quint64 m_req_id;
    ssh_session m_session;
    ssh_channel m_channel;

    QTimer m_timer;
    QByteArray m_buffer;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_SSH_REQUEST_H
