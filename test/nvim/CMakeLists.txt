# Setup busted.
message(STATUS "Enable nvim testing")
find_program(BUSTED_PROG NAMES busted busted.bat)
if(BUSTED_PROG MATCHES "NOTFOUND")
    message(WARNING "busted: program not find, testing skip ...")
    return()
endif()

find_program(BUSTED_LUA_PROG busted-lua)
if(BUSTED_LUA_PROG MATCHES "NOTFOUND")
    message(WARNING "busted-lua: program not find, testing skip ...")
    return()
endif()

find_program(LUACHECK_PROG luacheck)
if(LUACHECK_PROG MATCHES "NOTFOUND")
    message(WARNING "luacheck: program not find, testing skip ...")
    return()
endif()

# busted output type: http://olivinelabs.com/busted/#output-handlers
# utfTerminal, plainTerminal, junit, json, TAP
set(BUSTED_OUTPUT_TYPE "OutputHandlers")

# testing configuration
add_subdirectory(config)

# prerequisite fixtures for functional testing
set(NVIM_FUNCTIONAL_TEST_PREREQS nvim
                                 nvim-shell-test
                                 nvim-printargs-test)

if(NOT WIN32)
    list(APPEND NVIM_FUNCTIONAL_TEST_PREREQS nvim-tty-test)
endif()

add_custom_target(nvim-functional-test-prereqs
                  DEPENDS ${NVIM_FUNCTIONAL_TEST_PREREQS})

add_custom_target(run-nvim-functional-test
                  COMMAND  ${CMAKE_COMMAND}
                           -DLUA_PROG=${LUA_PROG}
                           -DBUSTED_PROG=${BUSTED_PROG}
                           -DNVIM_PROG=$<TARGET_FILE:nvim>
                           -DLUA_PATH_ENV="${LUA_PATH_SEARCH_PATH}"
                           -DLUA_CPATH_ENV="${LUA_CPATH_SEARCH_PATH}"
                           -DWORKING_SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                           -DWORKING_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
                           -DBUSTED_OUTPUT_TYPE=${BUSTED_OUTPUT_TYPE}
                           -DTEST_TYPE=functional
                           -DBUILD_DIR=${CMAKE_BINARY_DIR}
                           -DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
                           -P ${PROJECT_SOURCE_DIR}/cmake/RunNvimTests.cmake
                  DEPENDS  nvim-functional-test-prereqs)
