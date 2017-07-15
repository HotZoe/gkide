#!/bin/bash
#

function run_test()
{
    grep '^Signed-off-by: ' "${1}" >/dev/null ||
    {
        echo "The commit message must have a Signed-off-by line."
        exit 1
    }
}

case "${1}" in
    --about )
        echo -n "Checks commit message for presence of Signed-off-by line."
        ;;
    * )
        run_test "$@"
        ;;
esac
