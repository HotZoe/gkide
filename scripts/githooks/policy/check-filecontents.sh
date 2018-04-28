#!/usr/bin/env bash

GIT_REPO_DIR="$(cd ${PWD} && pwd)"
POLICY_DIR="${GIT_REPO_DIR}/scripts/githooks/policy"
source "${POLICY_DIR}/utils.sh"

DEPS_SRC_CHANGED=false

function update_deps_sha1_marker()
{
    local sha_marker="${GIT_REPO_DIR}/deps/.deps_sha1_marker"
    # get the HEAD SHA1, the next one is current commit, which deps changed
    local cur_sha1="$(git log --pretty=oneline -1 HEAD | cut -d ' ' -f 1)"
    local pre_sha1="$(cat ${sha_marker})"
    echo "[C]SHA1: `msg_green ${cur_sha1}`"
    echo "[P]SHA1: `msg_red ${pre_sha1}`"
    echo "${cur_sha1}" > ${sha_marker}
}

function check_filecontents()
{
    file="${1}"

    if [ ! -f ${file} ]; then
        echo "delete : `msg_red ${file}`"
        return 1
    fi

    local str_1="`echo "${file}" | grep "^deps/CMakeLists.txt$"`"
    local str_2="`echo "${file}" | grep "^deps/cmake/.*"`"
    local deps_changed="${str_1}${str_2}"
    if [ "${deps_changed}" != "" ] ; then
        echo "deps changed: `msg_red ${file}`"
        DEPS_SRC_CHANGED=true
    fi
}

case "${1}" in
    --about)
        echo "Checking for changed files contents."
        ;;
    *)
        for file in $(git diff-index --cached --name-only HEAD); do
            check_filecontents "${file}"
        done
        ;;
esac

if ${DEPS_SRC_CHANGED}; then
    update_deps_sha1_marker
fi
