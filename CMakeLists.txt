cmake_minimum_required(VERSION 3.1.0)
project(gkide)

# Enable output of compile commands during generation
# generates compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GKIDE release check message for cmake
set(release_check_msg  "")
set(GKIDE_RELEASE_READY false)

set(GKIDE_LOGO "${PROJECT_SOURCE_DIR}/gkide.png")
set(GKIDE_HOME "https://github.com/gkide/gkide")
set(GKIDE_DESC "GKIDE is an elegant code editor of the natural vim way.")

# https://semver.org/spec/v2.0.0.html
#
# major: make incompatible API changes
# minor: add functionality in a backwards-compatible manner
# patch: make backwards-compatible bug fixes
set(GKIDE_VERSION_MAJOR   1)
set(GKIDE_VERSION_MINOR   0)
set(GKIDE_VERSION_PATCH   0)
# dev -> alpha -> beta -> rc -> release -> stable
set(GKIDE_VERSION_TRAIL beta)

# snail version
set(SNAIL_VERSION_MAJOR   0)
set(SNAIL_VERSION_MINOR   2)
set(SNAIL_VERSION_PATCH   7)

# nvim version
set(NVIM_VERSION_MAJOR    0)
set(NVIM_VERSION_MINOR    2)
set(NVIM_VERSION_PATCH    1)

# nvim API version
set(NVIM_API_VERSION      4) # nvim API version
set(NVIM_API_COMPATIBLE   1) # nvim compatible API version
set(NVIM_API_PRERELEASE   true)

option(GCOV_ENABLE "Enable gcov support." OFF)
option(ASSERTION_ENABLE "Enable assertion." ON)
option(TRAVIS_CI_ENABLE "Enable Travis CI build." OFF)

option(NVIM_TESTING_ENABLE   "Enable nvim testing." ON)
option(NVIM_LOGGING_ENABLE   "Enable nvim logging." ON)
option(NVIM_DEBUGGING_ENABLE "Enable nvim debugging." ON)

option(SNAIL_TESTING_ENABLE   "Enable snail testing." ON)
option(SNAIL_LOGGING_ENABLE   "Enable snail logging." ON)
option(SNAIL_DEBUGGING_ENABLE "Enable snail debugging." ON)

# GKIDE release package version
set(GKIDE_RELEASE_VERSION
    "${GKIDE_VERSION_MAJOR}.${GKIDE_VERSION_MINOR}.${GKIDE_VERSION_PATCH}")

math(EXPR GKIDE_VERSION_INT32
          "(${GKIDE_VERSION_MAJOR}<<16)
          + (${GKIDE_VERSION_MINOR}<<8)
          + ${GKIDE_VERSION_PATCH}")

# snail basic version
set(SNAIL_VERSION_BASIC
    "${SNAIL_VERSION_MAJOR}.${SNAIL_VERSION_MINOR}.${SNAIL_VERSION_PATCH}")

math(EXPR SNAIL_VERSION_INT32
          "(${SNAIL_VERSION_MAJOR}<<16)
           + (${SNAIL_VERSION_MINOR}<<8)
           + ${SNAIL_VERSION_PATCH}")

# nvim basic version
set(NVIM_VERSION_BASIC
    "${NVIM_VERSION_MAJOR}.${NVIM_VERSION_MINOR}.${NVIM_VERSION_PATCH}")

math(EXPR NVIM_VERSION_INT32
          "(${NVIM_VERSION_MAJOR}<<16)
           + (${NVIM_VERSION_MINOR}<<8)
           + ${NVIM_VERSION_PATCH}")

# Set available build types for CMake GUIs.
set_property(CACHE CMAKE_BUILD_TYPE
             PROPERTY STRINGS "Dev"  "Debug"  "Release"  "MinSizeRel")

# Set default build type, in case of not given
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Dev" CACHE STRING "Choose the build type ..." FORCE)
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} build_type)

# Check release type
if(GKIDE_RELEASE_TYPE)
    if(GKIDE_RELEASE_TYPE STREQUAL    "dev"
       OR GKIDE_RELEASE_TYPE STREQUAL "alpha"
       OR GKIDE_RELEASE_TYPE STREQUAL "rc"
       OR GKIDE_RELEASE_TYPE STREQUAL "release"
       OR GKIDE_RELEASE_TYPE STREQUAL "beta"
       OR GKIDE_RELEASE_TYPE STREQUAL "stable")
        set(GKIDE_VERSION_TRAIL "${GKIDE_RELEASE_TYPE}")
    else()
        set(warning_msg
            "GKIDE_RELEASE_TYPE = ${GKIDE_RELEASE_TYPE}, "
            "it must one of followings:")

        set(warning_msg "${warning_msg}\ndev, alpha, beta, rc, release, stable")
        set(warning_msg "${warning_msg}\nset to default: beta")
        message(WARNING "${warning_msg}")
        set(GKIDE_RELEASE_TYPE "${GKIDE_VERSION_TRAIL}")
    endif()
else()
    set(GKIDE_RELEASE_TYPE "${GKIDE_VERSION_TRAIL}")
endif()

# Append custom cmake modules search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# do NOT want building in source-tree
include(PreventInTreeBuilds)

# If git in the env-var 'PATH', it will auto find by
# cmake; if not given it by hand.
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
        set(GKIDE_RELEASE_READY  false)
        if(NOT release_check_msg)
            set(release_check_msg "Release version must be the master branch.")
        endif()
    endif()

    if(${GIT_COMMIT_TAG} MATCHES "NOTAGNAME")
        # no release tag set for current commit
        set(GIT_COMMIT_TAG  "Not Set")
        set(GKIDE_RELEASE_READY  false)

        if(NOT release_check_msg)
            set(info_msg "gkide-v${GKIDE_RELEASE_VERSION}-${GKIDE_VERSION_TRAIL}")
            set(release_check_msg "GKIDE release tag should be: ${info_msg}")
        endif()
    elseif(NOT ${GIT_COMMIT_TAG} MATCHES "^v[0-9]+\.[0-9]+\.[0-9]+[-]*[a-z]*$")
        # if the current commit is ready to release, must set a tag
        # tag format like:  v1.0.0-dev
        set(GKIDE_RELEASE_READY  false)

        if(NOT release_check_msg)
            set(info_msg "gkide-v${GKIDE_RELEASE_VERSION}-${GKIDE_VERSION_TRAIL}")
            set(release_check_msg "GKIDE release tag should be: ${info_msg}")
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
else()
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/DistInfo)
        file(STRINGS
             ${CMAKE_CURRENT_LIST_DIR}/DistInfo dist_info NEWLINE_CONSUME)

        string(REGEX REPLACE
                     ".*GITHASH[ \t]?=[ \t]?([0-9a-f]+).*"
                     "\\1" GIT_COMMIT_HASH_FULL "${dist_info}")

        string(REGEX REPLACE
                     ".*GITTIME[ \t]?=[ \t]?([0-9+-: ]+).*"
                     "\\1" GIT_COMMIT_TIMESTAMP "${dist_info}")
    else()
        message(WARNING "DistInfo file missing. "
                        "You can ignore this and continue.")

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

