# config the lua path
configure_file("${CMAKE_CURRENT_LIST_DIR}/paths.lua.in"
               "${CMAKE_CURRENT_BINARY_DIR}/paths.lua")

# functional testing
add_subdirectory(fixtures)
