# get_filename_component(<VAR> <FileName> <COMP> [CACHE])
#     Set <VAR> to a component of <FileName>, where <COMP> is one of:
#     DIRECTORY = Directory without file name
#     NAME      = File name without directory
#     EXT       = File name longest extension (.b.c from d/a.b.c)
#     NAME_WE   = File name without directory or longest extension
#     PATH      = Legacy alias for DIRECTORY (use for CMake <= 2.8.11)
#
# get_filename_component(<VAR> <FileName>
#                        <COMP> [BASE_DIR <BASE_DIR>]
#                        [CACHE])
# Set <VAR> to the absolute path of <FileName>, where <COMP> is one of:
# ABSOLUTE  = Full path to file
# REALPATH  = Full path to existing file with symlinks resolved
#
# another solation
# CMake has two undocumented options
# Add this options before PROJECT()/project() keyword, like following
# this solation is not good, becouse it will recursely creat CMakeCache.txt
# file and CMakeFiles directory
#
# cmake_minimum_required(VERSION 3.4)
# set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
# set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
# project(snail)

function(PreventInTreeBuilds)
    get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
    get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

    if("${srcdir}" STREQUAL "${bindir}")
        set(err_msg "stop building ...")
        set(err_msg "${err_msg}\n  ---------------------------------------------------------------")
        set(err_msg "${err_msg}\n                In-source builds are not permitted.")
        set(err_msg "${err_msg}\n  It's recommended that making a separate folder for building.")
        set(err_msg "${err_msg}\n  =>  $mkdir build; cd build; cmake ..")
        set(err_msg "${err_msg}\n  Remove all the files and directories already created in source tree.")
        set(err_msg "${err_msg}\n  =>  $rm -rf CMakeCache.txt CMakeFiles")
        set(err_msg "${err_msg}\n  ---------------------------------------------------------------")
        set(err_msg "${err_msg}\n")
        message(FATAL_ERROR "${err_msg}")
    endif()
endfunction()

PreventInTreeBuilds()
