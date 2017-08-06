# Dependencies library link to nvim

include(CMakeParseArguments)

# BuildLibvterm(<CONFIGURE_COMMAND ...>  <BUILD_COMMAND ...>  <INSTALL_COMMAND ...>)
function(BuildLibvterm)
    cmake_parse_arguments(_libvterm
    ""
    ""
    "CONFIGURE_COMMAND;BUILD_COMMAND;INSTALL_COMMAND;LOCAL_REPO_DIR"
    ${ARGN})

    if(NOT _libvterm_CONFIGURE_COMMAND AND
       NOT _libvterm_BUILD_COMMAND     AND
       NOT _libvterm_INSTALL_COMMAND)
        message(FATAL_ERROR "Must set CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND")
    endif()

    if(NOT _libvterm_LOCAL_REPO_DIR)
        set(_libvterm_LOCAL_REPO_DIR "${DEPS_DOWNLOAD_DIR}/libvterm")
    endif()

    externalproject_add(   libvterm
        PREFIX             ${DEPS_BUILD_DIR}
        DOWNLOAD_COMMAND   ${CMAKE_COMMAND}
                           -E copy_directory ${DEPS_DOWNLOAD_DIR}/libvterm
                                             ${DEPS_BUILD_DIR}/src/libvterm
        SOURCE_DIR         "${_libvterm_LOCAL_REPO_DIR}"
        CONFIGURE_COMMAND  "${_libvterm_CONFIGURE_COMMAND}"
        BUILD_IN_SOURCE    0
        BINARY_DIR         "${DEPS_BUILD_DIR}/src/libvterm"
        BUILD_COMMAND      "${_libvterm_BUILD_COMMAND}"
        INSTALL_COMMAND    "${_libvterm_INSTALL_COMMAND}")

    if(NOT (EXISTS       "${_libvterm_LOCAL_REPO_DIR}"        AND
            IS_DIRECTORY "${_libvterm_LOCAL_REPO_DIR}"        AND
            EXISTS       "${_libvterm_LOCAL_REPO_DIR}/.git"   AND
            IS_DIRECTORY "${_libvterm_LOCAL_REPO_DIR}/.git"))

            ExternalProject_Add_Step(libvterm libvterm-repo-clone
                COMMENT           "git clone ${LIBVTERM_GIT_REPO}"
                WORKING_DIRECTORY ${DEPS_DOWNLOAD_DIR}
                COMMAND           ${GIT_PROG} clone ${LIBVTERM_GIT_REPO})

    endif()
endfunction()

if(WIN32 OR MINGW)
    # Host=Windows, Target=Windows
    set(LIBVTERM_CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibvtermCMakeLists.txt
                                 ${DEPS_BUILD_DIR}/src/libvterm/CMakeLists.txt
                         COMMAND ${CMAKE_COMMAND} ${DEPS_BUILD_DIR}/src/libvterm
                                 -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
                                 -DCMAKE_C_COMPILER=${DEPS_C_COMPILER}
                                 "-DCMAKE_C_FLAGS:STRING=-fPIC"
                                 -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
                                 -DCMAKE_MAKE_PROGRAM:FILEPATH=${MAKE_PROG})

    set(LIBVTERM_BUILD_COMMAND
        ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE})

    set(LIBVTERM_INSTALL_COMMAND
        ${CMAKE_COMMAND} --build . --target install --config ${CMAKE_BUILD_TYPE})
else()
    # Host=Linux, Target=Linux
    # Host=MacOS, Target=MacOS
    set(LIBVTERM_INSTALL_COMMAND
        ${MAKE_PROG} -C ${DEPS_BUILD_DIR}/src/libvterm
                        CC=${DEPS_C_COMPILER}
                        PREFIX=${DEPS_INSTALL_DIR}
                        CFLAGS=-fPIC
                        install)
endif()

message(STATUS  "Building: libvterm-v${LIBVTERM_VERSION}")
BuildLibvterm(CONFIGURE_COMMAND ${LIBVTERM_CONFIGURE_COMMAND}
              BUILD_COMMAND     ${LIBVTERM_BUILD_COMMAND}
              INSTALL_COMMAND   ${LIBVTERM_INSTALL_COMMAND})

list(APPEND THIRD_PARTY_LIBS libvterm)
