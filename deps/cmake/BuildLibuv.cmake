# Dependencies library link to nvim

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

    ExternalProject_Add(   libuv
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LIBUV_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/libuv
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/libuv
                           -DDOWNLOAD_URL=${LIBUV_URL}
                           -DDOWNLOAD_VER=${LIBUV_VERSION}
                           -DBUILD_TARGET=libuv
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

message(STATUS  "Building: libuv-v${LIBUV_VERSION}")
if(UNIX)
    # Host=Linux, Target=Linux
    # Host=MacOS, Target=MacOS
    BuildLibuv(BUILD_IN_SOURCE
               CONFIGURE_COMMAND ${UNIX_CFGCMD}
               INSTALL_COMMAND   ${MAKE_PROG} V=1 install)
elseif(CMAKE_HOST_WIN32 AND WIN32 AND MINGW)
    # Host=Windows, Target=Windows
    BuildLibUv(BUILD_IN_SOURCE
               CONFIGURE_COMMAND ${MAKE_PROG} -f Makefile.mingw
               BUILD_COMMAND   "dir" # Here must have a command, so give 'dir' make it happy!
               INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPS_INSTALL_DIR}/lib
                               COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/libuv/libuv.a
                                                                ${DEPS_INSTALL_DIR}/lib
                               COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPS_BUILD_DIR}/src/libuv/include
                                                                          ${DEPS_INSTALL_DIR}/include)
else()
    set(err_msg "Trying to build [ libuv ] in an unsupported system.")
    set(err_msg "${err_msg}\n  Host System Name  : ${CMAKE_HOST_SYSTEM}")
    set(err_msg "${err_msg}\n  Target System Name: ${CMAKE_SYSTEM}")
    set(err_msg "${err_msg}\n  C Compiler Name   : ${CMAKE_C_COMPILER}")
    set(err_msg "${err_msg}\n  CXX Compiler Name : ${CMAKE_CXX_COMPILER}")
    message(FATAL_ERROR "${err_msg}")
endif()

list(APPEND THIRD_PARTY_LIBS libuv)
