# Luarocks is required when building snail.

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

if(UNIX OR CYGWIN)
    BuildLuarocks(CONFIGURE_COMMAND
                  ${DEPS_BUILD_DIR}/src/luarocks/configure
                  --prefix=${DEPS_INSTALL_DIR}
                  --force-config
                  # will check '${DEPS_INSTALL_DIR}/bin'
                  --with-lua=${DEPS_INSTALL_DIR}
                  --with-lua-include=${DEPS_INSTALL_DIR}/include
                  INSTALL_COMMAND   ${MAKE_PROG} bootstrap)

else()
    set(err_msg "Trying to build [ luarocks ] in an unsupported system.")
    set(err_msg "${err_msg}\n  Current System Name  : ${CMAKE_SYSTEM_NAME}")
    set(err_msg "${err_msg}\n  Current C Compiler ID: ${CMAKE_C_COMPILER_ID}")
    message(FATAL_ERROR "${err_msg}")
endif()

message(STATUS  "Building: luarocks-v${LUAROCKS_VERSION}")
add_dependencies(luarocks lua)
list(APPEND THIRD_PARTY_LIBS luarocks)

