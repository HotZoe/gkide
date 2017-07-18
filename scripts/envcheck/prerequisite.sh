#!/bin/sh

function cc_check_linux()
{
    prog=$(which gcc 2>/dev/null)
    if [ "${prog}" = "" ]; then
        check_status="false"
        echo -e "Not Found: \033[31mgcc\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(gcc --version | head -n1)
    fi

    prog=$(which g++ 2>/dev/null)
    if [ "${prog}" = "" ]; then
        check_status="false"
        echo -e "Not Found: \033[31mg++\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(g++ --version | head -n1)
    fi

    prog=$(which ldd 2>/dev/null)
    if [ "${prog}" = "" ]; then
        check_status="false"
        echo -e "Not Found: \033[31mldd\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(ldd --version | head -n1 | cut -d" " -f2-)
    fi
}

if ${os_macos}; then
    echo "todo for macos env checking"
fi

if ${os_windows}; then
    if ${os_cygwin}; then
        cc_check_linux
    fi

    if ${os_mingw}; then
        check_status="false"
        echo "todo for windows/mingw env checking"
    fi

    if ${os_msys}; then
        check_status="false"
        echo "todo for windows/msys env checking"
    fi
fi

if ${os_linux}; then
    cc_check_linux
fi

prog=$(which git 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mgit\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(git --version | head -n1)
fi

prog=$(which cmake 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mcmake\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(cmake --version | head -n1)
fi

prog=$(which curl 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mcurl\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(curl --version | head -n1 | cut -d" " -f1-3)
fi

prog=$(which libtool 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mlibtool\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(libtool --version | head -n1)
fi

prog=$(which autoconf 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mautoconf\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(autoconf --version | head -n1)
fi

prog=$(which automake 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mautomake\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(automake --version 2>/dev/null | head -n1)
fi

prog=$(which m4 2>/dev/null)
if [ "${prog}" = "" ]; then
    prog=$(which gm4 2>/dev/null)
    if [ "${prog}" = "" ]; then
        prog=$(which gnum4 2>/dev/null)
        if [ "${prog}" = "" ]; then
            check_status="false"
            echo -e "Not Found: \033[31mm4  gm4  gnum4\033[0m"
        else
            echo -e "Found: \033[32m${prog}\033[0m =>" $(gnum4 --version | head -n1)
        fi
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(gm4 --version | head -n1)
    fi
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(m4 --version | head -n1)
fi

prog=$(which unzip 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31munzip\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(unzip -v 2>&1 | head -n1)
fi

prog=$(which bzip2 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mbzip2\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m"
fi

if ${check_status}; then
    echo -e "\033[33mChecking Prerequisite Env\033[0m => \033[36mPASS\033[0m"
else
    echo -e "\033[33mChecking Prerequisite Env\033[0m => \033[31mFAIL\033[0m"
fi

echo ""