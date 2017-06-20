# True when the host system is Macos
if(CMAKE_HOST_APPLE)
    option(HOST_OS_MACOS "Host System: Macos" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." OFF)
    set(target_system "macos")
    message(FATAL_ERROR "Mac OS not supported yet!")
endif()

# True on Windows systems, including Win64, the host system is Windows and on Cygwin.
if(CMAKE_HOST_WIN32)
    option(HOST_OS_WIN "Host System: Windows" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." ON)
    set(target_system "windows")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    # for UNIX and UNIX like operating systems.
    if(NOT CYGWIN AND NOT CMAKE_HOST_APPLE)
        option(HOST_OS_LINUX "Build for Linux or Linux like operating systems." ON)
        option(CURRENT_HOST_SUPPORTED "Current host is supported" ON)
        set(target_system "linux")
    endif()
else()
    # True for UNIX and UNIX like operating systems.
    # true when the host system is UNIX or UNIX like (i.e. APPLE and CYGWIN).
    if(CMAKE_HOST_UNIX AND NOT CYGWIN AND NOT CMAKE_HOST_APPLE)
        option(HOST_OS_UNIX "Build for UNIX or UNIX like operating systems." ON)
        option(CURRENT_HOST_SUPPORTED "Current host is supported" ON)
        set(target_system "unix")
    endif()
endif()

if(NOT CURRENT_HOST_SUPPORTED)
    set(err_msg "Trying to build [ snail ] in an unsupported host system.")
    set(err_msg "${err_msg}\n  Current Host System Name: ${CMAKE_HOST_SYSTEM}")
    set(err_msg "${err_msg}\n  Current C Compiler ID   : ${CMAKE_C_COMPILER_ID}")
    set(err_msg "${err_msg}\n  Current C++ Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
    message(FATAL_ERROR "${err_msg}")
endif()

# Set to true when using Cygwin.
if(CYGWIN)
    option(HOST_OS_CYGWIN "Host System: Windows/Cygwin" ON)
endif()

# Set to true when the compiler is some version of MinGW.
if(MINGW)
    option(HOST_OS_MINGW "Host System: Windows/MinGW" ON)
endif()

if(HOST_OS_WIN AND (NOT HOST_OS_CYGWIN OR NOT HOST_OS_MINGW) )
    message(FATAL_ERROR "Windows native build is not supported!")
endif()

include(BasicBuildingInfo)

# get host name and user name
GetHostNameUserName(HOST_PC_USER HOST_PC_NAME)

# set target system index
SetTargetSystemIndex("")                   # automatically
#SetTargetSystemIndex(HOST_OS_IDX_DEBIAN)  # host system is debian
#SetTargetSystemIndex(HOST_OS_IDX_UBUNTU)  # host system is ubuntu
#SetTargetSystemIndex(HOST_OS_IDX_WIN7)    # host system is windows7
#SetTargetSystemIndex(HOST_OS_IDX_CYGWIN)  # host system is cygwin

# check build architecture
include(CheckBuildArch)

set(target_arch "x86")
if(HOST_OS_ARCH_64)
    set(target_arch "x86_64")
endif()

message(STATUS "C Compiler        : ${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION}")
message(STATUS "C++ Compiler      : ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Target System     : ${target_system}, ${target_arch}")
message(STATUS "CMake Generator   : ${CMAKE_GENERATOR}")
message(STATUS "CMake Build Type  : ${CMAKE_BUILD_TYPE}")

# Get Current System Data Time
# This have problem, not run each time ...
GetCurrentSystemTime(PACKAGE_BUILD_TIMESTAMP)

# For release package
string(SUBSTRING "${PACKAGE_BUILD_TIMESTAMP}" 00 10 PACKAGE_BUILD_DATE)
string(SUBSTRING "${PACKAGE_BUILD_TIMESTAMP}" 11 8  PACKAGE_BUILD_TIME)

string(REPLACE "-" "" PACKAGE_BUILD_DATE_NUM ${PACKAGE_BUILD_DATE})
string(REPLACE ":" "" PACKAGE_BUILD_TIME_NUM ${PACKAGE_BUILD_TIME})

set(SNAIL_PACKAGE_NUM "${PACKAGE_BUILD_DATE_NUM}${PACKAGE_BUILD_TIME_NUM}")

set(SNAIL_PACKAGE_SYS "${target_system}-${target_arch}")
string(SUBSTRING "${GIT_COMMIT_HASH_FULL}" 0 7 SNAIL_PACKAGE_SRC)
set(SNAIL_PACKAGE_VER "v${SNAIL_VERSION_BASIC}-${SNAIL_VERSION_TRAIL}")

set(SNAIL_PACKAGE_NAME "snail-${SNAIL_PACKAGE_VER}-${SNAIL_PACKAGE_SRC}")
set(SNAIL_PACKAGE_NAME "${SNAIL_PACKAGE_NAME}-${SNAIL_PACKAGE_SYS}-${SNAIL_PACKAGE_NUM}.tar.gz")
