if(WIN32)
    message(STATUS "Building libtermkey in Windows is not supported, skipping ...")
    return()
endif()

message(STATUS  "Building: libtermkey-v${LIBTERMKEY_VERSION}")

find_package(PkgConfig REQUIRED)
if(NOT CYGWIN)
    set(CFLAGS_ARGS "CFLAGS=-fPIC")
endif()

externalproject_add(   libtermkey
    PREFIX             ${DEPS_BUILD_DIR}
    URL                ${LIBTERMKEY_URL}
    DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/libtermkey
    DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                       -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/libtermkey
                       -DDOWNLOAD_URL=${LIBTERMKEY_URL}
                       -DDOWNLOAD_VER=${LIBTERMKEY_VERSION}
                       -DBUILD_TARGET=libtermkey
                       -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                       -DBUILD_INTREE=1
                       -DEXPECT_SHA256=${LIBTERMKEY_SHA256}
                       -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LIBTERMKEY}
                       -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
    CONFIGURE_COMMAND  ""
    BUILD_IN_SOURCE    1
    WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/libtermkey
    BUILD_COMMAND      ""
    INSTALL_COMMAND    ${MAKE_PROG}
                       CC=${DEPS_C_COMPILER}
                       PREFIX=${DEPS_INSTALL_DIR}
                       PKG_CONFIG_PATH=${DEPS_LIB_DIR}/pkgconfig
                       ${CFLAGS_ARGS}
                       install)

add_dependencies(libtermkey  unibilium)
list(APPEND THIRD_PARTY_LIBS libtermkey)
