/// @file config/nvimapi/auto/nvim.cpp
///
/// Auto generated: UTC {{datetime}}

#include "config/nvimapi/auto/nvim.h"

#include "plugins/bin/snail/attributes.h"
#include "plugins/bin/snail/util.h"
#include "plugins/bin/snail/nvimconnector.h"
#include "plugins/bin/snail/msgpackrequest.h"
#include "plugins/bin/snail/msgpackiodevice.h"

namespace SnailNvimQt {

/// Unpack Nvim EXT types: Window, Buffer Tabpage, which are all uint64_t
/// see Nvim:msgpack_rpc_to_
QVariant unpackBuffer(MsgpackIODevice *FUNC_ATTR_ARGS_UNUSED_REALY(dev),
                      const char *in,
                      quint32 size)
{
    msgpack_unpacked result;
    msgpack_unpacked_init(&result);
    msgpack_unpack_return ret = msgpack_unpack_next(&result, in, size, NULL);
    msgpack_unpacked_destroy(&result);

    if(ret != MSGPACK_UNPACK_SUCCESS)
    {
        return QVariant();
    }

    return QVariant((quint64)result.data.via.u64);
}

#define unpackWindow  unpackBuffer
#define unpackTabpage unpackBuffer

Nvim::Nvim(NvimConnector *c) :m_c(c)
{
    // Ext types of msgpack
{%- for extName in apiExtTypes %}
    m_c->m_dev->registerExtType({{ apiExtTypes[extName] }}, unpack{{ extName }});
{%- endfor %}

    connect(m_c->m_dev, &MsgpackIODevice::notification,
            this, &Nvim::neovimNotification);
}

// Slots
{%- for api in nvimAPIs %}
MsgpackRequest *Nvim::{{ api.name }}(
    {%- for char in api.argstring -%}
        {% if char == ',' -%}
            ,
            {# fixed indent for func #}
            {%- for var in [1,1,1,1,1,1,1,1,1,1] -%}
                {{' '}}
            {%- endfor -%}
            {# dynamic indent for func #}
            {%- for var in api.name|list -%}
                {{' '}}
            {%- endfor -%}
        {% else -%}
            {{ char }}
        {%- endif -%}
    {% endfor %})
{
    MsgpackRequest *r = 
        m_c->m_dev->startRequestUnchecked("{{ api.name }}", {{ api.argcount }});

    r->setFunction(kNvimAPI_{{ api.name.upper() }});

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    {% for arg in api.parameters -%}
    m_c->m_dev->{{ arg.sendmethod }}({{ arg.name }});
    {% endfor -%}

    return r;
}
{% endfor %}

// Handlers
void Nvim::handleResponseError(quint32 FUNC_ATTR_ARGS_UNUSED_REALY(msgid),
                               NvimApiFuncID fun,
                               const QVariant &res)
{
    // TODO: support Neovim error types Exception/Validation/etc
    QString errMsg;
    const QVariantList asList = res.toList();

    if(asList.size() >= 2)
    {
        if(asList.at(1).canConvert<QByteArray>())
        {
            errMsg = m_c->m_dev->decode(asList.at(1).toByteArray());
        }
        else
        {
            errMsg = tr("Received unsupported Neovim error type");
        }
    }

    switch(fun)
    {
        {%- for api in nvimAPIs %}
        case kNvimAPI_{{ api.name.upper() }}:
            emit err_{{ api.name }}(errMsg, res);
            break;
        {% endfor -%}

        default:
            m_c->setError(NvimConnector::RuntimeMsgpackError,
                          QString("Received error for function "
                                  "that should not fail: %s").arg(fun));
    }
}

void Nvim::handleResponse(quint32 FUNC_ATTR_ARGS_UNUSED_REALY(msgid),
                          NvimApiFuncID fun,
                          const QVariant &res)
{
    switch(fun)
    {
    {% for api in nvimAPIs %}
        case kNvimAPI_{{ api.name.upper() }}:
        {
        {%- if api.return_type.native_type != 'void' %}
            {{ api.return_type.native_type }} data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for {{ api.name }}");
                return;
            }
            else
            {
                emit on_{{ api.name }}(data);
            }
        {% else %}
            emit on_{{ api.name }}();
        {%- endif -%}
        }
        break;
    {% endfor %}
        default:
            qWarning() << "Received unexpected response";
    }
}

} // [Namespace] SnailNvimQt
