# search for static Qt5 deps libraries
#
# Link sequence is much more important

# static build Qt5 bundled library: plugins/platforms/libqxcb.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qxcb${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QXCB
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/platforms
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)

if(EXISTS ${QT5_LIB_QXCB})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QXCB})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5XcbQpa.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5XcbQpa${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5XCBQPA
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)

if(EXISTS ${QT5_LIB_QT5XCBQPA})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5XCBQPA})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5ServiceSupport.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5ServiceSupport${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5SERVICESUPPORT
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5SERVICESUPPORT})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5SERVICESUPPORT})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5ThemeSupport.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5ThemeSupport${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5THEMESUPPORT
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5THEMESUPPORT})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5THEMESUPPORT})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5DBus.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5DBus${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5DBUS
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5DBUS})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5DBUS})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5EventDispatcherSupport.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5EventDispatcherSupport${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5EVENTDISPATCHERSUPPORT
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5EVENTDISPATCHERSUPPORT})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5EVENTDISPATCHERSUPPORT})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5FontDatabaseSupport.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5FontDatabaseSupport${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5FONTDATABASESUPPORT
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5FONTDATABASESUPPORT})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5FONTDATABASESUPPORT})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# see for details:
# https://wiki.ubuntu.com/MultiarchSpec
# https://err.no/debian/amd64-multiarch-3
if(HOST_OS_ARCH_64)
    if(TARGET_ARCH_32)
        list(APPEND multiarch_seeks_dirs "/lib32")
        list(APPEND multiarch_seeks_dirs "/usr/lib32")
        list(APPEND multiarch_seeks_dirs "/lib/i386-linux-gnu")
        list(APPEND multiarch_seeks_dirs "/usr/lib/i386-linux-gnu")
    else()
        list(APPEND multiarch_seeks_dirs "/lib64")
        list(APPEND multiarch_seeks_dirs "/usr/lib64")
        list(APPEND multiarch_seeks_dirs "/lib/x86_64-linux-gnu")
        list(APPEND multiarch_seeks_dirs "/usr/lib/x86_64-linux-gnu")
    endif()
endif()

# static build Qt5 system library: libfontconfig.a, libfontconfig.so, fontconfig
set(sys_lib_names
    "fontconfig") # static libray come first
list(INSERT sys_lib_names
     0 ${CMAKE_SHARED_LIBRARY_PREFIX}fontconfig${CMAKE_SHARED_LIBRARY_SUFFIX})
list(INSERT sys_lib_names
     0 ${CMAKE_STATIC_LIBRARY_PREFIX}fontconfig${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(SYS_LIB_FONTCONFIG
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_FONTCONFIG})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_FONTCONFIG})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 system library: libexpat.a, libexpat.so, expat
set(sys_lib_names "expat") # static libray come first
list(INSERT sys_lib_names
     0 ${CMAKE_SHARED_LIBRARY_PREFIX}expat${CMAKE_SHARED_LIBRARY_SUFFIX})
list(INSERT sys_lib_names
     0 ${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(SYS_LIB_EXPAT
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_EXPAT})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_EXPAT})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 system library: libfreetype.a, libfreetype.so, freetype
set(sys_lib_names "freetype") # static libray come first
list(INSERT sys_lib_names
     0 ${CMAKE_SHARED_LIBRARY_PREFIX}freetype${CMAKE_SHARED_LIBRARY_SUFFIX})
list(INSERT sys_lib_names
     0 ${CMAKE_STATIC_LIBRARY_PREFIX}freetype${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(SYS_LIB_FREETYPE
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_FREETYPE})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_FREETYPE})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# The X.Org project provides an open source implementation of the X Window System.
# X Window System is widely used in linux system, so just link to the dynamic libraries.
#
# FindX11.cmake: Find X11 installation
#
# X11_FOUND        - True if X11 is available
# X11_INCLUDE_DIR  - include directories to use X11
# X11_LIBRARIES    - link against these to use X11
find_package(X11 REQUIRED)
if(NOT X11_FOUND)
    # libX11.so: library for the X Window System
    message(FATAL_ERROR "Do not find 'libX11.so' library for snail static build.")
