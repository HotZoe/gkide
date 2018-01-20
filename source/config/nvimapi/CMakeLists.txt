find_package(PythonInterp QUIET)
if(PYTHONINTERP_FOUND)
    add_custom_target(update-nvimapi-bindings
        COMMAND  ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_LIST_DIR}/auto/*
        COMMAND  ${PYTHON_EXECUTABLE}
                 ${CMAKE_CURRENT_LIST_DIR}/bindings/nvimbindings.py
                 $<TARGET_FILE:nvim> ${CMAKE_CURRENT_LIST_DIR}/auto
        COMMENT  "Update nvim API bindings"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

    add_custom_target(preview-nvimapi-bindings
        COMMAND  ${PYTHON_EXECUTABLE}
                 ${CMAKE_CURRENT_LIST_DIR}/bindings/nvimbindings.py
                 $<TARGET_FILE:nvim>
        COMMENT  "Preview nvim API bindings"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

    add_dependencies(update-nvimapi-bindings nvim)
    add_dependencies(preview-nvimapi-bindings nvim)
endif()
