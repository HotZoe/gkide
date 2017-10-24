# Logging Level:
# TRACE(0), DEBUG(1), STATE(2), ALERT(3), ERROR(4), FATAL(5), LOGOFF(6)
#
# TRACE(0) => qInfo()       QT_NO_INFO_OUTPUT     informational messages
# DEBUG(1) => qDebug()      QT_NO_DEBUG_OUTPUT    custom debug message
# STATE(2) => qDebug()      QT_NO_DEBUG_OUTPUT    custom debug message
# ALERT(3) => qWarning()    QT_NO_WARNING_OUTPUT  writing warnings and recoverable errors messages
# ERROR(4) => qCritical()                         writing critical/system errors messages
# FATAL(5) => qFatal()                            writing fatal error messages before exiting
#
if(NOT SNAIL_LOG_LEVEL_MIN)
    if(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(SNAIL_LOG_LEVEL_MIN  6) # LOGOFF(6)
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(SNAIL_LOG_LEVEL_MIN  3) # ALERT(3)
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(SNAIL_LOG_LEVEL_MIN  1) # DEBUG(1)
        set(PACKAGE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "Debug build, not ready to release.")
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "Dev")
        set(SNAIL_LOG_LEVEL_MIN  0) # TRACE(0)
        set(PACKAGE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "Dev build, not ready to release.")
        endif()
    else()
        set(SNAIL_LOG_LEVEL_MIN  2) # STATE(2)
        set(PACKAGE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "${CMAKE_BUILD_TYPE} build, not ready to release.")
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
    add_definitions("-DTRACE_LOG_ENABLE")
    message(STATUS "snail *enable TRACE(1) logging")
else()
    add_definitions("-DQT_NO_INFO_OUTPUT")
    message(STATUS "snail disable TRACE(1) logging")
endif()

if(SNAIL_DEBUG_ENABLE)
    add_definitions("-DDEBUG_LOG_ENABLE")
    message(STATUS "snail *enable DEBUG(2) logging")
else()
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
    message(STATUS "snail disable DEBUG(2) logging")
endif()

if(SNAIL_STATE_ENABLE)
    add_definitions("-DSTATE_LOG_ENABLE")
    message(STATUS "snail *enable STATE(3) logging")
else()
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
    message(STATUS "snail disable STATE(3) logging")
endif()

if(SNAIL_ALERT_ENABLE)
    add_definitions("-DALERT_LOG_ENABLE")
    message(STATUS "snail *enable ALERT(4) logging")
else()
    add_definitions("-DQT_NO_WARNING_OUTPUT")
    message(STATUS "snail disable ALERT(4) logging")
endif()

if(SNAIL_ERROR_ENABLE)
    add_definitions("-DERROR_LOG_ENABLE")
    message(STATUS "snail *enable ERROR(5) logging")
else()
    message(STATUS "snail disable ERROR(5) logging")
endif()

if(SNAIL_FATAL_ENABLE)
    add_definitions("-DFATAL_LOG_ENABLE")
    message(STATUS "snail *enable FATAL(6) logging")
else()
    message(STATUS "snail disable FATAL(6) logging")
endif()

# Checking Qt assertion
if(NOT ASSERTION_ENABLE)
    # Q_ASSERT(cond)                 => QT_NO_DEBUG
    # Q_ASSERT_X(cond, where, what)  => QT_NO_DEBUG
    # Q_CHECK_PTR(ptr)               => QT_NO_DEBUG & QT_NO_EXCEPTIONS
    add_definitions("-DQT_NO_DEBUG")
    add_definitions("-DQT_NO_EXCEPTIONS")
endif()

include(ConfigSnail) # generate configurations for snail only

# Build flags for Dev build type
if(CMAKE_COMPILER_IS_GNUCXX)
    check_cxx_compiler_flag(-Og GNUCXX_HAS_OG_FLAG)
else()
    set(GNUCXX_HAS_OG_FLAG false)
endif()

if(GNUCXX_HAS_OG_FLAG)
    set(CMAKE_CXX_FLAGS_DEV "-Og -g" CACHE STRING "CXXFlags used for development build." FORCE)
else()
    set(CMAKE_CXX_FLAGS_DEV "-O2 -g" CACHE STRING "CXXFlags used for development build." FORCE)
endif()

if(SNAIL_USE_STATIC_QT5)
    add_definitions("-DQT_STATICPLUGIN")
endif()

add_compile_options("-std=c++11")

# for config, hard code into source
set(SNAIL_CONFIG_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
set(SNAIL_CONFIG_CXX_FLAGS_${build_type} ${CMAKE_CXX_FLAGS_${build_type}} PARENT_SCOPE)

add_subdirectory(app)
add_subdirectory(libs)

if(SNAIL_TESTING_ENABLE)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test/snail
                     ${PROJECT_BINARY_DIR}/test/snail)
endif()
