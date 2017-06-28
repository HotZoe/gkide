# prerequisite fixtures for functional testing
add_subdirectory(fixtures)

set(NVIM_FUNCTIONAL_TEST_PREREQS nvim
                                 nvim-shell-test
                                 nvim-printargs-test)

if(NOT WIN32)
    list(APPEND NVIM_FUNCTIONAL_TEST_PREREQS nvim-tty-test)
endif()

add_custom_target(nvim-functional-test-prereqs
                  DEPENDS ${NVIM_FUNCTIONAL_TEST_PREREQS})
