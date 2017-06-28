if(NVIM_PROG)
    set(ENV{NVIM_PROG} "${NVIM_PROG}")
endif()

if(DEFINED ENV{NVIM_TEST_FILE})
    set(TEST_PATH "$ENV{NVIM_TEST_FILE}")
else()
    set(TEST_PATH "${TEST_DIR}/${TEST_TYPE}")
endif()

if(DEFINED ENV{NVIM_TEST_TAG})
    set(TEST_TAGS "--tags=$ENV{NVIM_TEST_TAG}")
endif()

if(DEFINED ENV{NVIM_TEST_FILTER})
    set(TEST_FILTER "--filter=$ENV{NVIM_TEST_FILTER}")
endif()

if(BUSTED_OUTPUT_TYPE STREQUAL "junit")
    set(TEST_EXTRA_ARGS "OUTPUT_FILE ${BUILD_DIR}/nvim-${TEST_TYPE}-test-junit.xml")
endif()

set(ENV{SYSTEM_NAME} ${SYSTEM_NAME})
set(ENV{VIMRUNTIME} ${WORKING_SRC_DIR}/runtime)
set(ENV{XDG_DATA_HOME} ${WORKING_SRC_DIR}/Xtest_xdg/share)
set(ENV{XDG_CONFIG_HOME} ${WORKING_SRC_DIR}/Xtest_xdg/config)
set(ENV{NVIM_RPLUGIN_MANIFEST} ${WORKING_SRC_DIR}/Xtest_rplugin_manifest)

set(ENV{TMPDIR} ${WORKING_BIN_DIR}/tmpdir)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKING_BIN_DIR}/tmpdir)

execute_process(COMMAND ${CMAKE_COMMAND} -E
                        env LUA_PATH="${DEP_LUA_PATH}\;$ENV{LUA_PATH}\;"
                        env LUA_CPATH="${DEP_LUA_CPATH}\;$ENV{LUA_CPATH}\;"
                        ${BUSTED_PROG} ${TEST_TAGS}
                                       ${TEST_FILTER}
                                       -v
                                       --lazy
                                       -o ${BUSTED_OUTPUT_TYPE}
                                       --lua=${LUA_PROG}
                                       --helper=${TEST_DIR}/${TEST_TYPE}/preload.lua
                                       --lpath=${BUILD_DIR}/?.lua ${TEST_PATH}
                WORKING_DIRECTORY      ${WORKING_SRC_DIR}
                OUTPUT_VARIABLE        msg
                ERROR_VARIABLE         err
                RESULT_VARIABLE        res
                ${TEST_EXTRA_ARGS})

if(NOT res EQUAL 0)
    message(STATUS "Output to stderr:\n${err}")
    message(FATAL_ERROR "Running [ ${TEST_TYPE} ] tests failed with error: ${res}")
endif()
