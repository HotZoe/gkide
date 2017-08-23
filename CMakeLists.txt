cmake_minimum_required(VERSION 3.1.0)

set(GKIDE_LOGO "${PROJECT_SOURCE_DIR}/gkide.png")
set(GKIDE_HOME "https://github.com/gkide/gkide")
set(GKIDE_DESC "GKIDE is an elegant and sophisticated code editor based on vim/nvim.")

project(gkide)

# Enable output of compile commands during generation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # generates compile_commands.json

option(GCOV_ENABLE "Enable gcov support." OFF)
option(ASSERTION_ENABLE "Enable assertion." ON)
option(TRAVIS_CI_ENABLE "Enable Travis CI build." OFF)

set(release_check_msg  "") # release check message
set(PACKAGE_RELEASE_READY true)
set(PACKAGE_VERSION_TRAIL beta) # dev -> alpha -> beta -> rc -> release -> stable

option(NVIM_TESTING_ENABLE "Enable nvim testing." ON)
option(NVIM_LOGGING_ENABLE "Enable nvim logging." ON)
option(NVIM_DEBUGGING_ENABLE "Enable nvim debugging." ON)

option(SNAIL_TESTING_ENABLE "Enable snail testing." ON)
option(SNAIL_LOGGING_ENABLE "Enable snail logging." ON)
option(SNAIL_DEBUGGING_ENABLE "Enable snail debugging." ON)

set(GKIDE_VERSION_MAJOR   1) # bump when snail/nvim major changing
set(GKIDE_VERSION_MINOR   0) # bump when snail/nvim minor changing
set(GKIDE_VERSION_PATCH   0) # bump when snail/nvim patch changing

# package release version
set(PACKAGE_VERSION_BASIC "${GKIDE_VERSION_MAJOR}.${GKIDE_VERSION_MINOR}.${GKIDE_VERSION_PATCH}")

# snail full version: Major.Minor.Patch-Trail-GitShortHash-GitDateTime
set(SNAIL_VERSION_MAJOR   0) # bump for big version
set(SNAIL_VERSION_MINOR   2) # bump when: feature, plugin, etc.
set(SNAIL_VERSION_PATCH   7) # bump when: bug fix, none user perception changing, etc.

# nvim version
set(NVIM_VERSION_MAJOR    0) # bump for big version
set(NVIM_VERSION_MINOR    2) # bump when: feature, api, plugin, etc.
set(NVIM_VERSION_PATCH    1) # bump when: bug fix, none user perception changing, etc.

# nvim API version
set(NVIM_API_VERSION      1) # nvim API version
set(NVIM_API_COMPATV      1) # nvim compat API version
set(NVIM_API_PRERELEASE   true)

# gkide basic version
set(GKIDE_VERSION_BASIC "${GKIDE_VERSION_MAJOR}.${GKIDE_VERSION_MINOR}.${GKIDE_VERSION_PATCH}")
math(EXPR GKIDE_VERSION_INT32
          "(${GKIDE_VERSION_MAJOR}<<16) + (${GKIDE_VERSION_MINOR}<<8) + ${GKIDE_VERSION_PATCH}")

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
if(PACKAGE_RELEASE_TYPE)
    if(PACKAGE_RELEASE_TYPE STREQUAL "dev"  OR PACKAGE_RELEASE_TYPE STREQUAL "alpha"   OR
       PACKAGE_RELEASE_TYPE STREQUAL "rc"   OR PACKAGE_RELEASE_TYPE STREQUAL "release" OR
       PACKAGE_RELEASE_TYPE STREQUAL "beta" OR PACKAGE_RELEASE_TYPE STREQUAL "stable")
        set(PACKAGE_VERSION_TRAIL  "${PACKAGE_RELEASE_TYPE}")
    else()
        set(warning_msg "PACKAGE_RELEASE_TYPE = ${PACKAGE_RELEASE_TYPE}, it must one of followings:")
        set(warning_msg "${warning_msg}\ndev, alpha, beta, rc, release, stable")
        set(warning_msg "${warning_msg}\nset to default: beta")
        message(WARNING "${warning_msg}")
    endif()
