# cmake-source-tree/Modules/TestBigEndian.cmake
#
# Define macro to determine endian type
# Check if the system is big endian or little endian
#
#   TEST_BIG_ENDIAN(VARIABLE)
#   VARIABLE - variable to store the result to
#
# Copyright 2002-2009 Kitware, Inc.
# =============================================================================
# macro
#      When it is invoked, the commands recorded in the macro are first
#      modified by replacing formal parameters (${arg1}) with the arguments
#      passed, and then invoked as normal commands
# function
#      When it is invoked, the commands recorded in the function are first
#      modified by replacing formal parameters (${arg1}) with the arguments
#      passed, and then invoked as normal commands
#
# There is at least one other important, albeit fairly obvious difference
# between function and macro: the semantics of return():
# When used in a macro   , won't return
# When used in a function, could return
#
# Example:
# -----------------------------------------------------------------------------
#   set(var "ABC")
#   macro(Moo arg)
#       message("arg = ${arg}")
#       set(arg "abc")
#       message("# After change the value of arg.")
#       message("arg = ${arg}")
#   endmacro()
#   message("=== Call macro ===")
#   Moo(${var})
#
#   function(Foo arg)
#       message("arg = ${arg}")
#       set(arg "abc")
#       message("# After change the value of arg.")
#       message("arg = ${arg}")
#   endfunction()
#   message("=== Call function ===")
#   Foo(${var})
# -----------------------------------------------------------------------------
# Output:
#   === Call macro ===
#   arg = ABC
#   After change the value of arg.
#   arg = ABC
#   === Call function ===
#   arg = ABC
#   After change the value of arg.
#   arg = abc
# -----------------------------------------------------------------------------
# So it seems arg is assigned the value of var when call Foo and ${arg}
# are just string replaced with ${var} when call Moo.
# They are string replacements much like the C preprocessor would do with
# a macro. If you want true CMake variables and/or better CMake scope control
# you should look at the function command.
# -----------------------------------------------------------------------------
#
get_filename_component(CheckHostEndianType_Dir ${CMAKE_CURRENT_LIST_FILE} PATH)

function(CheckHostEndianType endian_type)
    #message(STATUS "check the system endian type")
    #message(STATUS "searching 16 bit integer")
    include(CheckTypeSize)

    check_type_size("unsigned short" CMAKE_SIZEOF_UNSIGNED_SHORT)
    if(CMAKE_SIZEOF_UNSIGNED_SHORT EQUAL 2)
        #message(STATUS "using unsigned short")
        set(CMAKE_16BIT_TYPE "unsigned short")
    else()
        check_type_size("unsigned int" CMAKE_SIZEOF_UNSIGNED_INT)
        if(CMAKE_SIZEOF_UNSIGNED_INT)
            #message(STATUS "using unsigned int")
            set(CMAKE_16BIT_TYPE "unsigned int")
        else()
            check_type_size("unsigned long" CMAKE_SIZEOF_UNSIGNED_LONG)
            if(CMAKE_SIZEOF_UNSIGNED_LONG)
                #message(STATUS "using unsigned long")
                set(CMAKE_16BIT_TYPE "unsigned long")
            else()
                message(FATAL_ERROR "no suitable type found.")
            endif()
        endif()
    endif()

    set(CUSTOM_CMAKE_MODULES_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CustomModules)
    configure_file("${CheckHostEndianType_Dir}/CheckHostEndianType.c.in"
                   "${CUSTOM_CMAKE_MODULES_DIR}/CheckHostEndianType.c"
                   @ONLY)

    file(READ "${CUSTOM_CMAKE_MODULES_DIR}/CheckHostEndianType.c"
         CHECK_ENDIAN_TYPE_C_CONTENT)

    try_compile(${endian_type}
                "${CMAKE_BINARY_DIR}"
                "${CUSTOM_CMAKE_MODULES_DIR}/CheckHostEndianType.c"
                OUTPUT_VARIABLE OUTPUT
                COPY_FILE "${CUSTOM_CMAKE_MODULES_DIR}/CheckHostEndianType.bin")

    if(${endian_type})
        file(STRINGS "${CUSTOM_CMAKE_MODULES_DIR}/CheckHostEndianType.bin"
             CMAKE_TEST_ENDIANESS_STRINGS_LE
             LIMIT_COUNT 1
             REGEX "THIS IS LITTLE ENDIAN")

        file(STRINGS "${CUSTOM_CMAKE_MODULES_DIR}/CheckHostEndianType.bin"
             CMAKE_TEST_ENDIANESS_STRINGS_BE
             LIMIT_COUNT 1
             REGEX "THIS IS BIG ENDIAN")

        # on mac, if there are universal binaries built both will be true
        # return the result depending on the machine on which cmake runs
        if(CMAKE_TEST_ENDIANESS_STRINGS_BE  AND  CMAKE_TEST_ENDIANESS_STRINGS_LE)
            if(CMAKE_SYSTEM_PROCESSOR MATCHES powerpc)
                set(CMAKE_TEST_ENDIANESS_STRINGS_BE TRUE)
                set(CMAKE_TEST_ENDIANESS_STRINGS_LE FALSE)
            else()
                set(CMAKE_TEST_ENDIANESS_STRINGS_BE FALSE)
                set(CMAKE_TEST_ENDIANESS_STRINGS_LE TRUE)
            endif()

            message(STATUS "CheckHostEndianType found different results")
            message(STATUS "consider setting CMAKE_OSX_ARCHITECTURES or "
                           "CMAKE_TRY_COMPILE_OSX_ARCHITECTURES")
            message(STATUS "to one or no architecture !")
        endif()

        if(CMAKE_TEST_ENDIANESS_STRINGS_LE)
            set(${endian_type} "little endian"
                CACHE INTERNAL "Result of CheckHostEndianType" FORCE)
            #message(STATUS "check the system endian type - little endian")
        endif()

        if(CMAKE_TEST_ENDIANESS_STRINGS_BE)
            set(${endian_type} "big endian"
                CACHE INTERNAL "Result of CheckHostEndianType" FORCE)
            #message(STATUS "check the system endian type - big endian")
        endif()

        if(NOT CMAKE_TEST_ENDIANESS_STRINGS_BE
           AND NOT CMAKE_TEST_ENDIANESS_STRINGS_LE)
            message(FATAL_ERROR "CheckHostEndianType found no result!")
        endif()
    else()
        message(FATAL_ERROR "check the system endian - failed!")
    endif()
endfunction()
