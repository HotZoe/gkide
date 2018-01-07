find_package(PythonInterp QUIET)
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
