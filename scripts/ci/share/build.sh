# $@: cmake args
function run_cmake()
{
    if [ "${UPLOAD_BUILD_LOG}" = "ON" ]; then
        cmake -G "Unix Makefiles" "$@" | tee --append ${GKIDESRC_DIR}/build.log
    else
        cmake -G "Unix Makefiles" "$@"
    fi
}

# ${1}: build target
function run_make()
{
    local build_target="$1"
    if [ "$1" = "" ]; then
        build_target="deps"
    fi

    if [ "${UPLOAD_BUILD_LOG}" = "ON" ]; then
        make -C "${BUILD_DIR}" "$@ VERBOSE=1" | tee --append ${GKIDESRC_DIR}/build.log
    else
        make -C "${BUILD_DIR}" "$@"
    fi

    if [ $? -ne 0 ]; then
        echo "Build Target: ${build_target}"
        echo "Build Directory: ${BUILD_DIR}"
        echo "${BUILD_ERR_MSG}"
        exit 1
    fi
}

function build_deps()
{
    mkdir -p "${DEPS_BUILD_DIR}"
    mkdir -p "${DEPS_DOWNLOAD_DIR}"

    # Use cached dependencies
    if test -f "${DEPS_CACHE_MARKER}"; then
        echo "Using dependency library from Travis cache, timestamp: $(stat_cmd "${DEPS_CACHE_MARKER}")"
        cp -r "${HOME}/.cache/nvim-deps" "${DEPS_BUILD_DIR}"
        cp -r "${HOME}/.cache/nvim-deps-downloads" "${DEPS_DOWNLOAD_DIR}"
    fi

    if test -n "${SHARED_CMAKE_BUILD_FLAGS}"; then
        DEPS_CMAKE_FLAGS="${DEPS_CMAKE_FLAGS} ${SHARED_CMAKE_BUILD_FLAGS}"
    fi

    if [ "${UPLOAD_BUILD_LOG}" = "ON" ]; then
        DEPS_CMAKE_FLAGS="${DEPS_CMAKE_FLAGS} -DCMAKE_VERBOSE_MAKEFILE=ON"
    fi

    # Even if using cached dependencies, run CMake and make to
    # update CMake configuration and update to newer deps versions.
    cd "${DEPS_BUILD_DIR}" > /dev/null
    echo "Deps build flags: ${DEPS_CMAKE_FLAGS}"
    run_cmake ${DEPS_CMAKE_FLAGS} "${TRAVIS_BUILD_DIR}/deps/"

    BUILD_DIR="${DEPS_BUILD_DIR}"
    BUILD_ERR_MSG="Error: build deps libraries failed!"
    run_make
}

function prepare_gkide_build()
{
    mkdir -p "${GKIDE_LOG_DIR}"
    mkdir -p "${GKIDE_BUILD_DIR}"

    if test -n "${SHARED_CMAKE_BUILD_FLAGS}"; then
        GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} ${SHARED_CMAKE_BUILD_FLAGS}"
    fi

    if test "${BUILD_NVIM_ONLY}" = ON ; then
        GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DGKIDE_BUILD_NVIM_ONLY=ON"
    else
        if test "${USE_SHARED_QT5}" = ON ; then
            GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DSNAIL_USE_SHARED_QT5=ON"
            GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DSNAIL_USE_STATIC_QT5=OFF"
        else
            GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DSNAIL_USE_SHARED_QT5=OFF"
            GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DSNAIL_USE_STATIC_QT5=ON"
        fi
        GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DCMAKE_PREFIX_PATH=${QT5_INSTALL_PREFIX}"
    fi

    if [ "${UPLOAD_BUILD_LOG}" = "ON" ]; then
        GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DCMAKE_VERBOSE_MAKEFILE=ON"
    fi

    cd "${GKIDE_BUILD_DIR}" > /dev/null
    echo "gkide build flags: ${GKIDE_CMAKE_FLAGS}"
    run_cmake ${GKIDE_CMAKE_FLAGS} "${TRAVIS_BUILD_DIR}"
}
