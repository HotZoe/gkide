cmake_minimum_required(VERSION 3.0.0)

set(SNAIL_INFO_DESC  "snail is a elegant and sophisticated editor for programmers.")
set(SNAIL_INFO_LOGO  "${PROJECT_SOURCE_DIR}/snail.png")

# Full version: Major.Minor.Patch-GitShortHash-GitDateTime-Trail
set(SNAIL_VERSION_MAJOR   0)
set(SNAIL_VERSION_MINOR   0)
set(SNAIL_VERSION_PATCH   0)

# dev -> alpha -> beta -> rc -> release -> stable
set(SNAIL_RELEASE_CHECK  false)
set(SNAIL_RELEASE_READY  false)
set(SNAIL_VERSION_TRAIL  "dev")

project(snail)

# Basic version
set(SNAIL_VERSION_BASIC "${SNAIL_VERSION_MAJOR}.${SNAIL_VERSION_MINOR}.${SNAIL_VERSION_PATCH}")

# Set available build types for CMake GUIs.
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
             "Debug"  "Dev"  "Release"  "MinSizeRel"  "RelWithDebInfo")

# Set default build type, in case of not given
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Dev" CACHE STRING "Choose the build type ..." FORCE)
endif()

# Check release type
if(SNAIL_RELEASE_TYPE)
    if(NOT SNAIL_RELEASE_TYPE STREQUAL "dev"  AND NOT SNAIL_RELEASE_TYPE STREQUAL "alpha"   AND
       NOT SNAIL_RELEASE_TYPE STREQUAL "rc"   AND NOT SNAIL_RELEASE_TYPE STREQUAL "release" AND
       NOT SNAIL_RELEASE_TYPE STREQUAL "beta" AND NOT SNAIL_RELEASE_TYPE STREQUAL "stable")

        set(warning_msg "SNAIL_RELEASE_TYPE = ${SNAIL_RELEASE_TYPE}, it must one of followings:")
        set(warning_msg "${warning_msg}\ndev, alpha, beta, rc, release, stable")
        set(warning_msg "${warning_msg}\nset to default: beta")
        message(WARNING "${err_msg}")

        set(SNAIL_VERSION_TRAIL  "beta")
    else()
        set(SNAIL_VERSION_TRAIL "${SNAIL_RELEASE_TYPE}")
    endif()
endif()

set(rce_msg  "") # release check error message

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
        if(NOT rce_msg)
            set(rce_msg "Release version must be the 'master' branch.")
        endif()
        set(SNAIL_RELEASE_READY  false)
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

    file(WRITE  ${CMAKE_BINARY_DIR}/GitInfo "HASH:${GIT_COMMIT_HASH_FULL}\n")
    file(APPEND ${CMAKE_BINARY_DIR}/GitInfo "TIME:${GIT_COMMIT_TIMESTAMP}\n")

    message(STATUS "Git commit tag    : ${git_commit_tag}")
    message(STATUS "Git commit hash   : ${GIT_COMMIT_HASH_SHORT}")
    message(STATUS "Git commit time   : ${GIT_COMMIT_TIMESTAMP}")
    message(STATUS "Git commit branch : ${GIT_BRANCH_NAME}")
else()
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/GitInfo)
        file(STRINGS  ${CMAKE_CURRENT_LIST_DIR}/GitInfo git_info)

        string(FIND "${git_info}"  ";"     sepc_idx)
        string(FIND "${git_info}"  "HASH:" hash_idx)
        string(FIND "${git_info}"  "TIME:" time_idx)

        string(SUBSTRING "${git_info}" ${hash_idx} ${sepc_idx} hash_str)
        string(SUBSTRING "${git_info}" ${time_idx} -1 time_str)

        string(REGEX REPLACE "^HASH:( *)([0-9a-zA-Z]*)( *)$" "\\2" GIT_COMMIT_HASH_FULL "${hash_str}")
        string(REGEX REPLACE "^TIME:( *)([-0-9:+ ]*)( *)$"   "\\2" GIT_COMMIT_TIMESTAMP "${time_str}")
    else()
        message(WARNING "The snail tarball missing GitInfo file. You can ignore this and continue.")
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

option(GCOV_ENABLE "Enable gcov support." OFF)
option(JEMALLOC_ENABLE "Enable jemalloc library." ON)
option(TRAVIS_CI_BUILD_ENABLE "Enable Travis CI build." OFF)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Generate a compile_commands.json" ON)

# check building environment
include(CheckBuildingEnv)

# Place targets in bin/, lib/ for all build configurations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)

# Dev, Debug, Release, MinSizeRel, RelWithDebInfo, etc.
foreach(item ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CFGNAME} item)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${item}  ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${item}  ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${item}  ${CMAKE_BINARY_DIR}/lib)
endforeach()

#add_subdirectory(source)
#add_subdirectory(tests)

if(SNAIL_RELEASE_READY)
    message(STATUS "=================================================")
    message(STATUS "Snail Release Package Name: ${SNAIL_PACKAGE_NAME}")
    message(STATUS "=================================================")
    include(ReleasePackage)
endif()

if(SNAIL_RELEASE_CHECK AND rce_msg)
    message(STATUS "================================================")
    message(STATUS "${rce_msg}")
    message(STATUS "================================================")
endif()
