include(CMakeParseArguments)

# BuildMsgpack(<CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildLibiconv)
    cmake_parse_arguments(_libiconv
    ""
    ""
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _libiconv_CONFIGURE_COMMAND AND
       NOT _libiconv_BUILD_COMMAND     AND
       NOT _libiconv_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    ExternalProject_Add(   libiconv
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LIBICONV_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/libiconv
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/libiconv
                           -DDOWNLOAD_URL=${LIBICONV_URL}
                           -DDOWNLOAD_VER=${LIBICONV_VERSION}
                           -DBUILD_TARGET=libiconv
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=1
                           -DEXPECT_SHA256=${LIBICONV_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LIBICONV}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        CONFIGURE_COMMAND  ${_libiconv_CONFIGURE_COMMAND}
        BUILD_IN_SOURCE    1
        WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/libiconv
        BUILD_COMMAND      ${_libiconv_BUILD_COMMAND}
        INSTALL_COMMAND    ${_libiconv_INSTALL_COMMAND})
endfunction()


# Linux/Unix
set(LIBICONV_CONFIGURE_CMD sh ${DEPS_BUILD_DIR}/src/libiconv/configure
                              --prefix=${DEPS_INSTALL_DIR}
                              --enable-static=yes
                              --enable-extra-encodings)
set(LIBICONV_BUILD_CMD     make)
set(LIBICONV_INSTALL_CMD   make install)

# Native binaries, built using the mingw tool chain.
if(MINGW)
    if(HOST_OS_ARCH_32)
        #Building 32-bit binaries for mingw
        set(LIBICONV_CONFIGURE_CMD ${DEPS_BUILD_DIR}/src/libiconv/configure
                                   --host=i686-w64-mingw32
                                   --prefix=${DEPS_INSTALL_DIR}
                                   CC=i686-w64-mingw32-gcc
                                   CPPFLAGS="-I/usr/local/mingw32/include -Wall"
                                   LDFLAGS="-L/usr/local/mingw32/lib")
    else()
        #Building 64-bit binaries for mingw
        set(LIBICONV_CONFIGURE_CMD ${DEPS_BUILD_DIR}/src/libiconv/configure
                                   --host=x86_64-w64-mingw32
                                   --prefix=${DEPS_INSTALL_DIR}
                                   CC=x86_64-w64-mingw32-gcc
                                   CPPFLAGS="-I/usr/local/mingw64/include -Wall"
                                   LDFLAGS="-L/usr/local/mingw64/lib")
    endif()
endif()

# Binaries for the Cygwin environment.
if(CYGWIN)
    if(HOST_OS_ARCH_32)
        #Building 32-bit binaries for mingw
        set(LIBICONV_CONFIGURE_CMD ${DEPS_BUILD_DIR}/src/libiconv/configure
                                   --host=i686-pc-cygwin
                                   --prefix=${DEPS_INSTALL_DIR}
                                   CC=i686-pc-cygwin-gcc
                                   CPPFLAGS="-I/usr/local/cygwin32/include -Wall"
                                   LDFLAGS="-L/usr/local/cygwin32/lib")
    else()
        #Building 64-bit binaries for mingw
        set(LIBICONV_CONFIGURE_CMD ${DEPS_BUILD_DIR}/src/libiconv/configure
                                   --host=x86_64-pc-cygwin
                                   --prefix=${DEPS_INSTALL_DIR}
                                   CC=x86_64-pc-cygwin-gcc
                                   CPPFLAGS="-I/usr/local/cygwin64/include -Wall"
                                   LDFLAGS="-L/usr/local/cygwin64/lib")
    endif()
endif()

message(STATUS  "Building: libiconv-v${LIBICONV_VERSION}")
BuildLibiconv(CONFIGURE_COMMAND  ${LIBICONV_CONFIGURE_CMD}
              BUILD_COMMAND      ${LIBICONV_BUILD_CMD}
              INSTALL_COMMAND    ${LIBICONV_INSTALL_CMD})

list(APPEND THIRD_PARTY_LIBS libiconv)
