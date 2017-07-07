# check the qtshell library
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)

include_directories($<TARGET_PROPERTY:qtshell,QTSHELL_TESTS_INCLUDE_DIR>)

add_custom_target(run-snail-libs-qtshell-tests
                  COMMAND ${CMAKE_CTEST_COMMAND}
                  COMMENT "Run test/snail/libs/qtshell tests")

add_dependencies(run-snail-libs-test run-snail-libs-qtshell-tests)

function(add_ctest SOURCE_NAME)
    add_executable(${SOURCE_NAME} ${SOURCE_NAME}.cpp)
    set_target_properties(${SOURCE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                    ${CMAKE_CURRENT_BINARY_DIR}/bin)
    target_link_libraries(${SOURCE_NAME} Qt5::Widgets Qt5::Test
                                         $<TARGET_LINKER_FILE:qtshell>)
    add_test(NAME ${SOURCE_NAME} COMMAND ${SOURCE_NAME}
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
    add_dependencies(run-snail-libs-qtshell-tests ${SOURCE_NAME})
endfunction()

add_ctest(test_cell)
add_ctest(test_shellcontents)
add_ctest(test_shellwidget)
add_ctest(bench_scroll)
add_ctest(bench_cell)
