# Generated doxygen configuration file
set(DOXYGEN_MANUAL_NAME        "nvim")
set(DOXYGEN_MANUAL_OUTPUT_DIR  "nvim")
set(DOXYGEN_MANUAL_INPUT "${GEN_CONFIG_DIR} ${PROJECT_SOURCE_DIR}/source/nvim")

# TODO, use doxygen to generated the docs for the files,
# and it can not convert to pdf, need to fix
set(DOXYGEN_MANUAL_EXCLUDE  "${PROJECT_SOURCE_DIR}/source/nvim/eval/typval.c \
                             ${PROJECT_SOURCE_DIR}/source/nvim/edit.c \
                             ${PROJECT_SOURCE_DIR}/source/nvim/ex_getln.c")

configure_file("${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in"
                "${GENERATED_DIR}/doxygen/Doxyfile.nvim")

