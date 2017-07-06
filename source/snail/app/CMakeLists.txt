# snail source/header files
file(GLOB SNAIL_APP_HEADERS *.h)
file(GLOB SNAIL_APP_SOURCES *.cpp)

add_executable(snail ${SNAIL_APP_SOURCES})

#target_include_directories(snail SYSTEM PUBLIC wlc/xyz)
list(APPEND SNAIL_EXEC_LINK_LIBS "$<TARGET_LINKER_FILE:nvimapi>")
list(APPEND SNAIL_EXEC_LINK_LIBS "$<TARGET_LINKER_FILE:qtshell>")


target_link_libraries(snail ${SNAIL_EXEC_LINK_LIBS})


