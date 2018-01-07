# get snail compile flags
include(GetCompileFlags)
GetTargetCompileFlags(snail SNAIL_BUILD_CXXFLAGS true)

# set the project root directory to ~/project/gkide
# unifying the information to be shown in the program
string(REPLACE "${CMAKE_SOURCE_DIR}" "gkide"
               SNAIL_BUILD_CXXFLAGS "${SNAIL_BUILD_CXXFLAGS}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/configsnail.h.in"
               "${GEN_CONFIG_DIR}/configsnail.h")
message(STATUS
        "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/configsnail.h")

set(DOXYGEN_MANUAL_NAME        "snail")
set(DOXYGEN_MANUAL_OUTPUT_DIR  "snail")
set(DOXYGEN_MANUAL_EXCLUDE     "")
set(DOXYGEN_MANUAL_INPUT
    "${GEN_CONFIG_DIR} ${PROJECT_SOURCE_DIR}/source/plugins/bin/snail")

configure_file("${CMAKE_CURRENT_LIST_DIR}/doxygen/Doxyfile.in"
                "${GENERATED_DIR}/doxygen/Doxyfile.snail")
message(STATUS "Doxygen Config: ${GENERATED_DIR}/doxygen/Doxyfile.snail")
