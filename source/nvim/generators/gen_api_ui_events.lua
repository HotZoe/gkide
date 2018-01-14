local luapathcfg = require('config')
package.path  = luapathcfg.path .. package.path
package.cpath = luapathcfg.cpath .. package.cpath

mpack = require('mpack')

local nvimdir = arg[1]
package.path = nvimdir .. '/?.lua;' .. package.path

assert(#arg == 7)

ui_event_func = io.open(arg[2], 'rb')          -- input file
ui_func_proto_output = io.open(arg[3], 'wb')   -- output: func proto
ui_func_call_output  = io.open(arg[4], 'wb')   -- output: func calls
ui_func_remote_output = io.open(arg[5], 'wb')  -- output: func remote
ui_func_bridge_output = io.open(arg[6], 'wb')  -- output: func bridge
ui_func_metadata_output = io.open(arg[7], 'wb')-- output: func metadata  

c_grammar = require('generators.c_grammar')
local ui_event_apis = c_grammar.grammar:match(ui_event_func:read('*all'))

function write_signature(output, ev, prefix, notype)
    output:write('(' .. prefix)

    if prefix == "" and #ev.parameters == 0 then
        output:write('void')
    end

    for j = 1, #ev.parameters do
        if j > 1 or prefix ~= '' then
            output:write(', ')
        end

        local param = ev.parameters[j]
        if not notype then
            output:write(param[1]..' ')
        end

        output:write(param[2])
    end

    output:write(')')
end

function write_arglist(output, ev, need_copy)
    output:write('    Array args = ARRAY_DICT_INIT;\n')
  
    for j = 1, #ev.parameters do
        local param = ev.parameters[j]
        local kind = string.upper(param[1])
        local do_copy = need_copy and (kind == "ARRAY" or kind == "DICTIONARY" or kind == "STRING")
        output:write('    ADD(args, ')

        if do_copy then
            output:write('copy_object(')
        end

        output:write(kind..'_OBJ('..param[2]..')')

        if do_copy then
            output:write(')')
        end

        output:write(');\n')
    end
end

for i = 1, #ui_event_apis do
    ev = ui_event_apis[i]
    assert(ev.return_type == 'void')

    if ev.since == nil then
        print("ui_st event "..ev.name.." lacks since field.\n")
        os.exit(1)
    end

    ev.since = tonumber(ev.since)

    if not ev.remote_only then
        -- ui event function proto
        ui_func_proto_output:write('    void (*' .. ev.name .. ')')
        write_signature(ui_func_proto_output, ev, 'ui_st *ui') -- func args
        ui_func_proto_output:write(';\n')

        if not ev.remote_impl then
            ui_func_remote_output:write('static void remote_ui_' .. ev.name)
            write_signature(ui_func_remote_output, ev, 'ui_st *ui') -- func args
            ui_func_remote_output:write('\n{\n')
            write_arglist(ui_func_remote_output, ev, true)
            ui_func_remote_output:write('    push_call(ui, "' .. ev.name .. '", args);\n')
            ui_func_remote_output:write('}\n\n')
        end

        if not ev.bridge_impl then
            send, argv, recv, recv_argv, recv_cleanup = '', '', '', '', ''
            argc = 1

            for j = 1, #ev.parameters do
                local param = ev.parameters[j]
                copy = 'copy_'..param[2]

                if param[1] == 'String' then
                    send = send .. '  String copy_' .. param[2] 
                           .. ' = copy_string(' .. param[2] .. ');\n'
                    argv = argv .. ', ' .. copy 
                           .. '.data, INT2PTR(' .. copy .. '.size)'
                    recv = (recv .. '  String ' .. param[2] 
                           .. ' = (String){.data = argv[' .. argc .. '],' 
                           .. '.size = (size_t)argv[' .. (argc+1) .. ']};\n')
                    recv_argv = recv_argv .. ', ' .. param[2]
                    recv_cleanup = recv_cleanup .. '  api_free_string(' 
                                   .. param[2] .. ');\n'
                    argc = argc+2
                elseif param[1] == 'Array' then
                    send = send .. '  Array copy_' .. param[2] 
                           .. ' = copy_array(' .. param[2] .. ');\n'
                    argv = argv .. ', ' .. copy .. '.items, INT2PTR(' .. copy .. '.size)'
                    recv = (recv .. '  Array ' .. param[2] 
                           .. ' = (Array){.items = argv[' .. argc .. '],'
                           .. '.size = (size_t)argv[' .. (argc+1) .. ']};\n')
                    recv_argv = recv_argv .. ', ' .. param[2]
                    recv_cleanup = recv_cleanup .. '  api_free_array(' .. param[2] .. ');\n'
                    argc = argc+2
                elseif param[1] == 'Integer' or param[1] == 'Boolean' then
                    argv = argv .. ', INT2PTR(' .. param[2] .. ')'
                    recv_argv = recv_argv .. ', PTR2INT(argv[' .. argc .. '])'
                    argc = argc+1
                else
                    assert(false)
                end
            end

        ui_func_bridge_output:write('// nvim-api: ' .. ev.name .. '()\n')
        ui_func_bridge_output:write('static void ui_bridge_' .. ev.name .. '_event(void **argv)\n') 
        ui_func_bridge_output:write('{\n') -- func body
        ui_func_bridge_output:write('    ui_st *ui = UI_PTR(argv[0]);\n')
        ui_func_bridge_output:write(recv)
        ui_func_bridge_output:write('    ui->' .. ev.name .. '(ui' .. recv_argv .. ');\n')
        ui_func_bridge_output:write(recv_cleanup)
        ui_func_bridge_output:write('}\n')

        ui_func_bridge_output:write('static void ui_bridge_' .. ev.name)
        write_signature(ui_func_bridge_output, ev, 'ui_st *ui') -- func args
        ui_func_bridge_output:write('\n{\n') -- func body 
        ui_func_bridge_output:write(send)
        ui_func_bridge_output:write('    UI_CALL(ui, ')
        ui_func_bridge_output:write(ev.name .. ', ')
        ui_func_bridge_output:write(argc .. ', ui')
        ui_func_bridge_output:write(argv .. ');\n')
        ui_func_bridge_output:write('}\n\n')
        end
    end

    ui_func_call_output:write('void ui_call_' .. ev.name)
    write_signature(ui_func_call_output, ev, '')
    ui_func_call_output:write('\n{\n')

    if ev.remote_only then
        write_arglist(ui_func_call_output, ev, false)
        ui_func_call_output:write('    ui_event("' .. ev.name .. '", args);\n')
    else
        ui_func_call_output:write('    UI_CALL')
        write_signature(ui_func_call_output, ev, ev.name, true)
        ui_func_call_output:write(";\n")
    end

    ui_func_call_output:write("}\n\n")
end

ui_event_func:close()
ui_func_proto_output:close()
ui_func_call_output:close()
ui_func_remote_output:close()
ui_func_bridge_output:close()

-- don't expose internal attributes like "impl_name" in public metadata
exported_attributes = {
    'name', 
    'parameters',
    'since', 
    'deprecated_since'
}

exported_events = { }

for _,ev in ipairs(ui_event_apis) do
    local ev_exported = {}

    for _,attr in ipairs(exported_attributes) do
        ev_exported[attr] = ev[attr]
    end

    for _,p in ipairs(ev_exported.parameters) do
        if p[1] == 'HlAttrs' then
            p[1] = 'Dictionary'
        end
    end

    exported_events[#exported_events+1] = ev_exported
end

packed = mpack.pack(exported_events)
dump_bin_array = require("generators.dump_bin_array")
dump_bin_array(ui_func_metadata_output, 'ui_events_metadata', packed)
ui_func_metadata_output:close()
