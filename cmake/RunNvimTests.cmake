set(ENV{SYSTEM_NAME} ${SYSTEM_NAME})

# not child process of the main process, need to set lua path
foreach(item ${LUA_PATH_ENV})
    string(STRIP "${item}" item)
    set(ENV{LUA_PATH} "${item};$ENV{LUA_PATH}")
endforeach()

foreach(item ${LUA_CPATH_ENV})
    string(STRIP "${item}" item)
    set(ENV{LUA_CPATH} "${item};$ENV{LUA_CPATH}")
endforeach()

if(NVIM_PROG)
    set(ENV{NVIM_PROG} "${NVIM_PROG}")
endif()

if(DEFINED ENV{NVIM_TEST_TAGS})
    set(TEST_TAGS "--tags=$ENV{NVIM_TEST_TAGS}")
endif()

if(DEFINED ENV{NVIM_TEST_EXCLUDE_TAGS})
    set(TEST_EXCLUDE_TAGS "--exclude-tags=$ENV{NVIM_TEST_EXCLUDE_TAGS}")
endif()

if(DEFINED ENV{NVIM_TEST_FILTER})
    set(TEST_FILTER "--filter=$ENV{NVIM_TEST_FILTER}")
endif()

# set the testing spec root directory
if(DEFINED ENV{NVIM_TEST_SPEC_DIR})
    set(TEST_SPEC_DIR "$ENV{NVIM_TEST_SPEC_DIR}")
else()
    set(TEST_SPEC_DIR "${WORKING_SRC_DIR}/${TEST_TYPE}")
endif()

# busted output type, environment value have high priority
if(DEFINED ENV{NVIM_TEST_LOG_TYPE})
    set(BUSTED_OUTPUT_TYPE "$ENV{NVIM_TEST_LOG_TYPE}")
endif()

# busted default output type, in case not given
if(NOT BUSTED_OUTPUT_TYPE)
    set(BUSTED_OUTPUT_TYPE "utfTerminal")
endif()

# check if we want to save the testing log
if(DEFINED ENV{NVIM_TEST_LOG})
    set(TEST_LOG_FILE OUTPUT_FILE $ENV{NVIM_TEST_LOG})
else()
    if(BUSTED_OUTPUT_TYPE STREQUAL "junit")
        set(TEST_LOG_FILE OUTPUT_FILE ${BUILD_DIR}/nvim-${TEST_TYPE}-test-junit.xml)
    endif()

    if(BUSTED_OUTPUT_TYPE STREQUAL "json")
        set(TEST_LOG_FILE OUTPUT_FILE ${BUILD_DIR}/nvim-${TEST_TYPE}-test.json)
    endif()
endif()

# run busted testing
execute_process(COMMAND ${BUSTED_PROG} ${TEST_TAGS}
                                       ${TEST_EXCLUDE_TAGS}
                                       ${TEST_FILTER}
                                       -v
                                       --lazy
                                       -o ${BUSTED_OUTPUT_TYPE}
                                       --lua=${LUA_PROG}
                                       --helper=${WORKING_SRC_DIR}/${TEST_TYPE}/preload.lua
                                       --lpath=${WORKING_BIN_DIR}/?.lua
                                       --lpath=${WORKING_SRC_DIR}/?.lua
                                       --lpath=$ENV{NVIM_TEST_LUA_PATH}
                                       ${TEST_SPEC_DIR}
                WORKING_DIRECTORY      ${WORKING_SRC_DIR}
                ERROR_VARIABLE         err_msg
                RESULT_VARIABLE        res
                ${TEST_LOG_FILE})

if(NOT res EQUAL 0)
    message(STATUS "Output to stderr:\n${err_msg}\n")
    message(FATAL_ERROR "Running nvim ${TEST_TYPE} tests failed with error code: ${res}")
endif()
