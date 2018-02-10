include(CheckTypeSize)

# Check type size
check_type_size("int"       CHECK_SIZEOF_INT)
check_type_size("long"      CHECK_SIZEOF_LONG)
check_type_size("off_t"     CHECK_SIZEOF_OFF_T)
check_type_size("time_t"    CHECK_SIZEOF_TIME_T)
check_type_size("size_t"    CHECK_SIZEOF_SIZE_T)
check_type_size("intmax_t"  CHECK_SIZEOF_INTMAX_T)
check_type_size("long long" CHECK_SIZEOF_LONG_LONG)

# Generated configuration files for source
configure_file("${CMAKE_CURRENT_LIST_DIR}/config.h.in"
                "${GEN_CONFIG_DIR}/config.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/buildinfo.h.in"
                "${GEN_CONFIG_DIR}/buildinfo.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/gkideversion.h.in"
                "${GEN_CONFIG_DIR}/gkideversion.h")

configure_file("${CMAKE_CURRENT_LIST_DIR}/gkideenvs.h.in"
                "${GEN_CONFIG_DIR}/gkideenvs.h")

add_subdirectory(nvimapi)
