# $@: cmake args
function run_cmake()
{
    if [ "${UPLOAD_BUILD_LOG}" = "ON" ]; then
        cmake -G "Unix Makefiles" "$@" | tee --append ${BUILD_LOG_DIR}/build.log
    else
        cmake -G "Unix Makefiles" "$@"
    fi
}

# ${1}: build target
function run_make()
{
    local build_target="$1"

    if [ "${UPLOAD_BUILD_LOG}" = "ON" ]; then
        make "$@ VERBOSE=1" | tee --append ${BUILD_LOG_DIR}/build.log
    else
        make "$@"
    fi

    if [ $? -ne 0 ]; then
        echo "Build Target: ${build_target}"
        echo "Build Directory: ${BUILD_DIR}"
        echo "${BUILD_ERR_MSG}"
        exit 1
    fi
}

function deps_cache_check()
{
    clone_robot
    
    local deps_prev_sha1="ci/target/autodeps/.deps_prev_sha1"
    local pre_deps_sha1=`cat ${ROBOT_DIR}/${deps_prev_sha1}`
    local cur_deps_sha1=`git_repo_sha1 GKIDESRC master deps`

    echo "cur_deps_sha1: ${cur_deps_sha1}"
    echo "pre_deps_sha1: ${pre_deps_sha1}"

    if [ "${cur_deps_sha1}" != "${pre_deps_sha1}" ]; then
        echo "'gkide/deps' changed, update '${ROBOT_DIR}/${deps_prev_sha1}' ..."
        echo "${cur_deps_sha1}" > "${ROBOT_DIR}/${deps_prev_sha1}"

        commit_push_robot "${deps_prev_sha1}"
    fi

    # CI deps-cache valid and do not force to rebuild from fresh
    if [ -f "${DEPS_CACHE_MARKER}" -a ! "${DEPS_CACHE_ENABLE}" = "OFF" ]; then
        echo "Using Travis cache deps, last update: $(stat_cmd "${DEPS_CACHE_MARKER}")"
        mv -f "${DEPS_CACHE_DIR}/*" "${TRAVIS_BUILD_DIR}/deps"
    fi
}

# write to gkide/local.mk, do local config before build
# ${1}: config arguments to write to the local config file
function append_local_mk()
{
    printf "${1}\n" >> "${GKIDESRC_DIR}/local.mk";
}

