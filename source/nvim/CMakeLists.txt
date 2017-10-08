set(NVIM_TRACE_ENABLE false) # disable nvim trace logging by default
set(NVIM_DEBUG_ENABLE false) # disable nvim debug logging by default
set(NVIM_STATE_ENABLE false) # disable nvim state logging by default
set(NVIM_ALERT_ENABLE false) # enable  nvim alert logging by default
set(NVIM_ERROR_ENABLE false) # enable  nvim error logging by default
set(NVIM_FATAL_ENABLE false) # enable  nvim fatal logging by default

set(NVIM_LANGUAGE_DEBUG_ENABLE false) # disable nvim language debug by default

# Logging Level:
# TRACE(0), DEBUG(1), STATE(2), ALERT(3), ERROR(4), FATAL(5), LOGOFF(6)
if(NOT NVIM_LOG_LEVEL_MIN)
    if(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(NVIM_LOG_LEVEL_MIN  6) # LOGOFF(6)
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(NVIM_LOG_LEVEL_MIN  3) # ALERT(3)
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(NVIM_LOG_LEVEL_MIN  1) # DEBUG(1)
        set(PACKAGE_RELEASE_READY false)
        set(NVIM_LANGUAGE_DEBUG_ENABLE true)
        if(NOT release_check_msg)
            set(release_check_msg "Debug build, not ready to release.")
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "Dev")
        set(NVIM_LOG_LEVEL_MIN  0) # TRACE(0)
        set(PACKAGE_RELEASE_READY false)
        set(NVIM_LANGUAGE_DEBUG_ENABLE true)
        if(NOT release_check_msg)
            set(release_check_msg "Dev build, not ready to release.")
        endif()
    else()
        set(NVIM_LOG_LEVEL_MIN  2) # STATE(2)
        set(PACKAGE_RELEASE_READY false)
        set(NVIM_LANGUAGE_DEBUG_ENABLE true)
        if(NOT release_check_msg)
            set(release_check_msg "${CMAKE_BUILD_TYPE} build, not ready to release.")
        endif()
    endif()
endif()

# Checking logging level
if(NVIM_LOG_LEVEL_MIN MATCHES "^[0-6]$")
    if(NVIM_LOG_LEVEL_MIN EQUAL 0)
        set(log_level_msg "TRACE(0)") # Dev
        set(NVIM_TRACE_ENABLE true) # 0
        set(NVIM_DEBUG_ENABLE true) # 1
        set(NVIM_STATE_ENABLE true) # 2
        set(NVIM_ALERT_ENABLE true) # 3
        set(NVIM_ERROR_ENABLE true) # 4
        set(NVIM_FATAL_ENABLE true) # 5
    elseif(NVIM_LOG_LEVEL_MIN EQUAL 1)
        set(log_level_msg "DEBUG(1)") # Debug
        set(NVIM_DEBUG_ENABLE true) # 1
        set(NVIM_STATE_ENABLE true) # 2
        set(NVIM_ALERT_ENABLE true) # 3
        set(NVIM_ERROR_ENABLE true) # 4
        set(NVIM_FATAL_ENABLE true) # 5
    elseif(NVIM_LOG_LEVEL_MIN EQUAL 2)
        set(log_level_msg "STATE(2)") # XXXX
        set(NVIM_STATE_ENABLE true) # 2
        set(NVIM_ALERT_ENABLE true) # 3
        set(NVIM_ERROR_ENABLE true) # 4
        set(NVIM_FATAL_ENABLE true) # 5
    elseif(NVIM_LOG_LEVEL_MIN EQUAL 3)
        set(log_level_msg "ALERT(3)") # Release
        set(NVIM_ALERT_ENABLE true) # 3
        set(NVIM_ERROR_ENABLE true) # 4
        set(NVIM_FATAL_ENABLE true) # 5
    elseif(NVIM_LOG_LEVEL_MIN EQUAL 4)
        set(log_level_msg "ERROR(4)")
        set(NVIM_ERROR_ENABLE true) # 4
        set(NVIM_FATAL_ENABLE true) # 5
    elseif(NVIM_LOG_LEVEL_MIN EQUAL 5)
        set(log_level_msg "FATAL(5)")
        set(NVIM_FATAL_ENABLE true) # 5
    else()
        set(log_level_msg "LOGOFF(6)")# MinSizeRel
        add_definitions("-DNVIM_LOGGING_DISABLE")
    endif()
    message(STATUS "nvim logging level: ${log_level_msg}")
