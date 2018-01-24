/// @file plugins/bin/snail/msgpackrequest.cpp

#include "plugins/bin/snail/msgpackrequest.h"
#include "plugins/bin/snail/msgpackiodevice.h"

namespace SnailNvimQt {

/// Creates a new MsgpackRequest, identified by id
///
/// @see SnailNvimQt::MsgpackIODevice::msgId
MsgpackRequest::MsgpackRequest(quint32 id,
                               MsgpackIODevice *dev,
                               QObject *parent)
    : QObject(parent), m_msgid(id),
      m_dev(dev), m_funid(kNvimAPI_NULL)
{
    connect(&m_timer, &QTimer::timeout,
            this, &MsgpackRequest::requestTimeout);
}

/// The function id for the function signature associated with this call.
/// The value kNvimAPI_NULL indicates this call will not go through the
/// the generated function handlers.
NvimApiFuncID MsgpackRequest::funcId(void)
{
    return m_funid;
}

/// Associate a function id with this request
///
/// SnailNvimQt has auto-generated call handlers (in
/// SnailNvimQt::NvimConnector::neovimObject)
/// that will be used to process the response
void MsgpackRequest::setFunction(NvimApiFuncID f)
{
    m_funid = f;
}

void MsgpackRequest::setTimeout(int msec)
{
    m_timer.setInterval(msec);
    m_timer.setSingleShot(true);
    m_timer.start();
}

void MsgpackRequest::requestTimeout(void)
{
    emit timeout(this->m_msgid);
}

} // namespace::SnailNvimQt
