#!/usr/bin/env bash

function run_test()
{
    grep '^Signed-off-by: ' "${1}" >/dev/null ||
    {
        echo "The commit message must have a Signed-off-by line."
        exit 1
    }

    # catches duplicate Signed-off-by lines.
    test "" = "$(grep '^Signed-off-by: ' "$1" | sort | uniq -c | sed -e '/^[ 	]*1[ 	]/d')" ||
    {
        echo >&2 Duplicate Signed-off-by lines.
        exit 1
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
            echo -e "    $\033[33m git config --global user.signingkey <YourGpgKeyID>\033[0m"
            exit 0
        fi

        # This maybe a bad idea, so just do not use it
        #
        # gpgsign=$(git config --bool commit.gpgsign)
        # if [ "${gpgsign}" != "true" ]; then
        #     echo "For automatically make GPG signed for all commit, run:"
        #     echo -e "    $\033[33m git config commit.gpgsign true\033[0m"
        # fi

        run_test "$@"
        ;;
esac
