local luapathcfg = require('config')
package.path  = luapathcfg.path .. package.path
package.cpath = luapathcfg.cpath .. package.cpath

-- http://www.inf.puc-rio.br/~roberto/lpeg/
lpeg = require('lpeg')

-- lpeg grammar for building api metadata from a set of header files.
-- It ignores comments and preprocessor commands and parses a very small subset
-- of C prototypes with a limited set of types

local P  = lpeg.P  -- lpeg.P(string)  Matches string literally
local R  = lpeg.R  -- lpeg.R("xy")    Matches any character between x and y (Range)
local S  = lpeg.S  -- lpeg.S(string)  Matches any character in string (Set)

local C  = lpeg.C  -- lpeg.C(patt)            the match for patt plus all captures made by patt
local Ct = lpeg.Ct -- lpeg.Ct(patt)           a table with all captures from patt
local Cc = lpeg.Cc -- lpeg.Cc(values)         the given values (matches the empty string)
local Cg = lpeg.Cg -- lpeg.Cg(patt [, name])  the values produced by patt, optionally tagged with name

local any    = P(1) -- consume one character

local num    = R('09') -- match any number character
local letter = R('az', 'AZ') + S('_$')
local alpha  = letter + num

local nl     = P('\r\n') + P('\n')
local not_nl = any - nl

local ws   = S(' \t') + nl
local fill = ws ^ 0

local c_comment = P('//') * (not_nl ^ 0)
local c_preproc = P('#')  * (not_nl ^ 0)

local typed_container =
    (P('ArrayOf(') + P('DictionaryOf('))
    * ((any - P(')')) ^ 1)
    * P(')')

local c_id   = (typed_container + (letter * (alpha ^ 0)))
local c_void = P('void')

local c_param_type =
    (((P('error_st')  * fill * P('*') * fill) * Cc('error'))
     + (C(c_id) * (ws ^ 1)))

local c_type = (C(c_void) * (ws ^ 1)) + c_param_type

local c_param = Ct(c_param_type * C(c_id))
local c_param_list = c_param * (fill * (P(',') * fill * c_param) ^ 0)
local c_params = Ct(c_void + c_param_list)

local c_proto = Ct(
    Cg(c_type, 'return_type')       -- function return type
    * Cg(c_id, 'name')              -- function name
    * fill * P('(') * fill          -- the open parentheses
    * Cg(c_params, 'parameters')    -- the function arguments list
    * fill * P(')')                 -- the close parentheses
    * Cg(Cc(false), 'async')
    * (fill * Cg((P('FUNC_API_SINCE(') * C(num ^ 1)) * P(')'), 'since')^ -1)
    * (fill * Cg((P('FUNC_API_DEPRECATED_SINCE(') * C(num^ 1)) * P(')'), 'deprecated_since')^ -1)
    * (fill * Cg((P('FUNC_API_ASYNC') * Cc(true)), 'async')^ -1)
    * (fill * Cg((P('FUNC_API_NOEXPORT') * Cc(true)), 'noexport')^ -1)
    * (fill * Cg((P('FUNC_API_REMOTE_ONLY') * Cc(true)), 'remote_only')^ -1)
    * (fill * Cg((P('FUNC_API_REMOTE_IMPL') * Cc(true)), 'remote_impl')^ -1)
    * (fill * Cg((P('FUNC_API_BRIDGE_IMPL') * Cc(true)), 'bridge_impl')^ -1)
    * fill * P(';')
    )

local grammar = Ct((c_proto + c_comment + c_preproc + ws) ^ 1)

local function grammar_debug_print(func)
    print('function type: ' .. func.return_type)
    print('function name: ' .. func.name)

    print('function flag: async            = ' .. tostring(func.async))
    print('function flag: since            = ' .. tostring(func.since))
    print('function flag: deprecated_since = ' .. tostring(func.deprecated_since))
    print('function flag: noexport         = ' .. tostring(func.noexport))
    print('function flag: remote_only      = ' .. tostring(func.remote_only))
    print('function flag: remote_impl      = ' .. tostring(func.remote_impl))
    print('function flag: bridge_impl      = ' .. tostring(func.bridge_impl))

    print('\n')
end

return {
    grammar = grammar,
    typed_container = typed_container,
    func_dbg_print = grammar_debug_print
}
