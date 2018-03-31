#
# This is not meant to be included by the top-level cmake.
#
cmake_minimum_required(VERSION 3.1.0)
project(gkide-deps)

# check for dependencies build environment first
if(UNIX)
    find_program(MAKE_PROG NAMES gmake make)
    if(MAKE_PROG)
        execute_process(COMMAND "${MAKE_PROG}" --version
                        OUTPUT_VARIABLE MAKE_VERSION_INFO)

        if(NOT "${MAKE_VERSION_INFO}" MATCHES ".*GNU.*")
            message(STATUS "GNU Make: ${MAKE_PROG}, is not GNU Make.")
            unset(MAKE_PROG)
        endif()
    endif()

    if(NOT MAKE_PROG)
        message(FATAL_ERROR "GNU Make is required to build the dependencies.")
    else()
        message(STATUS "GNU Make: ${MAKE_PROG}")
    endif()
else()
    find_program(MAKE_PROG NAMES mingw32-make)
    if(NOT MAKE_PROG)
        message(FATAL_ERROR "GNU Make is required to build the dependencies.")
    else()
        message(STATUS "GNU Make: ${MAKE_PROG}")
    endif()
endif()

# When using make, use the $(MAKE) variable to avoid warning about the job server.
if(CMAKE_GENERATOR MATCHES "Makefiles")
    set(MAKE_PROG "$(MAKE)")
endif()

if(NOT GIT_PROG)
    find_program(GIT_PROG git)
    if(NOT GIT_PROG)
        message(FATAL_ERROR "Git is needed to download deps, but NOT found.")
    endif()
endif()

file(TO_CMAKE_PATH "${GIT_PROG}" GIT_PROG)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(NOT SED_PROG)
        message(FATAL_ERROR "MSYS version of 'sed' is needed, but NOT found.")
    endif()

    if(NOT MKDIR_PROG)
        message(FATAL_ERROR "MSYS version of 'mkdir' is needed, but NOT found.")
    endif()

    if(NOT INSTALL_PROG)
        message(FATAL_ERROR "MSYS version of 'install' is needed, but NOT found.")
    endif()

    file(TO_CMAKE_PATH "${SED_PROG}" SED_PROG)
    file(TO_CMAKE_PATH "${MKDIR_PROG}" MKDIR_PROG)
    file(TO_CMAKE_PATH "${INSTALL_PROG}" INSTALL_PROG)
endif()

if(NOT SED_PROG)
    set(SED_PROG sed)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(DEPS_BUILD_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "Deps build perfix directory.")
set(DEPS_DOWNLOAD_DIR "${CMAKE_SOURCE_DIR}/downloads" CACHE PATH "Deps download root directory.")

# Dependencies built for the TARGET system
set(DEPS_C_COMPILER   "${CMAKE_C_COMPILER}")
set(DEPS_CXX_COMPILER "${CMAKE_CXX_COMPILER}")

set(DEPS_INSTALL_DIR "${CMAKE_BINARY_DIR}/usr" CACHE PATH "Deps install perfix directory.")
set(DEPS_BIN_DIR     "${DEPS_INSTALL_DIR}/bin" CACHE PATH "Deps binary install directory.")
set(DEPS_LIB_DIR     "${DEPS_INSTALL_DIR}/lib" CACHE PATH "Deps library install directory.")

# Cross Compiling:
# Dependencies built for the HOST system, when not crosscompiling these should be the same as DEPS_*.
# Except when target is Unix, in which case all the dependencies the same as DEPS_*.
if(CMAKE_CROSSCOMPILING AND NOT UNIX)
    set(HOSTDEPS_C_COMPILER   "${HOST_C_COMPILER}")
    set(HOSTDEPS_CXX_COMPILER "${HOST_CXX_COMPILER}")
    set(HOSTDEPS_INSTALL_DIR  "${CMAKE_BINARY_DIR}/host")
    set(HOSTDEPS_BIN_DIR      "${HOSTDEPS_INSTALL_DIR}/bin")
    set(HOSTDEPS_LIB_DIR      "${HOSTDEPS_INSTALL_DIR}/lib")
else()
    set(HOSTDEPS_C_COMPILER   "${DEPS_C_COMPILER}")
    set(HOSTDEPS_CXX_COMPILER "${DEPS_CXX_COMPILER}")
    set(HOSTDEPS_INSTALL_DIR  "${DEPS_INSTALL_DIR}")
    set(HOSTDEPS_BIN_DIR      "${DEPS_BIN_DIR}")
    set(HOSTDEPS_LIB_DIR      "${DEPS_LIB_DIR}")
endif()

option(NVIM_TESTING_ENABLE      "Enable nvim testing."         ON)
option(SNAIL_TESTING_ENABLE     "Enable snail testing."        ON)
option(SKIP_DOWNLOAD_DEPS       "Skip download dependencies."  OFF)

option(SKIP_DOWNLOAD_LUV        "Skip download : luv"        ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LUA        "Skip download : lua"        ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_GPERF      "Skip download : gperf"      ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBUV      "Skip download : libuv"      ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBSSH     "Skip download : libssh"     ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LUAJIT     "Skip download : luajit"     ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_MSGPACK    "Skip download : msgpack"    ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LUAROCKS   "Skip download : luarocks"   ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_JEMALLOC   "Skip download : jemalloc"   ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBICONV   "Skip download : libiconv"   ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBVTERM   "Skip download : libvterm"   ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_UNIBILIUM  "Skip download : unibilium"  ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBTERMKEY "Skip download : libtermkey" ${SKIP_DOWNLOAD_DEPS})

# create custom targets to build external projects
include(ExternalProject)

# check the build architecture
include(CheckBuildArch)

# all the URLs of deps
include(GkideDepsURLs)

# Build host use tools
#
# dependencies: none
include(BuildLua)

# dependencies: none
#include(BuildLuajit)

# dependencies: none
include(BuildGperf)

# dependencies: libuv, lua
include(BuildLuv)

# dependencies: lua
include(BuildLuarocks)

# dependencies: luarocks
include(BuildLuarocksModules)

# Build target link libraries
#
# dependencies: none
include(BuildJeMalloc)

# dependencies: none
include(BuildMsgpack)

# dependencies: none
include(BuildLibuv)

# dependencies: none
#include(BuildLibiconv)

# dependencies: none
include(BuildLibssh)

# for nvim TUI
# dependencies: none
include(BuildLibvterm)

# for nvim TUI
# dependencies: none
include(BuildUnibilium)

# for nvim TUI
# dependencies: unibilium
include(BuildLibtermkey)

set(all_shared_lib_files "${CMAKE_SHARED_LIBRARY_PREFIX}*${CMAKE_SHARED_LIBRARY_SUFFIX}*")
add_custom_target(clean-shared-libraries
                  COMMAND ${CMAKE_COMMAND}
                          -DREMOVE_FILES=${DEPS_INSTALL_DIR}/lib/${all_shared_lib_files}
                          -P ${PROJECT_SOURCE_DIR}/cmake/RemoveFiles.cmake
                  DEPENDS ${THIRD_PARTY_LIBS})

add_custom_target(deps    ALL
                  COMMAND ${CMAKE_COMMAND} -E touch .update-deps
                  DEPENDS clean-shared-libraries)
