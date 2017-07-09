find_package(PythonInterp)
if(PYTHONINTERP_FOUND)
    add_custom_target(update-nvimbindings
                      COMMAND  ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_LIST_DIR}/auto/*
                      COMMAND  ${PYTHON_EXECUTABLE}
                               ${CMAKE_CURRENT_LIST_DIR}/bindings/nvimbindings.py
                               $<TARGET_FILE:nvim> ${CMAKE_CURRENT_LIST_DIR}/auto
                      COMMENT  "Update generating nvim bindings"
                      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

    add_custom_target(preview-nvimbindings
                      COMMAND  ${PYTHON_EXECUTABLE}
                               ${CMAKE_CURRENT_LIST_DIR}/bindings/nvimbindings.py
                               $<TARGET_FILE:nvim>
                      COMMENT  "Preview auto generating nvim bindings"
                      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

    add_dependencies(update-nvimbindings nvim)
    add_dependencies(preview-nvimbindings nvim)
endif()

file(GLOB LIBS_NVIMAPI_HEADERS *.h)
file(GLOB LIBS_NVIMAPI_SOURCES *.cpp)

list(APPEND LIBS_NVIMAPI_AUTOSRC auto/nvim.cpp)

if(NOT WIN32)
    list(REMOVE_ITEM LIBS_NVIMAPI_SOURCES stdinreader.cpp)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

message(===========${MSGPACK_LIBRARIES})
add_library(nvimapi STATIC ${LIBS_NVIMAPI_HEADERS}
                           ${LIBS_NVIMAPI_SOURCES}
                           ${LIBS_NVIMAPI_AUTOSRC})

target_link_libraries(nvimapi Qt5::Network ${MSGPACK_LIBRARIES})

if($ENV{UPDATE_NVIM_BINDINGS})
    add_dependencies(nvimapi update-nvimbindings)
endif()

add_dependencies(snail nvimapi)
