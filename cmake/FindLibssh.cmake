# - Try to find libssh
# Once done, the following will define
#
#  LIBSSH_FOUND        - system has libssh
#  LIBSSH_INCLUDE_DIRS - the libssh include directories
#  LIBSSH_LIBRARIES    - link these to use libssh
#

if(NOT LIBSSH_USE_BUNDLED)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        # When the QUIET argument is set, no status messages will be printed.
        pkg_check_modules(PC_LIBSSH QUIET libssh>=${Libssh_FIND_VERSION})
    endif()
else()
    # see 'pkg_check_modules' for details
    set(PC_LIBSSH_INCLUDEDIR)   # include-dir of the module
    set(PC_LIBSSH_INCLUDE_DIRS) # the '-I' preprocessor flags (w/o the '-I')
    set(PC_LIBSSH_LIBDIR)       # lib-dir of the module
    set(PC_LIBSSH_LIBRARY_DIRS) # the paths of the libraries (w/o the '-L')
    set(LIMIT_SEARCH NO_DEFAULT_PATH) # only the libraries (w/o the '-l')
endif()

# This command is used to find a directory containing the named file
# If nothing is found, the result will be 'LIBUV_INCLUDE_DIR-NOTFOUND'
# and the search will be attempted again the next time find_path is
# invoked with the same variable If 'NO_DEFAULT_PATH' is specified,
# then no additional paths are added to the search.
find_path(LIBSSH_INCLUDE_DIR libssh/libssh.h
          HINTS ${PC_LIBSSH_INCLUDEDIR} ${PC_LIBSSH_INCLUDE_DIRS} ${LIMIT_SEARCH})

find_path(LIBSSH_VERSION_DIR libssh/libssh.h
          HINTS ${PC_LIBSSH_INCLUDEDIR} ${PC_LIBSSH_INCLUDE_DIRS})

if(LIBSSH_VERSION_DIR)
    file(READ ${LIBSSH_VERSION_DIR}/libssh/libssh.h libssh_h)
    string(REGEX REPLACE ".*LIBSSH_VERSION_MAJOR +([0-9]+).*"
                         "\\1" LIBSSH_VERSION_MAJOR "${libssh_h}")
    string(REGEX REPLACE ".*LIBSSH_VERSION_MINOR +([0-9]+).*"
                         "\\1" LIBSSH_VERSION_MINOR "${libssh_h}")
    string(REGEX REPLACE ".*LIBSSH_VERSION_MICRO +([0-9]+).*"
                         "\\1" LIBSSH_VERSION_PATCH "${libssh_h}")
    set(LIBSSH_VERSION_STRING
        "${LIBSSH_VERSION_MAJOR}.${LIBSSH_VERSION_MINOR}.${LIBSSH_VERSION_PATCH}")
else()
    set(LIBSSH_VERSION_STRING)
endif()

# Use static/shared linkage
# CMAKE_STATIC_LIBRARY_PREFIX
#   'lib' on UNIX; null on Windows
# CMAKE_STATIC_LIBRARY_SUFFIX
#   '.a' on UNIX; '.lib' on Windows
# CMAKE_SHARED_LIBRARY_PREFIX
#   'lib' on UNIX; null on WIndows
# CMAKE_SHARED_LIBRARY_SUFFIX
#   '.so' on Unix; '.dll' on Windows
#
list(APPEND LIBSSH_NAMES libssh.a)

find_library(LIBSSH_LIBRARY NAMES ${LIBSSH_NAMES}
             HINTS ${PC_LIBSSH_LIBDIR} ${PC_LIBSSH_LIBRARY_DIRS} ${LIMIT_SEARCH})

mark_as_advanced(LIBSSH_INCLUDE_DIR LIBSSH_LIBRARY)

set(LIBSSH_LIBRARIES    ${LIBSSH_LIBRARY})
set(LIBSSH_INCLUDE_DIRS ${LIBSSH_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set LIBUV_FOUND to TRUE
# if all listed variables are TRUE
# Specifies either <PackageName>_FOUND or <PACKAGENAME>_FOUND as the result
# variable. This exists only for compatibility with older versions of CMake
# and is now ignored. Result variables of both names are always set for
# compatibility.
#
# see 'find_package_handle_standard_args' for details
find_package_handle_standard_args(libssh
                                  REQUIRED_VARS LIBSSH_LIBRARY LIBSSH_INCLUDE_DIR
                                  VERSION_VAR   LIBSSH_VERSION_STRING)

mark_as_advanced(LIBSSH_INCLUDE_DIR LIBSSH_LIBRARY)
