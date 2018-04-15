if arg[1] == '--help' then
    print('Usage: genoptions.lua src/nvim options_file')
    os.exit(0)
end

local nvimsrcdir = arg[1]
local options_file = arg[2]

package.path = nvimsrcdir .. '/?.lua;' .. package.path

local opt_fd = io.open(options_file, 'w')

local options = require('options')

cstr = options.cstr

local type_flags=
{
    bool='kOptAttrBool',
    number='kOptAttrNumber',
    string='kOptAttrString',
}

local redraw_flags=
{
    statuslines='kOptAttrRStatusLine',
    current_window='kOptAttrRCurWindow',
    current_window_only='kOptAttrRCurWinOnly',
    current_buffer='kOptAttrRCurBuffer',
    all_windows='kOptAttrRAllWindow',
    everything='kOptAttrREverything',
    curswant='kOptAttrCursorWant',
}

local list_flags=
{
    comma='kOptAttrCommaList',
    onecomma='kOptAttrCommaListOne',
    flags='kOptAttrFlagList',
    flagscomma='kOptAttrCommaList | kOptAttrFlagList',
}

local get_flags = function(o)
    local ret = {type_flags[o.type]}

    local add_flag = function(f) 
        ret[1] = ret[1] .. '\n                 | ' .. f 
    end

    if o.list then
        add_flag(list_flags[o.list])
    end

    if o.redraw then
        for _, r_flag in ipairs(o.redraw) do
            add_flag(redraw_flags[r_flag])
        end
    end

    if o.expand then
        add_flag('kOptAttrExpand')

        if o.expand == 'nodefault' then
            add_flag('kOptAttrNoExpDefVal')
        end
    end

    local namedefs = {
        {'alloced',             'kOptAttrAllocate'},
        {'nodefault',           'kOptAttrNoDefaultVal'},
        {'no_mkrc',             'kOptAttrNoMkrcOut'},
        {'vi_def',              'kOptAttrUseViDefVal'},
        {'vim',                 'kOptAttrNvimOptOnly'},
        {'secure',              'kOptAttrSecure'},
        {'gettext',             'kOptAttrGettext'},
        {'noglob',              'kOptAttrNoGlobal'},
        {'normal_fname_chars',  'kOptAttrNFNameChar'},
        {'pri_mkrc',            'kOptAttrPriMkrc'},
        {'deny_in_modelines',   'kOptAttrNoModeline'},
        {'deny_duplicates',     'kOptAttrNoDuplicate'},
    }

    for _, flag_desc in ipairs(namedefs) do
        local key_name = flag_desc[1]
        local def_name = flag_desc[2]

        if o[key_name] then
            add_flag(def_name)
        end
    end

    return ret[1]
end

local get_cond = function(c, base_string)
    local cond_string = base_string or '#if '
  
    if type(c) == 'table' then
        cond_string = cond_string .. get_cond(c[1], '')

        for i, subc in ipairs(c) do
            if i > 1 then
                cond_string = cond_string .. ' && ' .. get_cond(subc, '')
            end
        end
    elseif c:sub(1, 1) == '!' then
        cond_string = cond_string .. '!defined(' .. c:sub(2) .. ')'
    else
        cond_string = cond_string .. 'defined(' .. c .. ')'
    end

    return cond_string
end

value_dumpers = 
{
    ['function']=function(v) return v() end,
    string=cstr,
    boolean=function(v) return v and 'true' or 'false' end,
    number=function(v) return ('%iL'):format(v) end,
    ['nil']=function(v) return '0L' end,
}

local get_value = function(v)
    return '(uchar_kt *) ' .. value_dumpers[type(v)](v)
end

local get_defaults = function(d)
    return ('{' .. get_value(d.vi) .. ', ' .. get_value(d.vim) .. '}')
end

local w = function(s, flag)
    if flag then
        opt_fd:write(s .. ',\n')
    else
        opt_fd:write(s .. '\n')
    end
end

local defines = {}

local dump_option = function(i, o)
    w('    [' .. ('%u'):format(i - 1) .. '] = {')
    w('        .fullname = ' .. cstr(o.full_name), true)

    if o.abbreviation then
        w('        .shortname = ' .. cstr(o.abbreviation), true)
    end

    w('        .flags = ' .. get_flags(o), true)

    if o.enable_if then
        w(get_cond(o.enable_if))
    end

    if o.varname then
        w('        .var =(uchar_kt *)&' .. o.varname, true)
    elseif #o.scope == 1 and o.scope[1] == 'window' then
        w('        .var = VAR_WIN', true)
    end

    if o.enable_if then
        w('#endif')
    end

    if #o.scope == 1 and o.scope[1] == 'global' then
        w('        .indir = PV_NONE', true)
    else
        assert (#o.scope == 1 or #o.scope == 2)
        assert (#o.scope == 1 or o.scope[1] == 'global')

        local min_scope = o.scope[#o.scope]
        local varname = o.pv_name 
                        or o.varname 
                        or ('p_' .. (o.abbreviation or o.full_name))
        local pv_name = ('OPT_' .. min_scope:sub(1, 3):upper() 
                         .. '(' .. (min_scope:sub(1, 1):upper() 
                                    .. 'V_' .. varname:sub(3):upper()) .. ')')
        if #o.scope == 2 then
        pv_name = 'OPT_BOTH(' .. pv_name .. ')'
        end

        defines['PV_' .. varname:sub(3):upper()] = pv_name
        w('        .indir = ' .. pv_name, true)
    end

    if o.defaults then
        if o.defaults.condition then
            w(get_cond(o.defaults.condition))
        end

        w('        .def_val = ' .. get_defaults(o.defaults.if_true), true)
    
        if o.defaults.condition then
            if o.defaults.if_false then
                w('#else')
                w('        .def_val = ' .. get_defaults(o.defaults.if_false), true)
            end
            w('#endif')
        end
    end

    w('    },')
end

-- wriet the option table
w('// please note that the following table must sort by alphabetical order!')
w('static nvimoption_st options[] = {')

for i, o in ipairs(options.options) do
    dump_option(i, o)
end

w('    [' .. ('%u'):format(#options.options) .. '] = { .fullname = NULL }')
w('};')
w('')

-- write the defines
for k, v in pairs(defines) do
    w('#define ' .. k .. ' ' .. v)
end

opt_fd:close()
