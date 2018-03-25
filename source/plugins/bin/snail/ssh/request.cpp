/// @file plugins/bin/snail/ssh/request.cpp

#include "plugins/bin/snail/ssh/request.h"

namespace SnailNvimQt {

SshRequest::SshRequest(quint64 req_id,
                       ssh_session session,
                       ssh_channel channel)
{
    m_req_id = req_id;
    m_session = session;
    m_channel = channel;

    connect(&m_timer, &QTimer::timeout,
            this, &SshRequest::requestTimeout);
}

quint64 SshRequest::getReqId(void) const
{
    return m_req_id;
}

ssh_session SshRequest::getSession(void) const
{
    return m_session;
}

ssh_channel SshRequest::getChannel(void) const
{
    return m_channel;
}

void SshRequest::setTimeoutStart(int msec)
{
    m_timer.setInterval(msec);
    m_timer.setSingleShot(true);
    m_timer.start();

    //emit dataAvailable(m_buffer);
}

void SshRequest::requestTimeout(void)
{
    emit timeoutNoData(m_req_id);
}

} // namespace::SnailNvimQt
