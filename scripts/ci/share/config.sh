GKIDE_GITHUB_URL="https://github.com/gkide"

# $TRAVIS_BUILD_DIR => /home/travis/build/gkide/robot
# The working directory for current job
# => /home/travis/build/gkide
REPO_HOME_DIR=$(cd ${TRAVIS_BUILD_DIR}/.. && pwd)

GKIDESRC_DIR="${REPO_HOME_DIR}/gkide"

AUTODOCS_BRANCH="master"
AUTODOCS_REPO="${GKIDE_GITHUB_URL}/autodocs.git"
AUTODOCS_DIR="${REPO_HOME_DIR}/autodocs"

AUTODEPS_BRANCH="master"
AUTODEPS_REPO="${GKIDE_GITHUB_URL}/autodeps.git"
AUTODEPS_DIR="${REPO_HOME_DIR}/autodeps"

ROBOT_BRANCH="master"
ROBOT_REPO="${GKIDE_GITHUB_URL}/robot.git"
ROBOT_DIR="${REPO_HOME_DIR}/robot"

MARKER_ALL_DONE="${BUILD_LOG_DIR}/marker_all_done"
GKIDE_FULL_BUILD_LOG="${BUILD_LOG_DIR}/gkide_full_build.log"
TEST_FAIL_SUMMARY_LOG="${BUILD_LOG_DIR}/test_fail_summary.log"

BUILD_DIR=""
BUILD_ERR_MSG=""
