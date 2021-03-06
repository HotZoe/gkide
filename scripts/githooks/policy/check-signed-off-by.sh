#!/usr/bin/env bash

GIT_REPO_DIR="$(cd ${PWD} && pwd)"
POLICY_DIR="${GIT_REPO_DIR}/scripts/githooks/policy"
source "${POLICY_DIR}/utils.sh"

function check_signed_off_by()
{
    grep '^Signed-off-by: ' "${1}" >/dev/null || {
        echo "The commit message must have a Signed-off-by line."
        return 1
    }

    # catches duplicate Signed-off-by lines.
    test "" = "$(grep '^Signed-off-by: ' "$1" | sort | uniq -c | sed -e '/^[ 	]*1[ 	]/d')" ||
    {
        echo >&2 "Duplicate Signed-off-by lines."
        return 1
    }

}

case "${1}" in
    --about)
        echo -n "Checks commit message for presence of Signed-off-by line."
        ;;
    * )
        signingkey=$(git config --get user.signingkey)
        if [ "${signingkey}" = "" ]; then
            echo "Please set your GPG signingkey, run:"
            echo "    $ `msg_red 'git config --global user.signingkey <YourGpgKeyID>'`"
            exit 1
        fi

        # This maybe a bad idea, so just do not use it
        #
        # gpgsign=$(git config --bool commit.gpgsign)
        # if [ "${gpgsign}" != "true" ]; then
        #     echo "For automatically make GPG signed for all commit, run:"
        #     echo -e "    $\033[33m git config commit.gpgsign true\033[0m"
        # fi

        check_signed_off_by "$@"
        ;;
esac
