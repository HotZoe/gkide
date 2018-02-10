set(DOXYGEN_MANUAL_NAME        "snail")
set(DOXYGEN_MANUAL_OUTPUT_DIR  "snail")
set(DOXYGEN_MANUAL_EXCLUDE     "")
set(DOXYGEN_MANUAL_INPUT
    "${GEN_CONFIG_DIR} ${PROJECT_SOURCE_DIR}/source/plugins/bin/snail")

configure_file("${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in"
                "${GENERATED_DIR}/doxygen/Doxyfile.snail")

