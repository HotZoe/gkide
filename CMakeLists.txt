cmake_minimum_required(VERSION 3.1.0)
project(snail)

option(LOGGING_ENABLE "Enable logging." ON)
option(DEBUGGING_ENABLE "Enable debugging." ON)
option(ASSERTION_ENABLE "Enable assertion." ON)
option(GCOV_ENABLE "Enable gcov support." OFF)
option(TRAVIS_CI_ENABLE "Enable Travis CI build." OFF)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Generate a compile_commands.json" OFF)

set(SNAIL_INFO_LOGO  "${PROJECT_SOURCE_DIR}/snail.png")
set(SNAIL_INFO_DESC  "snail is a elegant and sophisticated editor for programmers.")

# snail full version: Major.Minor.Patch-Trail-GitShortHash-GitDateTime
set(SNAIL_VERSION_MAJOR   0)
set(SNAIL_VERSION_MINOR   0)
set(SNAIL_VERSION_PATCH   1)

# nvim version
set(NVIM_VERSION_MAJOR    0)
set(NVIM_VERSION_MINOR    2)
set(NVIM_VERSION_PATCH    1)

# nvim API version
set(NVIM_API_VERSION      1) # nvim API version
set(NVIM_API_COMPATV      1) # nvim compat API version
set(NVIM_API_PRERELEASE   true)

# dev -> alpha -> beta -> rc -> release -> stable
set(SNAIL_VERSION_TRAIL  beta)
set(SNAIL_RELEASE_READY  true)
set(rce_msg  "") # release check error message

# snail basic version
set(SNAIL_VERSION_BASIC "${SNAIL_VERSION_MAJOR}.${SNAIL_VERSION_MINOR}.${SNAIL_VERSION_PATCH}")
math(EXPR SNAIL_VERSION_INT32
          "(${SNAIL_VERSION_MAJOR}<<16) + (${SNAIL_VERSION_MINOR}<<8) + ${SNAIL_VERSION_PATCH}")

# nvim basic version
set(NVIM_VERSION_BASIC "${NVIM_VERSION_MAJOR}.${NVIM_VERSION_MINOR}.${NVIM_VERSION_PATCH}")
math(EXPR NVIM_VERSION_INT32
          "(${NVIM_VERSION_MAJOR}<<16) + (${NVIM_VERSION_MINOR}<<8) + ${NVIM_VERSION_PATCH}")

# Set available build types for CMake GUIs.
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Dev"  "Debug"  "Release"  "MinSizeRel")

# Set default build type, in case of not given
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Dev" CACHE STRING "Choose the build type ..." FORCE)
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} build_type)

# Check release type
if(SNAIL_RELEASE_TYPE)
    if(SNAIL_RELEASE_TYPE STREQUAL "dev"  OR SNAIL_RELEASE_TYPE STREQUAL "alpha"   OR
       SNAIL_RELEASE_TYPE STREQUAL "rc"   OR SNAIL_RELEASE_TYPE STREQUAL "release" OR
       SNAIL_RELEASE_TYPE STREQUAL "beta" OR SNAIL_RELEASE_TYPE STREQUAL "stable")
        set(SNAIL_VERSION_TRAIL  "${SNAIL_RELEASE_TYPE}")
    else()
        set(warning_msg "SNAIL_RELEASE_TYPE = ${SNAIL_RELEASE_TYPE}, it must one of followings:")
        set(warning_msg "${warning_msg}\ndev, alpha, beta, rc, release, stable")
        set(warning_msg "${warning_msg}\nset to default: beta")
        message(WARNING "${err_msg}")
    endif()
endif()

# Append custom cmake modules search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# do NOT want building in source-tree
include(PreventInTreeBuilds)

