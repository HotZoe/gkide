# https://github.com/rpavlik/cmake-modules
# Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
# Copyright Iowa State University 2009-2010.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
#
# The following is modified version by charlie-wong
# https://github.com/charlie-wong
# https://github.com/snail-community
# 2016/12/31 Huizhou, Guangdong, China
#
# GetGitCurrentBranchInfo(<bn> <bh>)
#     get current branch name and hash
#
# GetGitRepoDir(<var>)
#     find current project git repository: .git
#
# GegGitBranchRecentTagName(<var> [<additional arguments to git describe> ...])
#     get the recent tag of current branch, set it to <var>
#
#  GetGitBranchExactTag(<var> [<additional arguments to git describe> ...])
#     get exactly current branch's tag name, set it to <var>
#
# GetGitBranchTimestamp(<var> [<additional arguments to git log> ...])
#     get the current branch's commit date time
#
#
# string(REGEX REPLACE <regular_expression>
#                      <replace_expression> <output variable>
#                      <input> [<input>...])
# Match the regular expression as many times as possible and substitute the
# replacement expression for the match in the output. All <input> arguments are
# concatenated before matching.
# The replace expression may refer to paren-delimited subexpressions of the
# match using \1, \2, ..., \9. Note that two backslashes (\\1) are required
# in CMake code to get a backslash through argument parsing.
#
# The following characters have special meaning in regular expressions:
# ^         Matches at beginning of input
# $         Matches at end of input
# .         Matches any single character
# [ ]       Matches any character(s) inside the brackets
# [^ ]      Matches any character(s) not inside the brackets
# -         Inside brackets, specifies an inclusive range between
#           characters on either side e.g. [a-f] is [abcdef]
#           To match a literal - using brackets, make it the first
#           or the last character e.g. [+*/-] matches basic
#           mathematical operators.
# *         Matches preceding pattern zero or more times
# +         Matches preceding pattern one or more times
# ?         Matches preceding pattern zero or once only
# |         Matches a pattern on either side of the |
# ()        Saves a matched subexpression, which can be referenced
# \         escape beging character
#           in the REGEX REPLACE operation. Additionally it is saved
#           by all regular expression-related commands, including
#           e.g. if( MATCHES ), in the variables CMAKE_MATCH_(0..9).
#
# *, + and ? have higher precedence than concatenation.
# | has lower precedence than concatenation.
# This means that the regular expression ^ab+d$ matches abbd but not ababd,
# and the regular expression ^(ab|cd)$ matches ab but not abd

# Just include and processing once
if(GetGitRepositoryInfo_Flag)
    return()
endif()
set(GetGitRepositoryInfo_Flag TRUE)

# We must run the following at "include" time, not at function call time,
# to find the path to this module(GetGitRepositoryInfo) rather than
# the path to a calling list file(the CMakeLists.txt which include this module)
get_filename_component(GetGitRepositoryInfo_Dir ${CMAKE_CURRENT_LIST_FILE} PATH)

