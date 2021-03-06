# Version 1.0 (2013-04-12)
# Public Domain, originally written by Lasse Kärkkäinen <tronic@zi.fi>
# Published at http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
# Changelog:
# 2017-04-04  Improved the script by charlie WONG
# 2013-04-12  Added version number (1.0) and this header, no other changes
# 2009-10-08  Originally published
#
#
# if(<constant>)
#     True : if the <constant> is 1, ON, YES,  TRUE, Y, or a non-zero number;
#     False: if the <constant> is 0, OFF, NO, FALSE, N, IGNORE, NOTFOUND,
#                                 the empty string, or a string ends in the
#                                 suffix -NOTFOUND;
#
#     Named boolean constants are case-insensitive. If the argument is not
#     one of these specific <constant>, it is treated as a variable or string,
#     see CMake doc 'if' for details.
#
#
# Works the same as find_package, but forwards the "REQUIRED" and "QUIET"
# arguments used for the current package. For this to work, the first parameter
# must be the prefix of the current package, then the prefix of the new package
# etc, which are passed to find_package.
macro(libfind_package PREFIX)
    set(LIBFIND_PACKAGE_ARGS ${ARGN})

    if(${PREFIX}_FIND_QUIETLY)
        set(LIBFIND_PACKAGE_ARGS ${LIBFIND_PACKAGE_ARGS} QUIET)
    endif()

    if(${PREFIX}_FIND_REQUIRED)
        set(LIBFIND_PACKAGE_ARGS ${LIBFIND_PACKAGE_ARGS} REQUIRED)
    endif()

    find_package(${LIBFIND_PACKAGE_ARGS})
endmacro()

# CMake developers made the UsePkgConfig system deprecated in the same
# release (2.6) where they added pkg_check_modules. Consequently I need
# to support both in my scripts to avoid those deprecated warnings.
# Here's a helper that does just that. Works identically to pkg_check_modules,
# except that no checks are needed prior to use.
macro(libfind_pkg_check_modules PREFIX PKGNAME)
    if(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
        include(UsePkgConfig)
        pkgconfig(${PKGNAME}
                  ${PREFIX}_INCLUDE_DIRS
                  ${PREFIX}_LIBRARY_DIRS
                  ${PREFIX}_LDFLAGS
                  ${PREFIX}_CFLAGS)
    else()
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(${PREFIX} QUIET ${PKGNAME})
        endif()
    endif()
endmacro()

# Do the final processing once the paths have been detected.
# If include dirs are needed, ${PREFIX}_PROCESS_INCLUDES should be
# set to contain all the variables, each of which contain one include directory.
# Ditto for ${PREFIX}_PROCESS_LIBS and library files.
# Will set ${PREFIX}_FOUND, ${PREFIX}_INCLUDE_DIRS and ${PREFIX}_LIBRARIES.
# Also handles errors in case library detection was required, etc.
macro(libfind_process PREFIX)
    # Skip processing if already processed during this run
    if(NOT ${PREFIX}_FOUND)
        # Start with the assumption that the library was found
        set(${PREFIX}_FOUND TRUE)

        # Process all includes and set _FOUND to false if any are missing
        foreach(i ${${PREFIX}_PROCESS_INCLUDES})
            if(${i})
                set(${PREFIX}_INCLUDE_DIRS ${${PREFIX}_INCLUDE_DIRS} ${${i}})
                mark_as_advanced(${i})
            else()
                set(${PREFIX}_FOUND FALSE)
            endif()
        endforeach()

        # Process all libraries and set _FOUND to false if any are missing
        foreach(i ${${PREFIX}_PROCESS_LIBS})
            if(${i})
                set(${PREFIX}_LIBRARIES ${${PREFIX}_LIBRARIES} ${${i}})
                mark_as_advanced(${i})
            else()
                set(${PREFIX}_FOUND FALSE)
            endif()
        endforeach()

        # Print message and/or exit on fatal error
        if(${PREFIX}_FOUND)
            if(NOT ${PREFIX}_FIND_QUIETLY)
                message(STATUS "Found ${PREFIX} ${${PREFIX}_VERSION}")
            endif()
        else()
            if(${PREFIX}_FIND_REQUIRED)
                foreach(i ${${PREFIX}_PROCESS_INCLUDES} ${${PREFIX}_PROCESS_LIBS})
                    message("${i}=${${i}}")
                endforeach()

                message(FATAL_ERROR
                        "Required library ${PREFIX} NOT FOUND.\n"
                        "Install the library (dev version) and try again.\n"
                        "If the library is already installed, use ccmake to set "
                        "the missing variables manually.")
            endif()
        endif()
    endif()
endmacro()

macro(libfind_library PREFIX basename)
    set(TMP "")
    if(MSVC80)
        set(TMP -vc80)
    endif()
    if(MSVC90)
        set(TMP -vc90)
    endif()
    set(${PREFIX}_LIBNAMES ${basename}${TMP})
    if(${ARGC} GREATER 2)
        set(${PREFIX}_LIBNAMES ${basename}${TMP}-${ARGV2})
        string(REGEX REPLACE "\\." "_" TMP ${${PREFIX}_LIBNAMES})
        set(${PREFIX}_LIBNAMES ${${PREFIX}_LIBNAMES} ${TMP})
    endif()
    find_library(${PREFIX}_LIBRARY
                 NAMES ${${PREFIX}_LIBNAMES}
                 PATHS ${${PREFIX}_PKGCONF_LIBRARY_DIRS})
endmacro()
