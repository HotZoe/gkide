# Dependencies library link to snail

include(CMakeParseArguments)

if(CMAKE_HOST_WIN32)
    # Host=Windows, Target=Windows
    message(STATUS "Building: libssh for Windows is not supported, skipping ...")
    return()
endif()

# BuildLibssh(<PATCH_COMMAND ...> <CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildLibssh)
    cmake_parse_arguments(_libssh
    ""
    ""
    "PATCH_COMMAND;CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _libssh_CONFIGURE_COMMAND AND
       NOT _libssh_BUILD_COMMAND     AND
       NOT _libssh_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    ExternalProject_Add(libssh
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LIBSSH_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/libssh
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/libssh
                           -DDOWNLOAD_URL=${LIBSSH_URL}
                           -DDOWNLOAD_VER=${LIBSSH_VERSION}
                           -DBUILD_TARGET=libssh
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=0
                           -DEXPECT_SHA256=${LIBSSH_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LIBSSH}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        PATCH_COMMAND      "${_libssh_PATCH_COMMAND}"
        CONFIGURE_COMMAND  "${_libssh_CONFIGURE_COMMAND}"
        WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/libssh
        BUILD_COMMAND      "${_libssh_BUILD_COMMAND}"
        INSTALL_COMMAND    "${_libssh_INSTALL_COMMAND}")
endfunction()

set(LIBSSH_CONFIGURE_COMMAND ${CMAKE_COMMAND} ${DEPS_BUILD_DIR}/src/libssh
                             -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
                             -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
                             -DWITH_STATIC_LIB=ON)
set(LIBSSH_BUILD_COMMAND     ${MAKE_PROG})
set(LIBSSH_INSTALL_COMMAND   ${MAKE_PROG} install)

message(STATUS  "Building: libssh-v${LIBSSH_VERSION}")
BuildLibssh(CONFIGURE_COMMAND ${LIBSSH_CONFIGURE_COMMAND}
            BUILD_COMMAND     ${LIBSSH_BUILD_COMMAND}
            INSTALL_COMMAND   ${LIBSSH_INSTALL_COMMAND})

list(APPEND THIRD_PARTY_LIBS libssh)
