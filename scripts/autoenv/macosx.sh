#!/bin/sh

# for macosx

# install tools
brew install cmake
brew install libtool
brew install automake
brew install pkg-config
brew install gettext
brew install openssl
brew install git
brew install python
brew install cppcheck
brew install libx11-dev

# install dependencies libraries development version
brew install libfontconfig1-dev
brew install libfreetype6-dev
brew install libx11-dev
brew install libxext-dev
brew install libxfixes-dev
brew install libxi-dev
brew install libxrender-dev
brew install libxcb1-dev
brew install libx11-xcb-dev
brew install libxcb-glx0-dev
brew install libicu-dev
brew install libglib2.0-dev

# git commit style configurations following

# make sure working directory file's EOL is always LF
git config core.eol lf

# make sure no mixing LF and CRLF
git config core.safecrlf true

# make sure convert CRLF to LF if any
git config core.autocrlf input

# git commit style configuration
git config commit.template ${PWD}/.gitcommitstyle

# install nodejs
brew install npm
brew install node

npm install -g validate-commit-msg
npm install -g standard-version

# install the Qt5, e.g. Qt5.7.1
brew install qt@5.7.1

