require('coxpcall')
local luv = require('luv')
local lfs = require('lfs')
local mpack = require('mpack')
local global_helper = require('globalhelper')

-- nvim lua client: deps/build/usr/share/lua/<version>/nvim/
local nlc_session = require('nvim.session')
local nlc_stream_tcp = require('nvim.tcp_stream')
local nlc_stream_socket = require('nvim.socket_stream')
local nlc_stream_child_process = require('nvim.child_process_stream')

local ok = global_helper.ok
local eq = global_helper.eq
local neq = global_helper.neq
local map = global_helper.map
local uname = global_helper.uname
local filter = global_helper.filter
local dedent = global_helper.dedent
local tmpname = global_helper.tmpname
local check_logs = global_helper.check_logs
local check_cores = global_helper.check_cores

local start_dir = lfs.currentdir()
local nvim_prog = global_helper.nvim_prog
local config_path = global_helper.config_path
local nvim_bin_dir = nvim_prog:gsub("[/\\][^/\\]+$", "")

if nvim_bin_dir == nvim_prog then
    nvim_dir = "."
end

local session
local last_error
local loop_running
local prepend_argv

-- Default settings for the test session.
local sessionarg= 'set shortmess+=I background=light noswapfile noautoindent ' ..
                  'laststatus=1 undodir=. directory=. viewdir=. backupdir=. '  ..
                  'belloff= noshowcmd noruler nomore'
local nvim_argv = {nvim_prog, '-u', 'NONE', '-i', 'NONE', '-N', '--cmd', sessionarg, '--embed'}

if os.getenv('GDB') then
    local gdb_server_port = '7777'

    if os.getenv('GDB_SERVER_PORT') then
        gdb_server_port = os.getenv('GDB_SERVER_PORT')
    end

    prepend_argv = {'gdbserver', 'localhost:' .. gdb_server_port}
end

if prepend_argv then
    local argv_tmp = {}
    local len = #prepend_argv

    for i = 1, len do
        argv_tmp[i] = prepend_argv[i]
    end

    for i = 1, #nvim_argv do
        argv_tmp[i + len] = nvim_argv[i]
    end

    nvim_argv = argv_tmp
end

local function set_session(s, keep)
    if session and not keep then
        session:close()
    end

    session = s
end

local function request(method, ...)
    local status, rv = session:request(method, ...)

    if not status then
        if loop_running then
            last_error = rv[2]
            session:stop()
        else
            error(rv[2])
        end
    end

    return rv
end

local function next_message()
    return session:next_message()
end

local function call_and_stop_on_error(...)
    local status, result = copcall(...)  -- luacheck: ignore

    if not status then
        session:stop()
        last_error = result
        return ''
    end

    return result
end

local function stop()
    session:stop()
end

local function run(request_cb, notification_cb, setup_cb, timeout)
    local on_setup
    local on_request
    local on_notification

    if request_cb then
        function on_request(method, args)
            return call_and_stop_on_error(request_cb, method, args)
        end
    end

    if notification_cb then
        function on_notification(method, args)
            return call_and_stop_on_error(notification_cb, method, args)
        end
    end

    if setup_cb then
        function on_setup()
            return call_and_stop_on_error(setup_cb)
        end
    end

    loop_running = true
    session:run(on_request, on_notification, on_setup, timeout)
    loop_running = false

    if last_error then
        local err = last_error
        last_error = nil
        error(err)
    end
end

-- Executes an ex-command.
-- VimL errors manifest as client (lua) errors, v:errmsg will not be updated.
local function nvim_command(cmd)
    request('nvim_command', cmd)
end

-- Evaluates a VimL expression.
-- Fails on VimL error, but does not update v:errmsg.
local function nvim_eval(expr)
    return request('nvim_eval', expr)
end

local function get_os_name()
    local name = nil

    local function nvim_get_os_name()
        if not name then
            if nvim_eval('has("win32")') == 1 then
                name = 'windows'
            elseif nvim_eval('has("macunix")') == 1 then
                name = 'osx'
            else
                name = 'unix'
            end
        end
        return name
    end

  return nvim_get_os_name
end
local os_name = get_os_name()

local function is_linux()
    return os_name() == 'unix'
end

local function is_macos()
    return os_name() == 'osx'
end

local function is_windows()
    return os_name() == 'windows'
end

-- Executes a VimL function.
-- Fails on VimL error, but does not update v:errmsg.
local function nvim_call(name, ...)
    return request('nvim_call_function', name, {...})