endif()

# Append custom cmake modules search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# do NOT want building in source-tree
include(PreventInTreeBuilds)

# If git in the env-var 'PATH', it will auto find by cmake; if not given it by hand.
if(GIT_PROG)
    file(TO_CMAKE_PATH "${GIT_PROG}" GIT_PROG)
endif()

# Check if git repo exist
file(TO_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR}/.git GKIDE_GIT_REPO_DIR)

# Get GKIDE distribution info: from git repo or 'DistInfo' file.
if(EXISTS "${GKIDE_GIT_REPO_DIR}" AND IS_DIRECTORY "${GKIDE_GIT_REPO_DIR}")
    include(GetGitRepositoryInfo)
    option(ENABLE_VERSION_MIDDLE "Make git info into version info." ON)

    GetGitCurrentBranchInfo(GIT_BRANCH_NAME GIT_COMMIT_HASH_FULL)
    GetGitBranchTimestamp(GIT_COMMIT_TIMESTAMP)
    GetGitBranchExactTag(GIT_COMMIT_TAG)

    if(NOT ${GIT_BRANCH_NAME} STREQUAL "master")
        # The release version must be the 'master' branch of git repo
        set(PACKAGE_RELEASE_READY  false)
        if(NOT release_check_msg)
            set(release_check_msg "Release version must be the 'master' branch.")
        endif()
    endif()

    if(${GIT_COMMIT_TAG} MATCHES "NOTAGNAME")
        # no release tag set for current commit
        set(GIT_COMMIT_TAG  "Not Set")
        set(PACKAGE_RELEASE_READY  false)
        if(NOT release_check_msg)
            set(release_check_msg "Release tag should be: v${PACKAGE_VERSION_BASIC}-${PACKAGE_VERSION_TRAIL}")
        endif()
    elseif(NOT ${GIT_COMMIT_TAG} MATCHES "^v[0-9]+\.[0-9]+\.[0-9]+[-]*[a-z]*$")
        # if the current commit is ready to release, must set a tag
        # tag format like:  v1.0.0-dev
        set(PACKAGE_RELEASE_READY  false)
        if(NOT release_check_msg)
            set(release_check_msg "Release tag should be: ${PACKAGE_VERSION_BASIC}-${PACKAGE_VERSION_TRAIL}")
        endif()
    endif()

    string(REPLACE "\"" "" GIT_COMMIT_TIMESTAMP "${GIT_COMMIT_TIMESTAMP}")

    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 00 10 GIT_COMMIT_DATE)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 11 08 GIT_COMMIT_TIME)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 20 05 GIT_COMMIT_ZONE)

    string(REPLACE "-" "" GIT_COMMIT_DATE_NUMS ${GIT_COMMIT_DATE})
    string(REPLACE ":" "" GIT_COMMIT_TIME_NUMS ${GIT_COMMIT_TIME})

    set(GIT_COMMIT_DATE_TIME "${GIT_COMMIT_DATE_NUMS}${GIT_COMMIT_TIME_NUMS}")

    string(SUBSTRING "${GIT_COMMIT_HASH_FULL}" 0 7 GIT_COMMIT_HASH_SHORT)

    message(STATUS "Git commit tag    : ${GIT_COMMIT_TAG}")
    message(STATUS "Git commit hash   : ${GIT_COMMIT_HASH_SHORT}")
    message(STATUS "Git commit time   : ${GIT_COMMIT_TIMESTAMP}")
    message(STATUS "Git commit branch : ${GIT_BRANCH_NAME}")
