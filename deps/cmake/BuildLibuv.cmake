message(STATUS  "Building: libuv-v${LIBUV_VERSION}")
include(CMakeParseArguments)

# BuildLibuv(<TARGET targetname> <CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildLibuv)
    cmake_parse_arguments(_libuv
    "BUILD_IN_SOURCE"
    "TARGET"
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _libuv_CONFIGURE_COMMAND AND
       NOT _libuv_BUILD_COMMAND     AND
       NOT _libuv_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    if(NOT _libuv_TARGET)
        set(_libuv_TARGET "libuv")
    endif()

    ExternalProject_Add(   ${_libuv_TARGET}
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LIBUV_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/libuv
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/libuv
                           -DDOWNLOAD_URL=${LIBUV_URL}
                           -DDOWNLOAD_VER=${LIBUV_VERSION}
                           -DBUILD_TARGET=${_libuv_TARGET}
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=${_libuv_BUILD_IN_SOURCE}
                           -DEXPECT_SHA256=${LIBUV_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LIBUV}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        CONFIGURE_COMMAND  ${_libuv_CONFIGURE_COMMAND}
        BUILD_IN_SOURCE    ${_libuv_BUILD_IN_SOURCE}
        WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/libuv
        BUILD_COMMAND      ${_libuv_BUILD_COMMAND}
        INSTALL_COMMAND    ${_libuv_INSTALL_COMMAND})
endfunction()

set(UNIX_CFGCMD sh ${DEPS_BUILD_DIR}/src/libuv/autogen.sh &&
                   ${DEPS_BUILD_DIR}/src/libuv/configure
                   --with-pic
                   --disable-shared
                   --prefix=${DEPS_INSTALL_DIR}
                   --libdir=${DEPS_INSTALL_DIR}/lib
                   CC=${DEPS_C_COMPILER})

if(UNIX OR CYGWIN)
    BuildLibuv(BUILD_IN_SOURCE
               CONFIGURE_COMMAND ${UNIX_CFGCMD}
               INSTALL_COMMAND   ${MAKE_PROG} V=1 install)
else()
    set(err_msg "Trying to build [ libuv ] in an unsupported system.")
    set(err_msg "${err_msg}\n  Current System Name  : ${CMAKE_SYSTEM_NAME}")
    set(err_msg "${err_msg}\n  Current C Compiler ID: ${CMAKE_C_COMPILER_ID}")
    message(FATAL_ERROR "${err_msg}")
endif()

list(APPEND THIRD_PARTY_LIBS libuv)
