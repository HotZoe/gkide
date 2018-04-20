#!/usr/bin/env bash

set -e
set -o pipefail

echo "GKIDE building success ..."

# get the build result status
echo "GKIDE build status: ${TRAVIS_TEST_RESULT}"
