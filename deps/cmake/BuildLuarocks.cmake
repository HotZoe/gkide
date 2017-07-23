# Host use only, no link to nvim or snail

include(CMakeParseArguments)

# BuildLuarocks(<CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildLuarocks)
    cmake_parse_arguments(_luarocks
    ""
    ""
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _luarocks_CONFIGURE_COMMAND AND
       NOT _luarocks_BUILD_COMMAND     AND
       NOT _luarocks_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    ExternalProject_Add(   luarocks
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LUAROCKS_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/luarocks
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/luarocks
                           -DDOWNLOAD_URL=${LUAROCKS_URL}
                           -DDOWNLOAD_VER=${LUAROCKS_VERSION}
                           -DBUILD_TARGET=luarocks
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=1
                           -DEXPECT_SHA256=${LUAROCKS_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LUAROCKS}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        CONFIGURE_COMMAND  "${_luarocks_CONFIGURE_COMMAND}"
        BUILD_IN_SOURCE    1
        WORKING_DIRECTORY  ${DEPS_BUILD_DIR}/src/luarocks
        BUILD_COMMAND      "${_luarocks_BUILD_COMMAND}"
        INSTALL_COMMAND    "${_luarocks_INSTALL_COMMAND}")
endfunction()

message(STATUS  "Building: luarocks-v${LUAROCKS_VERSION}")
if(UNIX OR (MINGW AND CMAKE_CROSSCOMPILING))
    # Host=Linux, Target=Linux
    # Host=Linux, Target=Windows
    BuildLuarocks(CONFIGURE_COMMAND ${DEPS_BUILD_DIR}/src/luarocks/configure
                                    --prefix=${HOSTDEPS_INSTALL_DIR}
                                    --force-config
                                    --with-lua=${HOSTDEPS_INSTALL_DIR} # check '${HOSTDEPS_INSTALL_DIR}/bin'
                                    --with-lua-include=${HOSTDEPS_INSTALL_DIR}/include
                  INSTALL_COMMAND   ${MAKE_PROG} bootstrap)
elseif(MINGW AND NOT CMAKE_CROSSCOMPILING)
    # Host=Windows, Target=Windows
    configure_file(${PROJECT_SOURCE_DIR}/cmake/InstallLuarocks.bat.in
                   ${DEPS_BUILD_DIR}/src/luarocks/InstallLuarocks.bat)

    BuildLuarocks(INSTALL_COMMAND ${DEPS_BUILD_DIR}/src/luarocks/InstallLuarocks.bat)
else()
    set(err_msg "Trying to build [ luarocks ] in an unsupported system.")
    set(err_msg "${err_msg}\n  Host System Name  : ${CMAKE_HOST_SYSTEM}")
    set(err_msg "${err_msg}\n  Target System Name: ${CMAKE_SYSTEM}")
    set(err_msg "${err_msg}\n  C Compiler Name   : ${CMAKE_C_COMPILER}")
    set(err_msg "${err_msg}\n  CXX Compiler Name : ${CMAKE_CXX_COMPILER}")
    message(FATAL_ERROR "${err_msg}")
endif()

add_dependencies(luarocks lua)
list(APPEND THIRD_PARTY_LIBS luarocks)

