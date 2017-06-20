# Host arch normalized name: X86 or X86_64
# See https://github.com/axr/solar-cmake/blob/master/TargetArch.cmake
include(CheckSymbolExists)

# X86
check_symbol_exists("_M_IX86"   ""  T_M_IX86)
check_symbol_exists("__i386__"  ""  T_I386)
if(T_M_IX86 OR T_I386)
    option(host_os_supported_x86 "Host system support 32-bits." ON)
endif()

# X86_64
check_symbol_exists("_M_AMD64"    ""  T_M_AMD64)
check_symbol_exists("__x86_64__"  ""  T_X86_64)
check_symbol_exists("__amd64__"   ""  T_AMD64)

if(T_M_AMD64 OR T_X86_64 OR T_AMD64)
    option(host_os_supported_x86_64 "Host system support 64-bits." ON)
endif()


if(ARCH_32_ENABLE AND host_os_supported_x86)
    option(HOST_OS_ARCH_32 "Host System is 32-bits." ON)
elseif(ARCH_64_ENABLE AND host_os_supported_x86_64)
    option(HOST_OS_ARCH_64 "Host System is 64-bits." ON)
else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND host_os_supported_x86_64)
        option(HOST_OS_ARCH_64 "Host System is 64-bits." ON)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND host_os_supported_x86)
        option(HOST_OS_ARCH_32 "Host System is 32-bits." ON)
    else()
        message(FATAL_ERROR "Unknown host os architecture!")
    endif()
endif()
