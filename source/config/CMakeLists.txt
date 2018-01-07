include(CheckTypeSize)

# Check type size
check_type_size("int"       CHECK_SIZEOF_INT)
check_type_size("long"      CHECK_SIZEOF_LONG)
check_type_size("off_t"     CHECK_SIZEOF_OFF_T)
check_type_size("time_t"    CHECK_SIZEOF_TIME_T)
check_type_size("size_t"    CHECK_SIZEOF_SIZE_T)
check_type_size("intmax_t"  CHECK_SIZEOF_INTMAX_T)
check_type_size("long long" CHECK_SIZEOF_LONG_LONG)

# get nvim compile flags
include(GetCompileFlags)
GetTargetCompileFlags(nvim NVIM_BUILD_CFLAGS false)

# set the project root directory to ~/project/gkide
# unifying the information to be shown in the program
string(REPLACE "${CMAKE_SOURCE_DIR}" "gkide"
               NVIM_BUILD_CFLAGS "${NVIM_BUILD_CFLAGS}")

# Generated configuration files for source
configure_file("${CMAKE_CURRENT_LIST_DIR}/config.h.in"
                "${GEN_CONFIG_DIR}/config.h")
message(STATUS
        "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/config.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/miscinfo.h.in"
                "${GEN_CONFIG_DIR}/miscinfo.h")
message(STATUS
        "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/miscinfo.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/versiondef.h.in"
                "${GEN_CONFIG_DIR}/versiondef.h")
message(STATUS
        "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/versiondef.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/envdefs.h.in"
                "${GEN_CONFIG_DIR}/envdefs.h")
message(STATUS
        "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/envdefs.h")

# Generated doxygen configuration file
set(DOXYGEN_MANUAL_NAME        "nvim")
set(DOXYGEN_MANUAL_OUTPUT_DIR  "nvim")
set(DOXYGEN_MANUAL_INPUT "${GEN_CONFIG_DIR} ${PROJECT_SOURCE_DIR}/source/nvim")

# TODO, use doxygen to generated the docs for the files,
# and it can not convert to pdf, need to fix
set(DOXYGEN_MANUAL_EXCLUDE  "${PROJECT_SOURCE_DIR}/source/nvim/eval/typval.c \
                             ${PROJECT_SOURCE_DIR}/source/nvim/edit.c \
                             ${PROJECT_SOURCE_DIR}/source/nvim/ex_getln.c")

configure_file("${CMAKE_CURRENT_LIST_DIR}/doxygen/Doxyfile.in"
                "${GENERATED_DIR}/doxygen/Doxyfile.nvim")
message(STATUS "Doxygen Config: ${GENERATED_DIR}/doxygen/Doxyfile.nvim")

add_subdirectory(nvimapi)
