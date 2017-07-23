# Host use only, no link to nvim or snail

include(CMakeParseArguments)

# BuildLuarocks(<CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildGperf)
    cmake_parse_arguments(_gperf
    ""
    ""
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _gperf_CONFIGURE_COMMAND AND
       NOT _gperf_BUILD_COMMAND     AND
       NOT _gperf_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    ExternalProject_Add(   gperf
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${GPERF_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/gperf
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/gperf
                           -DDOWNLOAD_URL=${GPERF_URL}
                           -DDOWNLOAD_VER=${GPERF_VERSION}
                           -DBUILD_TARGET=gperf
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=1
                           -DEXPECT_SHA256=${GPERF_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_GPERF}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        CONFIGURE_COMMAND  "${_gperf_CONFIGURE_COMMAND}"
        BUILD_IN_SOURCE    1
        WORKING_DIRECTORY  "${DEPS_BUILD_DIR}/src/gperf"
        BUILD_COMMAND      "${_gperf_BUILD_COMMAND}"
        INSTALL_COMMAND    "${_gperf_INSTALL_COMMAND}")
endfunction()

find_program(GPERF_PROG gperf)
if(GPERF_PROG)
    message(STATUS "Building: gperf is found, skip building ...")
    return()
endif()

if(UNIX)
    message(STATUS "Building: gperf-v${GPERF_VERSION}")
    BuildGperf(CONFIGURE_COMMAND ${DEPS_BUILD_DIR}/src/gperf/configure
                                 --prefix=${HOSTDEPS_INSTALL_DIR}
               INSTALL_COMMAND   ${MAKE_PROG} install)

else()
    set(err_msg "Trying to build [ gperf ] in an unsupported system.")
    set(err_msg "${err_msg}\n  Host System Name  : ${CMAKE_HOST_SYSTEM}")
    set(err_msg "${err_msg}\n  Target System Name: ${CMAKE_SYSTEM}")
    set(err_msg "${err_msg}\n  C Compiler Name   : ${CMAKE_C_COMPILER}")
    set(err_msg "${err_msg}\n  CXX Compiler Name : ${CMAKE_CXX_COMPILER}")
    message(FATAL_ERROR "${err_msg}")
endif()

list(APPEND THIRD_PARTY_LIBS gperf)
