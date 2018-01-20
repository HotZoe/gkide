/// @file config/nvimapi/auto/nvim.h
///
/// Auto generated: UTC {{datetime}}

#ifndef CONFIG_NVIMAPI_AUTO_NVIM_H
#define CONFIG_NVIMAPI_AUTO_NVIM_H

#include <msgpack.h>
#include "plugins/bin/snail/nvimapi.h"

namespace SnailNvimQt {

class NvimConnector;
class MsgpackRequest;

class Nvim: public QObject
{
    Q_OBJECT
public:
    Nvim(NvimConnector *);

protected slots:
    void handleResponse(quint32 id, NvimApiFuncID fun, const QVariant &);
    void handleResponseError(quint32 id, NvimApiFuncID fun, const QVariant &);

signals:
    void error(const QString &errmsg, const QVariant &errObj);
    void neovimNotification(const QByteArray &name, const QVariantList &args);

private:
    NvimConnector *m_c;

public slots:
{%- for api in nvimAPIs %}
    {% if api.deprecated() %}
    /// @deprecated nvim API:
    {%- else %}
    /// nvim API:
    {%- endif %}
    {#- 'argStart': start -#}
    {#- 'argGoOn': go on the same line -#}
    {#- 'argNStart': prepare for next argument -#}
    {% set nvim_api_flg = 'argStart' %}
    {#- indent return length, 'stop' for stop -#}
    {% set api_ret_type = 'start' %}
    {% set api_ret_tount = 0 %}
    {%- for char in api.signature() -%}
        {%- if api_ret_type == 'start' -%}
            {%- if char == ' ' -%}
                {% set api_ret_type = 'stop' %}
            {%- else -%}
                {% set api_ret_tount = api_ret_tount+1 %}
            {%- endif -%}
        {%- endif -%}
        {%- if not loop.last -%}
            {#- skip the last third space -#}
            {%- if loop.index != loop.length - 2 -%}
                {%- if char == ',' -%}
                    {#- skip the last comma -#}
                    {%- if loop.index != loop.length - 3 -%}
                        {%- if nvim_api_flg == 'argGoOn' -%}
                            {%- set nvim_api_flg = 'argNStart' -%}
                        {%- endif -%}
                    {%- endif -%}
                {%- else -%}
                    {%- if nvim_api_flg == 'argStart' -%}
                        {%- set nvim_api_flg = 'argGoOn' -%}
    /// {{ char }}
                    {%- else -%}
                        {%- if nvim_api_flg == 'argNStart' -%}
                            {%- set nvim_api_flg = 'argGoOn' -%}
                            ,
    ///
                            {#- fixed indent for func -#}
                            {%- for var in range(api_ret_tount+3) -%}
                                {{' '}}
                            {%- endfor -%}
                            {#- dynamic indent for func -#}
                            {%- for var in api.name|list -%}
                                {{' '}}
                            {%- endfor -%}
                        {%- else -%}
                            {{ char }}{# for 'argGoOn' #}
                        {%- endif -%}
                    {%- endif -%}
                {%- endif -%}
            {%- endif -%}
        {%- else -%}
        ;
        {%- endif -%}
    {%- endfor %}
    MsgpackRequest *{{ api.name }}(
    {%- for char in api.argstring -%}
        {% if char == ',' -%}
            ,
            {# fixed indent for func #}
            {%- for var in [1,1,1,1,1,1,1,1] -%}
                {{' '}}
            {%- endfor -%}
            {# dynamic indent for func #}
            {%- for var in api.name|list -%}
                {{' '}}
            {%- endfor -%}
        {% else -%}
            {{ char }}
        {%- endif -%}
    {% endfor %});
{%- endfor %}

signals:
{%- for api in nvimAPIs %}
    void on_{{ api.name }}({{ api.return_type.native_type }});
    void err_{{ api.name }}(const QString &, const QVariant &);
{% endfor %}
};

} // namespace::SnailNvimQt

#endif // CONFIG_NVIMAPI_AUTO_NVIM_H

