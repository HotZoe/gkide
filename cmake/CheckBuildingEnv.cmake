# [1] Linux   => HOST_OS_LINUX   => "Unix Makefiles"
# [2] MacOS   => HOST_OS_MACOS   => "Unix Makefiles"
# [3] Windows => HOST_OS_WINDOWS
#     [3.1] MSYS   => "MSYS Makefiles"
#     [3.2] MinGW  => "MinGW Makefiles"
#     [3.3] Cygwin => "Unix Makefiles"
###############################################################################
# UNIX
#   true when the target system is UNIX or UNIX like (i.e. APPLE, CYGWIN).
# WIN32
#   true when the target system is Windows(32/64).
# MINGW
#   true when the compiler is some version of MinGW.
# CYGWIN
#   true when using Cygwin.
###############################################################################
# True on OS X.
if(APPLE)
    option(HOST_OS_MACOS "Host System: Macos" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." OFF)
    set(target_system "macos")
endif()

# True when the host system is Apple OS X.
if(CMAKE_HOST_APPLE)
    option(HOST_OS_MACOS "Host System: Macos" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." OFF)
    set(target_system "macos")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    option(HOST_OS_MACOS "Host System: Macos" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." OFF)
    set(target_system "macos")
endif()
###############################################################################
# True when the host system is Windows(32/64) and on Cygwin
if(CMAKE_HOST_WIN32)
    option(HOST_OS_WINDOWS "Host System: Windows" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." ON)
    set(target_system "windows")
endif()

# True when the target system is Windows(32/64).
if(WIN32)
    option(HOST_OS_WINDOWS "Host System: Windows" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." ON)
    set(target_system "windows")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    option(HOST_OS_WINDOWS "Host System: Windows" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." ON)
    set(target_system "windows")
endif()

if(HOST_OS_WINDOWS)
    if(NOT CYGWIN AND NOT MINGW)
        message(FATAL_ERROR "Windows native build is not supported!")
    endif()
endif()
###############################################################################
# True when the target system is UNIX or UNIX like (i.e. APPLE, CYGWIN).
if(UNIX)
    if(NOT APPLE AND NOT CYGWIN)
        option(HOST_OS_LINUX "Build for Linux or Linux like operating systems." ON)
        option(CURRENT_HOST_SUPPORTED "Current host is supported" ON)
        set(target_system "linux")
    endif()
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    option(HOST_OS_LINUX "Build for Linux or Linux like operating systems." ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported" ON)
    set(target_system "linux")
endif()
###############################################################################
if(NOT CURRENT_HOST_SUPPORTED)
    set(err_msg "Trying to build snail in an unsupported host system: ${CMAKE_HOST_SYSTEM}")
    message(FATAL_ERROR "${err_msg}")
endif()
###############################################################################
include(BasicBuildingInfo)

# get host name and user name
GetHostNameUserName(BUILD_USER_NAME BUILD_HOST_NAME)
# get host system name
GetHostSystemName("")

# check host architecture
include(CheckBuildArch)

set(target_arch "x86")
if(HOST_ARCH_64)
    set(target_arch "x86_64")
endif()

# Check host(target) system endian
include(CheckHostEndianType)
CheckHostEndianType(target_endian)

if(target_endian STREQUAL "little endian")
    option(HOST_ENDIAN_L "Host system little endian" ON)
elseif(target_endian STREQUAL "big endian")
    option(HOST_ENDIAN_B "Host system big endian" ON)
endif()

message(STATUS "C Compiler        : ${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION}")
message(STATUS "C++ Compiler      : ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Target System     : ${target_system}, ${target_arch}, ${target_endian}")
message(STATUS "CMake Generator   : ${CMAKE_GENERATOR}")
message(STATUS "CMake Build Type  : ${CMAKE_BUILD_TYPE}")

# Get Current System Data Time
GetCurrentSystemTime(PACKAGE_BUILD_TIMESTAMP)

# For release package
string(SUBSTRING "${PACKAGE_BUILD_TIMESTAMP}" 00 10 PACKAGE_BUILD_DATE)
string(SUBSTRING "${PACKAGE_BUILD_TIMESTAMP}" 11 8  PACKAGE_BUILD_TIME)

string(REPLACE "-" "" PACKAGE_BUILD_DATE_NUM ${PACKAGE_BUILD_DATE})
string(REPLACE ":" "" PACKAGE_BUILD_TIME_NUM ${PACKAGE_BUILD_TIME})

set(RELEASE_PACKAGE_NUM "${PACKAGE_BUILD_DATE_NUM}${PACKAGE_BUILD_TIME_NUM}")
set(RELEASE_PACKAGE_SYS "${target_system}-${target_arch}")
set(RELEASE_PACKAGE_VER "${PACKAGE_VERSION_BASIC}-${PACKAGE_VERSION_TRAIL}")

set(RELEASE_PACKAGE_NAME "${RELEASE_PACKAGE_VER}-${GIT_COMMIT_HASH_SHORT}")
set(RELEASE_PACKAGE_NAME "${RELEASE_PACKAGE_NAME}-${RELEASE_PACKAGE_SYS}-${RELEASE_PACKAGE_NUM}.tar.gz")

configure_file("${CMAKE_CURRENT_LIST_DIR}/PackageInfo.in"
               "${CMAKE_BINARY_DIR}/PackageInfo")
