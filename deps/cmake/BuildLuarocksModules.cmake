# Arguments for calls to 'luarocks build'
set(LUAROCKS_BUILDARGS CC=${HOSTDEPS_C_COMPILER} LD=${HOSTDEPS_C_COMPILER})
if(CMAKE_HOST_WIN32)
    set(LUAROCKS_BUILDARGS ${LUAROCKS_BUILDARGS} GIT=${GIT_PROG})
    set(mingw_bin_dir $ENV{GKIDE_MINGW_BIN_DIR})
    set(mingw_inc_dir $ENV{GKIDE_MINGW_INC_DIR})
    set(mingw_lib_dir $ENV{GKIDE_MINGW_LIB_DIR})
    file(TO_CMAKE_PATH "${mingw_bin_dir}" mingw_bin_dir)
    file(TO_CMAKE_PATH "${mingw_inc_dir}" mingw_inc_dir)
    file(TO_CMAKE_PATH "${mingw_lib_dir}" mingw_lib_dir)
endif()

if(CMAKE_HOST_APPLE)
    set(openssl_brew_dir /usr/local/Cellar/openssl)
    if(EXISTS "${openssl_brew_dir}" AND IS_DIRECTORY "${openssl_brew_dir}")
        file(GLOB openssl_versioned_dirs "${openssl_brew_dir}/*")
        foreach(openssl_versioned_dir ${openssl_versioned_dirs})
            message(STATUS "Found openssl: ${openssl_versioned_dir}")
            set(macos_openssl_inc_dir "${openssl_versioned_dir}/include")
            set(macos_openssl_lib_dir "${openssl_versioned_dir}/lib")
        endforeach()
    else()
        message(FATAL_ERROR "Openssl library not found, run '$ brew install openssl' to install it!")
    endif()
endif()


message(STATUS  "Building: luarocks => luasocket")
add_custom_command(OUTPUT  ${HOSTDEPS_LIB_DIR}/luarocks/rocks/luasocket
                   COMMAND ${LUAROCKS_BINARY} build luasocket ${LUAROCKS_BUILDARGS}
                   DEPENDS luarocks)
add_custom_target(luasocket
                  DEPENDS  ${HOSTDEPS_LIB_DIR}/luarocks/rocks/luasocket)
list(APPEND THIRD_PARTY_LIBS luasocket)


message(STATUS  "Building: luarocks => luasec")
if(CMAKE_HOST_WIN32)
    set(OPENSSL_INC_LIB_ARGS OPENSSL_INCDIR=${mingw_inc_dir} OPENSSL_LIBDIR=${mingw_bin_dir})
endif()
if(CMAKE_HOST_APPLE)
    set(OPENSSL_INC_LIB_ARGS OPENSSL_INCDIR=${macos_openssl_inc_dir} OPENSSL_LIBDIR=${macos_openssl_lib_dir})
endif()
add_custom_command(OUTPUT  ${HOSTDEPS_LIB_DIR}/luarocks/rocks/luasec
                   COMMAND ${LUAROCKS_BINARY} build luasec ${LUAROCKS_BUILDARGS} ${OPENSSL_INC_LIB_ARGS}
                   DEPENDS luasocket)
add_custom_target(luasec
                  DEPENDS  ${HOSTDEPS_LIB_DIR}/luarocks/rocks/luasec)
list(APPEND THIRD_PARTY_LIBS luasec)


message(STATUS  "Building: luarocks => luabitop")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/luabitop
                   COMMAND ${LUAROCKS_BINARY} build luabitop ${LUAROCKS_BUILDARGS}
                   DEPENDS luarocks)
add_custom_target(luabitop
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/luabitop)
list(APPEND THIRD_PARTY_LIBS luabitop)


message(STATUS  "Building: luarocks => mpack")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/mpack
                   COMMAND ${LUAROCKS_BINARY} build mpack ${LUAROCKS_BUILDARGS}
                   DEPENDS luarocks)
add_custom_target(mpack
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/mpack)
list(APPEND THIRD_PARTY_LIBS mpack)


message(STATUS  "Building: luarocks => lpeg")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/lpeg
                   COMMAND ${LUAROCKS_BINARY} build lpeg ${LUAROCKS_BUILDARGS}
                   DEPENDS luarocks)
