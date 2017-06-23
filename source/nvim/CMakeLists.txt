# Build flags for Dev build type
if(MSVC)
    set(CMAKE_C_FLAGS_DEV "" CACHE STRING "CFlags used for development build." FORCE)
else()
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
endif()
mark_as_advanced(CMAKE_C_FLAGS_DEV)

# Default to -O2 on release build
if(CMAKE_C_FLAGS_RELEASE MATCHES "-O3")
    message(STATUS "Replacing -O3 in CMAKE_C_FLAGS_RELEASE with -O2 for release build.")
    string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()

# Default to -O3 on MinSizeRel build
if(CMAKE_C_FLAGS_MINSIZEREL MATCHES "-O2")
    message(STATUS "Replacing -O2 in CMAKE_C_FLAGS_MINSIZEREL with -O3 for MinSizeRel build.")
    string(REPLACE "-O2" "-O3" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
endif()

# Fixing case-insensitive filenames for Windows and Mac
if(HOST_OS_WIN OR HOST_OS_MACOS)
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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")

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

add_definitions(-DINCLUDE_GENERATED_DECLARATIONS)

set(GENERATOR_DIR ${PROJECT_SOURCE_DIR}/scripts/generators)

#file(MAKE_DIRECTORY ${AUTO_NVIM_DIR})
#file(MAKE_DIRECTORY ${AUTO_INCLUDE_DIR})

# header generator: static/global function declarations
set(GENERATOR_HEADERS  ${GENERATOR_DIR}/gen_header.lua)

file(GLOB NVIM_SOURCES *.c)
file(GLOB NVIM_HEADERS *.h)
file(GLOB NVIM_API_HEADERS api/*.h)


list(APPEND subdir_set "os")
list(APPEND subdir_set "api")
list(APPEND subdir_set "lua")
list(APPEND subdir_set "eval")
list(APPEND subdir_set "event")
list(APPEND subdir_set "api/private")
list(APPEND subdir_set "rpc/msgpack")

foreach(subdir ${subdir_set})
    file(MAKE_DIRECTORY ${AUTO_NVIM_DIR}/${subdir})
    file(MAKE_DIRECTORY ${AUTO_INCLUDE_DIR}/${subdir})

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

set_source_files_properties(${CONV_SOURCES} PROPERTIES COMPILE_FLAGS
                            "${NVIM_COMPILE_FLAGS} -Wno-conversion")

# gperf generates ANSI-C with incorrect linkage, ignore it.
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

# NVIM_GEN_SRC_FILES  : generated source files
# NVIM_GEN_SOURCE_INC : generated headers to be included in sources, static
# NVIM_GEN_HEADER_INC : generated headers to be included in headers, global
#
# These lists must be mutually exclusive.

foreach(sfile ${NVIM_SOURCES})
    get_filename_component(full_path ${sfile} DIRECTORY) # get directory without file name

    file(RELATIVE_PATH relative_path "${CMAKE_CURRENT_LIST_DIR}" "${full_path}")

    if(${relative_path} MATCHES "auto/nvim")
        file(RELATIVE_PATH relative_path "${AUTO_NVIM_DIR}" "${full_path}")
    endif()

    get_filename_component(file_name_we ${sfile} NAME)    # file name with extension
    get_filename_component(file_name_oe ${sfile} NAME_WE) # file name without extension

    if(NOT ${relative_path} EQUAL ".")
        set(file_name_we "${relative_path}/${file_name_we}")
        set(file_name_oe "${relative_path}/${file_name_oe}")
    endif()

    # generated files
    set(gf_i   "${AUTO_NVIM_DIR}/${file_name_oe}.i") # processor output file
    set(gf_c_h "${AUTO_NVIM_DIR}/${file_name_oe}.c.generated.h") # static, to be included in sources
    set(gf_h_h "${AUTO_INCLUDE_DIR}/${file_name_oe}.h.generated.h") # global, to be included in headers

    # set preprocessor output directory for source file
    set(PREPROC_OUTPUT -E -o ${gf_i})

    # generating the header files
    add_custom_command(OUTPUT   ${gf_c_h}
                                ${gf_h_h}
                       COMMAND  ${CMAKE_C_COMPILER}
                                ${sfile}
                                ${PREPROC_OUTPUT}
                                ${gen_cflags}
                       COMMAND  ${CMAKE_COMMAND} -E
                                env LUA_PATH="${LUA_PATH_SEARCH_PATH}\;$ENV{LUA_PATH}\;"
                                env LUA_CPATH="${LUA_CPATH_SEARCH_PATH}\;$ENV{LUA_CPATH}\;"
                                ${LUA_PROG} ${GENERATOR_HEADERS}
                                ${sfile}
                                ${gf_c_h}
                                ${gf_h_h}
                                ${gf_i}
                       DEPENDS  ${GENERATOR_HEADERS}
                                ${sfile})

    list(APPEND NVIM_GEN_SOURCE_INC "${gf_c_h}")
    list(APPEND NVIM_GEN_HEADER_INC "${gf_h_h}")

    if(${d} MATCHES "^api$" AND NOT ${f} MATCHES "^api/helpers.c$")
        list(APPEND NVIM_API_HEADERS ${gf_h_h})
    endif()
endforeach()

if(1)
add_executable(nvim ${NVIM_GEN_SOURCE_INC}
                    ${NVIM_GEN_HEADER_INC}
                    ${NVIM_GEN_SRC_FILES}
                    ${NVIM_SOURCES}
                    ${NVIM_HEADERS})

target_link_libraries(nvim ${NVIM_EXEC_LINK_LIBRARIES})
endif()
