# Dependencies library link to nvim & snail

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

set(common_configuration ${CMAKE_COMMAND} ${DEPS_BUILD_DIR}/src/msgpack
                         -DMSGPACK_ENABLE_CXX=OFF
                         -DMSGPACK_BUILD_TESTS=OFF
                         -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
                         -DCMAKE_C_COMPILER=${DEPS_C_COMPILER}
                         -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                         "-DCMAKE_C_FLAGS:STRING=-fPIC"
                         -DCMAKE_GENERATOR=${CMAKE_GENERATOR})
set(common_cmake_cfg     ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE})

# target=Linux/Unix
set(MSGPACK_CONFIGURE_COMMAND ${common_configuration} "-DCMAKE_C_FLAGS:STRING=-fPIC")
set(MSGPACK_BUILD_COMMAND     ${common_cmake_cfg})
set(MSGPACK_INSTALL_COMMAND   ${common_cmake_cfg} --target install)

if(MINGW)
    # Target=Windows, cross build
    # Target=Windows, native build
    set(MSGPACK_CONFIGURE_COMMAND ${common_configuration} -DCMAKE_MAKE_PROGRAM:FILEPATH=${MAKE_PROG})
    set(MSGPACK_INSTALL_COMMAND   ${common_cmake_cfg} --target install
                                  COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/msgpack/libmsgpack.dll
                                                                   ${DEPS_INSTALL_DIR}/bin/libmsgpack.dll)
endif()

message(STATUS  "Building: msgpack-v${MSGPACK_VERSION}")
BuildMsgpack(CONFIGURE_COMMAND  ${MSGPACK_CONFIGURE_COMMAND}
             BUILD_COMMAND      ${MSGPACK_BUILD_COMMAND}
             INSTALL_COMMAND    ${MSGPACK_INSTALL_COMMAND})

list(APPEND THIRD_PARTY_LIBS msgpack)

