add_executable(nvim-tty-test tty-test.c)
target_link_libraries(nvim-tty-test ${LIBUV_LIBRARIES})

add_executable(nvim-shell-test shell-test.c)
add_executable(nvim-printargs-test printargs-test.c)
