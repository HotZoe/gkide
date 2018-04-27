#!/usr/bin/env bash

function check_filecontents()
{
    file="${1}"

    if [ ! -f ${file} ]; then
        echo "delete : `msg_red ${file}`"
        return 1
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
