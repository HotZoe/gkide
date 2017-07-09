file(GLOB LIBS_NVIMGUI_HEADERS *.h)
file(GLOB LIBS_NVIMGUI_SOURCES *.cpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

add_library(nvimgui STATIC ${LIBS_NVIMGUI_HEADERS} ${LIBS_NVIMGUI_SOURCES})
set_target_properties(nvimgui PROPERTIES NVIMGUI_TESTS_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(nvimgui Qt5::Widgets Qt5::Network)

add_dependencies(snail nvimgui)

# build nvimgui library usage examples
add_executable(nvimgui-example1 EXCLUDE_FROM_ALL usage/example1.cpp)
set_target_properties(nvimgui-example1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                  ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(nvimgui-example1 nvimgui)

add_executable(nvimgui-example2 EXCLUDE_FROM_ALL usage/example2.cpp)
set_target_properties(nvimgui-example2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                  ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(nvimgui-example2 nvimgui)

add_executable(nvimgui-example3 EXCLUDE_FROM_ALL usage/example3.cpp)
set_target_properties(nvimgui-example3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                  ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(nvimgui-example3 nvimgui)

add_executable(nvimgui-example4 EXCLUDE_FROM_ALL usage/example4.cpp)
set_target_properties(nvimgui-example4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                  ${CMAKE_CURRENT_BINARY_DIR}/bin)
target_link_libraries(nvimgui-example4 nvimgui)

