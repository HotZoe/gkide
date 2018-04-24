#!/usr/bin/env bash

set -e
set -o pipefail

echo "GKIDE building ..."

BUILD_DIR=""
BUILD_ERR_MSG=""
QT5_INSTALL_PREFIX=""
SHARED_CMAKE_BUILD_FLAGS=""

CI_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${CI_DIR}/share/utils.sh"
source "${CI_DIR}/share/prepare.sh"
source "${CI_DIR}/share/build.sh"

# check env setting before build anything
env_check_pre_build

# build deps
build_deps

# prepare for GKIDE build
prepare_gkide_build

# build nvim
BUILD_DIR="${GKIDE_BUILD_DIR}"
BUILD_ERR_MSG="Error: build nvim failed!"
run_make "nvim" "VERBOSE=1 | tee build.log"

# build snail
#BUILD_DIR="${GKIDE_BUILD_DIR}"
#BUILD_ERR_MSG="Error: build snail failed!"
#run_make snail
