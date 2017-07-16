#!/bin/bash
#
# check the file contents

function test_file()
{
    file="${1}"

    if [ ! -f ${file} ]; then
        #echo "delete file: ${file}"
        return
    fi
}

case "${1}" in
    --about)
        echo "Check for changed files contents."
        ;;
    *)
        for file in $(git diff-index --cached --name-only HEAD); do
            test_file "${file}"
        done
        ;;
esac