add_custom_target(lpeg
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/lpeg)
list(APPEND THIRD_PARTY_LIBS lpeg)


message(STATUS  "Building: luarocks => inspect")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/inspect
                   COMMAND ${LUAROCKS_BINARY} build inspect ${LUAROCKS_BUILDARGS}
                   DEPENDS luarocks)
add_custom_target(inspect
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/inspect)
list(APPEND THIRD_PARTY_LIBS inspect)

# check if need to install the lua modules for testing
if(NOT NVIM_TESTING_ENABLE)
    return()
endif()

if(CMAKE_HOST_WIN32)
    # TODO, the following is for nvim testing, need fix for windows build
    # for now, just skip testing
    return()
endif()

# for unit test, see: https://github.com/Olivine-Labs/busted/
message(STATUS  "Building: luarocks => penlight, for nvim testing")
add_custom_command(OUTPUT ${DEPS_LIB_DIR}/luarocks/rocks/penlight/1.3.2-2
                   COMMAND ${LUAROCKS_BINARY} build penlight 1.3.2-2 ${LUAROCKS_BUILDARGS}
                   DEPENDS inspect)
add_custom_target(penlight
                  DEPENDS ${DEPS_LIB_DIR}/luarocks/rocks/penlight/1.3.2-2)
list(APPEND THIRD_PARTY_LIBS penlight)

# Run tests
message(STATUS  "Building: luarocks => busted, for nvim testing")
set(BUSTED_JIT  "${DEPS_BIN_DIR}/busted")
set(busted_rockspec_url
    "https://raw.githubusercontent.com/Olivine-Labs/busted/v2.0.rc11-0/busted-2.0.rc11-0.rockspec")
add_custom_command(OUTPUT  ${BUSTED_JIT}
                   COMMAND ${LUAROCKS_BINARY} build ${busted_rockspec_url} ${LUAROCKS_BUILDARGS}
                   DEPENDS penlight)
add_custom_target(busted
                  DEPENDS  ${BUSTED_JIT})
list(APPEND THIRD_PARTY_LIBS busted)

# By default, busted use luajit, change to lua
message(STATUS  "Building: luarocks => busted-lua, for nvim testing")
set(BUSTED_LUA  "${DEPS_BIN_DIR}/busted-lua")
add_custom_command(OUTPUT ${BUSTED_LUA}
                   COMMAND sed -e 's/^exec/exec $$LUA_DEBUGGER/' -e 's/jit//g' < ${BUSTED_JIT} > ${BUSTED_LUA}
                   COMMAND chmod +x ${BUSTED_LUA}
                   DEPENDS busted)
add_custom_target(busted-lua DEPENDS ${BUSTED_LUA})
list(APPEND THIRD_PARTY_LIBS busted-lua)

message(STATUS  "Building: luarocks => luacheck, for nvim testing")
set(luacheck_rockspec_url
    "https://raw.githubusercontent.com/mpeterv/luacheck/master/luacheck-scm-1.rockspec")
add_custom_command(OUTPUT  ${DEPS_BIN_DIR}/luacheck
                   COMMAND ${LUAROCKS_BINARY} build ${luacheck_rockspec_url} ${LUAROCKS_BUILDARGS}
                   DEPENDS busted)
add_custom_target(luacheck
                  DEPENDS  ${DEPS_BIN_DIR}/luacheck)
list(APPEND THIRD_PARTY_LIBS luacheck)

message(STATUS  "Building: luarocks => nvim-client, for nvim testing")
set(nvim_client_rockspec_url
    "https://raw.githubusercontent.com/neovim/lua-client/0.0.1-26/nvim-client-0.0.1-26.rockspec")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/nvim-client
                   COMMAND ${LUAROCKS_BINARY} build ${nvim_client_rockspec_url} ${LUAROCKS_BUILDARGS}
                   DEPENDS luacheck)
add_custom_target(nvim-client
                  DEPENDS ${DEPS_LIB_DIR}/luarocks/rocks/nvim-client)
list(APPEND THIRD_PARTY_LIBS nvim-client)

