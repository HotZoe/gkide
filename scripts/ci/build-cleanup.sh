#!/usr/bin/env bash

set -e
set -o pipefail

echo "Cleanup after building ..."
du -d 2 "${HOME}/.cache" | sort -n

CI_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${CI_DIR}/share/utils.sh"
source "${CI_DIR}/share/config.sh"

# Update the third-party dependency cache only if the build & test successful.
if all_done_successfully; then
    rm -rf "${DEPS_CACHE_DIR}/*"
    mv "${GKIDESRC_DIR}/deps/build" "${DEPS_CACHE_DIR}/"
    mv "${GKIDESRC_DIR}/deps/downloads" "${DEPS_CACHE_DIR}/"

    touch "${DEPS_CACHE_MARKER}"
    echo "Updated CI deps cache, timestamp: $(stat_cmd "${DEPS_CACHE_MARKER}")"
fi
