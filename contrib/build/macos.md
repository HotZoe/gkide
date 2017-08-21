# MacOS

- install **brew**, run:
  ```
  /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
  ```
- install prerequisite tools and libraries, run:
  ```
  $ brew install cmake libtool automake pkg-config gettext openssl
  ```
- about qt5
  - for shared qt5, run: `$ brew search qt`, based on the result, selected qt version 5 to install,
    e.g, run: `$ brew install qt@5.7.1`
  - for static qt5, download the qt5 source, config and build, see **tools/scripts/qt5-static-cfg.macos**
    for details
- do local config by copy and modify **contrib/local.mk.eg**
- run: `$ make`
- run: `$ make install`