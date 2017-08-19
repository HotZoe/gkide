# Host use only, no link to nvim or snail

include(CMakeParseArguments)

# BuildLua(<CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildLua)
    cmake_parse_arguments(_lua
    ""
    ""
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _lua_CONFIGURE_COMMAND AND
       NOT _lua_BUILD_COMMAND     AND
       NOT _lua_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    ExternalProject_Add(   lua
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LUA_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/lua
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/lua
                           -DDOWNLOAD_URL=${LUA_URL}
                           -DDOWNLOAD_VER=${LUA_VERSION}
                           -DBUILD_TARGET=lua
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=1
                           -DEXPECT_SHA256=${LUA_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LUA}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        CONFIGURE_COMMAND  "${_lua_CONFIGURE_COMMAND}"
        BUILD_IN_SOURCE    1
        WORKING_DIRECTORY  "${DEPS_BUILD_DIR}/src/lua"
        BUILD_COMMAND      "${_lua_BUILD_COMMAND}"
        INSTALL_COMMAND    "${_lua_INSTALL_COMMAND}")
endfunction()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(LUA_TARGET linux)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(LUA_TARGET macosx)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(LUA_TARGET mingw)
    if(NOT MINGW)
        set(err_msg "The compiler should some version of MinGW\n")
        set(err_msg "${err_msg}  C Compiler: ${HOSTDEPS_C_COMPILER}\n")
        set(err_msg "${err_msg}C++ Compiler: ${HOSTDEPS_CXX_COMPILER}\n")
        message(FATAL_ERROR "${err_msg}")
    endif()
else()
    if(UNIX)
        set(LUA_TARGET posix)
    else()
        set(LUA_TARGET generic)
    endif()
endif()

set(LUA_CFLAGS "-O0 -g3 -fPIC")
set(LUA_LDFLAGS "")

if(CMAKE_HOST_APPLE)
    # "$ sed -i file_to_edit"               linux usage format
    # "$ sed -i 'extension' file_to_edit"   macos usage format
    set(arg '.org') 
endif()

set(LUA_CONFIGURE_COMMAND ${SED_PROG} -e "/^CC/s@gcc@${HOSTDEPS_C_COMPILER}@"
                                      -e "/^CFLAGS/s@-O2@${LUA_CFLAGS}@"
                                      -e "/^MYLDFLAGS/s@$@${LUA_LDFLAGS}@"
                                      -e "s@-lreadline@@g"
                                      -e "s@-lhistory@@g"
                                      -e "s@-lncurses@@g"
                                      -i ${arg} ${DEPS_BUILD_DIR}/src/lua/src/Makefile
                  COMMAND ${SED_PROG} -e "/#define LUA_USE_READLINE/d"
                                      -i ${arg} ${DEPS_BUILD_DIR}/src/lua/src/luaconf.h)

set(LUA_BUILD_COMMAND ${MAKE_PROG} ${LUA_TARGET})

# Host=Linux, Target=Linux
# Host=MacOS, Target=MacOS
set(LUA_INSTALL_COMMAND ${MAKE_PROG} INSTALL_TOP=${HOSTDEPS_INSTALL_DIR} install)

if(CMAKE_HOST_WIN32 AND WIN32 AND MINGW)
    # Host=Windows, Target=Windows
    set(LUA_CONFIGURE_COMMAND ${LUA_CONFIGURE_COMMAND}
                      COMMAND ${SED_PROG} -e "/^MKDIR/s@mkdir@${MKDIR_PROG}@"
                                          -e "/^INSTALL/s@install@${INSTALL_PROG}@"
                                          -i ${DEPS_BUILD_DIR}/src/lua/Makefile)
    set(LUA_INSTALL_COMMAND ${LUA_INSTALL_COMMAND}
                    COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/lua/src/lua52.dll
                                                     ${HOSTDEPS_BIN_DIR}/lua52.dll)
endif()

# The lua must be dynamic linked, and install dynamic lua library
message(STATUS  "Building: lua-v${LUA_VERSION} => ${LUA_TARGET}")
BuildLua(CONFIGURE_COMMAND ${LUA_CONFIGURE_COMMAND}
         BUILD_COMMAND     ${LUA_BUILD_COMMAND}
         INSTALL_COMMAND   ${LUA_INSTALL_COMMAND})

list(APPEND THIRD_PARTY_LIBS lua)
