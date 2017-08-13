if(NOT DEFINED BUILD_PREFIX)
    message(FATAL_ERROR "BUILD_PREFIX must be defined.")
endif()

if(NOT DEFINED DOWNLOAD_URL)
    message(FATAL_ERROR "DOWNLOAD_URL must be defined.")
endif()

if(NOT DEFINED DOWNLOAD_DIR)
    message(FATAL_ERROR "DOWNLOAD_DIR must be defined.")
endif()
if(NOT DEFINED DOWNLOAD_VER)
    message(FATAL_ERROR "DOWNLOAD_VER must be defined.")
endif()

if(NOT DEFINED EXPECT_SHA256)
    message(FATAL_ERROR "EXPECT_SHA256 must be defined.")
endif()

if(NOT DEFINED BUILD_TARGET)
    message(FATAL_ERROR "BUILD_TARGET must be defined.")
endif()

set(BIN_DIR  ${BUILD_PREFIX}/src/${BUILD_TARGET}-build) # build  directory
set(SRC_DIR  ${BUILD_PREFIX}/src/${BUILD_TARGET})       # source directory

# Check whether the source has been downloaded. If true, skip it.
if(SKIP_DOWNLOAD)
    if(EXISTS "${SRC_DIR}" AND IS_DIRECTORY "${SRC_DIR}")
        file(GLOB EXISTED_FILES "${SRC_DIR}/*")
        if(EXISTED_FILES)
            message(STATUS "Skipping download and extraction: ${SRC_DIR}")
            return()
        endif()
    elseif(EXISTS "${DOWNLOAD_DIR}" AND IS_DIRECTORY "${DOWNLOAD_DIR}")
        file(GLOB EXISTED_FILES "${DOWNLOAD_DIR}/*")
        if(EXISTED_FILES)
            message(STATUS "Skipping download: ${BUILD_TARGET}")
        endif()
    else()
        set(err_msg "No usable source found, but skip download.")
        set(err_msg "${err_msg}\n  build target : ${BUILD_TARGET}")
        set(err_msg "${err_msg}\n  skip download: SKIP_DOWNLOAD set")
        message(FATAL_ERROR "${err_msg}")
    endif()
endif()

# two backslashes are required in CMake code to get a backslash through argument parsing
string(REGEX MATCH "[^/\\?]*$" fn_extension "${DOWNLOAD_URL}") # / or ?

# file_name.extension
#           bz2  tar  tgz  tar.gz  zip tar.bz2
if("${fn_extension}" MATCHES "(\\.)(tar\\.)*(bz2|bz2|tar|tgz|gz|zip|xz)$")
    set(file_extension "${CMAKE_MATCH_2}${CMAKE_MATCH_3}") # download file extension
else()
    set(err_msg "can NOT extract tarball filename from URL.")
    set(err_msg "${err_msg}\n  build target : ${BUILD_TARGET}")
    set(err_msg "${err_msg}\n  URL          : ${DOWNLOAD_URL}")
    message(FATAL_ERROR "${err_msg}")
endif()

set(download_file_name  "${BUILD_TARGET}-v${DOWNLOAD_VER}.${file_extension}")
set(download_file  "${DOWNLOAD_DIR}/${download_file_name}")

set(NULL_SHA256  "0000000000000000000000000000000000000000000000000000000000000000")
set(EMPTY_FILE_HASH256  "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")

if(EXPECT_SHA256 STREQUAL NULL_SHA256 OR EXPECT_SHA256 STREQUAL "SKIP")
    set(hash_msg   "NOT check") # do not check hash
    set(hash_args  "")
    set(do_hash_check  0)
else()
    set(hash_args  EXPECTED_HASH SHA256=${EXPECT_SHA256})
    set(hash_msg   "${EXPECT_SHA256}")
    set(do_hash_check  1)
endif()

# file hash256 not checked yet
set(do_hash_check_done  0)

