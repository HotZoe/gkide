# Qt5 CMake   : http://doc.qt.io/qt-5/cmake-manual.html
# Qt5 Plugins : http://doc.qt.io/qt-5/plugins-howto.html
#             : http://doc.qt.io/qt-5/deployment-plugins.html
# Deployment  : http://doc.qt.io/qt-5/windows-deployment.html
#             : http://wiki.qt.io/Deploying_a_Qt5_Application_Linux
# To search Qt documention for more details.

# snail source/header files
file(GLOB SNAIL_APP_HEADERS *.h)
file(GLOB SNAIL_APP_SOURCES *.cpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(SNAIL_USE_STATIC_QT5)
    # create qt-plugin file for static link Qt
    set(qt_plugin_moc_cpp "${CMAKE_CURRENT_BINARY_DIR}/qt_plugin_moc.cpp")
    file(WRITE ${qt_plugin_moc_cpp} "#include <QtPlugin>\n")

    if(HOST_OS_WINDOWS)
        file(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)\n")
    else()
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(AccessibleFactory)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QDDSPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QICNSPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QICOPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QJp2Plugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QMngPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QTgaPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QTiffPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWbmpPlugin)\n")
        FILE(APPEND ${qt_plugin_moc_cpp} "Q_IMPORT_PLUGIN(QWebpPlugin)\n")
    endif()

    list(APPEND SNAIL_APP_SOURCES ${qt_plugin_moc_cpp})
endif()

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

add_executable(snail WIN32 MACOSX_BUNDLE ${SNAIL_APP_HEADERS} ${SNAIL_APP_SOURCES})

list(APPEND SNAIL_EXEC_LINK_LIBS "$<TARGET_LINKER_FILE:nvimcore>")
list(APPEND SNAIL_EXEC_LINK_LIBS "${MSGPACK_LIBRARIES}")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Network")
list(APPEND SNAIL_EXEC_LINK_LIBS "Qt5::Widgets")

target_link_libraries(snail ${SNAIL_EXEC_LINK_LIBS})

if(SNAIL_USE_SHARED_QT5 AND HOST_OS_WINDOWS)
    # Auto deploy when using shared Qt5 for windows build
    find_program(WINDEPLOYQT_PROG windeployqt)
    if(NOT WINDEPLOYQT_PROG)
        message(WARNING "'windeployqt' program not find, you may want to run it by hand!")
    endif()

    message("==============${WINDEPLOYQT_PROG}")
    add_custom_target(run_windeployqt_prog ALL
                      COMMAND echo "here==========================="
                      COMMENT "Run 'windeployqt.exe' for 'snail.exe'"
                      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()