else()
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/DistInfo)
        file(STRINGS  ${CMAKE_CURRENT_LIST_DIR}/DistInfo dist_info NEWLINE_CONSUME)
        string(REGEX REPLACE ".*GITHASH[ \t]?=[ \t]?([0-9a-f]+).*"  "\\1" GIT_COMMIT_HASH_FULL "${dist_info}")
        string(REGEX REPLACE ".*GITTIME[ \t]?=[ \t]?([0-9+-: ]+).*" "\\1" GIT_COMMIT_TIMESTAMP "${dist_info}")
    else()
        message(WARNING "DistInfo file missing. You can ignore this and continue.")
        set(GIT_COMMIT_HASH_FULL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
        set(GIT_COMMIT_TIMESTAMP "yyyy-mm-dd hh:mm:ss +xxxx")
    endif()

    string(REPLACE "\"" "" GIT_COMMIT_TIMESTAMP ${GIT_COMMIT_TIMESTAMP})

    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 00 10 GIT_COMMIT_DATE)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 11 08 GIT_COMMIT_TIME)
    string(SUBSTRING "${GIT_COMMIT_TIMESTAMP}" 20 05 GIT_COMMIT_ZONE)

    string(REPLACE "-" "" GIT_COMMIT_DATE_NUMS ${GIT_COMMIT_DATE})
    string(REPLACE ":" "" GIT_COMMIT_TIME_NUMS ${GIT_COMMIT_TIME})

    set(GIT_COMMIT_DATE_TIME "${GIT_COMMIT_DATE_NUMS}${GIT_COMMIT_TIME_NUMS}")

    string(SUBSTRING "${GIT_COMMIT_HASH_FULL}" 0 7 GIT_COMMIT_HASH_SHORT)

    message(STATUS "Git commit hash   : ${GIT_COMMIT_HASH_SHORT}")
    message(STATUS "Git commit time   : ${GIT_COMMIT_TIMESTAMP}")
endif()

set(PACKAGE_VERSION_GIT "${GIT_COMMIT_HASH_SHORT}-${GIT_COMMIT_DATE_TIME}")

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

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CMAKE_INSTALL_PREFIX  "/opt/gkide"  CACHE  PATH  ""  FORCE)

    if(HOST_OS_WINDOWS)
        set(CMAKE_INSTALL_PREFIX  "C:/Program Files/gkide"  CACHE  PATH  ""  FORCE)
    endif()

    set(CMAKE_INSTALL_BIN_DIR  "${CMAKE_INSTALL_PREFIX}/bin"  CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_ETC_DIR  "${CMAKE_INSTALL_PREFIX}/etc"  CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_DOC_DIR  "${CMAKE_INSTALL_PREFIX}/doc"  CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_PLG_DIR  "${CMAKE_INSTALL_PREFIX}/plg"  CACHE PATH "" FORCE)

    # $ENV{GKIDE_HOME}, $ENV{HOME}/.gkide
    set(USR_CONFIG_DIR_PREFIX  "~/.gkide"  CACHE PATH "" FORCE)
else()
    # none MinSizeRel/Release type build
    set(CMAKE_INSTALL_PREFIX   "${PROJECT_BINARY_DIR}/disk/gkide"   CACHE PATH "" FORCE)

    set(CMAKE_INSTALL_BIN_DIR  "${CMAKE_INSTALL_PREFIX}/bin"  CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_ETC_DIR  "${CMAKE_INSTALL_PREFIX}/etc"  CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_DOC_DIR  "${CMAKE_INSTALL_PREFIX}/doc"  CACHE PATH "" FORCE)
    set(CMAKE_INSTALL_PLG_DIR  "${CMAKE_INSTALL_PREFIX}/plg"  CACHE PATH "" FORCE)

    set(USR_CONFIG_DIR_PREFIX  "${PROJECT_BINARY_DIR}/disk/.gkide"  CACHE PATH "" FORCE)
endif()
message(STATUS "Sys Install Perfix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "User Config Perfix: ${USR_CONFIG_DIR_PREFIX}")

add_subdirectory(source)

if(PACKAGE_RELEASE_READY)
    message(STATUS "==================================================================")
    message(STATUS "Release Package: ${RELEASE_PACKAGE_NAME}")
    message(STATUS "==================================================================")
    include(ReleasePackage)
else()
    message(STATUS "==================================================================")
    message(STATUS "${release_check_msg}")
    message(STATUS "==================================================================")
endif()
