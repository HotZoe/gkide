GKIDE_GITHUB_URL="https://github.com/gkide"

# $TRAVIS_BUILD_DIR => /home/travis/build/gkide/robot
# The working directory for current job
# => /home/travis/build/gkide
REPO_HOME_DIR=$(cd ${TRAVIS_BUILD_DIR}/.. && pwd)

GKIDESRC_DIR="${REPO_HOME_DIR}/gkide"

AUTODOCS_BRANCH="master"
AUTODOCS_REPO="${GKIDE_GITHUB_URL}/autodocs.git"
AUTODOCS_DIR="${REPO_HOME_DIR}/autodocs"

