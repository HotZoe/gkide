# Qt5 CMake   : http://doc.qt.io/qt-5/cmake-manual.html
# Qt5 Plugins : http://doc.qt.io/qt-5/plugins-howto.html
#             : http://doc.qt.io/qt-5/deployment-plugins.html
# Deployment  : http://doc.qt.io/qt-5/windows-deployment.html
#             : http://wiki.qt.io/Deploying_a_Qt5_Application_Linux
# To search Qt documention for more details.

# snail source/header/resource files
file(GLOB SNAIL_APP_HEADERS *.h)
file(GLOB SNAIL_APP_SOURCES *.cpp)
file(GLOB SNAIL_APP_QTRESRC *.qrc)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Gui     REQUIRED)
find_package(Qt5Core    REQUIRED)

list(APPEND SNAIL_EXEC_LINK_LIBS "$<TARGET_LINKER_FILE:nvimcore>")
list(APPEND SNAIL_EXEC_LINK_LIBS "${MSGPACK_LIBRARIES}")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Network")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Widgets")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Core")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Gui")

if(SNAIL_USE_STATIC_QT5)
    # create qt-plugin file for static link Qt
    set(qt_plugin_moc_cpp "${CMAKE_CURRENT_BINARY_DIR}/qt_plugin_moc.cpp")
    file(WRITE ${qt_plugin_moc_cpp} "#include <QtPlugin>\n")

    if(HOST_OS_WINDOWS)
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)\n")
    else()
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QGifPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QICNSPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QICOPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QJpegPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QTgaPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QTiffPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWbmpPlugin)\n")
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWebpPlugin)\n")

        include(FindStaticQt3rdLib)

        list(APPEND SNAIL_EXEC_LINK_LIBS ${CMAKE_THREAD_LIBS_INIT})
    endif()

    list(APPEND SNAIL_APP_SOURCES ${qt_plugin_moc_cpp})
endif()

if(HOST_OS_WINDOWS)
    include_directories(${PROJECT_BINARY_DIR})
    list(APPEND SNAIL_APP_SOURCES snail.rc)
endif()

if(HOST_OS_MACOS)
    #list(APPEND SNAIL_APP_SOURCES info.plist)
endif()

list(APPEND SNAIL_APP_SOURCES ${SNAIL_APP_QTRESRC})

add_executable(snail WIN32 MACOSX_BUNDLE ${SNAIL_APP_HEADERS} ${SNAIL_APP_SOURCES})
install_helper(TARGETS snail)

target_link_libraries(snail ${SNAIL_EXEC_LINK_LIBS}) # --verbose