end

-- Sends user input to Nvim.
-- Does not fail on VimL error, but v:errmsg will be updated.
local function nvim_feed(input)
    while #input > 0 do
        local written = request('nvim_input', input)
        input = input:sub(written + 1)
    end
end

local function feed(...)
    for _, v in ipairs({...}) do
        nvim_feed(dedent(v))
    end
end

local function rawfeed(...)
    for _, v in ipairs({...}) do
        nvim_feed(dedent(v))
    end
end

local function merge_args(...)
    local i = 1
    local argv = {}
    for idx = 1, select('#', ...) do
        local args = select(idx, ...)
        if args then
            for _, arg in ipairs(args) do
                argv[i] = arg
                i = i + 1
            end
        end
    end
    return argv
end

local function spawn(argv, prepend, env)
    local new_argv

    if prepend then
        new_argv = merge_args(prepend_argv, argv)
    else
        new_argv = argv
    end

    local new_stream = nlc_stream_child_process.spawn(new_argv, env)

    return nlc_session.new(new_stream)
end

-- Creates a new Session connected by domain socket (named pipe) or TCP.
local function connect(file_or_address)
    local addr, port = string.match(file_or_address, "(.*):(%d+)")
    local is_tcp = addr and port
    local new_stream

    if is_tcp then
        new_stream = nlc_stream_tcp.open(addr, port)
    else
        new_stream = nlc_stream_socket.open(file_or_address)
    end

    return nlc_session.new(new_stream)
end

-- Calls fn() until it succeeds, up to 'max' times or until 'max_ms' milliseconds have passed.
local function retry(max, max_ms, fn)
    local tries = 1
    local timeout = (max_ms and max_ms > 0) and max_ms or 10000
    local start_time = luv.now()

    while true do
        local status, result = pcall(fn)

        if status then
            return result
        end

        if (max and tries >= max) or (luv.now() - start_time > timeout) then
            if type(result) == "string" then
                result = "\nretry() attempts: "..tostring(tries).."\n"..result
            end
            error(result)
        end

        tries = tries + 1
    end
end

