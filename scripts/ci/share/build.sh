source ${SHARE_DIR}/share/utils.sh
source ${SHARE_DIR}/share/prepare.sh

SHARED_CMAKE_BUILD_FLAGS=""
CURRENT_BUILD_DIR=""

function run_make()
{
    echo '======================================================================'
    make -C "${CURRENT_BUILD_DIR}" "$@"
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

    # Even if using cached dependencies, run CMake and make to
    # update CMake configuration and update to newer deps versions.
    cd "${DEPS_BUILD_DIR}"
    echo "Deps build flags: ${DEPS_CMAKE_FLAGS}"
    CC= cmake -G "Unix Makefiles" ${DEPS_CMAKE_FLAGS} "${TRAVIS_BUILD_DIR}/deps/"

    CURRENT_BUILD_DIR="${DEPS_BUILD_DIR}"
    run_make
    if ! $?; then
        echo "Error: build deps libraries failed!"
        exit 1
    fi
    
    cd "${TRAVIS_BUILD_DIR}"
}

function prepare_build_gkide()
{
    # check env setting before build
    env_check_pre_build
    
    build_deps

    mkdir -p "${GKIDE_LOG_DIR}"
    mkdir -p "${GKIDE_BUILD_DIR}"

    if test -n "${SHARED_CMAKE_BUILD_FLAGS}"; then
        GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} ${SHARED_CMAKE_BUILD_FLAGS}"
    fi

    GKIDE_CMAKE_FLAGS="${GKIDE_CMAKE_FLAGS} -DGKIDE_BUILD_NVIM_ONLY=ON"

    cd "${GKIDE_BUILD_DIR}"
    echo "gkide build flags: ${GKIDE_CMAKE_FLAGS}"
    CC= cmake -G "Unix Makefiles" ${GKIDE_CMAKE_FLAGS} "${TRAVIS_BUILD_DIR}"
}

function build_gkide()
{
    prepare_build_gkide

    cd "${GKIDE_BUILD_DIR}"
    CURRENT_BUILD_DIR="${GKIDE_BUILD_DIR}"

    run_make nvim
    if ! $?; then
        echo "Error: build nvim failed!"
        exit 1
    fi
}

