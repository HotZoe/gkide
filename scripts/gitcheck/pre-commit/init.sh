#!/bin/sh

# if error, it may be the difference between 'bash' & 'dash'
# - $ ls -al $(which sh)
# - $ ls -al $(which bash)
# - $ ls -al $(which dash)

GKIDE_PRE_COMMIT_CHECK_DIR=$(pwd)/scripts/gitcheck/pre-commit

# check if we have non-ASCII filenames
${GKIDE_PRE_COMMIT_CHECK_DIR}/check-filename.sh

# check the changed files contents
${GKIDE_PRE_COMMIT_CHECK_DIR}/check-filecontents.sh

# check the changed files contents
${GKIDE_PRE_COMMIT_CHECK_DIR}/check-signed-off-by.sh

echo "ok"