local function clear(...)
    local env = {}
    local args_extra = {}

    local args = { unpack(nvim_argv) }
    local opts = select(1, ...)

    if type(opts) == 'table' then
        if opts.env then
            local env_tbl = {}

            for k, v in pairs(opts.env) do
                assert(type(k) == 'string')
                assert(type(v) == 'string')
                env_tbl[k] = v
            end

            local env_check = {'HOME', 'ASAN_OPTIONS', 'LD_LIBRARY_PATH', 'PATH',
                               'NVIM_LOG_FILE', 'NVIM_RPLUGIN_MANIFEST',}
            for _, k in ipairs(env_check) do
                if not env_tbl[k] then
                    env_tbl[k] = os.getenv(k)
                end
            end

            for k, v in pairs(env_tbl) do
                env[#env + 1] = k .. '=' .. v
            end
        end

        args_extra = opts.args or {}
    else
        args_extra = {...}
    end

    for _, arg in ipairs(args_extra) do
        table.insert(args, arg)
    end

    set_session(spawn(args, nil, env))
end

local function insert(...)
    nvim_feed('i')

    for _, v in ipairs({...}) do
        local escaped = v:gsub('<', '<lt>')
        rawfeed(escaped)
    end

    nvim_feed('<ESC>')
end

-- Executes an ex-command by user input
local function feed_command(...)
    for _, v in ipairs({...}) do
        if v:sub(1, 1) ~= '/' then
            -- not a search command, prefix with colon
            nvim_feed(':')
        end
        nvim_feed(v:gsub('<', '<lt>'))
        nvim_feed('<CR>')
    end
end

-- Dedent the given text and write it to the file name.
local function write_file(name, text, dont_dedent)
    local file = io.open(name, 'w')

    if not dont_dedent then
        text = dedent(text)
    end

    file:write(text)
    file:flush()
    file:close()
end

local function read_file(name)
    local file = io.open(name, 'r')

    if not file then
        return nil
    end

    local ret = file:read('*a')
    file:close()
    return ret
end

local function source(code)
    local fname = tmpname()

    write_file(fname, code)
    nvim_command('source '..fname)
    os.remove(fname)
    return fname
end

local function nvim(method, ...)
    return request('nvim_'..method, ...)
end

local function nvim_ui(method, ...)
    return request('nvim_ui_'..method, ...)
end

local function nvim_buf(method, ...)
    return request('nvim_buf_'..method, ...)
end

local function nvim_win(method, ...)
    return request('nvim_win_'..method, ...)
end

local function nvim_tab(method, ...)
    return request('nvim_tabpage_'..method, ...)
end

local function nvim_async(method, ...)
    session:notify('nvim_'..method, ...)
end

local function curbuf(method, ...)
    if not method then
        return nvim('get_current_buf')
    end
    return nvim_buf(method, 0, ...)
end

local function curwin(method, ...)
    if not method then
        return nvim('get_current_win')
    end
    return nvim_win(method, 0, ...)
end

local function curtab(method, ...)
    if not method then
        return nvim('get_current_tabpage')
    end
    return nvim_tab(method, 0, ...)
end

local function wait()
    -- Execute 'nvim_eval' (a deferred function) to block until all pending input is processed.
    session:request('nvim_eval', '1')
end

-- sleeps the test runner, not the nvim instance
local function sleep(ms)
    local function notification_cb(method, _)
        if method == "redraw" then
            error("Screen is attached; use screen:sleep() instead.")
        end
        return true
    end

    run(nil, notification_cb, nil, ms)
end

local function curbuf_contents()
    wait()  -- Before inspecting the buffer, process all input.
    return table.concat(curbuf('get_lines', 0, -1, true), '\n')
end

local function expect(contents)
    return eq(dedent(contents), curbuf_contents())
end

local function expect_any(contents)
    contents = dedent(contents)
    return ok(nil ~= string.find(curbuf_contents(), contents, 1, true))
end

local function do_rmdir(path)
    if lfs.attributes(path, 'mode') ~= 'directory' then
        return -- Don't complain.
    end

    for file in lfs.dir(path) do
        if file ~= '.' and file ~= '..' then
            local abspath = path..'/'..file

            if lfs.attributes(abspath, 'mode') == 'directory' then
                do_rmdir(abspath) -- recurse
            else
                local rv, msg, rc = os.remove(abspath) -- for lua 5.2/5.3
                if not rv then
                    if not session then
                        error('os.remove: ' .. msg .. ', error code: ' .. rc)
                    else
                        -- Try Nvim delete(): it handles `readonly` attribute on Windows,
                        -- and avoids Lua cross-version/platform incompatibilities.
                        if -1 == nvim_call('delete', abspath) then
                            local hint = ''
                            if is_windows() then
                                hint = ' (hint: try :%bwipeout! before rmdir())'
                            end
                            error('delete() failed' .. hint .. ': ' .. abspath)
                        end
                    end
                end
            end
        end
    end

    local rv, msg = lfs.rmdir(path)

    if not rv then
        error('lfs.rmdir(' .. path .. '): ' .. msg)
    end
end

local function rmdir(path)
    local ret, _ = pcall(do_rmdir, path)

    if not ret and is_windows() then
        -- Maybe "Permission denied"; try again after changing the nvim
        -- process to the top-level directory.
        nvim_command([[exe 'cd '.fnameescape(']]..start_dir.."')")
        ret, _ = pcall(do_rmdir, path)
    end

    -- During teardown, the nvim process may not exit quickly enough, then rm_dir()
    -- will fail (on Windows).
    if not ret then  -- Try again.
        sleep(1000)
        do_rmdir(path)
    end
end

local exc_exec = function(cmd)
    nvim_command(
    ([[
    try
        execute "%s"
    catch
        let g:__exception = v:exception
    endtry
    ]]):format(cmd:gsub('\n', '\\n'):gsub('[\\"]', '\\%0')))

    local ret = nvim_eval('get(g:, "__exception", 0)')
    nvim_command('unlet! g:__exception')
    return ret
end

local function redir_exec(cmd)
    nvim_command(
    ([[
    redir => g:__output
        silent! execute "%s"
    redir END
    ]]):format(cmd:gsub('\n', '\\n'):gsub('[\\"]', '\\%0')))

    local ret = nvim_eval('get(g:, "__output", 0)')
    nvim_command('unlet! g:__output')
    return ret
end

-- Helper to skip tests.
-- Returns true in Windows systems, pending_fn is pending() from busted
local function pending_win32(pending_fn)
    if uname() == 'Windows' then
        if pending_fn ~= nil then
            pending_fn('FIXME: Windows', function() end)
        end
        return true
    else
        return false
    end
end

-- Calls pending() and returns 'true' if the system is too slow to run fragile or expensive tests.
-- Else returns 'false'.
local function skip_fragile(pending_fn, cond)
    if pending_fn == nil or type(pending_fn) ~= type(function()end) then
        error("invalid pending_fn")
    end

    if cond then
        pending_fn("skipped (test is fragile on this system)", function() end)
        return true
    elseif os.getenv("TEST_SKIP_FRAGILE") then
        pending_fn("skipped (TEST_SKIP_FRAGILE is set)", function() end)
        return true
    end

    return false
end

local function meth_pcall(...)
    local ret = { pcall(...) }

    if type(ret[2]) == 'string' then
        ret[2] = ret[2]:gsub('^[^:]+:%d+: ', '')
    end

    return ret
end

local function create_callindex(func)
    local function tmpfunc(tbl, arg1)
        local ret = function(...) return func(arg1, ...) end
        tbl[arg1] = ret
        return ret
    end

    local table = {}
    local tmptb = { __index = tmpfunc, }

    setmetatable(table, tmptb)
    return table
end

local funcs = create_callindex(nvim_call)     -- Executes a VimL function
local meths = create_callindex(nvim)          -- call nvim API
local uimeths = create_callindex(nvim_ui)     -- call nvim API, UI part
local bufmeths = create_callindex(nvim_buf)   -- call nvim API, buffer part
local winmeths = create_callindex(nvim_win)   -- call nvim API, tabpage part
local tabmeths = create_callindex(nvim_tab)
local curbufmeths = create_callindex(curbuf)
local curwinmeths = create_callindex(curwin)
local curtabmeths = create_callindex(curtab)

local function missing_provider(provider)
    if provider == 'ruby' then
        local prog = funcs['provider#' .. provider .. '#Detect']()
        return prog == '' and (provider .. ' not detected') or false
    elseif provider == 'python' or provider == 'python3' then
        local py_major_version = (provider == 'python3' and 3 or 2)
        local errors = funcs['provider#pythonx#Detect'](py_major_version)[2]
        return errors ~= '' and errors or false
    else
        assert(false, 'Unknown provider: ' .. provider)
    end
end

local function get_pathsep()
    return funcs.fnamemodify('.', ':p'):sub(-1)
end

local module =
{
    prepend_argv = prepend_argv,
    clear = clear,
    connect = connect,
    retry = retry,
    spawn = spawn,
    dedent = dedent,
    source = source,
    rawfeed = rawfeed,
    insert = insert,
    iswin = is_windows,
    feed = feed,
    feed_command = feed_command,
    eval = nvim_eval,
    call = nvim_call,
    command = nvim_command,
    request = request,
    next_message = next_message,
    run = run,
    stop = stop,
    eq = eq,
    neq = neq,
    expect = expect,
    expect_any = expect_any,
    ok = ok,
    map = map,
    filter = filter,
    nvim = nvim,
    nvim_async = nvim_async,
    nvim_prog = nvim_prog,
    nvim_argv = nvim_argv,
    nvim_set = nvim_set,
    nvim_dir = nvim_dir,
    buffer = nvim_buf,
    window = nvim_win,
    tabpage = nvim_tab,
    curbuf = curbuf,
    curwin = curwin,
    curtab = curtab,
    curbuf_contents = curbuf_contents,
    wait = wait,
    sleep = sleep,
    set_session = set_session,
    write_file = write_file,
    read_file = read_file,
    os_name = os_name,
    rmdir = rmdir,
    mkdir = lfs.mkdir,
    exc_exec = exc_exec,
    redir_exec = redir_exec,
    merge_args = merge_args,
    funcs = funcs,
    meths = meths,
    bufmeths = bufmeths,
    winmeths = winmeths,
    tabmeths = tabmeths,
    uimeths = uimeths,
    curbufmeths = curbufmeths,
    curwinmeths = curwinmeths,
    curtabmeths = curtabmeths,
    pending_win32 = pending_win32,
    skip_fragile = skip_fragile,
    set_shell_powershell = set_shell_powershell,
    tmpname = tmpname,
    meth_pcall = meth_pcall,
    NIL = mpack.NIL,
    get_pathsep = get_pathsep,
    missing_provider = missing_provider,
}

local function run_after_nested_test(after_each)
    local function run_checks()
        check_logs()
        check_cores()
    end

    if after_each then
        after_each(run_checks)
    end

    return module
end

return run_after_nested_test