function do_local_make_config()
{
    deps_cache_check

    if [ "${TARGET_ARCH_32}" = "ON" ]; then
        append_local_mk "TARGET_ARCH_32 := ON"
        append_local_mk "TARGET_ARCH_64 := OFF"
    fi

    if [ "${TARGET_ARCH_64}" = "ON" ]; then
        append_local_mk "TARGET_ARCH_32 := OFF"
        append_local_mk "TARGET_ARCH_64 := ON"
    fi

    if [ "${TESTING_ENABLE}" = "OFF" ]; then
        append_local_mk "TESTING_ENABLE := OFF"
    elif [ "${TESTING_ENABLE}" = "nvim" ]; then
        append_local_mk "TESTING_ENABLE := nvim"
    elif [ "${TESTING_ENABLE}" = "snail" ]; then
        append_local_mk "TESTING_ENABLE := snail"
    fi

    if [ -z "${GKIDE_CMAKE_BUILD_TYPE}" ]; then
        GKIDE_CMAKE_BUILD_TYPE="Dev"
    else
        local valid_build_type=false
        local build_types=("Dev" "Debug" "Release" "MinSizeRel")
        for bt in ${build_types[@]}; do
            if [ "${GKIDE_CMAKE_BUILD_TYPE}" = "${bt}" ]; then
                valid_build_type=true
                break
            fi
        done
        
        if ! ${valid_build_type}; then
            GKIDE_CMAKE_BUILD_TYPE="Debug"
        fi
    fi
    append_local_mk "GKIDE_CMAKE_BUILD_TYPE := ${GKIDE_CMAKE_BUILD_TYPE}"

    if [ -z "${GKIDE_RELEASE_TYPE}" ]; then
        GKIDE_RELEASE_TYPE="dev"
    else
        local valid_release_type=false
        local release_types=("dev" "alpha" "beta" "rc" "release" "stable")
        for rt in ${release_types[@]}; do
            if [ "${GKIDE_RELEASE_TYPE}" = "${rt}" ]; then
                valid_release_type=true
                break
            fi
        done
        
        if ! ${valid_release_type}; then
            GKIDE_RELEASE_TYPE="alpha"
        fi 
    fi
    append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DGKIDE_RELEASE_TYPE=${GKIDE_RELEASE_TYPE}"

    if test "${BUILD_NVIM_ONLY}" = "ON" ; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DGKIDE_BUILD_NVIM_ONLY=ON"
    else
        local QT5_INSTALL_PREFIX=""
        if test "${USE_SHARED_QT5}" = ON ; then
            if test "${BUILD_TARGET_32BIT}" = ON ; then
                QT5_INSTALL_PREFIX="/usr/lib/i386-linux-gnu"
            else
                QT5_INSTALL_PREFIX="/usr/lib/x86_64-linux-gnu"
            fi
            append_local_mk "QT5_SHARED_LIB_PREFIX := ${QT5_INSTALL_PREFIX}"
        else
            # TODO: download prebuild static Qt5 library

            if test "${BUILD_TARGET_32BIT}" = ON ; then
                QT5_INSTALL_PREFIX="todo_static_qt5_x86"
            else
                QT5_INSTALL_PREFIX="todo_static_qt5_x64"
            fi
            append_local_mk "QT5_STATIC_LIB_PREFIX := ${QT5_INSTALL_PREFIX}"
        fi
    fi

    if [ -n "${GKIDE_INSTALL_PREFIX}" ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DCMAKE_INSTALL_PREFIX=${GKIDE_INSTALL_PREFIX}"
    fi

    if [ "${NVIM_LOG_LEVEL_MIN}" -ge 0 -a "${NVIM_LOG_LEVEL_MIN}" -le 6 ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DNVIM_LOG_LEVEL_MIN=${NVIM_LOG_LEVEL_MIN}"
    fi

    if [ "${SNAIL_LOG_LEVEL_MIN}" -ge 0 -a "${SNAIL_LOG_LEVEL_MIN}" -le 6 ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DSNAIL_LOG_LEVEL_MIN=${SNAIL_LOG_LEVEL_MIN}"
    fi

    if [ "${ASSERTION_ENABLE}" = "OFF" ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DASSERTION_ENABLE=OFF"
    fi

    if [ "${GCOV_ENABLE}" = "ON" ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DGCOV_ENABLE=ON"
    fi

    if [ "${JEMALLOC_ENABLE}" = "OFF" ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DJEMALLOC_ENABLE=OFF"
    fi

    if [ -n "${GKIDE_C_FLAGS}" ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DCMAKE_C_FLAGS=${GKIDE_C_FLAGS}"
    fi

    if [ -n "${GKIDE_CXX_FLAGS}" ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DCMAKE_CXX_FLAGS=${GKIDE_CXX_FLAGS}"
    fi

    if [ -n "${DEPS_C_FLAGS}" ]; then
        append_local_mk "DEPS_CMAKE_EXTRA_FLAGS += -DCMAKE_C_FLAGS=${DEPS_C_FLAGS}"
    fi

    if [ -n "${DEPS_CXX_FLAGS}" ]; then
        append_local_mk "DEPS_CMAKE_EXTRA_FLAGS += -DCMAKE_CXX_FLAGS=${DEPS_CXX_FLAGS}"
    fi

    if [ -n "${CMAKE_EXTRA_FLAGS}" ]; then
        append_local_mk "DEPS_CMAKE_EXTRA_FLAGS += ${CMAKE_EXTRA_FLAGS}"
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += ${CMAKE_EXTRA_FLAGS}"
    fi

    if [ "${TRAVIS_CI_ENABLE}" = "ON" ]; then
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DTRAVIS_CI_ENABLE=ON"
    else
        append_local_mk "GKIDE_CMAKE_EXTRA_FLAGS += -DTRAVIS_CI_ENABLE=OFF"
    fi
    
    echo "local.mk dump ..."
    cat "${GKIDESRC_DIR}/local.mk"
}

function build_deps()
{
    do_local_make_config

    cd "${GKIDESRC_DIR}" > /dev/null
    BUILD_DIR="${GKIDESRC_DIR}/deps/build"
    BUILD_ERR_MSG="Error: build deps libraries failed!"
    run_make "deps"
}

function build_nvim()
{
    cd "${GKIDESRC_DIR}" > /dev/null
    BUILD_DIR="${GKIDESRC_DIR}/build"
    BUILD_ERR_MSG="Error: build nvim failed!"
    run_make "nvim"
}

function build_snail()
{
    cd "${GKIDESRC_DIR}" > /dev/null
    BUILD_DIR="${GKIDESRC_DIR}/build"
    BUILD_ERR_MSG="Error: build nvim failed!"
    run_make "snail"
}

