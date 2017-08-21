# Debian/Ubuntu

- install prerequisite tools and libraries, run:
  ```
  $ sudo apt install cmake libtool automake pkg-config gettext openssl git python
  ```
- about qt5
  - install shared qt5, or
  - download the qt5 source, config and build the static to use, see **tools/scripts/qt5-static-cfg.linux**
    for details
- do local config by copy and modify **contrib/local.mk.eg**
- run: `$ make`
- run: `$ make install`