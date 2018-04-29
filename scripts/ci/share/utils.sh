function stat_cmd()
{
    if test "${TRAVIS_OS_NAME}" = osx ; then
        stat -f %Sm "${@}"
    else
        stat -c %y "${@}"
    fi
}

function all_done_successfully()
{
    if test -f "${TEST_FAIL_SUMMARY_LOG}" ; then
        echo 'Test failed, complete summary:'
        cat "${TEST_FAIL_SUMMARY_LOG}"
        return 1
    fi

    if [ -f ${MARKER_ALL_DONE} ]; then
        return 0
    fi

    return 1
}