# If git repo exist, get the current commit info
file(TO_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/.git SNAIL_GIT_REPO_DIR)
if(EXISTS "${SNAIL_GIT_REPO_DIR}" AND IS_DIRECTORY "${SNAIL_GIT_REPO_DIR}")
    include(GetGitRepositoryInfo)
    option(ENABLE_VERSION_MIDDLE "Make git info into version info." ON)

    GetGitCurrentBranchInfo(GIT_BRANCH_NAME GIT_COMMIT_HASH_FULL)
    GetGitBranchTimestamp(GIT_COMMIT_TIMESTAMP)
    GetGitBranchExactTag(GIT_COMMIT_TAG)

    if(NOT ${GIT_BRANCH_NAME} STREQUAL "master")
        # The release version must be the 'master' branch of git repo
        set(SNAIL_RELEASE_READY  false)
        if(NOT rce_msg)
            set(rce_msg "Release version must be the 'master' branch.")
        endif()
    endif()

    if(${GIT_COMMIT_TAG} MATCHES "NOTAGNAME")
        # no release tag set for current commit
        set(git_commit_tag  "Not Set")
        set(SNAIL_RELEASE_READY  false)
        if(NOT rce_msg)
            set(rce_msg "Release tag should be: v${SNAIL_VERSION_BASIC}-${SNAIL_VERSION_TRAIL}")
        endif()
    elseif(NOT ${GIT_COMMIT_TAG} MATCHES "^v[0-9]*\.[0-9]*\.[0-9]*-[a-z]*$")
        # if the current commit is ready to release, must set a tag
        # tag format like:  v0.0.0-dev
        set(SNAIL_RELEASE_READY  false)
        if(NOT rce_msg)
            set(rce_msg "Release tag should be: v${SNAIL_VERSION_BASIC}-${SNAIL_VERSION_TRAIL}")
        endif()
    endif()

    string(REPLACE "\"" "" GIT_COMMIT_TIMESTAMP ${GIT_COMMIT_TIMESTAMP})

    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 00 10 GIT_COMMIT_DATE)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 11 08 GIT_COMMIT_TIME)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 20 05 GIT_COMMIT_ZONE)

    string(REPLACE "-" "" GIT_COMMIT_DATE_NUMS ${GIT_COMMIT_DATE})
    string(REPLACE ":" "" GIT_COMMIT_TIME_NUMS ${GIT_COMMIT_TIME})

    set(SNAIL_VERSION_TIME "${GIT_COMMIT_DATE_NUMS}${GIT_COMMIT_TIME_NUMS}")

    string(SUBSTRING "${GIT_COMMIT_HASH_FULL}" 0 7 SNAIL_VERSION_HASH)
    string(SUBSTRING "${GIT_COMMIT_HASH_FULL}" 0 7 GIT_COMMIT_HASH_SHORT)

    message(STATUS "Git commit tag    : ${git_commit_tag}")
    message(STATUS "Git commit hash   : ${GIT_COMMIT_HASH_SHORT}")
    message(STATUS "Git commit time   : ${GIT_COMMIT_TIMESTAMP}")
    message(STATUS "Git commit branch : ${GIT_BRANCH_NAME}")
else()
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/PackageInfo)
        file(STRINGS  ${CMAKE_CURRENT_LIST_DIR}/PackageInfo package_info NEWLINE_CONSUME)

        string(FIND "${package_info}"  "GITHASH:" hash_idx)
        string(FIND "${package_info}"  "GITTIME:" time_idx)
        string(FIND "${package_info}"  "PACKAGE:" pack_idx)

        math(EXPR hash_len ${time_idx}-1)
        math(EXPR time_len ${pack_idx}-${time_idx}-1)

        string(SUBSTRING "${package_info}" ${hash_idx} ${hash_len} hash_str)
        string(SUBSTRING "${package_info}" ${time_idx} ${time_len} time_str)

        string(REGEX REPLACE "^GITHASH:( *)([0-9a-zA-Z]*)( *)$" "\\2" GIT_COMMIT_HASH_FULL "${hash_str}")
        string(REGEX REPLACE "^GITTIME:( *)([-0-9:+ ]*)( *)$"   "\\2" GIT_COMMIT_TIMESTAMP "${time_str}")

        if(hash_idx EQUAL -1 OR time_idx EQUAL -1 OR pack_idx EQUAL -1)
            message(WARNING "PackageInfo format error. You can ignore this and continue.")
            set(GIT_COMMIT_HASH_FULL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
            set(GIT_COMMIT_TIMESTAMP "yyyy-mm-dd hh:mm:ss +xxxx")
        endif()
    else()
        message(WARNING "PackageInfo file missing. You can ignore this and continue.")
        set(GIT_COMMIT_HASH_FULL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
        set(GIT_COMMIT_TIMESTAMP "yyyy-mm-dd hh:mm:ss +xxxx")
    endif()

    string(REPLACE "\"" "" GIT_COMMIT_TIMESTAMP ${GIT_COMMIT_TIMESTAMP})

    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 00 10 GIT_COMMIT_DATE)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 11 08 GIT_COMMIT_TIME)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 20 05 GIT_COMMIT_ZONE)

    string(REPLACE "-" "" GIT_COMMIT_DATE_NUMS ${GIT_COMMIT_DATE})
    string(REPLACE ":" "" GIT_COMMIT_TIME_NUMS ${GIT_COMMIT_TIME})

    set(SNAIL_VERSION_TIME "${GIT_COMMIT_DATE_NUMS}${GIT_COMMIT_TIME_NUMS}")

    string(SUBSTRING "${GIT_COMMIT_HASH_FULL}" 0 7 GIT_COMMIT_HASH_SHORT)
    string(SUBSTRING "${GIT_COMMIT_HASH_FULL}" 0 7 SNAIL_VERSION_HASH)

    message(STATUS "Git commit hash   : ${GIT_COMMIT_HASH_SHORT}")
    message(STATUS "Git commit time   : ${GIT_COMMIT_TIMESTAMP}")
