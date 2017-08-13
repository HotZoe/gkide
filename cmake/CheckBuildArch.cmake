# Sets HOST_OS_ARCH to a normalized name: X86 or X86_64
# See https://github.com/axr/solar-cmake/blob/master/TargetArch.cmake
include(CheckSymbolExists)

# X86
check_symbol_exists("_M_IX86"   ""  T_M_IX86)
check_symbol_exists("__i386__"  ""  T_I386)
if(T_M_IX86 OR T_I386)
    option(host_pc_supported_bit32 "Host system support 32-bits." ON)
endif()

# X86_64
check_symbol_exists("_M_AMD64"    ""  T_M_AMD64)
check_symbol_exists("__x86_64__"  ""  T_X86_64)
check_symbol_exists("__amd64__"   ""  T_AMD64)

if(T_M_AMD64 OR T_X86_64 OR T_AMD64)
    option(host_pc_supported_bit64 "Host system support 64-bits." ON)
endif()

# check current build host arch
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND host_pc_supported_bit64)
    set(host_pc_arch "x86_64")
    option(HOST_PC_ARCH_64 "Host System is 64-bits." ON)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND host_pc_supported_bit32)
    set(host_pc_arch "x86_32")
    option(HOST_PC_ARCH_32 "Host System is 32-bits." ON)
else()
    message(FATAL_ERROR "Unknown host os architecture!")
endif()
