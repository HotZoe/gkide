#!/bin/bash

function run_test()
{
    which aspell > /dev/null
    if [ ! $? -eq 0 ] ; then
        exit 0
    fi

    set -e

    warnings=$(cat "${1}" | grep -v '^#.*' | aspell list)

    if [ ! -z "${warnings}" ] ; then
        echo >&2 "Possible spelling errors in the commit message:"
        echo -e >&2 "    \033[31m"${warnings}"\033[0m\n";
        exit 1
    fi
}

case "${1}" in
    --about)
        echo -n "Spell check the commit message using aspell which is: "
        which aspell > /dev/null
        if [ ! $? -eq 0 ] ; then
            echo "not installed"
        else
            echo "installed"
        fi
        ;;
    *)
        run_test "$@"
        ;;
esac
