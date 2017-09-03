#!/usr/bin/env bash

function toolchain_check_linux()
{
    prog=$(which gcc 2> /dev/null)
    if [ "${prog}" = "" ]; then
        check_status="false"
        echo -e "Not Found: \033[31mgcc\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(gcc --version | head -n1)
    fi

    prog=$(which g++ 2> /dev/null)
    if [ "${prog}" = "" ]; then
        check_status="false"
        echo -e "Not Found: \033[31mg++\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(g++ --version | head -n1)
    fi

    prog=$(which ldd 2> /dev/null)
    if [ "${prog}" = "" ]; then
        check_status="false"
        echo -e "Not Found: \033[31mldd\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(ldd --version | head -n1 | cut -d" " -f2-)
    fi
}

function toolchain_check_macos()
{
    toolchain_gcc="true"
    toolchain_clang="true"

    prog=$(which gcc 2> /dev/null)
    if [ "${prog}" = "" ]; then
        toolchain_gcc="false"
        echo -e "Not Found: \033[31mgcc\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(gcc --version 2> /dev/null | head -n1)
    fi

    prog=$(which g++ 2> /dev/null)
    if [ "${prog}" = "" ]; then
        toolchain_gcc="false"
        echo -e "Not Found: \033[31mg++\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(g++ --version 2> /dev/null | head -n1)
    fi

    prog=$(which ldd 2> /dev/null)
    if [ "${prog}" = "" ]; then
        toolchain_gcc="false"
        echo -e "Not Found: \033[31mldd\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(ldd --version 2> /dev/null | head -n1 | cut -d" " -f2-)
    fi

    prog=$(which clang 2> /dev/null)
    if [ "${prog}" = "" ]; then
        toolchain_clang="false"
        echo -e "Not Found: \033[31mclang\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(clang --version 2> /dev/null | head -n1)
    fi

    prog=$(which clang++ 2> /dev/null)
    if [ "${prog}" = "" ]; then
        toolchain_clang="false"
        echo -e "Not Found: \033[31mclang++\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(clang++ --version 2> /dev/null | head -n1)
    fi

    prog=$(which otool 2> /dev/null)
    if [ "${prog}" = "" ]; then
        toolchain_clang="false"
        echo -e "Not Found: \033[31mldd\033[0m"
    else
        echo -e "Found: \033[32m${prog}\033[0m =>" $(otool --version 2> /dev/null | head -n1 | cut -d" " -f2-)
    fi

    if [ ! ${toolchain_clang} -a ! ${toolchain_gcc} ]; then
        check_status="false"
    fi
}

function toolchain_check_windows()
{
    toolchain_gcc="true"

    check_status_x64_gcc=$(shell which x86_64-w64-mingw32-gcc 2> ${null_device})
    check_status_x64_gxx=$(shell which x86_64-w64-mingw32-g++ 2> ${null_device})
    check_status_x32_gcc=$(shell which i686-w64-mingw32-gcc 2> ${null_device})
    check_status_x32_gxx=$(shell which i686-w64-mingw32-g++ 2> ${null_device})
    check_status_gnu_ldd=$(shell which ldd 2> ${null_device})

    if [ "${check_status_x32_gcc}" != "" ]; then
        echo -e "Found: \033[32m${check_status_x32_gcc}\033[0m =>" $(i686-w64-mingw32-gcc --version | head -n1)
    elif [ "${check_status_x64_gcc}" != "" ]; then
        echo -e "Found: \033[32m${check_status_x64_gcc}\033[0m =>" $(x86_64-w64-mingw32-gcc --version | head -n1)
    else
        toolchain_gcc="false"
        echo -e "Not Found: \033[31mi686-w64-mingw32-gcc\033[0m"
        echo -e "Not Found: \033[31mx86_64-w64-mingw32-gcc\033[0m"
    fi

    if [ "${check_status_x32_gxx}" != "" ]; then
        echo -e "Found: \033[32m${check_status_x32_gxx}\033[0m =>" $(i686-w64-mingw32-g++ --version | head -n1)
    elif [ "${check_status_x64_gxx}" != "" ]; then
        echo -e "Found: \033[32m${check_status_x64_gxx}\033[0m =>" $(x86_64-w64-mingw32-g++ --version | head -n1)
    else
        toolchain_gcc="false"
        echo -e "Not Found: \033[31mi686-w64-mingw32-g++\033[0m"
        echo -e "Not Found: \033[31mx86_64-w64-mingw32-g++\033[0m"
    fi

    if [ "${check_status_gnu_ldd}" != "" ]; then
        echo -e "Found: \033[32m${check_status_gnu_ldd}\033[0m =>" $(ldd --version | head -n1 | cut -d" " -f2-)
    else
        toolchain_gcc="false"
        echo -e "Not Found: \033[31mldd\033[0m"
    fi

    if [ ! ${toolchain_gcc} ]; then
        check_status="false"
    fi
}

if ${host_macos}; then
    toolchain_check_macos
fi

if ${host_linux}; then
    toolchain_check_linux
fi

if ${host_windows}; then
    toolchain_check_windows
fi

prog=$(which git 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mgit\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(git --version | head -n1)
fi

prog=$(which cmake 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mcmake\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(cmake --version | head -n1)
fi

prog=$(which curl 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mcurl\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(curl --version | head -n1 | cut -d" " -f1-3)
fi

prog=$(which libtool 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mlibtool\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(libtool -V | head -n1)
fi

prog=$(which autoconf 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mautoconf\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(autoconf --version | head -n1)
fi

prog=$(which automake 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mautomake\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(automake --version 2> ${null_device} | head -n1)
fi

prog=$(which m4 2> ${null_device})
if [ "${prog}" = "" ]; then
    prog=$(which gm4 2> ${null_device})
    if [ "${prog}" = "" ]; then
        prog=$(which gnum4 2> ${null_device})
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

prog=$(which unzip 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31munzip\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(unzip -v 2>&1 | head -n1)
fi

prog=$(which bzip2 2> ${null_device})
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mbzip2\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m"
fi

if ${check_status}; then
    echo -e "\033[33mChecking Prerequisite Env\033[0m => \033[36mPASS\033[0m (nvim & snail can be build fine)"
else
    echo -e "\033[33mChecking Prerequisite Env\033[0m => \033[31mFAIL\033[0m (nvim & snail can not be build fine)"
fi

echo ""
