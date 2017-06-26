# Build flags for Dev build type
if(CMAKE_COMPILER_IS_GNUCCXX)
    check_cxx_compiler_flag(-Og GNUCCXX_HAS_OG_FLAG)
else()
    set(GNUCCXX_HAS_OG_FLAG false)
endif()

if(GNUCCXX_HAS_OG_FLAG)
    set(CMAKE_CXX_FLAGS_DEV "-Og -g" CACHE STRING "CFlags used for development build." FORCE)
else()
    set(CMAKE_CXX_FLAGS_DEV "-O2 -g" CACHE STRING "CFlags used for development build." FORCE)
endif()

# snail source/header files
file(GLOB SNAIL_SOURCES *.cpp)
file(GLOB SNAIL_HEADERS *.hpp)

add_compile_options("-std=c++11")
add_executable(snail ${SNAIL_SOURCES})

target_include_directories(snail SYSTEM PUBLIC wlc/xyz)
list(APPEND SNAIL_EXEC_LINK_LIBS ${SNAIL_DEP_LINK_LIBRARIES})

target_link_libraries(snail ${SNAIL_EXEC_LINK_LIBS})

