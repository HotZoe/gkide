#!/bin/bash

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
        run_test "$@"
        ;;
esac