else()
    set(NVIM_LOG_LEVEL_MIN 2) # illegal, reset to STATE(2)
    set(NVIM_STATE_ENABLE true)
    message(STATUS "nvim logging level: STATE(2)")
endif()

add_definitions("-DNVIM_LOG_LEVEL_MIN=${NVIM_LOG_LEVEL_MIN}")

if(NVIM_TRACE_ENABLE)
    message(STATUS "nvim *enable TRACE(0) logging")
else()
    message(STATUS "nvim disable TRACE(0) logging")
endif()

if(NVIM_DEBUG_ENABLE)
    message(STATUS "nvim *enable DEBUG(1) logging")
else()
    message(STATUS "nvim disable DEBUG(1) logging")
endif()

if(NVIM_STATE_ENABLE)
    message(STATUS "nvim *enable STATE(2) logging")
else()
    message(STATUS "nvim disable STATE(2) logging")
endif()

if(NVIM_ALERT_ENABLE)
    message(STATUS "nvim *enable ALERT(3) logging")
else()
    message(STATUS "nvim disable ALERT(3) logging")
endif()

if(NVIM_ERROR_ENABLE)
    message(STATUS "nvim *enable ERROR(4) logging")
else()
    message(STATUS "nvim disable ERROR(4) logging")
endif()

if(NVIM_FATAL_ENABLE)
    message(STATUS "nvim *enable FATAL(5) logging")
else()
    message(STATUS "nvim disable FATAL(5) logging")
endif()

if(NVIM_LANGUAGE_DEBUG_ENABLE)
    add_definitions("-DNVIM_LANGUAGE_DEBUG_ENABLE")
    message(STATUS "*enable nvim language debug")
else()
    message(STATUS "disable nvim language debug")
endif()

include(ConfigNvim) # generate configurations for nvim only

# Build flags for Dev build type
if(CMAKE_COMPILER_IS_GNUCC)
    check_c_compiler_flag(-Og GNUCC_HAS_OG_FLAG)
else()
    set(GNUCC_HAS_OG_FLAG false)
endif()

if(GNUCC_HAS_OG_FLAG)
    set(CMAKE_C_FLAGS_DEV "-Og -g" CACHE STRING "CFlags used for development build." FORCE)
else()
    set(CMAKE_C_FLAGS_DEV "-O2 -g" CACHE STRING "CFlags used for development build." FORCE)
endif()

mark_as_advanced(CMAKE_C_FLAGS_DEV)

# Default to -O2 on release build
if(${build_type} STREQUAL "RELEASE" AND CMAKE_C_FLAGS_RELEASE MATCHES "-O3")
    message(STATUS "Replacing -O3 in CMAKE_C_FLAGS_RELEASE with -O2 for release build.")
    string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()

# Default to -O3 on MinSizeRel build
if(${build_type} STREQUAL "MINSIZEREL" AND CMAKE_C_FLAGS_MINSIZEREL MATCHES "-O2")
    message(STATUS "Replacing -O2 in CMAKE_C_FLAGS_MINSIZEREL with -O3 for MinSizeRel build.")
    string(REPLACE "-O2" "-O3" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
endif()

# Fixing case-insensitive filenames for Windows and Mac
if(HOST_OS_WINDOWS OR HOST_OS_MACOS)
    add_definitions("-DFIX_CASE_INSENSITIVE")
    message(STATUS "Enable fix case-insensitive for Mac/Windows")
endif()

# gcc 4.0 and higher turn on _FORTIFY_SOURCE=2 automatically.
include(CheckCSourceCompiles)

# Include the build type's default flags in the checking for _FORTIFY_SOURCE,
# otherwise we may incorrectly identify the level as acceptable and find out
# later that it was not when optimizations were enabled.
# CFLAGS is applied even though you don't see it in CMAKE_REQUIRED_FLAGS.
if(CMAKE_C_FLAGS_${build_type})
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_C_FLAGS_${build_type}}")
endif()

