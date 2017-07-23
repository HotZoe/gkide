# Dependencies library link to nvim & snail

if(WIN32 OR MINGW)
    message(STATUS "Building: jemalloc for Windows is not supported, skipping ...")
    return()
endif()

message(STATUS  "Building: jemalloc-v${JEMALLOC_VERSION}")
ExternalProject_Add(   jemalloc
    PREFIX             ${DEPS_BUILD_DIR}
    URL                ${JEMALLOC_URL}
    DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/jemalloc
    DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                       -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/jemalloc
                       -DDOWNLOAD_URL=${JEMALLOC_URL}
                       -DDOWNLOAD_VER=${JEMALLOC_VERSION}
                       -DBUILD_TARGET=jemalloc
                       -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                       -DBUILD_INTREE=1
                       -DEXPECT_SHA256=${JEMALLOC_SHA256}
                       -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_JEMALLOC}
                       -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
    CONFIGURE_COMMAND  ${DEPS_BUILD_DIR}/src/jemalloc/configure
                       CC=${DEPS_C_COMPILER}
                       --prefix=${DEPS_INSTALL_DIR}
    BUILD_IN_SOURCE    1
    WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/jemalloc
    BUILD_COMMAND      ""
    INSTALL_COMMAND    ${MAKE_PROG} install_include install_lib_static)

list(APPEND THIRD_PARTY_LIBS jemalloc)
