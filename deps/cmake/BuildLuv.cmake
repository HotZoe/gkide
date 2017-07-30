# Dependencies library link to nvim

include(CMakeParseArguments)

# BuildLuv(<PATCH_COMMAND ...> <CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildLuv)
    cmake_parse_arguments(_luv
    ""
    ""
    "PATCH_COMMAND;CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _luv_CONFIGURE_COMMAND AND
       NOT _luv_BUILD_COMMAND     AND
       NOT _luv_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    ExternalProject_Add(luv-static
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LUV_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/luv
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/luv
                           -DDOWNLOAD_URL=${LUV_URL}
                           -DDOWNLOAD_VER=${LUV_VERSION}
                           -DBUILD_TARGET=luv
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=1
                           -DEXPECT_SHA256=${LUV_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LUV}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        PATCH_COMMAND      "${_luv_PATCH_COMMAND}"
        CONFIGURE_COMMAND  "${_luv_CONFIGURE_COMMAND}"
        BUILD_IN_SOURCE    1
        WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/luv
        BUILD_COMMAND      "${_luv_BUILD_COMMAND}"
        INSTALL_COMMAND    "${_luv_INSTALL_COMMAND}")
endfunction()

set(LUV_SRC_DIR        ${DEPS_BUILD_DIR}/src/luv)
set(LUV_INCLUDE_FLAGS  "-I${DEPS_INSTALL_DIR}/include -I${DEPS_INSTALL_DIR}/include")
set(LUV_PATCH_COMMAND  ${CMAKE_COMMAND}
                       -DLUV_SRC_DIR=${LUV_SRC_DIR}
                       -P ${PROJECT_SOURCE_DIR}/cmake/PatchLuv.cmake)

set(LUV_CONFIGURE_COMMAND_COMMON ${CMAKE_COMMAND} ${LUV_SRC_DIR}
                                 -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
                                 -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
                                 -DLUA_BUILD_TYPE=System
                                 -DWITH_SHARED_LIBUV=ON
                                 -DBUILD_SHARED_LIBS=OFF
                                 -DBUILD_MODULE=OFF)

set(cflags_fpic "-fPIC")

if(WIN32 AND MINGW)
    # Host=Linux, Target=Windows
    # Host=Windows, Target=Windows
    set(cflags_fpic "")
    set(LUV_CONFIGURE_COMMAND_COMMON
        ${LUV_CONFIGURE_COMMAND_COMMON}
        -DWITH_LUA_ENGINE=Lua
        -DCMAKE_MAKE_PROGRAM:FILEPATH=${MAKE_PROG})
endif()

# Host=Linux, Target=Linux
# Host=MacOS, Target=MacOS
set(LUV_CONFIGURE_COMMAND
    ${LUV_CONFIGURE_COMMAND_COMMON}
    -DCMAKE_C_COMPILER=${DEPS_C_COMPILER}
    "-DCMAKE_C_FLAGS:STRING=${LUV_INCLUDE_FLAGS} ${cflags_fpic}")

set(LUV_BUILD_COMMAND   ${CMAKE_COMMAND} --build .)
set(LUV_INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install)

message(STATUS  "Building: luv-v${LUV_VERSION}")
BuildLuv(PATCH_COMMAND     ${LUV_PATCH_COMMAND}
         CONFIGURE_COMMAND ${LUV_CONFIGURE_COMMAND}
         BUILD_COMMAND     ${LUV_BUILD_COMMAND}
         INSTALL_COMMAND   ${LUV_INSTALL_COMMAND})

add_dependencies(luv-static lua)
add_dependencies(luv-static libuv)

list(APPEND THIRD_PARTY_LIBS luv-static)
