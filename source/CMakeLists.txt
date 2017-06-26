option(JEMALLOC_ENABLE "Enable jemalloc library." ON)

# Qt5
if(NOT QT5_INSTALL_PREFIX AND NOT "$ENV{CMAKE_PREFIX_PATH}")
    message(FATAL_ERROR "Qt5 install perfix not set, see contrib/local.mk.eg.")
endif()

if(QT5_INSTALL_PREFIX)
    set(ENV{CMAKE_PREFIX_PATH} "${QT5_INSTALL_PREFIX}")
    message(STATUS "Qt5 Search Path   : ${QT5_INSTALL_PREFIX}")
else()
    message(STATUS "Qt5 Search Path   : $ENV{CMAKE_PREFIX_PATH}")
endif()

# Dependency libraries search path
set(BUNDLED_DEPS_PREFIX "${CMAKE_SOURCE_DIR}/deps/build/usr" CACHE PATH "Deps path search prefix.")
list(INSERT CMAKE_PREFIX_PATH 0 ${BUNDLED_DEPS_PREFIX})
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${BUNDLED_DEPS_PREFIX}/lib/pkgconfig")
message(STATUS "Deps Search Path  : ${BUNDLED_DEPS_PREFIX}")

# msgpack
find_package(Msgpack 1.0.0 REQUIRED)
include_directories(SYSTEM ${MSGPACK_INCLUDE_DIRS})
list(APPEND NVIM_DEP_LINK_LIBRARIES ${MSGPACK_LIBRARIES})
list(APPEND SNAIL_DEP_LINK_LIBRARIES ${MSGPACK_LIBRARIES})

# jemalloc
if(JEMALLOC_ENABLE)
    if(CYGWIN)
        set(FOUND_WORKING_JEMALLOC false)
        message(STATUS "Build in Windows(Cygwin), do not use jemalloc")
    else()
        find_package(JeMalloc REQUIRED)
        set(FOUND_WORKING_JEMALLOC true)
        include_directories(SYSTEM ${JEMALLOC_INCLUDE_DIRS})
        list(APPEND NVIM_DEP_LINK_LIBRARIES ${JEMALLOC_LIBRARIES})
        list(APPEND SNAIL_DEP_LINK_LIBRARIES ${JEMALLOC_LIBRARIES})
    endif()
else()
    set(FOUND_WORKING_JEMALLOC false)
    message(STATUS "Use standard libc")
endif()

# threads
# see 'FindThreads.cmake' for details
# This module determines the thread library of the system. The following variables are set:
#     CMAKE_THREAD_LIBS_INIT       - the thread library
#     CMAKE_USE_SPROC_INIT         - are we using sproc?
#     CMAKE_USE_WIN32_THREADS_INIT - using WIN32 threads?
#     CMAKE_USE_PTHREADS_INIT      - are we using pthreads
#     CMAKE_HP_PTHREADS_INIT       - are we using hp pthreads
find_package(Threads REQUIRED)
list(APPEND NVIM_DEP_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
list(APPEND SNAIL_DEP_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

# Lua program
include(CheckLuaProg)
set(LUA_MODULES lpeg mpack bit)
foreach(lua_prog lua lua5.2 lua5.1)
    unset(LUA_PROG CACHE)
    unset(LUA_PROG_WORKS)

    find_program(LUA_PROG ${lua_prog})

    if(LUA_PROG)
        check_lua_deps(${LUA_PROG} "${LUA_MODULES}" LUA_PROG_WORKS)
        if(LUA_PROG_WORKS)
            break()
        endif()
    endif()
endforeach()

if(NOT LUA_PROG_WORKS)
    message(FATAL_ERROR "A suitable Lua interpreter was not found.")
endif()

# gperf program
find_program(GPERF_PROG gperf)
if(GPERF_PROG MATCHES "NOTFOUND")
    message(FATAL_ERROR "gperf: NOT FOUND")
else()
    message(STATUS "gperf: ${GPERF_PROG}")
endif()

# Logging control: DEV(0), DEBUG(1), INFO(2), WARNING(3) or ERROR(4)
if(NOT LOG_LEVEL_MIN AND LOGGING_ENABLE)
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        set(LOG_LEVEL_MIN  2) # INFO(2)
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(LOG_LEVEL_MIN  1) # DEBUG(1)
        set(SNAIL_RELEASE_READY  false)
        if(NOT rce_msg)
            set(rce_msg "Debug build, not ready to release.")
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "Dev")
        set(LOG_LEVEL_MIN  0) # DEV(0)
        set(SNAIL_RELEASE_READY  false)
        if(NOT rce_msg)
            set(rce_msg "Dev build, not ready to release.")
        endif()
    else()
        set(LOG_LEVEL_MIN  1) # DEBUG(1)
        set(SNAIL_RELEASE_READY  false)
        if(NOT rce_msg)
            set(rce_msg "${CMAKE_BUILD_TYPE} build, not ready to release.")
        endif()
    endif()
endif()

# Checking logging level
if(LOGGING_ENABLE)
    if(LOG_LEVEL_MIN MATCHES "^[0-4]$")
        if(LOG_LEVEL_MIN EQUAL 0)
            set(log_level_msg "DEV(0)")
        elseif(LOG_LEVEL_MIN EQUAL 1)
            set(log_level_msg "DEBUG(1)")
        elseif(LOG_LEVEL_MIN EQUAL 2)
            set(log_level_msg "INFO(2)")
        elseif(LOG_LEVEL_MIN EQUAL 3)
            set(log_level_msg "WARNING(3)")
        else()
            set(log_level_msg "ERROR(4)")
        endif()
    else()
        set(LOG_LEVEL_MIN 2) # illegal value set, reset
        set(log_level_msg "INFO(2)")
    endif()

    if(CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel" )
        set(LOGGING_ENABLE OFF)
        message(STATUS "Disable logging")
    else()
        message(STATUS "Enable logging: ${log_level_msg}")
    endif()
else()
    set(LOGGING_ENABLE OFF)
    message(STATUS "Disable logging")
endif()

# Checking assertion
if(ASSERTION_ENABLE)
    # enable assertion, for nvim
    if(CMAKE_C_FLAGS_${build_type} MATCHES DNDEBUG)
        string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_${build_type} "${CMAKE_C_FLAGS_${build_type}}")
    endif()
    # enable assertion, for snail
    if(CMAKE_CXX_FLAGS_${build_type} MATCHES DNDEBUG)
        string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_${build_type} "${CMAKE_CXX_FLAGS_${build_type}}")
    endif()
    message(STATUS "Enable assertion")