function(GetGitRepoDir git_repo_dir)
    # check FORCED_GIT_DIR first
    if(FORCED_GIT_DIR)
        set(${git_repo_dir} ${FORCED_GIT_DIR} PARENT_SCOPE)
        return()
    endif()

    # check GIT_DIR in environment
    set(GIT_DIR $ENV{GIT_DIR})
    if(NOT GIT_DIR)
        set(GIT_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        set(GIT_DIR ${GIT_PARENT_DIR}/.git)
    endif()

    # .git dir not found, search parent directories
    while(NOT EXISTS ${GIT_DIR})
        set(GIT_PREVIOUS_PARENT ${GIT_PARENT_DIR})
        get_filename_component(GIT_PARENT_DIR ${GIT_PARENT_DIR} PATH)
        if(GIT_PARENT_DIR STREQUAL GIT_PREVIOUS_PARENT)
            return()
        endif()
        set(GIT_DIR ${GIT_PARENT_DIR}/.git)
    endwhile()

    # check if this is a submodule
    if(NOT IS_DIRECTORY ${GIT_DIR})
        # file(READ <filename> <variable>)
        #     Read content from a file called <filename> and store it in a <variable>
        file(READ ${GIT_DIR} submodule)
        string(REGEX REPLACE "gitdir: (.*)\n$" "\\1" GIT_DIR_RELATIVE ${submodule})
        get_filename_component(SUBMODULE_DIR ${GIT_DIR} PATH)
        get_filename_component(GIT_DIR ${SUBMODULE_DIR}/${GIT_DIR_RELATIVE} ABSOLUTE)
    endif()

    set(${git_repo_dir} ${GIT_DIR} PARENT_SCOPE)
endfunction()

function(GetGitCurrentBranchInfo _branch_name _hash_num)
    GetGitRepoDir(GIT_DIR)

    if(NOT GIT_DIR)
        return()
    endif()

    set(CUSTOM_CMAKE_MODULES_DIR ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CustomModules)
    if(NOT EXISTS ${CUSTOM_CMAKE_MODULES_DIR})
        file(MAKE_DIRECTORY ${CUSTOM_CMAKE_MODULES_DIR})
    endif()

    if(NOT EXISTS ${GIT_DIR}/HEAD)
        return()
    endif()

    set(HEAD_FILE ${CUSTOM_CMAKE_MODULES_DIR}/HEAD)
    configure_file(${GIT_DIR}/HEAD ${HEAD_FILE} COPYONLY)

    # @ONLY
    #      Restrict variable replacement to references of the form @VAR@
    #      This is useful for configuring scripts that use ${VAR} syntax
    configure_file(${GetGitRepositoryInfo_Dir}/GetGitRepositoryInfo.cmake.in
                   ${CUSTOM_CMAKE_MODULES_DIR}/GrabGitReferenceInfo.cmake
                   @ONLY)
    include(${CUSTOM_CMAKE_MODULES_DIR}/GrabGitReferenceInfo.cmake)

    set(${_branch_name} ${HEAD_BRANCH_NAME} PARENT_SCOPE)
    set(${_hash_num}    ${HEAD_HASH}        PARENT_SCOPE)
endfunction()

function(GegGitBranchRecentTagName _tag_name)
    GetGitRepoDir(GIT_DIR)

    if(NOT GIT_DIR)
        return()
    endif()

    if(NOT GIT_FOUND)
        # using "FindGit.cmake" in CMAKE_MODULE_PATH to do that
        # if find then set GIT_FOUND to true
        find_package(Git QUIET)
    endif()

    if(NOT GIT_FOUND)
        set(${_tag_name} "git-NOTFOUND" PARENT_SCOPE)
        return()
    endif()

    GetGitCurrentBranchInfo(cur_branch_name cur_branch_hash)
    if(NOT cur_branch_hash)
        set(${_tag_name} "head-hash-NOTFOUND" PARENT_SCOPE)
        return()
    endif()

    execute_process(COMMAND ${GIT_EXECUTABLE} describe ${cur_branch_hash} ${ARGN}
                    WORKING_DIRECTORY ${GIT_DIR}
                    RESULT_VARIABLE   res
                    OUTPUT_VARIABLE   out
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT res EQUAL 0)
        set(out "${cur_branch_hash}-NOTAGNAME")
    endif()

    set(${_tag_name} ${out} PARENT_SCOPE)
endfunction()

function(GetGitBranchTimestamp _timestamp)
    GetGitRepoDir(GIT_DIR)

    if(NOT GIT_DIR)
        return()
    endif()

    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()

    if(NOT GIT_FOUND)
        return()
    endif()

    GetGitCurrentBranchInfo(cur_branch_name cur_branch_hash)
    if(NOT cur_branch_hash)
        set(${_timestamp} "head-hash-NOTFOUND" PARENT_SCOPE)
        return()
    endif()

    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format="%ci" ${cur_branch_hash} ${ARGN}
        WORKING_DIRECTORY ${GIT_DIR}
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT res EQUAL 0)
        #string(REGEX REPLACE "[-\"\+ :]" "" out ${out})
        #string(SUBSTRING ${out} 0 12 out)
    #else()
        set(out "${cur_branch_hash}-NOCOMMITDATE")
    endif()

    set(${_timestamp} ${out} PARENT_SCOPE)
endfunction()

function(GetGitBranchExactTag _tag)
  GegGitBranchRecentTagName(out --exact-match ${ARGN})
  set(${_tag} ${out} PARENT_SCOPE)
endfunction()
