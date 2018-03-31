# Logging Level:
# TRACE(0), DEBUG(1), STATE(2), ALERT(3), ERROR(4), FATAL(5), LOGOFF(6)
#
# TRACE(0) => qInfo()
#   QT_NO_INFO_OUTPUT     informational messages
# DEBUG(1) => qDebug()
#   QT_NO_DEBUG_OUTPUT    custom debug message
# STATE(2) => qDebug()
#   QT_NO_DEBUG_OUTPUT    custom debug message
# ALERT(3) => qWarning()
#   QT_NO_WARNING_OUTPUT  writing warnings and recoverable errors messages
# ERROR(4) => qCritical()
#                         writing critical/system errors messages
# FATAL(5) => qFatal()
#                         writing fatal error messages before exiting
#
if(NOT SNAIL_LOG_LEVEL_MIN)
    if(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(SNAIL_LOG_LEVEL_MIN  6) # LOGOFF(6)
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(SNAIL_LOG_LEVEL_MIN  3) # ALERT(3)
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(SNAIL_LOG_LEVEL_MIN  1) # DEBUG(1)
        set(GKIDE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "Debug build, not ready to release.")
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "Dev")
        set(SNAIL_LOG_LEVEL_MIN  0) # TRACE(0)
        set(GKIDE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "Dev build, not ready to release.")
        endif()
    else()
        set(SNAIL_LOG_LEVEL_MIN  2) # STATE(2)
        set(GKIDE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg
                "${CMAKE_BUILD_TYPE} build, not ready to release.")
        endif()
    endif()
endif()

set(SNAIL_TRACE_ENABLE false) # disable snail trace logging by default
set(SNAIL_DEBUG_ENABLE false) # disable snail debug logging by default
set(SNAIL_STATE_ENABLE false) # disable snail state logging by default
set(SNAIL_ALERT_ENABLE false) # disable snail alert logging by default
set(SNAIL_ERROR_ENABLE false) # disable snail error logging by default
set(SNAIL_FATAL_ENABLE false) # disable snail fatal logging by default

# Checking logging level
if(SNAIL_LOG_LEVEL_MIN MATCHES "^[0-6]$")
    if(SNAIL_LOG_LEVEL_MIN EQUAL 0)
        set(log_level_msg "TRACE(0)")
        set(SNAIL_TRACE_ENABLE true)
        set(SNAIL_DEBUG_ENABLE true)
        set(SNAIL_STATE_ENABLE true)
        set(SNAIL_ALERT_ENABLE true)
        set(SNAIL_ERROR_ENABLE true)
        set(SNAIL_FATAL_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 1)
        set(log_level_msg "DEBUG(1)")
        set(SNAIL_DEBUG_ENABLE true)
        set(SNAIL_STATE_ENABLE true)
        set(SNAIL_ALERT_ENABLE true)
        set(SNAIL_ERROR_ENABLE true)
        set(SNAIL_FATAL_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 2)
        set(log_level_msg "STATE(2)")
        set(SNAIL_STATE_ENABLE true)
        set(SNAIL_ALERT_ENABLE true)
        set(SNAIL_ERROR_ENABLE true)
        set(SNAIL_FATAL_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 3)
        set(log_level_msg "ALERT(3)")
        set(SNAIL_ALERT_ENABLE true)
        set(SNAIL_ERROR_ENABLE true)
        set(SNAIL_FATAL_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 4)
        set(log_level_msg "ERROR(4)")
        set(SNAIL_ERROR_ENABLE true)
        set(SNAIL_FATAL_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 5)
        set(log_level_msg "FATAL(5)")
        set(SNAIL_FATAL_ENABLE true)
    else()
        set(log_level_msg "LOGOFF(6)")
        set(SNAIL_LOGGING_DISABLE true)
    endif()
    message(STATUS "snail logging level set to: ${log_level_msg}")