else()
    # disable assertion, for nvim
    if(NOT CMAKE_C_FLAGS_${build_type} MATCHES DNDEBUG)
        if(CMAKE_C_FLAGS_${build_type})
            set(CMAKE_C_FLAGS_${build_type}  "${CMAKE_C_FLAGS_${build_type}} -DNDEBUG")
        else()
            set(CMAKE_C_FLAGS_${build_type}  "-DNDEBUG")
        endif()
    endif()
    # disable assertion, for snail
    if(NOT CMAKE_CXX_FLAGS_${build_type} MATCHES DNDEBUG)
        if(CMAKE_CXX_FLAGS_${build_type})
            set(CMAKE_CXX_FLAGS_${build_type}  "${CMAKE_CXX_FLAGS_${build_type}} -DNDEBUG")
        else()
            set(CMAKE_CXX_FLAGS_${build_type}  "-DNDEBUG")
        endif()
    endif()
    message(STATUS "Disable assertion")
endif()

# Checking debugging
if(DEBUGGING_ENABLE)
    add_definitions("-DDEBUGGING_ENABLE")
    message(STATUS "Enable debugging")
else()
    message(STATUS "Disable debugging")
endif()

set(CMAKE_EXE_LINKER_FLAGS_DEV    "" CACHE STRING "CFlags used for development build." FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEV "" CACHE STRING "CFlags used for development build." FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_DEV "" CACHE STRING "CFlags used for development build." FORCE)

mark_as_advanced(CMAKE_EXE_LINKER_FLAGS_DEV)
mark_as_advanced(CMAKE_SHARED_LINKER_FLAGS_DEV)
mark_as_advanced(CMAKE_MODULE_LINKER_FLAGS_DEV)

# Remove --sort-common from linker flags
if(CMAKE_EXE_LINKER_FLAGS    MATCHES "--sort-common" OR
   CMAKE_SHARED_LINKER_FLAGS MATCHES "--sort-common" OR
   CMAKE_MODULE_LINKER_FLAGS MATCHES "--sort-common")
    message(STATUS "Removing --sort-common from linker flags")
    string(REGEX REPLACE ",--sort-common(=[^,]+)?" "" CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}")
    string(REGEX REPLACE ",--sort-common(=[^,]+)?" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
    string(REGEX REPLACE ",--sort-common(=[^,]+)?" "" CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")

    # If no linker flags remain for a -Wl argument, remove it.
    # '-Wl$' will match LDFLAGS="-Wl,--sort-common"
    # '-Wl ' will match LDFLAGS="-Wl,--sort-common -Wl,..."
    string(REGEX REPLACE "-Wl($| )" "" CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}")
    string(REGEX REPLACE "-Wl($| )" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
    string(REGEX REPLACE "-Wl($| )" "" CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
endif()

if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
endif()

add_compile_options("-Wall")
add_compile_options("-Wextra")
add_compile_options("-Wunused")
add_compile_options("-Winit-self")
add_compile_options("-Wconversion")
add_compile_options("-Wfatal-errors")
add_compile_options("-Wuninitialized")

if(TRAVIS_CI_ENABLE)
    add_compile_options("-Werror")
    message(STATUS "Enable Travis CI, add -Werror flag.")
else()
    message(STATUS "Disable Travis CI")
endif()

if(GCOV_ENABLE)
    message(STATUS "Enable Gcov")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
else()
    message(STATUS "Disable Gcov")
endif()

set(GENERATED_DIR       ${PROJECT_BINARY_DIR}/generated)
set(GEN_CONFIG_DIR      ${GENERATED_DIR}/config)
set(GEN_BINARY_DATA_DIR ${GENERATED_DIR})

include(InstallHelpers)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

include_directories("${GEN_CONFIG_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/source")

add_subdirectory(nvim)
add_subdirectory(snail)
add_subdirectory(config)