endif()
if(NOT X11_Xi_FOUND)
    # libXi.so: library for the X Input Extension
    message(FATAL_ERROR "Do not find 'libXi.so' library for snail static build.")
endif()
# libXau.so: A Sample Authorization Protocol for X
list(APPEND SNAIL_EXEC_LINK_LIBS ${X11_X11_LIB})
list(APPEND SNAIL_EXEC_LINK_LIBS ${X11_Xi_LIB})

# Xlib/XCB interface library
#
# provides functions needed by clients which take advantage of
# Xlib/XCB to mix calls to both Xlib and XCB over the same X connection.
#
# static build Qt5 system library: libX11-xcb.a, libX11-xcb.so, X11-xcb
set(sys_lib_names "X11-xcb") # static libray come first
list(INSERT sys_lib_names
     0 ${CMAKE_SHARED_LIBRARY_PREFIX}X11-xcb${CMAKE_SHARED_LIBRARY_SUFFIX})
list(INSERT sys_lib_names
     0 ${CMAKE_STATIC_LIBRARY_PREFIX}X11-xcb${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(SYS_LIB_X11_XCB
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_X11_XCB})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_X11_XCB})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libxcb-static.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}xcb-static${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_XCB_STATIC
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_XCB_STATIC})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_XCB_STATIC})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# The X protocol C-language Binding (XCB) is a replacement for Xlib featuring
# a small footprint, latency hiding, direct access to the protocol, improved
# threading support, and extensibility. On Linux, the xcb QPA
# (Qt Platform Abstraction) platform plugin is used.
#
# static build Qt5 system library: libxcb.so, libxcb.a
set(sys_lib_names "xcb") # dynamic libray come first
find_library(SYS_LIB_XCB
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_XCB})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_XCB})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 bundled library: plugins/imageformats/libqgif.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qgif${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QGIF
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QGIF})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QGIF})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: plugins/imageformats/libqicns.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qicns${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QICNS
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QICNS})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QICNS})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: plugins/imageformats/libqico.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qico${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QICO
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QICO})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QICO})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()


# static build Qt5 bundled library: plugins/imageformats/libqjpeg.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qjpeg${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QJPEG
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QJPEG})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QJPEG})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: plugins/imageformats/libqtga.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qtga${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QTGA
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QTGA})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QTGA})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: plugins/imageformats/libqtiff.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qtiff${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QTIFF
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QTIFF})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QTIFF})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: plugins/imageformats/libqwbmp.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qwbmp${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QWBMP
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QWBMP})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QWBMP})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: plugins/imageformats/libqwebp.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qwebp${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QWEBP
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/plugins/imageformats
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QWEBP})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QWEBP})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5Widgets.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5Widgets${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5WIDGETS
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5WIDGETS})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5WIDGETS})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5Gui.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5Gui${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5GUI
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5GUI})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5GUI})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 system library: libpng12.a, libpng.a, libpng12.so, libpng.so
set(sys_lib_names "png") # static libray come first
list(INSERT sys_lib_names 0 "png12")

list(INSERT sys_lib_names 0
     ${CMAKE_SHARED_LIBRARY_PREFIX}png${CMAKE_SHARED_LIBRARY_SUFFIX})
list(INSERT sys_lib_names 0
     ${CMAKE_SHARED_LIBRARY_PREFIX}png12${CMAKE_SHARED_LIBRARY_SUFFIX})
list(INSERT sys_lib_names 0
     ${CMAKE_STATIC_LIBRARY_PREFIX}png${CMAKE_STATIC_LIBRARY_SUFFIX})