else()
    set(SNAIL_LOG_LEVEL_MIN 2) # illegal, reset to STATE(2)
    set(SNAIL_STATE_ENABLE true)
    set(SNAIL_ALERT_ENABLE true)
    set(SNAIL_ERROR_ENABLE true)
    set(SNAIL_FATAL_ENABLE true)
    message(STATUS "snail logging level reset to: STATE(2)")
endif()

if(SNAIL_LOGGING_DISABLE)
    add_definitions("-DSNAIL_LOGGING_DISABLE")
    add_definitions("-DQT_NO_MESSAGELOGCONTEXT")
else()
    add_definitions("-DQT_MESSAGELOGCONTEXT")
endif()

if(SNAIL_TRACE_ENABLE)
    message(STATUS "snail *enable TRACE(0) logging")
else()
    add_definitions("-DQT_NO_INFO_OUTPUT")
    message(STATUS "snail disable TRACE(0) logging")
endif()

if(SNAIL_DEBUG_ENABLE)
    add_definitions("-DDEBUG_LOG_ENABLE")
    message(STATUS "snail *enable DEBUG(1) logging")
else()
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
    message(STATUS "snail disable DEBUG(1) logging")
endif()

if(SNAIL_STATE_ENABLE)
    add_definitions("-DSTATE_LOG_ENABLE")
    message(STATUS "snail *enable STATE(2) logging")
else()
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
    message(STATUS "snail disable STATE(2) logging")
endif()

if(SNAIL_ALERT_ENABLE)
    add_definitions("-DALERT_LOG_ENABLE")
    message(STATUS "snail *enable ALERT(3) logging")
else()
    add_definitions("-DQT_NO_WARNING_OUTPUT")
    message(STATUS "snail disable ALERT(3) logging")
endif()

if(SNAIL_ERROR_ENABLE)
    add_definitions("-DERROR_LOG_ENABLE")
    message(STATUS "snail *enable ERROR(4) logging")
else()
    message(STATUS "snail disable ERROR(4) logging")
endif()

if(SNAIL_FATAL_ENABLE)
    add_definitions("-DFATAL_LOG_ENABLE")
    message(STATUS "snail *enable FATAL(5) logging")
else()
    message(STATUS "snail disable FATAL(5) logging")
endif()

# Checking Qt assertion
if(NOT ASSERTION_ENABLE)
    # Q_ASSERT(cond)                 => QT_NO_DEBUG
    # Q_ASSERT_X(cond, where, what)  => QT_NO_DEBUG
    # Q_CHECK_PTR(ptr)               => QT_NO_DEBUG & QT_NO_EXCEPTIONS
    add_definitions("-DQT_NO_DEBUG")
    add_definitions("-DQT_NO_EXCEPTIONS")
endif()

# Build flags for Dev build type
if(CMAKE_COMPILER_IS_GNUCXX)
    check_cxx_compiler_flag(-Og CX_HAS_OG_FLAG)
else()
    set(CX_HAS_OG_FLAG false)
endif()

if(CX_HAS_OG_FLAG)
    set(CMAKE_CXX_FLAGS_DEV
        "-Og -g" CACHE STRING "CXXFlags used for development build." FORCE)
else()
    set(CMAKE_CXX_FLAGS_DEV
        "-O2 -g" CACHE STRING "CXXFlags used for development build." FORCE)
endif()

if(SNAIL_USE_STATIC_QT5)
    add_definitions("-DQT_STATICPLUGIN")
endif()

add_compile_options("-std=c++11")

# for config, hard code into source
set(SNAIL_CONFIG_CXX_FLAGS
    ${CMAKE_CXX_FLAGS})
set(SNAIL_CONFIG_CXX_FLAGS_${build_type}
    ${CMAKE_CXX_FLAGS_${build_type}})

file(GLOB SNAIL_HEADERS *.h)
file(GLOB SNAIL_SOURCES *.cpp)

