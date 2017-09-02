/// @file

#include "snail/libs/nvimcore/msgpackrequest.h"
#include "snail/libs/nvimcore/msgpackiodevice.h"

namespace SnailNvimQt {

/// @class SnailNvimQt::MsgpackRequest
///
/// @brief A MsgpackRequest represents an ongoing API call

/// @fn SnailNvimQt::MsgpackRequest::finished
///
/// @brief The request has finished

/// Creates a new MsgpackRequest, identified by id
///
/// @see SnailNvimQt::MsgpackIODevice::msgId
MsgpackRequest::MsgpackRequest(quint32 id, MsgpackIODevice *dev, QObject *parent)
    :QObject(parent), id(id), m_dev(dev), m_function(Function::NEOVIM_FN_NULL)
{
    connect(&m_timer, &QTimer::timeout, this, &MsgpackRequest::requestTimeout);
}

/// The function id for the function signature associated with this call.
/// The value NEOVIM_FN_NULL indicates this call will not go through the
/// the generated function handlers.
Function::FunctionId MsgpackRequest::function()
{
    return m_function;
}

/// Associate a function id with this request
///
/// SnailNvimQt has auto-generated call handlers (in SnailNvimQt::NvimConnector::neovimObject)
/// that will be used to process the response
void MsgpackRequest::setFunction(Function::FunctionId f)
{
    m_function = f;
}

void MsgpackRequest::setTimeout(int msec)
{
    m_timer.setInterval(msec);
    m_timer.setSingleShot(true);
    m_timer.start();
}

void MsgpackRequest::requestTimeout()
{
    emit timeout(this->id);
}

} // [Namespace] SnailNvimQt
