#!/bin/sh

# for debian, ubuntu, ...

# install tools
sudo apt install cmake
sudo apt install libtool
sudo apt install libtool-bin
sudo apt install automake
sudo apt install pkg-config
sudo apt install gettext
sudo apt install openssl
sudo apt install git
sudo apt install curl
sudo apt install python
sudo apt install cppcheck
sudo apt install libx11-dev
sudo apt install gdb
sudo apt install unzip
sudo apt install locales
sudo apt install autoconf
sudo apt install g++-multilib
sudo apt install gcc-multilib
sudo apt install libc6-dev-i386
sudo apt install build-essential

# install dependencies libraries development version
sudo apt install libfontconfig1-dev
sudo apt install libfreetype6-dev
sudo apt install libx11-dev
sudo apt install libxext-dev
sudo apt install libxfixes-dev
sudo apt install libxi-dev
sudo apt install libxrender-dev
sudo apt install libxcb1-dev
sudo apt install libx11-xcb-dev
sudo apt install libxcb-glx0-dev
sudo apt install libicu-dev
sudo apt install libglib2.0-dev

# git commit style configurations following

# make sure working directory file's EOL is always LF
git config core.eol lf

# make sure no mixing LF and CRLF
git config core.safecrlf true

# make sure convert CRLF to LF if any
git config core.autocrlf input

# git commit style configuration
git config commit.template "${PWD}/.gitcommitstyle"

# git hooks directory configuration
# NOTE:
# - git version should be higher than 2.9.0
# - for older git version, do config by hand, see the hooks file header
git config core.hooksPath "${PWD}/scripts/githooks"

# install nodejs
sudo apt install npm
sudo apt install nodejs
sudo apt install nodejs-legacy

sudo npm install -g validate-commit-msg
sudo npm install -g standard-version

