message(STATUS  "Building: msgpack-v${MSGPACK_VERSION}")
include(CMakeParseArguments)

# BuildMsgpack(<CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildMsgpack)
    cmake_parse_arguments(_msgpack
    ""
    ""
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _msgpack_CONFIGURE_COMMAND AND
       NOT _msgpack_BUILD_COMMAND     AND
       NOT _msgpack_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    ExternalProject_Add(   msgpack
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${MSGPACK_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/msgpack
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/msgpack
                           -DDOWNLOAD_URL=${MSGPACK_URL}
                           -DDOWNLOAD_VER=${MSGPACK_VERSION}
                           -DBUILD_TARGET=msgpack
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=1
                           -DEXPECT_SHA256=${MSGPACK_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_MSGPACK}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        CONFIGURE_COMMAND  ${_msgpack_CONFIGURE_COMMAND}
        BUILD_IN_SOURCE    1
        WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/msgpack
        BUILD_COMMAND      ${_msgpack_BUILD_COMMAND}
        INSTALL_COMMAND    ${_msgpack_INSTALL_COMMAND})
endfunction()

# Linux/Unix
set(MSGPACK_CONFIGURE_COMMAND ${CMAKE_COMMAND} ${DEPS_BUILD_DIR}/src/msgpack
                              -DMSGPACK_ENABLE_CXX=OFF
                              -DMSGPACK_BUILD_TESTS=OFF
                              -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
                              -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                              -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                              "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_COMPILER_ARG1} -fPIC"
                              -DCMAKE_GENERATOR=${CMAKE_GENERATOR})
set(MSGPACK_BUILD_COMMAND     ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE})
set(MSGPACK_INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install --config ${CMAKE_BUILD_TYPE})

if(CYGWIN)
    # Same as Unix without fPIC
    set(MSGPACK_CONFIGURE_COMMAND ${CMAKE_COMMAND} ${DEPS_BUILD_DIR}/src/msgpack
                                  -DMSGPACK_ENABLE_CXX=OFF
                                  -DMSGPACK_BUILD_TESTS=OFF
                                  -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
                                  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                  "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_COMPILER_ARG1}"
                                  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                  # Make sure we use the same generator, otherwise we may
                                  # accidentaly end up using different MSVC runtimes
                                  -DCMAKE_GENERATOR=${CMAKE_GENERATOR})
    # Place the DLL in the bin folder, and name Cygwin DLLs with SOVERSION version
    # need to solved this
    add_custom_target(copy-cygwin-dll-msgpack ALL
                      COMMAND ${CMAKE_COMMAND}
                              -DSRC_DIR=${DEPS_INSTALL_DIR}/lib
                              # the actual file may be have version info, i.e. cygmsgpack-3.dll
                              -DREG_EXP=cygmsgpack
                              -DUSE_REG=${CMAKE_SHARED_LIBRARY_NAME_WITH_VERSION}
                              # if no version info for shared library, then use this name
                              -DDEFNAME=cygmsgpack.dll
                              -DDST_DIR=${DEPS_INSTALL_DIR}/bin
                              -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyFilesRegExp.cmake
                      DEPENDS msgpack)
endif()

BuildMsgpack(CONFIGURE_COMMAND  ${MSGPACK_CONFIGURE_COMMAND}
             BUILD_COMMAND      ${MSGPACK_BUILD_COMMAND}
             INSTALL_COMMAND    ${MSGPACK_INSTALL_COMMAND})

list(APPEND THIRD_PARTY_LIBS msgpack)

