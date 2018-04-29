#!/usr/bin/env bash

set -e
set -o pipefail

echo "GKIDE building and testing ..."

CI_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${CI_DIR}/share/utils.sh"
source "${CI_DIR}/share/config.sh"
source "${CI_DIR}/share/build.sh"
source "${CI_DIR}/share/git-repo.sh"

# build deps
build_deps

# build nvim
build_nvim

# build snail
build_snail

touch ${MARKER_ALL_DONE}
