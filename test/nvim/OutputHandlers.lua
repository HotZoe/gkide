local s = require("say")
local term = require("term")
local socket = require("socket")
local pretty = require("pl.pretty")

local colors

local isWindows = package.config:sub(1,1) == '\\'

if isWindows then
    colors = setmetatable({}, {__index = function() return function(s) return s end end})
else
    colors = require("term.colors")
end

return function(options)
    local busted = require("busted")
    local handler = require("busted.outputHandlers.base")()

    local c =
    {
        errr = function(s) return colors.bright(colors.red(s)) end,
        succ = function(s) return colors.bright(colors.green(s)) end,
        skip = function(s) return colors.bright(colors.yellow(s)) end,
        fail = function(s) return colors.bright(colors.magenta(s)) end,
        test = tostring,
        file = colors.cyan,
        time = colors.dim,
        note = colors.yellow,
        sect = function(s) return colors.green(colors.dim(s)) end,
        nmbr = colors.bright,
    }

    local globalSetup     = c.sect('[======]') .. ' Global test environment setup\n'
    local globalTeardown  = c.sect('[======]') .. ' Global test environment teardown\n'
    local suiteEndString  = c.sect('[Result]') .. ' ' .. c.nmbr('%03d') .. ' %s from'
                                               .. ' ' .. c.nmbr('%03d') .. ' spec %s:'
                                               .. ' ' .. c.time('elapsed %03.6f ms') .. '\n'
    local successStatus   = c.succ('[ PASS ]') .. ' ' .. c.nmbr('%03d') .. ' %s\n'
    local summaryStrings  =
    {
        skipped =
        {
            header = c.skip('[ SKIP ]') .. ' ' .. c.nmbr('%d') .. ' %s, listed below:\n',
            test   = c.skip('[ SKIP ]') .. ' %s\n',
            footer = c.skip('[ SKIP ]') .. ' ' .. c.nmbr('%d') .. ' %s\n',
        },

        failure =
        {
            header = c.fail('[ FAIL ]') .. ' ' .. c.nmbr('%d') .. ' %s, listed below:\n',
            test   = c.fail('[ FAIL ]') .. ' %s\n',
            footer = c.fail('[ FAIL ]') .. ' ' .. c.nmbr('%d') .. ' %s\n',
        },

        error =
        {
            header = c.errr('[ ERRR ]') .. ' ' .. c.nmbr('%d') .. ' %s, listed below:\n',
            test   = c.errr('[ ERRR ]') .. ' %s\n',
            footer = c.errr('[ ERRR ]') .. ' ' .. c.nmbr('%d') .. ' %s\n',
        },
    }

    local fileStartString = c.sect('[ Spec ]') .. ' ' .. c.file('%s') .. '\n'
    local runString       = c.sect('[ Runs ]') .. ' ' .. c.test('%s')
    local errorString     = c.errr('ERRR')
    local successString   = c.succ('PASS')
    local skippedString   = c.skip('SKIP')
    local failureString   = c.fail('FAIL')
    local fileEndString   = c.sect('[Result]') .. ' ' .. c.file('%s') .. ' has ' .. c.nmbr('%d') .. ' %s'

    local repeatSuiteString = c.sect('[ Note ]') .. ' ' ..
                              c.note('Repeating all tests (run ') .. c.nmbr('%d') .. c.note(' of ') .. c.nmbr('%d\n\n')
    local randomizeString   = c.sect('[ Note ]') .. ' ' ..
                              c.note('Randomizing test order with a seed of ') .. c.nmbr('%d\n')

    local timeString = c.time('%03.6f ms')

    c = nil

    local fileCount = 0
    local fileTestCount = 0

    local testCount = 0
    local successCount = 0

    local skippedCount = 0
    local failureCount = 0
    local errorCount = 0

    local suite_start_time = 0
    local test_start_time = 0
    local file_start_time = 0

    local pendingDescription = function(pending)
        local name = pending.name
        local string = ''

        if type(pending.message) == 'string' then
            string = string .. pending.message .. '\n'
        elseif pending.message ~= nil then
            string = string .. pretty.write(pending.message) .. '\n'
        end

        return string
    end

    local failureDescription = function(failure)
        local string = failure.randomseed and ('Random seed: ' .. failure.randomseed .. '\n') or ''
        if type(failure.message) == 'string' then
            string = string .. failure.message
        elseif failure.message == nil then
            string = string .. 'Nil error'
        else
            string = string .. pretty.write(failure.message)
        end

        string = string .. '\n'

        if options.verbose and failure.trace and failure.trace.traceback then
            string = string .. failure.trace.traceback .. '\n'
        end

        return string
    end

    local getFileLine = function(element)
        local fileline = ''

        if element.trace or element.trace.short_src then
            fileline = colors.cyan(element.trace.short_src) .. ' @ ' ..
                       colors.cyan(element.trace.currentline) .. ': '
        end

        return fileline
    end

    local getTestList = function(status, count, list, getDescription)
        local string = ''
        local header = summaryStrings[status].header
        if count > 0 and header then
            local tests = (count == 1 and 'test' or 'tests')
            local errors = (count == 1 and 'error' or 'errors')
            string = header:format(count, status == 'error' and errors or tests)

            local testString = summaryStrings[status].test
            if testString then
                for _, t in ipairs(list) do
                    local fullname = getFileLine(t.element) .. colors.bright(t.name)
                    string = string .. testString:format(fullname)
                    string = string .. getDescription(t)
                end
            end
        end

        return string
    end

    local getSummary = function(status, count)
        local string = ''
        local footer = summaryStrings[status].footer

        if count > 0 and footer then
            local tests = (count == 1 and 'test' or 'tests')
            local errors = (count == 1 and 'error' or 'errors')
            string = footer:format(count, status == 'error' and errors or tests)
        end

        return string
    end

    local getSummaryString = function()
        local tests = (successCount == 1 and 'test' or 'tests')
        local string = successStatus:format(successCount, tests)

        string = string .. getTestList('skipped', skippedCount, handler.pendings, pendingDescription)
        string = string .. getTestList('failure', failureCount, handler.failures, failureDescription)
        string = string .. getTestList('error', errorCount, handler.errors, failureDescription)

        string = string .. ((skippedCount + failureCount + errorCount) > 0 and '\n' or '')
        string = string .. getSummary('skipped', skippedCount)
        string = string .. getSummary('failure', failureCount)
        string = string .. getSummary('error', errorCount)

        return string
    end

    handler.suiteReset = function()
        fileCount = 0
        fileTestCount = 0
        testCount = 0
        successCount = 0
        skippedCount = 0
        failureCount = 0
        errorCount = 0

        return nil, true
    end

    handler.suiteStart = function(suite, count, total, randomseed)
        suite_start_time = socket.gettime()

        if total > 1 then
            io.write(repeatSuiteString:format(count, total))
        end

        if randomseed then
            io.write(randomizeString:format(randomseed))
        end

        io.write(globalSetup)
        io.flush()

        return nil, true
    end

    handler.suiteEnd = function(suite, count, total)
        local elapsedTime_ms = (socket.gettime() - suite_start_time) * 1000
        local tests = (testCount == 1 and 'test' or 'tests')
        local files = (fileCount == 1 and 'file' or 'files')

        io.write(globalTeardown)
        io.write(suiteEndString:format(testCount, tests, fileCount, files, elapsedTime_ms))
        io.write(getSummaryString())
        io.flush()

        return nil, true
    end

    handler.fileStart = function(file)
        fileTestCount = 0
        file_start_time = socket.gettime()

        io.write(fileStartString:format(file.name))
        io.flush()

        return nil, true
    end

    local function line_append_align(len)
        for i=len, 100 do
            io.write(' ')
        end
    end

    handler.fileEnd = function(file)
        local tests = (fileTestCount == 1 and 'test' or 'tests')
        local elapsedTime_ms = (socket.gettime() - file_start_time) * 1000

        fileCount = fileCount + 1
        io.write(fileEndString:format(file.name, fileTestCount, tests, elapsedTime_ms))

        local len = string.len(file.name .. tests)
        len = len + string.len(tostring(fileTestCount))
        line_append_align(len+1)
        io.write(timeString:format(elapsedTime_ms) .. '\n\n')
        io.flush()

        return nil, true
    end

    handler.testStart = function(element, parent)
        local full_file_name = handler.getFullName(element)
        test_start_time = socket.gettime()

        io.write(runString:format(full_file_name))
        line_append_align(string.len(full_file_name))
        io.flush()
        return nil, true
    end

    handler.testEnd = function(element, parent, status, debug)
        local string
        local elapsedTime_ms = (socket.gettime() - test_start_time) * 1000

        testCount = testCount + 1
        fileTestCount = fileTestCount + 1

        if status == 'success' then
            successCount = successCount + 1
            string = successString
        elseif status == 'pending' then
            skippedCount = skippedCount + 1
            string = skippedString
        elseif status == 'failure' then
            failureCount = failureCount + 1
            string = nil
        elseif status == 'error' then
            errorCount = errorCount + 1
            string = nil
        end

        if string ~= nil then
            string = string .. ' ' .. timeString:format(elapsedTime_ms) .. '\n'
            io.write(string)
            io.flush()
        end

        return nil, true
    end

    handler.testFailure = function(element, parent, message, debug)
        io.write(failureString)
        io.flush()

        io.write(failureDescription(handler.failures[#handler.failures]))
        io.flush()

        return nil, true
    end

    handler.testError = function(element, parent, message, debug)
        io.write(errorString)
        io.flush()

        io.write(failureDescription(handler.errors[#handler.errors]))
        io.flush()

        return nil, true
    end

    handler.error = function(element, parent, message, debug)
        if element.descriptor ~= 'it' then
            io.write(failureDescription(handler.errors[#handler.errors]))
            io.flush()

            errorCount = errorCount + 1
        end

        return nil, true
    end

    busted.subscribe({ 'suite', 'reset' }, handler.suiteReset)
    busted.subscribe({ 'suite', 'start' }, handler.suiteStart)
    busted.subscribe({ 'suite', 'end' }, handler.suiteEnd)
    busted.subscribe({ 'file', 'start' }, handler.fileStart)
    busted.subscribe({ 'file', 'end' }, handler.fileEnd)
    busted.subscribe({ 'test', 'start' }, handler.testStart, { predicate = handler.cancelOnPending })
    busted.subscribe({ 'test', 'end' }, handler.testEnd, { predicate = handler.cancelOnPending })
    busted.subscribe({ 'failure', 'it' }, handler.testFailure)
    busted.subscribe({ 'error', 'it' }, handler.testError)
    busted.subscribe({ 'failure' }, handler.error)
    busted.subscribe({ 'error' }, handler.error)

    return handler
end
