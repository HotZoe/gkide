# - Try to find libuv
# Once done, the following will define
#
#  LIBUV_FOUND        - system has libuv
#  LIBUV_INCLUDE_DIRS - the libuv include directories
#  LIBUV_LIBRARIES    - link these to use libuv
#
# Set the LIBUV_USE_STATIC variable to specify if static libraries should
# be preferred to shared ones.

if(NOT LIBUV_USE_BUNDLED)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        # When the QUIET argument is set, no status messages will be printed.
        pkg_check_modules(PC_LIBUV QUIET libuv>=${LibUV_FIND_VERSION})
    endif()
else()
    # see 'pkg_check_modules' for details
    set(PC_LIBUV_INCLUDEDIR) # include-dir of the module
    set(PC_LIBUV_INCLUDE_DIRS) # the '-I' preprocessor flags (w/o the '-I')
    set(PC_LIBUV_LIBDIR) # lib-dir of the module
    set(PC_LIBUV_LIBRARY_DIRS) # the paths of the libraries (w/o the '-L')
    set(LIMIT_SEARCH NO_DEFAULT_PATH) # only the libraries (w/o the '-l')
endif()

# This command is used to find a directory containing the named file
# If nothing is found, the result will be 'LIBUV_INCLUDE_DIR-NOTFOUND'
# and the search will be attempted again the next time find_path is
# invoked with the same variable If 'NO_DEFAULT_PATH' is specified,
# then no additional paths are added to the search.
find_path(LIBUV_INCLUDE_DIR uv.h
          HINTS ${PC_LIBUV_INCLUDEDIR} ${PC_LIBUV_INCLUDE_DIRS} ${LIMIT_SEARCH})

find_path(LIBUV_VERSION_DIR uv-version.h
          HINTS ${PC_LIBUV_INCLUDEDIR} ${PC_LIBUV_INCLUDE_DIRS})

if(LIBUV_VERSION_DIR)
    file(READ ${LIBUV_VERSION_DIR}/uv-version.h uv_version_h)
    string(REGEX REPLACE ".*UV_VERSION_MAJOR +([0-9]+).*"
                         "\\1" LIBUV_VERSION_MAJOR "${uv_version_h}")
    string(REGEX REPLACE ".*UV_VERSION_MINOR +([0-9]+).*"
                         "\\1" LIBUV_VERSION_MINOR "${uv_version_h}")
    string(REGEX REPLACE ".*UV_VERSION_PATCH +([0-9]+).*"
                         "\\1" LIBUV_VERSION_PATCH "${uv_version_h}")
    set(LIBUV_VERSION_STRING
        "${LIBUV_VERSION_MAJOR}.${LIBUV_VERSION_MINOR}.${LIBUV_VERSION_PATCH}")
else()
    set(LIBUV_VERSION_STRING)
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
# If we're asked to use static linkage, add libuv.a as a preferred library name.
if(LIBUV_USE_STATIC)
    list(APPEND LIBUV_NAMES
         "${CMAKE_STATIC_LIBRARY_PREFIX}uv${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()

if(MSVC)
    list(APPEND LIBUV_NAMES libuv)
else()
    list(APPEND LIBUV_NAMES uv)
endif()

find_library(LIBUV_LIBRARY NAMES ${LIBUV_NAMES}
             HINTS ${PC_LIBUV_LIBDIR} ${PC_LIBUV_LIBRARY_DIRS} ${LIMIT_SEARCH})

mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)

if(PC_LIBUV_LIBRARIES)
    list(REMOVE_ITEM PC_LIBUV_LIBRARIES uv)
endif()

set(LIBUV_LIBRARIES    ${LIBUV_LIBRARY} ${PC_LIBUV_LIBRARIES})
set(LIBUV_INCLUDE_DIRS ${LIBUV_INCLUDE_DIR})

# Check if the function exists.
# check_library_exists(LIBRARY FUNCTION LOCATION VARIABLE)
#   LIBRARY  - the name of the library you are looking for
#   FUNCTION - the name of the function
#   LOCATION - location where the library should be found
#   VARIABLE - variable to store the result
#              Will be created as an internal cache variable
#
# Deal with the fact that libuv.pc is missing important dependency information.
include(CheckLibraryExists)

check_library_exists(dl dlopen "dlfcn.h" HAVE_LIBDL)
if(HAVE_LIBDL)
    list(APPEND LIBUV_LIBRARIES dl)
endif()

check_library_exists(kstat kstat_lookup "kstat.h" HAVE_LIBKSTAT)
if(HAVE_LIBKSTAT)
    list(APPEND LIBUV_LIBRARIES kstat)
endif()

check_library_exists(kvm kvm_open "kvm.h" HAVE_LIBKVM)
if(HAVE_LIBKVM)
    list(APPEND LIBUV_LIBRARIES kvm)
endif()

check_library_exists(nsl gethostbyname "nsl.h" HAVE_LIBNSL)
if(HAVE_LIBNSL)
    list(APPEND LIBUV_LIBRARIES nsl)
endif()

check_library_exists(perfstat perfstat_cpu "libperfstat.h" HAVE_LIBPERFSTAT)
if(HAVE_LIBPERFSTAT)
    list(APPEND LIBUV_LIBRARIES perfstat)
endif()

check_library_exists(rt clock_gettime "time.h" HAVE_LIBRT)
if(HAVE_LIBRT)
    list(APPEND LIBUV_LIBRARIES rt)
endif()

check_library_exists(sendfile sendfile "" HAVE_LIBSENDFILE)
if(HAVE_LIBSENDFILE)
    list(APPEND LIBUV_LIBRARIES sendfile)
endif()

if(WIN32)
    # check_library_exists() does not work for Win32 API
    # calls in X86 due to name mangling calling conventions
    list(APPEND LIBUV_LIBRARIES iphlpapi)
    list(APPEND LIBUV_LIBRARIES psapi)
    list(APPEND LIBUV_LIBRARIES userenv)
    list(APPEND LIBUV_LIBRARIES ws2_32)
endif()

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set LIBUV_FOUND to TRUE
# if all listed variables are TRUE
# Specifies either <PackageName>_FOUND or <PACKAGENAME>_FOUND as the result
# variable. This exists only for compatibility with older versions of CMake
# and is now ignored. Result variables of both names are always set for
# compatibility.
#
# see 'find_package_handle_standard_args' for details
find_package_handle_standard_args(LibUV
                                  REQUIRED_VARS LIBUV_LIBRARY LIBUV_INCLUDE_DIR
                                  VERSION_VAR   LIBUV_VERSION_STRING)

mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)
