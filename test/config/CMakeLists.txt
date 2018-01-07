# Setup busted.
find_program(BUSTED_PROG NAMES busted busted.bat)
if(BUSTED_PROG MATCHES "NOTFOUND")
    message(WARNING "'busted' program not find, 'nvim' testing skip ...")
    return()
endif()

find_program(BUSTED_LUA_PROG busted-lua)
if(BUSTED_LUA_PROG MATCHES "NOTFOUND")
    message(WARNING "'busted-lua' program not find, 'nvim' testing skip ...")
    return()
endif()

find_program(LUACHECK_PROG luacheck)
if(LUACHECK_PROG MATCHES "NOTFOUND")
    message(WARNING "'luacheck' program not find, 'nvim' testing skip ...")
    return()
endif()

# busted output type: http://olivinelabs.com/busted/#output-handlers
# utfTerminal, plainTerminal, junit, json, TAP
set(BUSTED_OUTPUT_TYPE "OutputHandlers")
