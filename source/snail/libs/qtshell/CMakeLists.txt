file(GLOB LIBS_QTSHELL_HEADERS *.h)
file(GLOB LIBS_QTSHELL_SOURCES *.cpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)

add_library(qtshell STATIC ${LIBS_QTSHELL_HEADERS}
                           ${LIBS_QTSHELL_SOURCES})

target_link_libraries(qtshell Qt5::Widgets)

add_dependencies(snail qtshell)

add_executable(qtshell-example1 EXCLUDE_FROM_ALL usage/example1.cpp)
set_target_properties(qtshell-example1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(qtshell-example1 qtshell)

add_executable(qtshell-example2 EXCLUDE_FROM_ALL usage/example2.cpp)
set_target_properties(qtshell-example2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(qtshell-example2 qtshell)

add_executable(qtshell-example3 EXCLUDE_FROM_ALL usage/example3.cpp)
set_target_properties(qtshell-example3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(qtshell-example3 qtshell)

add_executable(qtshell-example4 EXCLUDE_FROM_ALL usage/example4.cpp)
set_target_properties(qtshell-example4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(qtshell-example4 qtshell)

add_subdirectory(test)
