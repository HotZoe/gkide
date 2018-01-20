/// @file plugins/bin/snail/nvimconnectorhelper.cpp

#include <QMap>
#include "plugins/bin/snail/attributes.h"
#include "plugins/bin/snail/util.h"
#include "plugins/bin/snail/nvimconnector.h"
#include "plugins/bin/snail/msgpackrequest.h"
#include "plugins/bin/snail/msgpackiodevice.h"
#include "plugins/bin/snail/nvimconnectorhelper.h"

namespace SnailNvimQt {

/// The helper deals with Nvim API internals,
/// it handles msgpack responses on behalf of the connector.
NvimConnectorHelper::NvimConnectorHelper(NvimConnector *c)
    :QObject(c), m_c(c)
{
    /* nothing */
}

/// Handle Msgpack-rpc errors when fetching the API metadata
void NvimConnectorHelper::handleMetadataError(
        quint32 FUNC_ATTR_ARGS_UNUSED_REALY(msgid),
        NvimApiFunc::NvimApiFuncID,
        const QVariant &FUNC_ATTR_ARGS_UNUSED_REALY(errobj))
{
    m_c->setError(NvimConnector::NoMetadata,
                  tr("Unable to get Nvim api information"));
    return; /// @todo: better error message (from result?)
}

/// Process metadata object returned by Nvim
///
/// - Set channel_id
/// - Check if all functions we need are available
void NvimConnectorHelper::handleMetadata(
        quint32 FUNC_ATTR_ARGS_UNUSED_REALY(msgid),
        NvimApiFunc::NvimApiFuncID,
        const QVariant &result)
{
    const QVariantList asList = result.toList();

    if(asList.size() != 2
       || !asList.at(0).canConvert<quint64>()
       || !asList.at(1).canConvert<QVariantMap>())
    {
        m_c->setError(NvimConnector::UnexpectedMsg,
                      tr("Unable to unpack metadata response "
                         "description, unexpected data type"));
    }

    m_c->m_channel = asList.at(0).toUInt();
    const QVariantMap metadata = asList.at(1).toMap();
    QMapIterator<QString,QVariant> it(metadata);

    while(it.hasNext())
    {
        it.next();

        if(it.key() == "functions")
        {
            if(!checkFunctions(it.value().toList()))
            {
                m_c->setError(NvimConnector::APIMisMatch,
                              tr("API methods mismatch: "
                                 "Cannot connect to nvim instance, its version "
                                 "is too old, or the API has changed!"));
                return;
            }
        }
    }

    if(m_c->errorCause() == NvimConnector::NoError)
    {
        // Get &encoding before we signal readyness
        connect(m_c->neovimObject(), &Nvim::on_vim_get_option,
                this, &NvimConnectorHelper::encodingChanged);

        MsgpackRequest *r = m_c->neovimObject()->vim_get_option("encoding");
        connect(r, &MsgpackRequest::timeout,
                m_c, &NvimConnector::fatalTimeout);
        r->setTimeout(10000);
    }
    else
    {
        qWarning() << "Error retrieving metadata" << m_c->errorString();
    }
}

/// Called after metadata discovery, to get the &encoding
void NvimConnectorHelper::encodingChanged(const QVariant  &obj)
{
    disconnect(m_c->neovimObject(), &Nvim::on_vim_get_option,
               this, &NvimConnectorHelper::encodingChanged);

    m_c->m_dev->setEncoding(obj.toByteArray());
    const QByteArray enc_name = obj.toByteArray();

    if(m_c->m_dev->setEncoding(enc_name))
    {
        m_c->m_ready = true;
        emit m_c->ready();
    }
    else
    {
        qWarning() << "Unable to set encoding" << obj;
    }
}

/// Check function table from api_metadata[1]
/// Returns false if there is an API mismatch
bool NvimConnectorHelper::checkFunctions(const QVariantList &ftable)
{
    QList<NvimApiFunc::NvimApiFuncID> supported;

    foreach(const QVariant &val, ftable)
    {
        NvimApiFunc::NvimApiFuncID fid =
            NvimApiFunc::nvimApiID(NvimApiFunc::fromVariant(val));

        if(fid != NvimApiFunc::kNvimAPI_NULL)
        {
            supported.append(fid);
        }
    }

    // true if all the generated functions are supported
    return NvimApiFunc::nvimAPIs.size() == supported.size();
}

} // namespace::SnailNvimQt
