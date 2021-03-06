# Dependencies library link to nvim

if(WIN32 OR MINGW)
    # Host=Windows, Target=Windows
    message(STATUS "Building: unibilium for Windows is not supported, skipping ...")
    return()
endif()

# Host=Linux, Target=Linux
# Host=MacOS, Target=MacOS
set(CFLAGS_ARGS "CFLAGS=-fPIC")

message(STATUS  "Building: unibilium-v${UNIBILIUM_VERSION}")
externalproject_add(   unibilium
    PREFIX             ${DEPS_BUILD_DIR}
    URL                ${UNIBILIUM_URL}
    DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/unibilium
    DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                       -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/unibilium
                       -DDOWNLOAD_URL=${UNIBILIUM_URL}
                       -DDOWNLOAD_VER=${UNIBILIUM_VERSION}
                       -DBUILD_TARGET=unibilium
                       -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                       -DBUILD_INTREE=1
                       -DEXPECT_SHA256=${UNIBILIUM_SHA256}
                       -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_UNIBILIUM}
                       -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
    CONFIGURE_COMMAND  ""
    BUILD_IN_SOURCE    1
    WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/unibilium
    BUILD_COMMAND      ${MAKE_PROG} CC=${DEPS_C_COMPILER} PREFIX=${DEPS_INSTALL_DIR} ${CFLAGS_ARGS}
    INSTALL_COMMAND    ${MAKE_PROG} CC=${DEPS_C_COMPILER} PREFIX=${DEPS_INSTALL_DIR} install)

list(APPEND THIRD_PARTY_LIBS unibilium)
