include(CMakeParseArguments)

# BuildLuajit(<TARGET targetname> <CONFIGURE_COMMAND ...> <BUILD_COMMAND ...> <INSTALL_COMMAND ...>)
function(BuildLuajit)
    cmake_parse_arguments(_luajit
    ""
    "TARGET"
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND"
    ${ARGN})

    if(NOT _luajit_CONFIGURE_COMMAND AND
       NOT _luajit_BUILD_COMMAND     AND
       NOT _luajit_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND.")
    endif()

    if(NOT _luajit_TARGET)
        set(_luajit_TARGET "luajit")
    endif()

    ExternalProject_Add(   ${_luajit_TARGET}
        PREFIX             ${DEPS_BUILD_DIR}
        URL                ${LUAJIT_URL}
        DOWNLOAD_DIR       ${DEPS_DOWNLOAD_DIR}/luajit
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/luajit
                           -DDOWNLOAD_URL=${LUAJIT_URL}
                           -DDOWNLOAD_VER=${LUAJIT_VERSION}
                           -DBUILD_TARGET=${_luajit_TARGET}
                           -DBUILD_PREFIX=${DEPS_BUILD_DIR}
                           -DBUILD_INTREE=1
                           -DEXPECT_SHA256=${LUAJIT_SHA256}
                           -DSKIP_DOWNLOAD=${SKIP_DOWNLOAD_LUAJIT}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadExtract.cmake
        CONFIGURE_COMMAND  "${_luajit_CONFIGURE_COMMAND}"
        BUILD_IN_SOURCE    1
        WORKING_DIRECTORY  "${DEPS_BUILD_DIR}/src/luajit"
        BUILD_COMMAND      "${_luajit_BUILD_COMMAND}"
        INSTALL_COMMAND    "${_luajit_INSTALL_COMMAND}")
endfunction()

if(NOT CYGWIN)
    set(cflags_fpic "CFLAGS=-fPIC")
endif()

set(INSTALLCMD_UNIX  ${MAKE_PRG} ${cflags_fpic}
                     CFLAGS+=-DLUAJIT_DISABLE_JIT
                     CFLAGS+=-DLUA_USE_APICHECK
                     CFLAGS+=-DLUA_USE_ASSERT
                     CCDEBUG+=-g Q=
                     install)

message(STATUS  "Building: luajit-v${LUAJIT_VERSION}")
if(UNIX)

    # Unix/Cygwin
    if(CYGWIN)
        set(cygwin_build_args
            COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/luajit/src/cyglua51.dll
                                             ${DEPS_INSTALL_DIR}/bin
            COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/luajit/src/cyglua51.dll
                                             ${DEPS_INSTALL_DIR}/lib)
            #COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/luajit/src/libluajit.a
            #                                 ${DEPS_INSTALL_DIR}/lib
    endif()
    BuildLuaJit(INSTALL_COMMAND ${INSTALLCMD_UNIX}
                                CC=${DEPS_C_COMPILER}
                                PREFIX=${DEPS_INSTALL_DIR}
                                ${cygwin_build_args})

elseif(MINGW)
    BuildLuaJit(
    BUILD_COMMAND   ${CMAKE_MAKE_PROGRAM}
                    CC=${DEPS_C_COMPILER}
                    PREFIX=${DEPS_INSTALL_DIR}
                    CFLAGS+=-DLUAJIT_DISABLE_JIT
                    CFLAGS+=-DLUA_USE_APICHECK
                    CFLAGS+=-DLUA_USE_ASSERT
                    CCDEBUG+=-g
                    BUILDMODE=static
                    # Build a DLL too
                    COMMAND ${CMAKE_MAKE_PROGRAM} CC=${DEPS_C_COMPILER} BUILDMODE=dynamic

    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPS_INSTALL_DIR}/bin
                    COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/luajit/src/luajit.exe
                                                     ${DEPS_INSTALL_DIR}/bin
                    COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/luajit/src/lua51.dll
                                                     ${DEPS_INSTALL_DIR}/bin
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPS_INSTALL_DIR}/lib
                    # Luarocks searches for lua51.dll in lib
                    COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/luajit/src/lua51.dll
                                                     ${DEPS_INSTALL_DIR}/lib
                    COMMAND ${CMAKE_COMMAND} -E copy ${DEPS_BUILD_DIR}/src/luajit/src/libluajit.a
                                                     ${DEPS_INSTALL_DIR}/lib
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPS_INSTALL_DIR}/include/luajit-2.1
                    COMMAND ${CMAKE_COMMAND} -DFROM_GLOB=${DEPS_BUILD_DIR}/src/luajit/src/*.h
                                             -DTO=${DEPS_INSTALL_DIR}/include/luajit-2.1
                                             -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyFilesGlob.cmake)

else()
    set(err_msg "Trying to build [ luajit ] in an unsupported system.")
    set(err_msg "${err_msg}\n  Current System Name  : ${CMAKE_SYSTEM_NAME}")
    set(err_msg "${err_msg}\n  Current C Compiler ID: ${CMAKE_C_COMPILER_ID}")
    message(FATAL_ERROR "${err_msg}")
endif()

list(APPEND THIRD_PARTY_LIBS luajit)
