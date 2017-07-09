// Auto generated {{date}}
#ifndef LIBS_NVIM_AUTO_FUNC_IDX_H
#define LIBS_NVIM_AUTO_FUNC_IDX_H
enum FunctionId
{
    {% for f in functions %}
	NEOVIM_FN_{{ f.name.upper() }},
    {% endfor %}
	NEOVIM_FN_NULL
};
#endif // LIBS_NVIM_AUTO_FUNC_IDX_H
