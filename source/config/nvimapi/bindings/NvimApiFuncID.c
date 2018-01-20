/// @enum NvimApiFuncID
///
/// Nvim API function identifiers ID, the list
/// SnailNvimQt::NvimApiFunc::nvimAPIs is indexed with this enum.
///
/// @note Bring in auto-generated nvim function API enum
///
/// Auto generated: UTC {{datetime}}
enum NvimApiFuncID
{
    {%- for api in nvimAPIs %}
    kNvimAPI_{{ api.name.upper() }},
    {%- endfor %}
    kNvimAPI_NULL
};