# change the default cmake value without overriding a user-provided value
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/gkide"  CACHE PATH "" FORCE)
endif()

set(CMAKE_INSTALL_BIN_DIR  "${CMAKE_INSTALL_PREFIX}/bin"  CACHE PATH "" FORCE)
set(CMAKE_INSTALL_ETC_DIR  "${CMAKE_INSTALL_PREFIX}/etc"  CACHE PATH "" FORCE)
set(CMAKE_INSTALL_DOC_DIR  "${CMAKE_INSTALL_PREFIX}/doc"  CACHE PATH "" FORCE)
set(CMAKE_INSTALL_PLG_DIR  "${CMAKE_INSTALL_PREFIX}/plg"  CACHE PATH "" FORCE)
set(CMAKE_INSTALL_MIS_DIR  "${CMAKE_INSTALL_PREFIX}/mis"  CACHE PATH "" FORCE)

# TUI support for nvim
if(HOST_OS_WINDOWS)
    # Disable TUI for windows build
    option(NVIM_FEAT_TUI "Enable the Terminal UI for nvim." OFF)
else()
    # Enable TUI for none windws build
    option(NVIM_FEAT_TUI "Enable the Terminal UI for nvim." ON)
endif()

add_subdirectory(source)

if(GKIDE_RELEASE_READY)
    include(ReleasePackage)
    set(gkide_release_info "GKIDE release package: ${GKIDE_PACKAGE_NAME}")
else()
    set(gkide_release_info "${release_check_msg}")
endif()

message(STATUS "==========================================================")
message(STATUS "nvim version        : ${NVIM_VERSION_BASIC}")
message(STATUS "nvim API level      : ${NVIM_API_VERSION}")
message(STATUS "nvim API compatible : ${NVIM_API_COMPATIBLE}")
message(STATUS "nvim API prerelease : ${NVIM_API_PRERELEASE}")
message(STATUS "snail version       : ${SNAIL_VERSION_BASIC}")
message(STATUS "GKIDE version       : ${GKIDE_RELEASE_VERSION}")
message(STATUS "GKIDE build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "GKIDE release type  : ${GKIDE_RELEASE_TYPE}")
message(STATUS "GKIDE commit tag    : ${GIT_COMMIT_TAG}")
message(STATUS "GKIDE commit branch : ${GIT_BRANCH_NAME}")
message(STATUS "GKIDE commit hash   : ${GIT_COMMIT_HASH_SHORT}")
message(STATUS "GKIDE commit time   : ${GIT_COMMIT_TIMESTAMP}")
message(STATUS "==========================================================")
message(STATUS "GKIDE nvim user home: ~/.gkide or \$GKIDE_USR_HOME")
message(STATUS "GKIDE install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==========================================================")
message(STATUS "${gkide_release_info}")
message(STATUS "==========================================================")