endif()
set(SNAIL_VERSION_GIT "${SNAIL_VERSION_HASH}-${SNAIL_VERSION_TIME}")

# check building environment
include(CheckBuildingEnv)

# Place targets in bin/, lib/ for all build configurations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)

# Dev, Debug, Release, MinSizeRel, etc.
foreach(item ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CFGNAME} item)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${item}  ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${item}  ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${item}  ${CMAKE_BINARY_DIR}/lib)
endforeach()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    # none MinSizeRel/Release type build
    set(CMAKE_INSTALL_PREFIX    "${PROJECT_BINARY_DIR}/disk/snail"   CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_BIN_DIR   "${CMAKE_INSTALL_PREFIX}/bin"        CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_DOC_DIR   "${CMAKE_INSTALL_PREFIX}/doc"        CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_LIB_DIR   "${CMAKE_INSTALL_PREFIX}/lib"        CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_PLG_DIR   "${CMAKE_INSTALL_PREFIX}/plugins"    CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_RUN_DIR   "${CMAKE_INSTALL_PREFIX}/runtime"    CACHE PATH "" FORCE)
    set(SNAIL_USR_HOME_PREFIX   "${PROJECT_BINARY_DIR}/disk/.snail"  CACHE PATH "" FORCE)
else()
    # MinSizeRel/Release type build
    set(CMAKE_INSTALL_PREFIX  "/opt/snail"  CACHE  PATH  ""  FORCE)

    if(HOST_OS_WINDOWS)
        set(CMAKE_INSTALL_PREFIX  "C:\\Program Files\\snail"  CACHE  PATH  ""  FORCE)
        message(AUTHOR_WARNING "todo for Windows")
    endif()

    set(CMAKE_INSTALL_BIN_DIR   "${CMAKE_INSTALL_PREFIX}/bin"        CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_DOC_DIR   "${CMAKE_INSTALL_PREFIX}/doc"        CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_LIB_DIR   "${CMAKE_INSTALL_PREFIX}/lib"        CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_PLG_DIR   "${CMAKE_INSTALL_PREFIX}/plugins"    CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_RUN_DIR   "${CMAKE_INSTALL_PREFIX}/runtime"    CACHE PATH "" FORCE)

    set(SNAIL_USR_HOME_PREFIX   "~/.snail"  CACHE PATH "" FORCE) # ${HOME}/.snail
endif()
message(STATUS "Sys Install Perfix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "User Config Perfix: ${SNAIL_USR_HOME_PREFIX}")

add_subdirectory(source)

if(SNAIL_RELEASE_READY)
    message(STATUS "==================================================================")
    message(STATUS "Release Package: ${SNAIL_PACKAGE_NAME}")
    message(STATUS "==================================================================")
    include(ReleasePackage)
else()
    message(STATUS "==================================================================")
    message(STATUS "${rce_msg}")
    message(STATUS "==================================================================")
endif()
