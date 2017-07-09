# Logging Level:
# LOGALL(0), TRACE(1), DEBUG(2), STATE(3), ALERT(4), ERROR(5), FATAL(6) or LOGOFF(7)
#
# TRACE(1) => qInfo()       QT_NO_INFO_OUTPUT     informational messages
# DEBUG(2) => qDebug()      QT_NO_DEBUG_OUTPUT    custom debug message
# STATE(3) => qDebug()      QT_NO_DEBUG_OUTPUT    custom debug message
# ALERT(4) => qWarning()    QT_NO_WARNING_OUTPUT  writing warnings and recoverable errors messages
# ERROR(5) => qCritical()                         writing critical/system errors messages
# FATAL(6) => qFatal()                            writing fatal error messages before exiting
#
if(NOT SNAIL_LOG_LEVEL_MIN)
    if(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(SNAIL_LOG_LEVEL_MIN  7) # LOGOFF(7)
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(SNAIL_LOG_LEVEL_MIN  4) # ALERT(4)
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(SNAIL_LOG_LEVEL_MIN  2) # DEBUG(2)
        set(PACKAGE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "Debug build, not ready to release.")
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "Dev")
        set(SNAIL_LOG_LEVEL_MIN  1) # TRACE(1)
        set(PACKAGE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "Dev build, not ready to release.")
        endif()
    else()
        set(SNAIL_LOG_LEVEL_MIN  3) # STATE(3)
        set(PACKAGE_RELEASE_READY false)
        if(NOT release_check_msg)
            set(release_check_msg "${CMAKE_BUILD_TYPE} build, not ready to release.")
        endif()
    endif()
endif()

set(SNAIL_TRACE_ENABLE false) # disable snail trace logging by default
set(SNAIL_DEBUG_ENABLE false) # disable snail debug logging by default
set(SNAIL_STATE_ENABLE false) # disable snail state logging by default
set(SNAIL_ALERT_ENABLE true)  # enable  snail alert logging by default
set(SNAIL_ERROR_ENABLE true)  # enable  snail error logging by default
set(SNAIL_FATAL_ENABLE true)  # enable  snail fatal logging by default

# Checking logging level
if(SNAIL_LOG_LEVEL_MIN MATCHES "^[0-7]$")
    if(SNAIL_LOG_LEVEL_MIN EQUAL 0)
        set(log_level_msg "LOGALL(0), enable loggings")
        set(SNAIL_TRACE_ENABLE true)
        set(SNAIL_DEBUG_ENABLE true)
        set(SNAIL_STATE_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 1)
        set(log_level_msg "TRACE(1)")
        set(SNAIL_TRACE_ENABLE true)
        set(SNAIL_DEBUG_ENABLE true)
        set(SNAIL_STATE_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 2)
        set(log_level_msg "DEBUG(2)")
        set(SNAIL_DEBUG_ENABLE true)
        set(SNAIL_STATE_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 3)
        set(log_level_msg "STATE(3)")
        set(SNAIL_STATE_ENABLE true)
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 4)
        set(log_level_msg "ALERT(4)")
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 5)
        set(log_level_msg "ERROR(5)")
    elseif(SNAIL_LOG_LEVEL_MIN EQUAL 6)
        set(log_level_msg "FATAL(6)")
    else()
        set(log_level_msg "LOGOFF(7), disable loggings")
        set(SNAIL_ALERT_ENABLE false)
        set(SNAIL_ERROR_ENABLE false)
        set(SNAIL_FATAL_ENABLE false)
        set(DISABLE_SNAIL_LOGGING true)
    endif()
    message(STATUS "snail logging level set to: ${log_level_msg}")
else()
    set(SNAIL_LOG_LEVEL_MIN 3) # illegal, reset to STATE(3)
    set(SNAIL_STATE_ENABLE true)
    message(STATUS "snail logging level reset to: STATE(3)")
endif()

if(DISABLE_SNAIL_LOGGING)
    add_definitions("-DQT_NO_MESSAGELOGCONTEXT")
else()
    add_definitions("-DQT_MESSAGELOGCONTEXT")
endif()

if(SNAIL_TRACE_ENABLE)
    add_definitions("-DTRACE_LOG_ENABLE")
    message(STATUS "snail *enable TRACE logging")
else()
    add_definitions("-DQT_NO_INFO_OUTPUT")
    message(STATUS "snail disable TRACE logging")
endif()

if(SNAIL_DEBUG_ENABLE)
    add_definitions("-DDEBUG_LOG_ENABLE")
    message(STATUS "snail *enable DEBUG logging")
else()
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
    message(STATUS "snail disable DEBUG logging")
endif()

if(SNAIL_STATE_ENABLE)
    add_definitions("-DSTATE_LOG_ENABLE")
    message(STATUS "snail *enable STATE logging")
else()
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
    message(STATUS "snail disable STATE logging")
endif()

if(SNAIL_ALERT_ENABLE)
    add_definitions("-DALERT_LOG_ENABLE")
    message(STATUS "snail *enable ALERT logging")
else()
    add_definitions("-DQT_NO_WARNING_OUTPUT")
    message(STATUS "snail disable ALERT logging")
endif()

if(SNAIL_ERROR_ENABLE)
    add_definitions("-DERROR_LOG_ENABLE")
    message(STATUS "snail *enable ERROR logging")
else()
    message(STATUS "snail disable ERROR logging")
endif()

if(SNAIL_FATAL_ENABLE)
    add_definitions("-DFATAL_LOG_ENABLE")
    message(STATUS "snail *enable FATAL logging")
else()
    message(STATUS "snail disable FATAL logging")
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

add_compile_options("-std=c++11")

# for config, hard code into source
set(SNAIL_CONFIG_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
set(SNAIL_CONFIG_CXX_FLAGS_${build_type} ${CMAKE_CXX_FLAGS_${build_type}} PARENT_SCOPE)

add_subdirectory(app)
add_subdirectory(libs)
add_subdirectory(shared)
add_subdirectory(plugins)
add_subdirectory(utilities)

if(SNAIL_TESTING_ENABLE)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test/snail
                     ${PROJECT_BINARY_DIR}/test/snail)
endif()
