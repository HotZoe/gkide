#!/bin/sh

# windows MSYS2

build_arch=$1
static_qt_lib=$2

if [ "${build_arch}" = "x86" ]; then
    pkg_prefix="mingw-w64-i686"
elif [ "${build_arch}" = "x86_64" ]; then
    pkg_prefix="mingw-w64-x86_64"
else
    echo "Arch Error: ${pkg_prefix}, should be 'x86' or 'x86_64'"
    exit 0
fi

if [ "${static_qt_lib}" = "static" ]; then
    pkg_suffix="-static"
else
    pkg_suffix=
fi

# install tools
pacman -S gperf
pacman -S unzip
pacman -S git
pacman -S cmake
pacman -S libtool
pacman -S automake
pacman -S pkg-config
pacman -S gettext
pacman -S openssl
pacman -S python
pacman -S cppcheck

pacman -S ${pkg_prefix}-qt5${pkg_suffix}

pacman -S ${pkg_prefix}-git
pacman -S ${pkg_prefix}-gcc
pacman -S ${pkg_prefix}-libtool
pacman -S ${pkg_prefix}-make
pacman -S ${pkg_prefix}-pkg-config
pacman -S ${pkg_prefix}-unibilium
pacman -S ${pkg_prefix}-cmake
pacman -S ${pkg_prefix}-perl
pacman -S ${pkg_prefix}-python2 
pacman -S ${pkg_prefix}-jasper

# git commit style configurations following

# make sure working directory file's EOL is always LF
git config core.eol lf

# make sure no mixing LF and CRLF
git config core.safecrlf true

# make sure convert CRLF to LF if any
git config core.autocrlf input

# git commit style configuration
git config core.filemode false
cp .gitcommitstyle gitcommitstyle
git config commit.template ${PWD}/gitcommitstyle
    
# install nodejs
# todo
# npm install -g validate-commit-msg
# npm install -g standard-version

