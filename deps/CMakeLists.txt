#
# This is not meant to be included by the top-level cmake.
#
cmake_minimum_required(VERSION 3.1.0)
project(snail-deps)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(DEPS_INSTALL_DIR  "${CMAKE_BINARY_DIR}/usr"       CACHE PATH "Deps install perfix directory.")
set(DEPS_BUILD_DIR    "${CMAKE_BINARY_DIR}"           CACHE PATH "Deps build perfix directory.")
set(DEPS_DOWNLOAD_DIR "${CMAKE_SOURCE_DIR}/downloads" CACHE PATH "Deps download root directory.")
set(DEPS_BIN_DIR      "${DEPS_INSTALL_DIR}/bin"       CACHE PATH "Deps binary install directory.")
set(DEPS_LIB_DIR      "${DEPS_INSTALL_DIR}/lib"       CACHE PATH "Deps library install directory.")

option(SKIP_DOWNLOAD_DEPS       "Skip download dependencies."  OFF)

option(SKIP_DOWNLOAD_LUV        "Skip download : luv"         ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LUA        "Skip download : lua"         ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_GPERF      "Skip download : gperf"       ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBUV      "Skip download : libuv"       ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_MSGPACK    "Skip download : msgpack"     ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LUAROCKS   "Skip download : luarocks"    ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_JEMALLOC   "Skip download : jemalloc"    ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBICONV   "Skip download : libiconv"    ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBVTERM   "Skip download : libvterm"    ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_UNIBILIUM  "Skip download : unibilium"   ${SKIP_DOWNLOAD_DEPS})
option(SKIP_DOWNLOAD_LIBTERMKEY "Skip download : libtermkey"  ${SKIP_DOWNLOAD_DEPS})

if(UNIX)
    find_program(MAKE_PROG NAMES gmake make)
    if(MAKE_PROG)
        execute_process(COMMAND          "${MAKE_PROG}" --version
                        OUTPUT_VARIABLE  MAKE_VERSION_INFO)

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
endif()

# When using make, use the $(MAKE) variable to avoid warning about the job server.
if(CMAKE_GENERATOR MATCHES "Makefiles")
    set(MAKE_PROG "$(MAKE)")
endif()

set(DEPS_C_COMPILER "${CMAKE_C_COMPILER}")

find_program(GIT_PRG git)
if(NOT GIT_PRG)
    message(FATAL_ERROR "Git is needed to download deps, but NOT found.")
endif()

# check the build architecture
include(CheckBuildArch)

# create custom targets to build external projects
include(ExternalProject)

# all the URLs of deps
include(SnailDepsURLSet)

# dependencies: none
include(BuildLua)

# dependencies: none
include(BuildGperf)

# dependencies: none
include(BuildJeMalloc)

# dependencies: none
include(BuildMsgpack)

# dependencies: none
include(BuildLibuv)

# dependencies: none
include(BuildLibiconv)

# dependencies: none
include(BuildLibvterm)

# dependencies: none
include(BuildUnibilium)

# dependencies: unibilium
include(BuildLibtermkey)

# dependencies: libuv, lua
include(BuildLuv)

# dependencies: lua
include(BuildLuarocks)

# dependencies: luarocks
include(BuildLuarocksModules)

set(all_shared_lib_files "${CMAKE_SHARED_LIBRARY_PREFIX}*${CMAKE_SHARED_LIBRARY_SUFFIX}*")
add_custom_target(clean-shared-libraries
                  COMMAND ${CMAKE_COMMAND}
                          -DREMOVE_FILES=${DEPS_INSTALL_DIR}/lib/${all_shared_lib_files}
                          -P ${PROJECT_SOURCE_DIR}/cmake/RemoveFiles.cmake
                  DEPENDS ${THIRD_PARTY_LIBS})

add_custom_target(deps    ALL
                  COMMAND ${CMAKE_COMMAND} -E touch .update-deps
                  DEPENDS clean-shared-libraries)
