# Target system and build system always the same:
#
# HOST_OS_WINDOWS, HOST_OS_LINUX, HOST_OS_MACOS

# Check host build: Windows
if(CMAKE_HOST_WIN32 AND CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    option(HOST_OS_WINDOWS "Host System: Windows" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." ON)
    set(target_system "windows")
endif()

if(HOST_OS_WINDOWS AND NOT MINGW)
    message(FATAL_ERROR "Windows build with MSVC is not supported!")
endif()


# Check host build: UNIX
if(CMAKE_HOST_UNIX AND CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    # Check UNIX first, because Macos is a kinde of UNIX
    option(HOST_OS_LINUX "Build for Linux or Linux like operating systems." ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported" ON)
    set(target_system "linux")
endif()


# Check host build: Mac OS X
if(CMAKE_HOST_APPLE AND CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    option(HOST_OS_MACOS "Host System: Macos" ON)
    option(CURRENT_HOST_SUPPORTED "Current host is supported." ON)
    set(target_system "macos")
endif()


if(NOT CURRENT_HOST_SUPPORTED)
    set(err_msg "Trying to build 'GKIDE' in an unsupported host system: ${CMAKE_HOST_SYSTEM}")
    message(FATAL_ERROR "${err_msg}")
endif()

# Get build host info: hostname, user name, host system
include(BasicBuildingInfo)
GetHostSystemInfo(BUILD_OS_NAME BUILD_OS_VERSION)
GetHostNameUserName(BUILD_USER_NAME BUILD_HOST_NAME)

# Check host architecture
include(CheckBuildArch)

# cmake arguments: TARGET_ARCH_32, TARGET_ARCH_64
# auto check host: HOST_PC_ARCH_32, HOST_PC_ARCH_64
# target to build: HOST_OS_ARCH_32, HOST_OS_ARCH_64
#
# If not do cross compiling, target just following the host
if(TARGET_ARCH_32 AND TARGET_ARCH_64)
    message(FATAL_ERROR "'TARGET_ARCH_32' and 'TARGET_ARCH_64' are mutually exclusive!")
elseif(TARGET_ARCH_32 AND NOT TARGET_ARCH_64)
    option(HOST_OS_ARCH_32 "Target GKIDE will running on 32-bits OS." ON)
elseif(NOT TARGET_ARCH_32 AND TARGET_ARCH_64)
    option(HOST_OS_ARCH_64 "Target GKIDE will running on 64-bits OS." ON)
else()
    # just following the build host
    if(HOST_PC_ARCH_32)
        option(HOST_OS_ARCH_32 "Target GKIDE will running on 32-bits OS." ON)
    elseif(HOST_PC_ARCH_64)
        option(HOST_OS_ARCH_64 "Target GKIDE will running on 64-bits OS." ON)
    else()
        message(FATAL_ERROR "Unknown target os architecture!")
    endif()
endif()

if(HOST_OS_ARCH_32)
    set(host_os_arch "x86_32")
else()
    set(host_os_arch "x86_64")
endif()

# Target endian type will always the same as Host PC
include(CheckHostEndianType)
CheckHostEndianType(host_pc_endian)

if(host_pc_endian STREQUAL "little endian")
    option(HOST_OS_ENDIAN_L "Target system little endian" ON)
elseif(host_pc_endian STREQUAL "big endian")
    option(HOST_OS_ENDIAN_B "Target system big endian" ON)
endif()

message(STATUS "C Compiler        : ${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION}")
message(STATUS "C++ Compiler      : ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Host System       : ${target_system}, ${host_pc_arch}, ${host_pc_endian}")
message(STATUS "Target System     : ${target_system}, ${host_os_arch}, ${host_pc_endian}")
message(STATUS "CMake Generator   : ${CMAKE_GENERATOR}")
message(STATUS "CMake Build Type  : ${CMAKE_BUILD_TYPE}")

# Get Current System Data Time
GetCurrentSystemTime(PACKAGE_BUILD_TIMESTAMP)

string(REPLACE "-" "" PACKAGE_BUILD_TIMESTAMP ${PACKAGE_BUILD_TIMESTAMP})
string(REPLACE ":" "" PACKAGE_BUILD_TIMESTAMP ${PACKAGE_BUILD_TIMESTAMP})
string(REPLACE " " "" RELEASE_BUILD_TIMESTAMP ${PACKAGE_BUILD_TIMESTAMP}) # build number

set(RELEASE_PACKAGE_SYS "${target_system}-${host_os_arch}")
set(RELEASE_PACKAGE_VER "${PACKAGE_VERSION_BASIC}-${PACKAGE_VERSION_TRAIL}")

set(RELEASE_PACKAGE_NAME "${RELEASE_PACKAGE_VER}-${GIT_COMMIT_HASH_SHORT}")
set(RELEASE_PACKAGE_NAME "gkide-v${RELEASE_PACKAGE_NAME}-${RELEASE_PACKAGE_SYS}-${RELEASE_BUILD_TIMESTAMP}.tar.gz")
