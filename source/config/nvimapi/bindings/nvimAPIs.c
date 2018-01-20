/// nvim API functions, indexed by SnailNvimQt::NvimApiFuncID
///
/// @note also see NvimApiFuncID
///
/// Auto generated: UTC {{datetime}}
const QList<NvimApiFunc> NvimApiFunc::nvimAPIs = QList<NvimApiFunc>()
{% for api in nvimAPIs -%}
<< NvimApiFunc("{{ api.return_type.neovim_type }}", // return type
               "{{ api.name }}", // function name
	           QList<QString>()
               {%- for param in api.parameters %}
                   << QString("{{ param.neovim_type }}")
	           {%- endfor %},
	           false)
{% endfor -%}
;

