include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckCSourceRuns)
include(CheckCSourceCompiles)

# Check type size
check_type_size("int"       CHECK_SIZEOF_INT)
check_type_size("long"      CHECK_SIZEOF_LONG)
check_type_size("off_t"     CHECK_SIZEOF_OFF_T)
check_type_size("time_t"    CHECK_SIZEOF_TIME_T)
check_type_size("size_t"    CHECK_SIZEOF_SIZE_T)
check_type_size("intmax_t"  CHECK_SIZEOF_INTMAX_T)
check_type_size("long long" CHECK_SIZEOF_LONG_LONG)

# Check header files existence
check_include_file("pwd.h"         HAVE_HDR_PWD_H)
check_include_file("iconv.h"       HAVE_HDR_ICONV_H)
check_include_file("fcntl.h"       HAVE_HDR_FCNTL_H)
check_include_file("utime.h"       HAVE_HDR_UTIME_H)
check_include_file("locale.h"      HAVE_HDR_LOCALE_H)
check_include_file("endian.h"      HAVE_HDR_ENDIAN_H)
check_include_file("sys/uio.h"     HAVE_HDR_SYS_UIO_H)
check_include_file("strings.h"     HAVE_HDR_STRINGS_H)
check_include_file("termios.h"     HAVE_HDR_TERMIOS_H)
check_include_file("sys/wait.h"    HAVE_HDR_SYS_WAIT_H)
check_include_file("langinfo.h"    HAVE_HDR_LANGINFO_H)
check_include_file("sys/endian.h"  HAVE_HDR_SYS_ENDIAN_H)
check_include_file("sys/utsname.h" HAVE_HDR_SYS_UTSNAME_H)

if(NOT HAVE_HDR_SYS_WAIT_H AND UNIX)
    # See if_cscope.c
    message(SEND_ERROR "<sys/wait.h> is required for Unix, but not found!")
endif()

set(INCLUDE_FILE_ENDIAN_H "endian.h")
if(HAVE_HDR_SYS_ENDIAN_H AND NOT HAVE_HDR_ENDIAN_H)
    set(INCLUDE_FILE_ENDIAN_H "sys/endian.h")
endif()

# Check if a C function can be linked
check_function_exists("utime"       HAVE_FUN_UTIME)
check_function_exists("readv"       HAVE_FUN_READV)
check_function_exists("setenv"      HAVE_FUN_SETENV)
check_function_exists("utimes"      HAVE_FUN_UTIMES)
check_function_exists("fseeko"      HAVE_FUN_FSEEKO)
check_function_exists("setsid"      HAVE_FUN_SETSID)
check_function_exists("opendir"     HAVE_FUN_OPENDIR)
check_function_exists("setpgid"     HAVE_FUN_SETPGID)
check_function_exists("readlink"    HAVE_FUN_READLINK)
check_function_exists("getpwent"    HAVE_FUN_GETPWENT)
check_function_exists("getpwnam"    HAVE_FUN_GETPWNAM)
check_function_exists("getpwuid"    HAVE_FUN_GETPWUID)
check_function_exists("unsetenv"    HAVE_FUN_UNSETENV)
check_function_exists("_putenv_s"   HAVE_FUN_PUTENV_S)
check_function_exists("sigaction"   HAVE_FUN_SIGACTION)
check_function_exists("strcasecmp"  HAVE_FUN_STRCASECMP)
check_function_exists("strncasecmp" HAVE_FUN_STRNCASECMP)

if(WIN32 AND NOT HAVE_FUN_PUTENV_S)
    message(SEND_ERROR "_putenv_s() function is required for Windows, but not found!")
endif()

if(UNIX AND NOT HAVE_FUN_SETENV)
    message(SEND_ERROR "setenv() function is required for Unix, but not found!")
endif()

if(HAVE_HDR_SYS_UIO_H AND NOT HAVE_FUN_READV)
    unset(HAVE_HDR_SYS_UIO_H)
endif()

# Check if a symbol exists
check_symbol_exists(FD_CLOEXEC  "fcntl.h"    HAVE_SYM_FD_CLOEXEC)
check_symbol_exists(CODESET     "langinfo.h" HAVE_SYM_CODESET)

check_c_source_compiles(
"
#define  _BSD_SOURCE      1
#define  _DEFAULT_SOURCE  1

#include <stdint.h>
#include <${INCLUDE_FILE_ENDIAN_H}>

#ifndef be64toh
#   error No be64toh macros
#endif

int main(int argc, char **argv)
{
    uint64_t i = 0x0102030405060708ULL;
    uint64_t j = be64toh(i);
    return (j == 0);  // j must not be zero
}
"
HAVE_BE64TOH_MACRO)

if(NOT HAVE_BE64TOH_MACRO)
    check_function_exists("be64toh" HAVE_BE64TOH_FUNC)
endif()

if(HAVE_BE64TOH_MACRO OR HAVE_BE64TOH_FUNC)
    set(HAVE_SYM_BE64TOH  1)
endif()

if(NOT "${HAVE_BSD_BE64TOH}")
    # It is safe to make ORDER_BIG_ENDIAN not defined if:
    # - *HAVE_BSD_BE64TOH* is true. In this case *be64toh* will be used unconditionally in
    #   any case and *ORDER_BIG_ENDIAN* will not be examined.
    #
    # - *HAVE_BSD_BE64TOH* is false. In this case *be64toh* function which uses cycle and
    #   arithmetic operations is used which will work regardless of endianess, this is
    #   sub-optimal though.
    check_c_source_runs(
    "
    #include <stdint.h>

    int main(int argc,char**argv)
    {
        uint64_t i = 0x0102030405060708ULL;
        char *s = (char *)(&i);

        return (s[0] == 0x01 &&
                s[1] == 0x02 &&
                s[2] == 0x03 &&
                s[3] == 0x04 &&
                s[4] == 0x05 &&
                s[5] == 0x06 &&
                s[6] == 0x07 &&
                s[7] == 0x08) ? 0 : 1;
    }
    "
    ORDER_BIG_ENDIAN)
endif()


# get nvim and snail compile flags
include(GetCompileFlags)
GetNvimCompileFlags(NVIM_BUILD_CFLAGS)
GetSnailCompileFlags(SNAIL_BUILD_CXXFLAGS)

# set the project root directory to ~/project/snail
# unifying the information to be shown in the program
string(REPLACE "${CMAKE_SOURCE_DIR}" "snail" NVIM_BUILD_CFLAGS "${NVIM_BUILD_CFLAGS}")
string(REPLACE "${CMAKE_SOURCE_DIR}" "snail" SNAIL_BUILD_CXXFLAGS "${SNAIL_BUILD_CXXFLAGS}")

message(STATUS "==================================================================")
message(STATUS "                      generated config files")
message(STATUS "==================================================================")
# Generated configuration files for source
configure_file("${CMAKE_CURRENT_LIST_DIR}/config.h.in"
                "${GEN_CONFIG_DIR}/config.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${GEN_CONFIG_DIR}/config.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/miscinfo.h.in"
                "${GEN_CONFIG_DIR}/miscinfo.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${GEN_CONFIG_DIR}/miscinfo.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/versiondef.h.in"
                "${GEN_CONFIG_DIR}/versiondef.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${GEN_CONFIG_DIR}/versiondef.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/pathdef.h.in"
                "${GEN_CONFIG_DIR}/pathdef.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${GEN_CONFIG_DIR}/pathdef.h")


