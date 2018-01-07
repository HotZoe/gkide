add_subdirectory(bin)

# for $GKIDE_SYS_HOME/etc/*
file(GLOB SYS_HOME_ETC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/*.nvl")

install_helper(FILES ${SYS_HOME_ETC_FILES}
               DESTINATION ${CMAKE_INSTALL_ETC_DIR})

# $GKIDE_SYS_HOME/plg/syntax/*
file(GLOB SYS_HOME_PLG_SYNTAX_FILES
     "${CMAKE_CURRENT_SOURCE_DIR}/plg/syntax/*.nvl")

install_helper(FILES ${SYS_HOME_PLG_SYNTAX_FILES}
               DESTINATION ${CMAKE_INSTALL_PLG_DIR}/syntax)
