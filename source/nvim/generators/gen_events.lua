if arg[1] == '--help' then
    print('Usage: gen_events.lua src/nvim enum_file event_names_file')
    os.exit(0)
end

local nvimsrcdir = arg[1]
local fileio_enum_file = arg[2]
local names_file = arg[3]

package.path = nvimsrcdir .. '/?.lua;' .. package.path

local auevents = require('auevents')
local events = auevents.events
local aliases = auevents.aliases

enum_tgt = io.open(fileio_enum_file, 'w')
names_tgt = io.open(names_file, 'w')

enum_tgt:write('typedef enum auto_event_e\n{')
names_tgt:write([[
static const struct event_name_s
{
    size_t len;
    char *name;
    auto_event_et event;
} event_names[] =
{]])

for i, event in ipairs(events) do
    if i > 1 then
        comma = ',\n'
    else
        comma = '\n'
    end

    enum_tgt:write(('%s    EVENT_%s = %u'):format(comma, 
                                                  event:upper(), 
                                                  i - 1))
    names_tgt:write(('%s    { %u, "%s", EVENT_%s }'):format(comma, 
                                                            #event, 
                                                            event, 
                                                            event:upper()))
end

for alias, event in pairs(aliases) do
    names_tgt:write((',\n    { %u, "%s", EVENT_%s }'):format(#alias, 
                                                             alias, 
                                                             event:upper()))
end

names_tgt:write(',\n    { 0, NULL, (auto_event_et)0 }')

enum_tgt:write('\n} auto_event_et;\n')
names_tgt:write('\n};\n')

enum_tgt:write(('\n#define NUM_EVENTS %u\n'):format(#events))
names_tgt:write('\nstatic autopat_st *first_autopat[NUM_EVENTS] =\n{\n')

line_len = 1
new_line = true
for i = 1,((#events) - 1) do
    if new_line then
        line_len = line_len + #('    NULL,')
    else
        line_len = line_len + #(' NULL,')
    end
  
    if line_len > 80 then
        new_line = true
        names_tgt:write('\n')
        line_len = 1 + #('    NULL,')
    end

    if new_line then
        new_line = false
        names_tgt:write('    NULL,')
    else
        names_tgt:write(' NULL,')
    end
end

if line_len + #(' NULL') > 80 then
    names_tgt:write('\n  NULL')
else
    names_tgt:write(' NULL')
end

names_tgt:write('\n};\n')

enum_tgt:close()
names_tgt:close()