message(STATUS "Downloading ...
   From URL       = ${DOWNLOAD_URL}
   Destination    = ${download_file}
   Expect HASH256 = ${hash_msg}")

if(NOT SKIP_DOWNLOAD)
    set(do_download  1)
    if(EXISTS "${DOWNLOAD_DIR}" AND IS_DIRECTORY "${DOWNLOAD_DIR}")
        file(GLOB EXISTED_FILES "${DOWNLOAD_DIR}/*")

        foreach(item_tarball ${EXISTED_FILES})
            if(item_tarball STREQUAL download_file)
                set(do_download  0)
                message(STATUS "")
                message(STATUS "Skip Downloading")
                message(STATUS "Found: ${EXISTED_FILES}")
                message(STATUS "")
            endif()
        endforeach()

    endif()

    if(do_download)
        file(DOWNLOAD
             ${DOWNLOAD_URL}  # download url
             ${download_file} # saving to local file
             ${hash_args}     # expected hash256
             SHOW_PROGRESS
             STATUS  status
             LOG     log_msg)

        list(GET status 0 status_code)
        list(GET status 1 status_string)

        if(NOT status_code EQUAL 0)
            set(err_msg "Downloading failed, remove it and try again.")
            set(err_msg "${err_msg}\n  status_code  : [${status_code}]")
            set(err_msg "${err_msg}\n  status_string: [${status_string}]")
            set(err_msg "${err_msg}\n  log message  : [${log_msg}]")
            message(FATAL_ERROR "${err_msg}")
        endif()

        # file hash256 checked
        set(do_hash_check_done  1)
    endif()
endif()

if(do_hash_check AND NOT do_hash_check_done)
    message(STATUS "Checking HASH256 ...")
    file(SHA256 ${download_file} ACTUAL_SHA256) # compute the actual hash256 value of the file

    if(ACTUAL_SHA256 STREQUAL EMPTY_FILE_HASH256)
        # File was empty.  It's likely due to lack of SSL support.
        message(FATAL_ERROR "Found empty file, remove it and try again.")
    elseif(NOT EXPECT_SHA256 STREQUAL ACTUAL_SHA256)
        # Wasn't a NULL SHA256 and we didn't match, so we fail.
        set(err_msg "Checking HASH256 failed, remove it and try again.")
        set(err_msg "${err_msg}\n  expected HASH256: [${EXPECT_SHA256}]")
        set(err_msg "${err_msg}\n  actual HASH256  : [${ACTUAL_SHA256}]")
        message(FATAL_ERROR "${err_msg}")
    endif()
    message(STATUS "Checking HASH256 ... done")
endif()

message(STATUS "Downloading ... done")

# Slurped from a generated extract-TARGET.cmake file.
message(STATUS "Extracting ...
   src = ${download_file}
   dst = ${SRC_DIR}")

if(NOT EXISTS "${download_file}")
    message(FATAL_ERROR "NOT exist: ${download_file}")
endif()

# Prepare a temp directory space for extracting
set(i 1)
while(EXISTS "${SRC_DIR}/../${BUILD_TARGET}-ex-${i}")
    math(EXPR i "${i} + 1")
endwhile()

set(tmp_ex_dir "${SRC_DIR}/../${BUILD_TARGET}-ex-${i}")
file(MAKE_DIRECTORY "${tmp_ex_dir}")

# Do extracting
message(STATUS "Extracting ... [tar xfz]")
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${download_file}
                        WORKING_DIRECTORY  ${tmp_ex_dir}
                        RESULT_VARIABLE    rest_val)

if(NOT rest_val EQUAL 0)
    message(STATUS "Extracting ERROR ... [clean up]")
    file(REMOVE_RECURSE "${tmp_ex_dir}")
    message(FATAL_ERROR "Extracting ... failed")
endif()

# Analyze what came out
message(STATUS "Extracting ... [analysis]")
file(GLOB contents "${tmp_ex_dir}/*")
list(LENGTH contents item_num)

if(item_num EQUAL 1 AND IS_DIRECTORY "${contents}")
    set(src_dir "${contents}")
else()
    set(src_dir "${tmp_ex_dir}")
endif()

# Move "the one" directory to the final directory:
message(STATUS "extracting ... [rename]")
get_filename_component(src_dir  ${contents}  ABSOLUTE)
file(REMOVE_RECURSE  ${SRC_DIR})
file(RENAME  ${src_dir}  ${SRC_DIR})

# Clean up:
message(STATUS "Extracting ... [clean up]")
file(REMOVE_RECURSE "${tmp_ex_dir}")

message(STATUS "Extracting ... done")

if(NOT BUILD_INTREE)
    file(MAKE_DIRECTORY "${BIN_DIR}")
endif()
