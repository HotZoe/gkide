# get snail compile flags
include(GetCompileFlags)
GetTargetCompileFlags(snail SNAIL_BUILD_CXXFLAGS true)

# set the project root directory to ~/project/gkide
# unifying the information to be shown in the program
string(REPLACE "${CMAKE_SOURCE_DIR}" "gkide"
               SNAIL_BUILD_CXXFLAGS "${SNAIL_BUILD_CXXFLAGS}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/configsnail.h.in"
               "${GEN_CONFIG_DIR}/configsnail.h")

add_subdirectory(doxygen)

