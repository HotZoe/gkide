function stat_cmd() 
{
    if test "${TRAVIS_OS_NAME}" = osx ; then
        stat -f %Sm "${@}"
    else
        stat -c %y "${@}"
    fi
}
