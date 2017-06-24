#
# ARGC  actual args number pass to function when called
# ARGV  actual args list when called(all)
# ARGV0 number 1 actual argument
# ARGV1 number 2 actual argument
# ARGV2 number 3
# ...
# ARGN  when call function with more arguments then want,
#       this is the list of the that part args
#
# (none)         = Important information
# STATUS         = Incidental information
# WARNING        = CMake Warning, continue processing
# AUTHOR_WARNING = CMake Warning (dev), continue processing
# SEND_ERROR     = CMake Error, continue processing,
#                  but skip generation
# FATAL_ERROR    = CMake Error, stop processing and generation
# DEPRECATION    = CMake Deprecation Error or Warning if variable
#                  CMAKE_ERROR_DEPRECATED or CMAKE_WARN_DEPRECATED
#                  is enabled, respectively, else no message.

function(get_host_name_user_name_linux _un _hn)
    find_program(WHOAMI_PROG   whoami)
    find_program(HOSTNAME_PROG hostname)

    # get user name
    if(EXISTS ${WHOAMI_PROG})
        execute_process(COMMAND ${WHOAMI_PROG} OUTPUT_STRIP_TRAILING_WHITESPACE
                        OUTPUT_VARIABLE        COMPILER_NAME)

        set(${_un} "${COMPILER_NAME}" PARENT_SCOPE)
    else()
        set(${_un} "anonymous" PARENT_SCOPE)
    endif()

    # get host name
    if(EXISTS ${HOSTNAME_PROG})
        execute_process(COMMAND ${HOSTNAME_PROG} OUTPUT_STRIP_TRAILING_WHITESPACE
                        OUTPUT_VARIABLE          HOST_NAME)

        set(${_hn} "${HOST_NAME}" PARENT_SCOPE)
    else()
        set(${_hn} "anonymous" PARENT_SCOPE)
    endif()
endfunction()

function(get_host_name_user_name_windows _un _hn)
    message(AUTHOR_WARNING "[windows] => get user/host name.")
endfunction()

function(get_host_name_user_name_macosx _un _hn)
    message(AUTHOR_WARNING "[macosx] => get user/host name.")
endfunction()

function(get_host_system_name_linux _hsn _rv)
    if(NOT _hsn STREQUAL auto)
        set(${_rv} ${_hsn} PARENT_SCOPE)
        return()
    endif()

    find_program(UNAME_PROG  uname)

    if(EXISTS ${UNAME_PROG})
        execute_process(COMMAND ${UNAME_PROG} --all
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        OUTPUT_VARIABLE  uname_all)

        if(uname_all MATCHES "Ubuntu")
            set(${_rv} "Ubuntu" PARENT_SCOPE)
        elseif(uname_all MATCHES "Debian")
            set(${_rv} "Debian" PARENT_SCOPE)
        else()
            message(AUTHOR_WARNING "[linux] => add support for new linux distribution.")
            set(${_rv} "Linux" PARENT_SCOPE)
        endif()
    else()
        message(WARNING "[linux] => current linux system has no 'uname' found.")
        set(${_rv} "Linux" PARENT_SCOPE)
    endif()
endfunction()

function(get_host_system_name_windows _hsn _rv)
    message(AUTHOR_WARNING "[windows] => set host category code")
endfunction()

function(get_host_system_name_macosx _hsn _rv)
    message(AUTHOR_WARNING "[macosx] => set host category code")
endfunction()

function(get_current_system_time_linux _sys_time)
    find_program(DATE_PROG date)

    if(EXISTS ${DATE_PROG})
        execute_process(COMMAND ${DATE_PROG} "+%Y-%m-%d\ %T"
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        OUTPUT_VARIABLE  _cur_date_time)

        set(${_sys_time} "${_cur_date_time}" PARENT_SCOPE)
    else()
        set(${_sys_time} "xxxx-xx-xx xx:xx:xx" PARENT_SCOPE)
    endif()
endfunction()

function(get_current_system_time_windows _sys_time)
    message(AUTHOR_WARNING "[windows] => get_current_system_time")
endfunction()

function(get_current_system_time_macosx _sys_time)
    message(AUTHOR_WARNING "[macosx] => get_current_system_time")
endfunction()

function(GetHostNameUserName _un _hn)
    if(HOST_OS_LINUX)
        get_host_name_user_name_linux(_user_name _host_name)
    elseif(HOST_OS_WINDOWS)
        get_host_name_user_name_windows(_user_name _host_name)
    elseif(HOST_OS_MACOS)
        get_host_name_user_name_macosx(_user_name _host_name)
    endif()

    set(${_un}  "${_user_name}"  PARENT_SCOPE)
    set(${_hn}  "${_host_name}"  PARENT_SCOPE)
endfunction()

function(GetHostSystemName _hsn)
    if(NOT _hsn)
        set(_hsn auto) # auto detected by cmake
    endif()

    if(HOST_OS_LINUX)
        get_host_system_name_linux(${_hsn} _rv)
    elseif(HOST_OS_WINDOWS)
        get_host_system_name_windows(${_hsn} _rv)
    elseif(HOST_OS_MACOS)
        get_host_system_name_macosx(${_hsn} _rv)
    endif()
    set(HOST_OS_NAME ${_rv} PARENT_SCOPE)
endfunction()

function(GetCurrentSystemTime cur_sys_time)

    if(HOST_OS_LINUX)
        get_current_system_time_linux(_sys_time)
    elseif(HOST_OS_WINDOWS)
        get_current_system_time_windows(_sys_time)
    elseif(HOST_OS_MACOS)
        get_current_system_time_macosx(_sys_time)
    endif()

    set(${cur_sys_time}  "${_sys_time}"  PARENT_SCOPE)

    # this is put here for each time running cmake, we can get
    # current system time.
    configure_file("${CMAKE_CURRENT_LIST_DIR}/PackageInfo.in"
                   "${CMAKE_BINARY_DIR}/PackageInfo"
                   @ONLY)
endfunction()
