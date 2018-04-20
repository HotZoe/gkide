#!/usr/bin/env bash

set -e
set -o pipefail

echo "Step_4: GKIDE build ..."

SHARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SHARE_DIR}/share/build.sh"

# build GKIDE
build_gkide
