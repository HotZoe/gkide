# Checking debugging of Qt
if(NOT DEBUGGING_ENABLE)
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
endif()

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
