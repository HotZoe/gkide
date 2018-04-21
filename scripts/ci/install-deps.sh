#!/usr/bin/env bash

set -e
set -o pipefail

echo "Install any dependencies that required ..."

if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
    brew update
    install_package="brew install"
else
    sudo apt-get -qq update
    install_package="sudo apt-get install -y"
fi

${install_package} cmake
${install_package} libtool
${install_package} automake
${install_package} pkg-config
${install_package} gettext
${install_package} openssl
${install_package} curl
${install_package} python
${install_package} cppcheck
${install_package} libx11-dev
${install_package} gdb
${install_package} unzip
${install_package} locales
${install_package} autoconf
${install_package} g++-multilib
${install_package} gcc-multilib
${install_package} libc6-dev-i386
${install_package} build-essential

if test "${BUILD_NVIM_ONLY}" != ON ; then
    if test ${USE_SHARED_QT5} = ON; then
        ${install_package} qt5-default
    else
        echo "[TODO]: install and config static Qt5 library."
        exit 1
    fi
fi
