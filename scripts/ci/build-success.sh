#!/usr/bin/env bash

set -e
set -o pipefail

echo "GKIDE building success ..."

CI_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${CI_DIR}/share/config.sh"
source "${CI_DIR}/share/git-repo.sh"

echo "update autodocs/dev/log/XXX/..."
upload_build_logs