list(INSERT sys_lib_names 0
     ${CMAKE_STATIC_LIBRARY_PREFIX}png12${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(SYS_LIB_PNG
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_PNG})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_PNG})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libqtharfbuzz.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qtharfbuzz${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QTHARFBUZZ
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QTHARFBUZZ})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QTHARFBUZZ})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5Core.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5Core${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5CORE
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5CORE})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5CORE})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libQt5Core.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}Qt5Core${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QT5CORE
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QT5CORE})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QT5CORE})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# ICU - International Components for Unicode
# if use the static ICU, the output will get bigger nearly double size.
#
# static build Qt5 system library: libicui18n.a, libicui18n.so
set(sys_lib_names "icui18n") # dynamic libray come first
list(INSERT sys_lib_names 0
     ${CMAKE_SHARED_LIBRARY_PREFIX}icui18n${CMAKE_SHARED_LIBRARY_SUFFIX})
if(SNAIL_USE_STATIC_LIB_ICU)
    list(INSERT sys_lib_names 0
         ${CMAKE_STATIC_LIBRARY_PREFIX}icui18n${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
find_library(SYS_LIB_ICUI18N
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_ICUI18N})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_ICUI18N})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 system library: libicuuc.a, libicuuc.so
set(sys_lib_names "icuuc") # dynamic libray come first
list(INSERT sys_lib_names 0
     ${CMAKE_SHARED_LIBRARY_PREFIX}icuuc${CMAKE_SHARED_LIBRARY_SUFFIX})
if(SNAIL_USE_STATIC_LIB_ICU)
    list(INSERT sys_lib_names 0
         ${CMAKE_STATIC_LIBRARY_PREFIX}icuuc${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
find_library(SYS_LIB_ICUUC
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_ICUUC})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_ICUUC})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 system library: libicudata.a, libicudata.so
set(sys_lib_names "icudata") # dynamic libray come first
list(INSERT sys_lib_names 0
     ${CMAKE_SHARED_LIBRARY_PREFIX}icudata${CMAKE_SHARED_LIBRARY_SUFFIX})
if(SNAIL_USE_STATIC_LIB_ICU)
    list(INSERT sys_lib_names 0
         ${CMAKE_STATIC_LIBRARY_PREFIX}icudata${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
find_library(SYS_LIB_ICUDATA
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_ICUDATA})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_ICUDATA})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 system library: libm.so, libm.a
set(sys_lib_names "m") # shared libray come first
find_library(SYS_LIB_M
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_M})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_M})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 system library: libm.so, libm.a
set(sys_lib_names "z") # shared libray come first
find_library(SYS_LIB_Z
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_Z})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_Z})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 bundled library: lib/libqtpcre2.a
set(qt5_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}qtpcre2${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(QT5_LIB_QTPCRE2
             NAMES ${qt5_lib_name} NAMES_PER_DIR
             PATHS ${QT_LIB_PREFIX_DIR}/lib
             NO_CMAKE_PATH
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)
if(EXISTS ${QT5_LIB_QTPCRE2})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${QT5_LIB_QTPCRE2})
else()
    message(FATAL_ERROR
            "Do not find '${qt5_lib_name}' library for snail static build.")
endif()

# static build Qt5 system library: libm.so, libm.a
set(sys_lib_names "dl") # shared libray come first
find_library(SYS_LIB_DL
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_DL})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_DL})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# static build Qt5 system library: libgthread-2.0.so, libgthread-2.0.a
# Gthread: part of Glib
# Pthread: POSIX thread standard
set(sys_lib_names "gthread-2.0") # shared libray come first
find_library(SYS_LIB_GTHREAD20
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_GTHREAD20})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_GTHREAD20})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()

# The GLib provides the core application building blocks for libraries and
# applications written in C.
#
# It contains low-level libraries useful for providing data structure handling
# for C, portability wrappers and interfaces for such runtime functionality as
# an event loop, threads, dynamic loading and an object system.
set(sys_lib_names "glib-2.0") # shared libray come first
find_library(SYS_LIB_GLIB20
             NAMES ${sys_lib_names} NAMES_PER_DIR
             PATHS ${multiarch_seeks_dirs})
if(EXISTS ${SYS_LIB_GLIB20})
    list(APPEND SNAIL_EXEC_LINK_LIBS ${SYS_LIB_GLIB20})
else()
    message(FATAL_ERROR
            "Do not find '${sys_lib_names}' library for snail static build.")
endif()
