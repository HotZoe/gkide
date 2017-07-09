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

file(GLOB LIBS_NVIMCORE_HEADERS *.h)
file(GLOB LIBS_NVIMCORE_SOURCES *.cpp)

list(APPEND LIBS_NVIMCORE_AUTOSRC auto/nvim.cpp)

if(NOT WIN32)
    list(REMOVE_ITEM LIBS_NVIMCORE_SOURCES stdinreader.cpp)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

add_library(nvimcore STATIC ${LIBS_NVIMCORE_HEADERS}
                            ${LIBS_NVIMCORE_SOURCES}
                            ${LIBS_NVIMCORE_AUTOSRC})

target_link_libraries(nvimcore Qt5::Widgets Qt5::Network)

if($ENV{UPDATE_NVIMCORE_BINDINGS})
    add_dependencies(nvimcore update-nvimbindings)
endif()

add_dependencies(snail nvimcore)