# include <string.h> because some toolchains define _FORTIFY_SOURCE=2 in
# internal header files, which should in turn be included by <string.h>
check_c_source_compiles(
"
#include <string.h>

#if defined(_FORTIFY_SOURCE) && _FORTIFY_SOURCE > 1
#   error \"_FORTIFY_SOURCE > 1\"
#endif

int main(void)
{
    return 0;
}
"
GNUCC_HAS_ACCEPTABLE_FORTIFY)

if(NOT GNUCC_HAS_ACCEPTABLE_FORTIFY)
    message(STATUS "Unsupported _FORTIFY_SOURCE found, force set _FORTIFY_SOURCE=1")

    # extract possible prefix to _FORTIFY_SOURCE (e.g. -Wp,-D_FORTIFY_SOURCE)
    string(REGEX MATCH "[^\ ]+-D_FORTIFY_SOURCE" _FORTIFY_SOURCE_WITH_PREFIX "${CMAKE_C_FLAGS}")
    string(REPLACE "-D_FORTIFY_SOURCE" "" _FORTIFY_SOURCE_PREFIX "${_FORTIFY_SOURCE_WITH_PREFIX}")
    if(NOT _FORTIFY_SOURCE_PREFIX STREQUAL "")
        message(STATUS "Detected _FORTIFY_SOURCE prefix=${_FORTIFY_SOURCE_PREFIX}")
    endif()

    # -U in add_definitions does not work, so add it to the flags variable instead
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${_FORTIFY_SOURCE_PREFIX}-U_FORTIFY_SOURCE")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${_FORTIFY_SOURCE_PREFIX}-D_FORTIFY_SOURCE=1")
endif()

if(MINGW)
    # Use POSIX compatible stdio in Mingw
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USE_MINGW_ANSI_STDIO")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WIN32_WINNT=0x0600")
endif()