list(APPEND subdir_set "ssh")

foreach(subdir ${subdir_set})
    file(GLOB sources ${subdir}/*.cpp)
    file(GLOB headers ${subdir}/*.h)

    list(APPEND SNAIL_SOURCES ${sources})
    list(APPEND SNAIL_HEADERS ${headers})
endforeach()

list(APPEND NVIM_API_SRC
     ${CMAKE_SOURCE_DIR}/source/config/nvimapi/auto/nvim.cpp)

file(GLOB SNAIL_QT_RESC *.qrc)

# libssh
find_package(Libssh 0.7.5 REQUIRED)
list(APPEND SNAIL_LINK_LIBS "${LIBSSH_LIBRARIES}")

if(HOST_OS_WINDOWS)
    # About MSYS2 build static programs
    # when use MSYS2 build windows C/C++ programs, by default, at least need:
    # libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll
    # if do not want these dlls, give '-static' to gcc/g++ is ok
    # please note that if there have some other linking libraries use dynamic link,
    # which relay on these dlls, the '-static' flag do not have affect.
    add_definitions("-DLIBSSH_STATIC") # no '__declspec(dllimport)'
    list(APPEND SNAIL_LINK_LIBS "-Wl,-Bstatic -lz")
    list(APPEND SNAIL_LINK_LIBS "-Wl,-Bstatic -lcrypto")
else()
    list(APPEND SNAIL_LINK_LIBS "-lcrypto -lz")
    include_directories(SYSTEM ${LIBSSH_INCLUDE_DIRS})
endif()

# msgpack
list(APPEND SNAIL_LINK_LIBS "${MSGPACK_LIBRARIES}")

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Gui     REQUIRED)
find_package(Qt5Core    REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

list(APPEND SNAIL_LINK_LIBS "Qt5::Gui")
list(APPEND SNAIL_LINK_LIBS "Qt5::Core")
list(APPEND SNAIL_LINK_LIBS "Qt5::Widgets")
list(APPEND SNAIL_LINK_LIBS "Qt5::Network")

if(SNAIL_USE_STATIC_QT5)
    # create qt-plugin file for static link Qt
    set(qt_plugin_moc_cpp "${CMAKE_CURRENT_BINARY_DIR}/qt_plugin_moc.cpp")
    file(WRITE ${qt_plugin_moc_cpp} "#include <QtPlugin>\n")

    if(HOST_OS_WINDOWS)
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)\n")
    else()
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QGifPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QICNSPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QICOPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QJpegPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QTgaPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QTiffPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWbmpPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWebpPlugin)\n")

        include(FindStaticQtLib)

        list(APPEND SNAIL_LINK_LIBS ${CMAKE_THREAD_LIBS_INIT})
    endif()

    list(APPEND SNAIL_SOURCES ${qt_plugin_moc_cpp})
endif()

if(HOST_OS_WINDOWS)
    include_directories(${PROJECT_BINARY_DIR})
    list(APPEND SNAIL_SOURCES snail.rc)
endif()

if(HOST_OS_MACOS)
    list(APPEND SNAIL_SOURCES info.plist)
endif()

list(APPEND SNAIL_SOURCES ${NVIM_API_SRC})
list(APPEND SNAIL_SOURCES ${SNAIL_QT_RESC})

add_executable(snail
               WIN32
               MACOSX_BUNDLE
               ${SNAIL_HEADERS} ${SNAIL_SOURCES})

install_helper(TARGETS snail)

# verbose show target linkge libraries: --verbose
target_link_libraries(snail ${SNAIL_LINK_LIBS})

add_subdirectory(config)

if($ENV{UPDATE_NVIMAPI_BINDINGS})
    add_dependencies(snail update-nvimapi-bindings)
endif()

if(SNAIL_TESTING_ENABLE)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test/snail
                     ${PROJECT_BINARY_DIR}/test/snail)
endif()
