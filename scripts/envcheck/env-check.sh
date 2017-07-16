#!/bin/sh

export qt_lib_path=${1}

export os_macos="false"
export os_linux="false"
export os_windows="false"
export os_cygwin="false"
export os_mingw="false"
export os_msys="false"

export check_status="true"

host_os=""

os_type=$(uname -a | grep '[L|l]inux')
if [ "$os_type" != "" ]; then
    os_linux="true"
    host_os="Linux"
fi

os_type=$(uname -a | grep '[C|c]ygwin')
if [ "$os_type" != "" ]; then
    os_windows="true"
    os_cygwin="true"
        host_os="Windows/Cygwin"
fi

# check for bash and /bin/sh
bash --version | head -n1 | cut -d" " -f2-4
prog=$(readlink -f /bin/sh)
echo "/bin/sh -> ${prog}"
echo ""

if [ "${qt_lib_path}" = "" ]; then
    check_status="false"
    echo "Not Found: \033[31mQt5\033[0m"
    echo "See 'contrib/local.mk.eg' for details\n"
fi

echo "\033[33mChecking Prerequisite Env\033[0m (\033[34m${host_os}\033[0m)"
# prerequisite check first
scripts/envcheck/prerequisite.sh
echo "\033[33mChecking Development Env\033[0m (\033[34m${host_os}\033[0m)"

prog=$(which grep)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo "Not Found: \033[31mgrep\033[0m"
else
    echo "Found: \033[32m${prog}\033[0m =>" $(grep --version | head -n1)
fi

prog=$(which python)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo "Not Found: \033[31mpython\033[0m"
else
    echo "Found: \033[32m${prog}\033[0m =>" $(python --version 2>&1)
fi

prog=$(which node)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo "Not Found: \033[31mnode\033[0m"
else
    echo "Found: \033[32m${prog}\033[0m =>" $(node --version | head -n1)
fi

prog=$(which npm)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo "Not Found: \033[31mnpm\033[0m"
else
    echo "Found: \033[32m${prog}\033[0m =>" $(npm --version | head -n1)
fi

prog=$(which validate-commit-msg)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo "Not Found: \033[31mvalidate-commit-msg\033[0m"
else
    echo "Found: \033[32m${prog}\033[0m"
fi

prog=$(which standard-version)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo "Not Found: \033[31mstandard-version\033[0m"
else
    echo "Found: \033[32m${prog}\033[0m =>" $(standard-version --version | head -n1)
fi

if ${check_status}; then
    echo "\033[33mChecking Development Env\033[0m => \033[36mPASS\033[0m"
else
    echo "\033[33mChecking Development Env\033[0m => \033[31mFAIL\033[0m"
fi