check_c_compiler_flag(-Wvla GNUCC_HAS_WVLA_FLAG)
if(GNUCC_HAS_WVLA_FLAG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wvla")
endif()

if(UNIX)
    # -fstack-protector breaks non Unix builds even in Mingw-w64
    check_c_compiler_flag(-fstack-protector GNUCC_HAS_FSTACK_PROTECTOR_FLAG)
    check_c_compiler_flag(-fstack-protector-strong GNUCC_HAS_FSTACK_PROTECTOR_STRONG_FLAG)

    if(GNUCC_HAS_FSTACK_PROTECTOR_STRONG_FLAG)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
    elseif(GNUCC_HAS_FSTACK_PROTECTOR_FLAG)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector --param ssp-buffer-size=4")
    endif()
endif()

check_c_compiler_flag(-fdiagnostics-color=auto GNUCC_HAS_DIAG_COLOR_FLAG)
if(GNUCC_HAS_DIAG_COLOR_FLAG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=auto")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")

# for config, hard code into source
set(NVIM_CONFIG_C_FLAGS ${CMAKE_C_FLAGS} PARENT_SCOPE)
set(NVIM_CONFIG_C_FLAGS_${build_type} ${CMAKE_C_FLAGS_${build_type}} PARENT_SCOPE)

# nvim source/header files
file(GLOB NVIM_SOURCES *.c)
file(GLOB NVIM_HEADERS *.h)

file(GLOB NVIM_API_HEADERS api/*.h)
list(REMOVE_ITEM NVIM_API_HEADERS ${CMAKE_CURRENT_LIST_DIR}/api/ui_events.in.h)

list(APPEND subdir_set "os")
list(APPEND subdir_set "api")
list(APPEND subdir_set "lua")
list(APPEND subdir_set "eval")
list(APPEND subdir_set "event")
list(APPEND subdir_set "api/private")
list(APPEND subdir_set "msgpack")

set(NVIM_AUTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/auto)
set(NVIM_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)

foreach(subdir ${subdir_set})
    file(MAKE_DIRECTORY ${NVIM_AUTO_DIR}/${subdir})
    file(MAKE_DIRECTORY ${NVIM_INCLUDE_DIR}/${subdir})

    file(GLOB sources ${subdir}/*.c)
    file(GLOB headers ${subdir}/*.h)

    list(APPEND NVIM_SOURCES ${sources})
    list(APPEND NVIM_HEADERS ${headers})
endforeach()

# Sort file lists to ensure generated files are created in the same order
list(SORT NVIM_SOURCES)
list(SORT NVIM_HEADERS)

foreach(sfile ${NVIM_SOURCES})
    get_filename_component(f ${sfile} NAME)

    if(${f} MATCHES "^(regexp_nfa.c)$")
        list(APPEND to_remove ${sfile})
    endif()

    if(WIN32 AND ${f} MATCHES "^(pty_process_unix.c)$")
        list(APPEND to_remove ${sfile})
    endif()
endforeach()

list(REMOVE_ITEM NVIM_SOURCES ${to_remove})

# Legacy files that do not yet pass -Wconversion
list(APPEND CONV_SOURCES "tag.c")
list(APPEND CONV_SOURCES "diff.c")
list(APPEND CONV_SOURCES "edit.c")
list(APPEND CONV_SOURCES "eval.c")
list(APPEND CONV_SOURCES "mbyte.c")
list(APPEND CONV_SOURCES "spell.c")
list(APPEND CONV_SOURCES "fileio.c")
list(APPEND CONV_SOURCES "syntax.c")
list(APPEND CONV_SOURCES "window.c")
list(APPEND CONV_SOURCES "regexp.c")
list(APPEND CONV_SOURCES "screen.c")
list(APPEND CONV_SOURCES "search.c")
list(APPEND CONV_SOURCES "ex_cmds.c")
list(APPEND CONV_SOURCES "memline.c")
list(APPEND CONV_SOURCES "message.c")
list(APPEND CONV_SOURCES "ex_docmd.c")
list(APPEND CONV_SOURCES "ex_getln.c")
list(APPEND CONV_SOURCES "spellfile.c")

foreach(sfile ${CONV_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${sfile}")
        message(FATAL_ERROR "[${sfile}] doesn't exist (it was added to CONV_SOURCES)")
    endif()
endforeach()

include_directories("${NVIM_AUTO_DIR}")
include_directories("${NVIM_INCLUDE_DIR}")
add_definitions(-DINCLUDE_GENERATED_DECLARATIONS)
set(GENERATOR_DIR ${CMAKE_CURRENT_LIST_DIR}/generators)

# Headers
set(GENERATOR_HEADERS ${GENERATOR_DIR}/gen_header.lua)

# API
file(GLOB NVIM_RPC_MSGPACK_HEADERS msgpack/*.h)
set(GENERATOR_API_DISPATCH ${GENERATOR_DIR}/gen_api_dispatch.lua)
set(NVIM_GEN_BIN_API_METADATA ${GEN_BINARY_DATA_DIR}/nvim_api_metadata.mpack)
set(NVIM_GEN_SRC_FUNCS_METADATA ${NVIM_AUTO_DIR}/api/private/funcs_metadata.generated.h)
set(NVIM_GEN_SRC_API_DISPATCH ${NVIM_AUTO_DIR}/api/private/dispatch_wrappers.generated.h)
set(NVIM_GEN_SRC_MSGPACK_LUA_C_BINDINGS ${NVIM_AUTO_DIR}/msgpack_lua_c_bindings.generated.c)

# UI events
set(GENERATOR_API_UI_EVENTS ${GENERATOR_DIR}/gen_api_ui_events.lua)
set(NVIM_GEN_SRC_UI_EVENTS ${NVIM_AUTO_DIR}/ui_events.generated.h)
set(NVIM_GEN_SRC_UI_EVENTS_CALL ${NVIM_AUTO_DIR}/ui_events_call.generated.h)
set(NVIM_GEN_SRC_UI_EVENTS_REMOTE ${NVIM_AUTO_DIR}/ui_events_remote.generated.h)
set(NVIM_GEN_SRC_UI_EVENTS_BRIDGE ${NVIM_AUTO_DIR}/ui_events_bridge.generated.h)
set(NVIM_GEN_SRC_UI_EVENTS_METADATA ${NVIM_AUTO_DIR}/api/private/ui_events_metadata.generated.h)

# events
set(GENERATOR_EVENTS ${GENERATOR_DIR}/gen_events.lua)
set(NVIM_GEN_SRC_EVENTS_ENUM ${NVIM_INCLUDE_DIR}/auevents_enum.generated.h)
set(NVIM_GEN_SRC_EVENTS_NAMES_MAP ${NVIM_AUTO_DIR}/auevents_name_map.generated.h)

# EX commands
set(GENERATOR_EX_CMDS ${GENERATOR_DIR}/gen_ex_cmds.lua)
set(NVIM_GEN_SRC_EX_CMDS_ENUM ${NVIM_INCLUDE_DIR}/ex_cmds_enum.generated.h)
set(NVIM_GEN_SRC_EX_CMDS_DEFS ${NVIM_AUTO_DIR}/ex_cmds_defs.generated.h)

# vim language
set(GENERATOR_FUNCS ${GENERATOR_DIR}/gen_eval.lua)
set(NVIM_GEN_SRC_FUNCS ${NVIM_AUTO_DIR}/funcs.generated.h)
set(NVIM_GEN_BIN_FUNCS_DATA ${GEN_BINARY_DATA_DIR}/nvim_funcs_data.mpack)

# vim module
set(GENERATOR_CHAR_BLOB ${GENERATOR_DIR}/gen_char_blob.lua)
set(VIM_MODULE_SOURCE ${CMAKE_CURRENT_LIST_DIR}/lua/vim.lua)
set(NVIM_GEN_SRC_VIM_MODULE ${NVIM_AUTO_DIR}/lua/vim_module.generated.h)

# options
set(GENERATOR_OPTIONS ${GENERATOR_DIR}/gen_options.lua)
set(NVIM_GEN_SRC_OPTIONS ${NVIM_AUTO_DIR}/options.generated.h)

# unicode
set(UNICODE_DIR ${CMAKE_CURRENT_LIST_DIR}/unicode)
file(GLOB UNICODE_FILES ${UNICODE_DIR}/*.txt)
set(GENERATOR_UNICODETAB ${GENERATOR_DIR}/gen_unicode_tables.lua)
set(NVIM_GEN_SRC_UNICODETABLE ${NVIM_AUTO_DIR}/unicode_tables.generated.h)

# NVIM_GEN_SOURCES   : generated source files
list(APPEND NVIM_GEN_SOURCES "${NVIM_GEN_SRC_MSGPACK_LUA_C_BINDINGS}")

# NVIM_GEN_FOR_HEADER: generated headers to be included in headers, global
list(APPEND NVIM_GEN_FOR_HEADER "${NVIM_GEN_SRC_EX_CMDS_ENUM}")
list(APPEND NVIM_GEN_FOR_HEADER "${NVIM_GEN_SRC_EVENTS_ENUM}")

# NVIM_GEN_FOR_SOURCE: generated headers to be included in sources, static
list(APPEND NVIM_GEN_FOR_SOURCE "${NVIM_GEN_SRC_API_DISPATCH}")
list(APPEND NVIM_GEN_FOR_SOURCE "${NVIM_GEN_SRC_EX_CMDS_DEFS}")
list(APPEND NVIM_GEN_FOR_SOURCE "${NVIM_GEN_SRC_EVENTS_NAMES_MAP}")
list(APPEND NVIM_GEN_FOR_SOURCE "${NVIM_GEN_SRC_OPTIONS}")
list(APPEND NVIM_GEN_FOR_SOURCE "${NVIM_GEN_SRC_UNICODETABLE}")
list(APPEND NVIM_GEN_FOR_SOURCE "${NVIM_GEN_SRC_VIM_MODULE}")
list(APPEND NVIM_GEN_FOR_SOURCE "${NVIM_GEN_SRC_FUNCS}")

# NVIM_GEN_FOR_HEADER and NVIM_GEN_FOR_SOURCE must be mutually exclusive.
foreach(hfile ${NVIM_GEN_FOR_HEADER})
    list(FIND NVIM_GEN_FOR_SOURCE ${hfile} hfile_idx)
    if(NOT ${hfile_idx} EQUAL -1)
        message(FATAL_ERROR "${hfile} in both: NVIM_GEN_FOR_HEADER, NVIM_GEN_FOR_SOURCE")
    endif()
endforeach()

set_source_files_properties(${CONV_SOURCES} PROPERTIES COMPILE_FLAGS
                            "${NVIM_COMPILE_FLAGS} -Wno-conversion")

check_c_compiler_flag(-Wno-static-in-inline GNUCC_HAS_WNO_STATIC_IN_INLINE_FLAG)
if(GNUCC_HAS_WNO_STATIC_IN_INLINE_FLAG)
    set_source_files_properties(eval.c PROPERTIES COMPILE_FLAGS
                                "${NVIM_COMPILE_FLAGS} -Wno-static-in-inline -Wno-conversion")
else()
    set_source_files_properties(eval.c PROPERTIES COMPILE_FLAGS
                                "${NVIM_COMPILE_FLAGS} -Wno-conversion")
endif()

get_directory_property(cflags_defs COMPILE_DEFINITIONS)
get_directory_property(includes_defs INCLUDE_DIRECTORIES)

foreach(cflag ${cflags_defs})
    if(NOT "${cflag}" MATCHES "INCLUDE_GENERATED_DECLARATIONS")
        list(APPEND gen_cflags "-D${cflag}")
    endif()
endforeach()

foreach(include ${includes_defs})
    list(APPEND gen_cflags "-I${include}")
endforeach()

separate_arguments(NVIM_CFLAGS UNIX_COMMAND ${CMAKE_C_FLAGS})
separate_arguments(NVIM_CFLAGS_${build_type} UNIX_COMMAND ${CMAKE_C_FLAGS_${build_type}})

set(gen_cflags ${gen_cflags} ${NVIM_CFLAGS_${build_type}} ${NVIM_CFLAGS})

foreach(sfile ${NVIM_SOURCES}
              ${CMAKE_CURRENT_LIST_DIR}/regexp_nfa.c
              ${NVIM_GEN_SRC_API_DISPATCH}
              ${NVIM_GEN_SRC_UI_EVENTS_CALL}
              ${NVIM_GEN_SRC_UI_EVENTS_REMOTE}
              ${NVIM_GEN_SRC_UI_EVENTS_BRIDGE})
    get_filename_component(full_path ${sfile} DIRECTORY) # get directory without file name

    file(RELATIVE_PATH relative_path "${CMAKE_CURRENT_LIST_DIR}" "${full_path}")

    if(${relative_path} MATCHES "nvim/auto")
        file(RELATIVE_PATH relative_path "${NVIM_AUTO_DIR}" "${full_path}")
    endif()

    get_filename_component(file_name_we ${sfile} NAME)    # file name with extension
    get_filename_component(file_name_oe ${sfile} NAME_WE) # file name without extension

    if(NOT ${relative_path} EQUAL ".")
        set(file_name_we "${relative_path}/${file_name_we}")
        set(file_name_oe "${relative_path}/${file_name_oe}")
    endif()

    # generated files
    set(gf_i   "${NVIM_AUTO_DIR}/${file_name_oe}.i") # processor output file
    set(gf_c_h "${NVIM_AUTO_DIR}/${file_name_oe}.c.generated.h") # static, to be included in sources
    set(gf_h_h "${NVIM_INCLUDE_DIR}/${file_name_oe}.h.generated.h") # global, to be included in headers

    # set preprocessor output directory for source file
    set(preproc_output -E -o ${gf_i})

    # generating the header files
    # not child process of the main process, need to set lua env
    add_custom_command(OUTPUT   ${gf_c_h}
                                ${gf_h_h}
                       COMMAND  ${CMAKE_C_COMPILER} ${sfile} ${preproc_output} ${gen_cflags}
                       COMMAND  ${LUA_PROG} ${GENERATOR_HEADERS}
                                            ${sfile}
                                            ${gf_c_h}
                                            ${gf_h_h}
                                            ${gf_i}
                       DEPENDS  ${GENERATOR_HEADERS}
                                ${sfile})

    list(APPEND NVIM_GEN_FOR_SOURCE "${gf_c_h}")
    list(APPEND NVIM_GEN_FOR_HEADER "${gf_h_h}")

    if(${relative_path} MATCHES "^api$" AND NOT ${file_name_we} MATCHES "^api/helpers.c$")
        list(APPEND NVIM_API_HEADERS ${gf_h_h})
    endif()
endforeach()

# generating: API
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_API_DISPATCH}
                            ${NVIM_GEN_SRC_FUNCS_METADATA}
                            ${NVIM_GEN_BIN_API_METADATA}
                            ${NVIM_GEN_SRC_MSGPACK_LUA_C_BINDINGS}
                   COMMAND  ${LUA_PROG} ${GENERATOR_API_DISPATCH} ${CMAKE_CURRENT_LIST_DIR}
                                        ${NVIM_GEN_SRC_API_DISPATCH}
                                        ${NVIM_GEN_SRC_FUNCS_METADATA}
                                        ${NVIM_GEN_BIN_API_METADATA}
                                        ${NVIM_GEN_SRC_MSGPACK_LUA_C_BINDINGS}
                                        ${NVIM_API_HEADERS}
                   DEPENDS  ${NVIM_API_HEADERS}
                            ${NVIM_RPC_MSGPACK_HEADERS}
                            ${GENERATOR_API_DISPATCH}
                            ${CMAKE_CURRENT_LIST_DIR}/api/dispatch_deprecated.lua)

# generating: vim module
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_VIM_MODULE}
                   COMMAND  ${LUA_PROG} ${GENERATOR_CHAR_BLOB}
                                        ${VIM_MODULE_SOURCE}
                                        ${NVIM_GEN_SRC_VIM_MODULE}
                                        vim_module
                   DEPENDS  ${GENERATOR_CHAR_BLOB}
                            ${VIM_MODULE_SOURCE})

# generating: UI events
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_UI_EVENTS}
                            ${NVIM_GEN_SRC_UI_EVENTS_CALL}
                            ${NVIM_GEN_SRC_UI_EVENTS_REMOTE}
                            ${NVIM_GEN_SRC_UI_EVENTS_BRIDGE}
                            ${NVIM_GEN_SRC_UI_EVENTS_METADATA}
                   COMMAND  ${LUA_PROG} ${GENERATOR_API_UI_EVENTS} ${CMAKE_CURRENT_LIST_DIR}
                                        ${CMAKE_CURRENT_LIST_DIR}/api/ui_events.in.h
                                        ${NVIM_GEN_SRC_UI_EVENTS}
                                        ${NVIM_GEN_SRC_UI_EVENTS_CALL}
                                        ${NVIM_GEN_SRC_UI_EVENTS_REMOTE}
                                        ${NVIM_GEN_SRC_UI_EVENTS_BRIDGE}
                                        ${NVIM_GEN_SRC_UI_EVENTS_METADATA}
                   DEPENDS  ${GENERATOR_API_UI_EVENTS}
                            ${CMAKE_CURRENT_LIST_DIR}/api/ui_events.in.h)

# generating: EX commands
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_EX_CMDS_ENUM}
                            ${NVIM_GEN_SRC_EX_CMDS_DEFS}
                   COMMAND  ${LUA_PROG} ${GENERATOR_EX_CMDS} ${CMAKE_CURRENT_LIST_DIR}
                                        ${NVIM_INCLUDE_DIR}
                                        ${NVIM_AUTO_DIR}
                   DEPENDS  ${GENERATOR_EX_CMDS}
                            ${CMAKE_CURRENT_LIST_DIR}/ex_cmds.lua)

# generating: events
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_EVENTS_ENUM}
                            ${NVIM_GEN_SRC_EVENTS_NAMES_MAP}
                   COMMAND  ${LUA_PROG} ${GENERATOR_EVENTS} ${CMAKE_CURRENT_LIST_DIR}
                                        ${NVIM_GEN_SRC_EVENTS_ENUM}
                                        ${NVIM_GEN_SRC_EVENTS_NAMES_MAP}
                   DEPENDS  ${GENERATOR_EVENTS}
                            ${CMAKE_CURRENT_LIST_DIR}/auevents.lua)

# generating: vim language
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_FUNCS}
                            ${NVIM_GEN_BIN_FUNCS_DATA}
                   COMMAND  ${LUA_PROG} ${GENERATOR_FUNCS} ${CMAKE_CURRENT_LIST_DIR}
                                        ${NVIM_AUTO_DIR}
                                        ${NVIM_GEN_BIN_API_METADATA}
                                        ${NVIM_GEN_BIN_FUNCS_DATA}
                   COMMAND  ${GPERF_PROG} ${NVIM_AUTO_DIR}/funcs.generated.h.gperf
                                          --output-file=${NVIM_GEN_SRC_FUNCS}
                   DEPENDS ${GENERATOR_FUNCS}
                           ${CMAKE_CURRENT_LIST_DIR}/eval.lua
                           ${NVIM_GEN_BIN_API_METADATA})

# generating: options
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_OPTIONS}
                   COMMAND  ${LUA_PROG} ${GENERATOR_OPTIONS} ${CMAKE_CURRENT_LIST_DIR}
                                        ${NVIM_GEN_SRC_OPTIONS}
                   DEPENDS  ${GENERATOR_OPTIONS}
                            ${CMAKE_CURRENT_LIST_DIR}/options.lua)

# generating: unicode
add_custom_command(OUTPUT   ${NVIM_GEN_SRC_UNICODETABLE}
                   COMMAND  ${LUA_PROG} ${GENERATOR_UNICODETAB}
                            ${UNICODE_DIR}
                            ${NVIM_GEN_SRC_UNICODETABLE}
                   DEPENDS  ${GENERATOR_UNICODETAB}
                            ${UNICODE_FILES})

list(APPEND NVIM_ALL_SOURCES ${NVIM_GEN_FOR_SOURCE})
list(APPEND NVIM_ALL_SOURCES ${NVIM_GEN_FOR_HEADER})
list(APPEND NVIM_ALL_SOURCES ${NVIM_GEN_SOURCES})
list(APPEND NVIM_ALL_SOURCES ${NVIM_SOURCES})
list(APPEND NVIM_ALL_SOURCES ${NVIM_HEADERS})

if(HOST_OS_WINDOWS)
    include_directories(${PROJECT_BINARY_DIR})
    list(APPEND NVIM_ALL_SOURCES nvim.rc)
endif()

if(HOST_OS_MACOS)
    #list(APPEND NVIM_ALL_SOURCES info.plist)
endif()

add_executable(nvim ${NVIM_ALL_SOURCES})
install_helper(TARGETS nvim)

# libuv
find_package(LibUV 1.9.1 REQUIRED)
target_include_directories(nvim SYSTEM PUBLIC ${LIBUV_INCLUDE_DIRS})
list(APPEND NVIM_DEP_LINK_LIBRARIES ${LIBUV_LIBRARIES})

# libvterm
find_package(LibVterm REQUIRED)
target_include_directories(nvim SYSTEM PUBLIC ${LIBVTERM_INCLUDE_DIRS})
list(APPEND NVIM_DEP_LINK_LIBRARIES ${LIBVTERM_LIBRARIES})

# libintl
find_package(LibIntl)
if(LibIntl_FOUND)
    target_include_directories(nvim SYSTEM PUBLIC ${LibIntl_INCLUDE_DIRS})
    list(APPEND NVIM_DEP_LINK_LIBRARIES ${LibIntl_LIBRARIES}) # exist separate libintl
    message(STATUS "Found libintl(separate library)")
else()
    if(FOUND_WORKING_LIBINTL)
        message(STATUS "Found libintl(built into glibc)")
    else()
        message(STATUS "NOT Found libintl")
    endif()
endif()

# libiconv
find_package(Iconv)
if(Iconv_FOUND)
    target_include_directories(nvim SYSTEM PUBLIC ${Iconv_INCLUDE_DIRS})
    list(APPEND NVIM_DEP_LINK_LIBRARIES ${Iconv_LIBRARIES})
    message(STATUS "Found libiconv")
    set(FOUND_WORKING_ICONV  1)
else()
    message(STATUS "NOT Found libiconv")
    set(FOUND_WORKING_ICONV  0)
endif()

# lua
find_package(Lua REQUIRED)
target_include_directories(nvim SYSTEM PUBLIC ${LUA_INCLUDE_DIR})
list(APPEND NVIM_DEP_LINK_LIBRARIES ${LUA_LIBRARIES})

if(UNIX)
    # Add libraries for unix.
    list(APPEND NVIM_EXEC_LINK_LIBS  m)
    list(APPEND NVIM_EXEC_LINK_LIBS  util)
endif()

list(APPEND NVIM_EXEC_LINK_LIBS ${NVIM_DEP_LINK_LIBRARIES})

if(CMAKE_HOST_APPLE)
    find_library(CoreFoundation_LIBRARIES CoreFoundation)
    list(APPEND NVIM_EXEC_LINK_LIBS ${CoreFoundation_LIBRARIES})
endif()

target_link_libraries(nvim ${NVIM_EXEC_LINK_LIBS})

if(NVIM_TESTING_ENABLE)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test/nvim
                     ${PROJECT_BINARY_DIR}/test/nvim)
endif()
