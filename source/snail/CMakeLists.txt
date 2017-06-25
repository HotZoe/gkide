# snail source/header files
file(GLOB SNAIL_SOURCES *.cpp)
file(GLOB SNAIL_HEADERS *.hpp)

add_executable(snail ${SNAIL_SOURCES})
