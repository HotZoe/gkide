include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckIncludeFiles)
include(CheckFunctionExists)

# Check type size
check_type_size("int" CHECK_SIZEOF_INT)
check_type_size("long" CHECK_SIZEOF_LONG)
check_type_size("off_t" CHECK_SIZEOF_OFF_T)
check_type_size("time_t" CHECK_SIZEOF_TIME_T)
check_type_size("size_t" CHECK_SIZEOF_SIZE_T)
check_type_size("void *" CHECK_SIZEOF_VOID_PTR)
check_type_size("intmax_t" CHECK_SIZEOF_INTMAX_T)
check_type_size("long long" CHECK_SIZEOF_LONG_LONG)

# Check header files existence
check_include_file("locale.h" HAVE_HDR_LOCALE_H)
check_include_file("iconv.h" HAVE_HDR_ICONV_H)
check_include_file("langinfo.h" HAVE_HDR_LANGINFO_H)
check_include_file("pwd.h" HAVE_HDR_PWD_H)
check_include_file("strings.h" HAVE_HDR_STRINGS_H)
check_include_file("sys/wait.h" HAVE_HDR_SYS_WAIT_H)
check_include_file("sys/utsname.h" HAVE_HDR_SYS_UTSNAME_H)
check_include_file("termios.h" HAVE_HDR_TERMIOS_H)
check_include_file("utime.h" HAVE_HDR_UTIME_H)
check_include_file("sys/uio.h" HAVE_HDR_SYS_UIO_H)
check_include_file("endian.h" HAVE_HDR_ENDIAN_H)
check_include_file("sys/endian.h" HAVE_HDR_SYS_ENDIAN_H)
set(INCLUDE_FILE_ENDIAN_H "endian.h")
if(HAVE_HDR_SYS_ENDIAN_H AND NOT HAVE_HDR_ENDIAN_H)
    set(INCLUDE_FILE_ENDIAN_H "sys/endian.h")
endif()

# Check if a C function can be linked
check_function_exists("strcasecmp" HAVE_FUN_STRCASECMP)
check_function_exists("fseeko" HAVE_FUN_FSEEKO)
check_function_exists("getpwent" HAVE_FUN_GETPWENT)
check_function_exists("getpwnam" HAVE_FUN_GETPWNAM)
check_function_exists("getpwuid" HAVE_FUN_GETPWUID)
check_function_exists("readv" HAVE_FUN_READV)
check_function_exists("opendir" HAVE_FUN_OPENDIR)
check_function_exists("readlink" HAVE_FUN_READLINK)
check_function_exists("setenv" HAVE_FUN_SETENV)
check_function_exists("unsetenv" HAVE_FUN_UNSETENV)
check_function_exists("setpgid" HAVE_FUN_SETPGID)
check_function_exists("setsid"  HAVE_FUN_SETSID)
check_function_exists("sigaction" HAVE_FUN_SIGACTION)
check_function_exists("strncasecmp" HAVE_FUN_STRNCASECMP)
check_function_exists("utime" HAVE_FUN_UTIME)
check_function_exists("utimes" HAVE_FUN_UTIMES)

# Check if a symbol exists as a function, variable, or macro
check_symbol_exists("LC_ALL"       "locale.h" HAVE_SYM_LC_ALL)
check_symbol_exists("LC_MESSAGES"  "locale.h" HAVE_SYM_LC_MESSAGES)
if(HAVE_HDR_LANGINFO_H)
    check_symbol_exists("CODESET"  "langinfo.h" HAVE_SYM_CODESET)
endif()

# Generated configuration files for source
configure_file("${CMAKE_CURRENT_LIST_DIR}/config.h.in"
                "${AUTO_CONFIG_DIR}/config.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${AUTO_CONFIG_DIR}/config.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/srcinfo.h.in"
                "${AUTO_CONFIG_DIR}/srcinfo.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${AUTO_CONFIG_DIR}/srcinfo.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/versiondef.h.in"
                "${AUTO_CONFIG_DIR}/versiondef.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${AUTO_CONFIG_DIR}/versiondef.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/pathdef.h.in"
                "${AUTO_CONFIG_DIR}/pathdef.generated.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Generated header: ${AUTO_CONFIG_DIR}/pathdef.h")
