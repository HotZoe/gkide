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

if(NOT BUSTED_OUTPUT_TYPE)
    set(BUSTED_OUTPUT_TYPE "nvim")
endif()

# functional testing
add_subdirectory(functional)

add_custom_target(run-nvim-functional-test
                  COMMAND  ${CMAKE_COMMAND}
                           -DLUA_PROG=${LUA_PROG}
                           -DBUSTED_PROG=${BUSTED_PROG}
                           -DNVIM_PROG=$<TARGET_FILE:nvim>
                           -DDEP_LUA_PATH="${LUA_PATH_SEARCH_PATH}"
                           -DDEP_LUA_CPATH="${LUA_CPATH_SEARCH_PATH}"
                           -DWORKING_SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                           -DWORKING_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
                           -DBUSTED_OUTPUT_TYPE=${BUSTED_OUTPUT_TYPE}
                           -DTEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                           -DBUILD_DIR=${CMAKE_BINARY_DIR}
                           -DTEST_TYPE=functional
                           -DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
                           -P ${PROJECT_SOURCE_DIR}/cmake/RunNvimTests.cmake
                  DEPENDS  nvim-functional-test-prereqs)
