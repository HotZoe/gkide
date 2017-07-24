#!/bin/sh

export qt_lib_path=${1}
export target_os=${2}

export host_macos="false"
export host_linux="false"
export host_windows="false"

export win_msys="false"
export win_mingw="false"

export check_status="true"

host_os="Unknown"

os_type=$(uname -a | grep '[L|l]inux')
if [ "${os_type}" != "" ]; then
    host_os="Linux"
    host_linux="true"
fi

os_type=$(uname -a | grep '^MINGW[3|6][2|4]_NT*')
if [ "${os_type}" != "" ]; then
    win_mingw="true"
    host_windows="true"
    host_os="Windows"
    win_check_shell=", MinGW-Shell checking"
fi

os_type=$(uname -a | grep '^MSYS_NT-*')
if [ "${os_type}" != "" ]; then
    win_msys="true"
    host_windows="true"
    host_os="Windows"
    win_check_shell=", MSYS-Shell checking"
fi

if [ "${host_os}" = "Unknown" ]; then
    echo -e "\033[31mError\033[0m: Host System Unknown"
    exit 1
fi

if [ "${target_os}" = "" ]; then
    target_os="${host_os}"
fi

echo -e "Host System  : \033[33m${host_os}\033[0m${win_check_shell}"
echo -e "Target System: \033[33m${target_os}\033[0m\n"

# check for bash and /bin/sh
bash --version | head -n1 | cut -d" " -f2-4
prog=$(readlink -f /bin/sh)
echo "/bin/sh -> ${prog}"
echo ""

if [ "${qt_lib_path}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mQt5\033[0m"
    echo -e "See \033[33mcontrib/local.mk.eg\033[0m for details\n"
else
    echo -e "Found Qt5: \033[32m${qt_lib_path}\033[0m\n"
fi

echo -e "\033[33mChecking Prerequisite Env\033[0m (\033[34m${host_os}\033[0m)"
# prerequisite check first
scripts/envcheck/prerequisite.sh
echo -e "\033[33mChecking Development Env\033[0m (\033[34m${host_os}\033[0m)"

prog=$(which grep 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mgrep\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(grep --version | head -n1)
fi

prog=$(which python 2>/dev/null)
if [ "${prog}" = "" ]; then
    check_status="false"
    echo -e "Not Found: \033[31mpython\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(python --version 2>&1)
fi

if ${check_status}; then
    echo -e "\033[33mChecking Development Env\033[0m => \033[36mPASS\033[0m"
else
    echo -e "\033[33mChecking Development Env\033[0m => \033[31mFAIL\033[0m"
fi

echo -e "\nFor \033[33mgit\033[0m commit use only:\n"

prog=$(which node 2>/dev/null)
if [ "${prog}" = "" ]; then
    echo -e "Not Found: \033[31mnode\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(node --version | head -n1)
fi

prog=$(which npm 2>/dev/null)
if [ "${prog}" = "" ]; then
    echo -e "Not Found: \033[31mnpm\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(npm --version | head -n1)
fi

prog=$(which validate-commit-msg 2>/dev/null)
if [ "${prog}" = "" ]; then
    echo -e "Not Found: \033[31mvalidate-commit-msg\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m"
fi

prog=$(which standard-version 2>/dev/null)
if [ "${prog}" = "" ]; then
    echo -e "Not Found: \033[31mstandard-version\033[0m"
else
    echo -e "Found: \033[32m${prog}\033[0m =>" $(standard-version --version | head -n1)
fi

echo -e "\nIf missing something, you can seak for help following:"
echo -e "    \033[33mcontrib/local.mk.eg\033[0m"
echo -e "    \033[33mcontrib/GitConfig.md\033[0m"
echo -e "    \033[33mcontrib/BuildInstall.md\033[0m"