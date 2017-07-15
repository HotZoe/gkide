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

    if grep 'Copyright (c)' "${file}" | grep -v 'grep' >/dev/null 2>&1 ; then
        set -e
        grep "Redistribution and use in source and binary forms, with or without" "${file}" >/dev/null
        grep "Redistributions of source code must retain the above copyright" "${file}" >/dev/null
        grep "Redistributions in binary form must reproduce the above copyright" "${file}" >/dev/null
        grep "Neither the name of the project nor the" "${file}" >/dev/null
        set +e
    fi
}

case "${1}" in
    --about)
        echo "Check for changed files contents."
        ;;
    *)
        for file in `git diff-index --cached --name-only HEAD` ; do
            test_file "${file}"
        done
        ;;
esac
