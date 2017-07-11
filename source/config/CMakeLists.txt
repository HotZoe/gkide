include(CheckTypeSize)

# Check type size
check_type_size("int"       CHECK_SIZEOF_INT)
check_type_size("long"      CHECK_SIZEOF_LONG)
check_type_size("off_t"     CHECK_SIZEOF_OFF_T)
check_type_size("time_t"    CHECK_SIZEOF_TIME_T)
check_type_size("size_t"    CHECK_SIZEOF_SIZE_T)
check_type_size("intmax_t"  CHECK_SIZEOF_INTMAX_T)
check_type_size("long long" CHECK_SIZEOF_LONG_LONG)

# get nvim and snail compile flags
include(GetCompileFlags)
GetNvimCompileFlags(NVIM_BUILD_CFLAGS)
GetSnailCompileFlags(SNAIL_BUILD_CXXFLAGS)

# set the project root directory to ~/project/snail
# unifying the information to be shown in the program
string(REPLACE "${CMAKE_SOURCE_DIR}" "snail" NVIM_BUILD_CFLAGS "${NVIM_BUILD_CFLAGS}")
string(REPLACE "${CMAKE_SOURCE_DIR}" "snail" SNAIL_BUILD_CXXFLAGS "${SNAIL_BUILD_CXXFLAGS}")

message(STATUS "==================================================================")
message(STATUS "                      configuration files")
message(STATUS "==================================================================")
# Generated configuration files for source
configure_file("${CMAKE_CURRENT_LIST_DIR}/config.h.in"
                "${GEN_CONFIG_DIR}/config.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/config.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/miscinfo.h.in"
                "${GEN_CONFIG_DIR}/miscinfo.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/miscinfo.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/versiondef.h.in"
                "${GEN_CONFIG_DIR}/versiondef.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/versiondef.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/pathdef.h.in"
                "${GEN_CONFIG_DIR}/pathdef.h")
message(STATUS "[${CMAKE_BUILD_TYPE}] Config header: ${GEN_CONFIG_DIR}/pathdef.h")


