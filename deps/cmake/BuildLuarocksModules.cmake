# lua => luarocks => luasec => luabitop
#                           => mpack   => lpeg
#                           => inspect => penlight => busted => luacheck

set(LUAROCKS_BINARY   ${DEPS_BIN_DIR}/luarocks) # The luarocks binary location
# I need this because I do not want the warnings when luarocks download modules using ssl
set(MODULE_DEP_ARGS   DEPENDS luarocks luasec)

# Arguments for calls to 'luarocks build'
if(NOT MSVC)
    # In MSVC don't pass the compiler/linker to luarocks
    # the bundled version already knows, and passing them here breaks the build
    set(LUAROCKS_BUILDARGS CC=${DEPS_C_COMPILER} LD=${DEPS_C_COMPILER})
endif()


message(STATUS  "          build => luasec")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/luasec
                   COMMAND ${LUAROCKS_BINARY} build luasec ${LUAROCKS_BUILDARGS}
                   DEPENDS luarocks)
add_custom_target(luasec
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/luasec)
list(APPEND THIRD_PARTY_LIBS luasec)


message(STATUS  "          build => luabitop")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/luabitop
                   COMMAND ${LUAROCKS_BINARY} build luabitop ${LUAROCKS_BUILDARGS}
                   ${MODULE_DEP_ARGS})
add_custom_target(luabitop
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/luabitop)
list(APPEND THIRD_PARTY_LIBS luabitop)


# The following each target depends on the previous module.
# this serializes all calls to luarocks since it is unhappy to be called in parallel.
message(STATUS  "          build => mpack")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/mpack
                   COMMAND ${LUAROCKS_BINARY} build mpack ${LUAROCKS_BUILDARGS}
                   ${MODULE_DEP_ARGS})
add_custom_target(mpack
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/mpack)
list(APPEND THIRD_PARTY_LIBS mpack)


message(STATUS  "          build => lpeg")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/lpeg
                   COMMAND ${LUAROCKS_BINARY} build lpeg ${LUAROCKS_BUILDARGS}
                   DEPENDS mpack)
add_custom_target(lpeg
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/lpeg)
list(APPEND THIRD_PARTY_LIBS lpeg)


message(STATUS  "          build => inspect")
add_custom_command(OUTPUT  ${DEPS_LIB_DIR}/luarocks/rocks/inspect
                   COMMAND ${LUAROCKS_BINARY} build inspect ${LUAROCKS_BUILDARGS}
                   DEPENDS mpack)
add_custom_target(inspect
                  DEPENDS  ${DEPS_LIB_DIR}/luarocks/rocks/inspect)
list(APPEND THIRD_PARTY_LIBS inspect)


# for unit test, see: https://github.com/Olivine-Labs/busted/
message(STATUS  "          build => penlight")
add_custom_command(OUTPUT ${DEPS_LIB_DIR}/luarocks/rocks/penlight/1.3.2-2
                   COMMAND ${LUAROCKS_BINARY} build penlight 1.3.2-2 ${LUAROCKS_BUILDARGS}
                   DEPENDS inspect)
add_custom_target(penlight
                  DEPENDS ${DEPS_LIB_DIR}/luarocks/rocks/penlight/1.3.2-2)
list(APPEND THIRD_PARTY_LIBS penlight)


# Run tests
message(STATUS  "          build => busted")
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
message(STATUS  "          build => busted-lua")
set(BUSTED_LUA  "${DEPS_BIN_DIR}/busted-lua")
add_custom_command(OUTPUT ${BUSTED_LUA}
                   COMMAND sed -e 's/^exec/exec $$LUA_DEBUGGER/' -e 's/jit//g' < ${BUSTED_JIT} > ${BUSTED_LUA}
                   COMMAND chmod +x ${BUSTED_LUA}
                   DEPENDS busted)
add_custom_target(busted-lua DEPENDS ${BUSTED_LUA})
list(APPEND THIRD_PARTY_LIBS busted-lua)


message(STATUS  "          build => luacheck")
set(luacheck_rockspec_url
    "https://raw.githubusercontent.com/mpeterv/luacheck/master/luacheck-scm-1.rockspec")
add_custom_command(OUTPUT  ${DEPS_BIN_DIR}/luacheck
                   COMMAND ${LUAROCKS_BINARY} build ${luacheck_rockspec_url} ${LUAROCKS_BUILDARGS}
                   DEPENDS busted)
add_custom_target(luacheck
                  DEPENDS  ${DEPS_BIN_DIR}/luacheck)
list(APPEND THIRD_PARTY_LIBS luacheck)
