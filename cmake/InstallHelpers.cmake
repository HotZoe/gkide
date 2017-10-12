# CMake is painful here.
#
# It will create the destination using the user's current umask, and we don't want that.
# So we try to create it with given permissions, if it's preexisting, leave it alone.
# For now, this seems like the best compromise. If we create it, then everyone can see it.

function(create_install_dir_with_perms)
    cmake_parse_arguments(_install_dir
    ""
    "DESTINATION"
    "DIRECTORY_PERMISSIONS"
    ${ARGN})

    if( NOT _install_dir_DESTINATION)
        message(FATAL_ERROR "Must specify DESTINATION")
    endif()

    if( NOT _install_dir_DIRECTORY_PERMISSIONS)
        set(_install_dir_DIRECTORY_PERMISSIONS
            OWNER_READ  OWNER_WRITE   OWNER_EXECUTE
            GROUP_READ  GROUP_EXECUTE
            WORLD_READ  WORLD_EXECUTE)
    endif()

    # handle value of CMAKE_INSTALL_PREFIX and env value of DESTDIR
    install(CODE
    "
    set(_current_dir     \"\${CMAKE_INSTALL_PREFIX}/${_install_dir_DESTINATION}\")
    set(_dir_permissions \"${_install_dir_DIRECTORY_PERMISSIONS}\")

    set(_parent_dirs)
    set(_prev_dir)

    # see 'CMAKE_INSTALL_PREFIX' for details about 'DESTDIR'
    # Explicitly prepend DESTDIR when using EXISTS.
    # file(INSTALL ...) implicitly respects DESTDIR, but EXISTS does not.
    #
    # Access environment variables.
    # Use the syntax $ENV{VAR} to read environment variable VAR.
    #
    while(NOT EXISTS \$ENV{DESTDIR}\${_current_dir}          AND
          NOT        \${_prev_dir} STREQUAL \${_current_dir})

        list(APPEND   _parent_dirs \${_current_dir})
        set(_prev_dir \${_current_dir})
        # make parent dir path as cur path
        get_filename_component(_current_dir \${_current_dir} DIRECTORY)

    endwhile()

    if(_parent_dirs)
        list(REVERSE _parent_dirs) # make parent directory come first
    endif()

    # Create any missing folders with the properly permissions.
    foreach(_current_dir \${_parent_dirs})
        if(NOT IS_DIRECTORY \${_current_dir})
            # file(INSTALL ...) implicitly respects DESTDIR, so there's no need to prepend it here.
            file(INSTALL
                 DESTINATION      \${_current_dir}
                 TYPE             DIRECTORY
                 DIR_PERMISSIONS  \${_dir_permissions}
                 FILES            \"\")
        endif()
    endforeach()
    ")
endfunction()

function(install_helper)

    # cmake_parse_arguments(<prefix>
    #                       <options>
    #                       <one_value_keywords>
    #                       <multi_value_keywords>
    #                       args...)
    cmake_parse_arguments(_install_helper
    ""
    "DESTINATION;DIRECTORY;RENAME"
    "FILES;PROGRAMS;TARGETS;DIRECTORY_PERMISSIONS;FILE_PERMISSIONS"
    ${ARGN})

    if( NOT _install_helper_DESTINATION  AND
        NOT _install_helper_TARGETS)
        message(FATAL_ERROR "Must specify the DESTINATION or TARGETS")
    endif()

    if( NOT _install_helper_FILES      AND
        NOT _install_helper_DIRECTORY  AND
        NOT _install_helper_PROGRAMS   AND
        NOT _install_helper_TARGETS)
        message(FATAL_ERROR "Must specify FILES, PROGRAMS, TARGETS, or a DIRECTORY to install")
    endif()

    if( NOT _install_helper_DIRECTORY_PERMISSIONS)
        set(_install_helper_DIRECTORY_PERMISSIONS
            OWNER_READ  OWNER_WRITE   OWNER_EXECUTE
            GROUP_READ  GROUP_EXECUTE
            WORLD_READ  WORLD_EXECUTE)
    endif()

    if( NOT _install_helper_FILE_PERMISSIONS)
        set(_install_helper_FILE_PERMISSIONS
            OWNER_READ  OWNER_WRITE
            GROUP_READ
            WORLD_READ)
    endif()

    if( NOT _install_helper_PROGRAM_PERMISSIONS)
        set(_install_helper_PROGRAM_PERMISSIONS
            OWNER_READ  OWNER_WRITE   OWNER_EXECUTE
            GROUP_READ  GROUP_EXECUTE
            WORLD_READ  WORLD_EXECUTE)
    endif()

    if(_install_helper_RENAME)
        set(RENAME_ARGS RENAME ${_install_helper_RENAME})
    endif()

    if(_install_helper_TARGETS)
        set(_install_helper_DESTINATION "")
    endif()

    # install executable (the build target)
    if(_install_helper_TARGETS)
        create_install_dir_with_perms(DESTINATION "bin")

        install(TARGETS ${_install_helper_TARGETS}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BIN_DIR}) # bin
    else()
        create_install_dir_with_perms(DESTINATION ${_install_helper_DESTINATION})
    endif()

    # install directory
    if(_install_helper_DIRECTORY)
        install(DIRECTORY             ${_install_helper_DIRECTORY}
                DESTINATION           ${_install_helper_DESTINATION}
                DIRECTORY_PERMISSIONS ${_install_helper_DIRECTORY_PERMISSIONS}
                FILE_PERMISSIONS      ${_install_helper_FILE_PERMISSIONS})
    endif()

    # install normal file
    if(_install_helper_FILES)
        install(FILES       ${_install_helper_FILES}
                DESTINATION ${_install_helper_DESTINATION}
                PERMISSIONS ${_install_helper_FILE_PERMISSIONS}
                ${RENAME_ARGS})
    endif()

    # install script file
    if(_install_helper_PROGRAMS)
        install(PROGRAMS    ${_install_helper_PROGRAMS}
                DESTINATION ${_install_helper_DESTINATION}
                PERMISSIONS ${_install_helper_PROGRAM_PERMISSIONS}
                ${RENAME_ARGS})
    endif()
endfunction()
