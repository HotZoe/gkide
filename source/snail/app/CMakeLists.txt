# snail source/header files
file(GLOB SNAIL_APP_HEADERS *.h)
file(GLOB SNAIL_APP_SOURCES *.cpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

add_executable(snail WIN32 MACOSX_BUNDLE ${SNAIL_APP_HEADERS} ${SNAIL_APP_SOURCES})

list(APPEND SNAIL_EXEC_LINK_LIBS "$<TARGET_LINKER_FILE:nvimcore>")
list(APPEND SNAIL_EXEC_LINK_LIBS "${MSGPACK_LIBRARIES}")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Network")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Widgets")

target_link_libraries(snail ${SNAIL_EXEC_LINK_LIBS})


