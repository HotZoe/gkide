#
# Checking Lua interpreter & Lua modules
# Config lua package search path

set(CONFIG_LUA_PATH)
set(CONFIG_LUA_CPATH)
set(CONFIG_LUA_PATH_ENV)
set(CONFIG_LUA_CPATH_ENV)

if(NOT HOST_OS_WINDOWS)
    # for LUA_PATH
    set(LUA_PATH_SEARCH_DIR_PERFIX "${BUNDLED_DEPS_PREFIX}/share/lua")
    # for LUA_CPATH
    set(LUA_CPATH_SEARCH_DIR_PERFIX "${BUNDLED_DEPS_PREFIX}/lib/lua")

    if(EXISTS "${LUA_PATH_SEARCH_DIR_PERFIX}"
       AND IS_DIRECTORY "${LUA_PATH_SEARCH_DIR_PERFIX}")
        file(GLOB lua_version_dirs "${LUA_PATH_SEARCH_DIR_PERFIX}/*")
        foreach(lvd ${lua_version_dirs})
            set(ENV{LUA_PATH} "${lvd}/?/init.lua;$ENV{LUA_PATH}")
            set(ENV{LUA_PATH} "${lvd}/?.lua;$ENV{LUA_PATH}")
            list(APPEND CONFIG_LUA_PATH "${lvd}/?/init.lua")
            list(APPEND CONFIG_LUA_PATH "${lvd}/?.lua")
            list(APPEND CONFIG_LUA_PATH_ENV "${lvd}/?/init.lua\;")
            list(APPEND CONFIG_LUA_PATH_ENV "${lvd}/?.lua\;")
        endforeach()
    endif()

    if(EXISTS "${LUA_CPATH_SEARCH_DIR_PERFIX}"
       AND IS_DIRECTORY "${LUA_CPATH_SEARCH_DIR_PERFIX}")
        file(GLOB lua_version_dirs "${LUA_CPATH_SEARCH_DIR_PERFIX}/*")
        foreach(lvd ${lua_version_dirs})
            list(APPEND CONFIG_LUA_CPATH "${lvd}/?.so")
            list(APPEND CONFIG_LUA_CPATH_ENV "${lvd}/?.so\;")
            set(ENV{LUA_CPATH} "${lvd}/?.so;$ENV{LUA_CPATH}")
        endforeach()
    endif()
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/config.lua.in
               ${PROJECT_BINARY_DIR}/source/nvim/config.lua)

function(check_lua_module LUA_PRG_PATH MODULE RESULT_VAR)
    execute_process(COMMAND ${LUA_PRG_PATH} -e "require('${MODULE}')"
                    RESULT_VARIABLE  module_missing
                    ERROR_QUIET)
    if(module_missing)
        set(${RESULT_VAR} False PARENT_SCOPE)
    else()
        set(${RESULT_VAR} True  PARENT_SCOPE)
    endif()
endfunction()

# Check Lua interpreter for dependencies
function(check_lua_deps LUA_PRG_PATH MODULES RESULT_VAR)
    # Check if the lua interpreter at the
    # given path satisfies all nvim dependencies
    message(STATUS "Lua interpreter: ${LUA_PRG_PATH}")
    if(NOT EXISTS ${LUA_PRG_PATH})
        message(STATUS "[${LUA_PRG_PATH}] not found")
		return() # Return from a file, directory or function.
    endif()

    foreach(module ${MODULES})
        check_lua_module(${LUA_PRG_PATH} ${module} has_module)
        if(NOT has_module)
            message(STATUS "[${LUA_PRG_PATH}] The '${module}' lua package is required.")
            set(${RESULT_VAR} False PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${RESULT_VAR} True PARENT_SCOPE)
endfunction()
