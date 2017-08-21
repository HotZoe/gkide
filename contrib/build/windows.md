# Windows(MSYS2)

- install [Msys2](http://www.msys2.org/)
- in **MSYS** shell install prerequisite libraries, run:
  ```
  $ pacman -S gperf unzip git

  #######################################################################################
  # for 32-bit build

  # using dynamic/static linking qt5 for snail(do not need all, one is enough)
  $ pacman -S mingw-w64-i686-qt5
  $ pacman -S mingw-w64-i686-qt5-static

  $ pacman -S mingw-w64-i686-gcc mingw-w64-i686-libtool mingw-w64-i686-make \
              mingw-w64-i686-pkg-config mingw-w64-i686-unibilium mingw-w64-i686-cmake \
              mingw-w64-i686-perl mingw-w64-i686-python2

  #######################################################################################
  # for 64-bit build

  # using dynamic/static linking qt5 for snail(do not need all, one is enough)
  $ pacman -S mingw-w64-x86_64-qt5
  $ pacman -S mingw-w64-x86_64-qt5-static

  $ pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-libtool mingw-w64-x86_64-make \
              mingw-w64-x86_64-pkg-config mingw-w64-x86_64-unibilium mingw-w64-x86_64-cmake \
              mingw-w64-x86_64-perl mingw-w64-x86_64-python2
  ```
- do local configuration:
  - `contrib/local.mk.eg` => `local.mk`
  - `contrib/mingw-w64.eg` => `mingw-w64`
- open `tools/cmder_mini/Cmder.exe` shell, then run: `$ make`

# About Qt & MSYS2

- [MSYS2 based Qt binary builds(pre-built)](https://github.com/Alexpux/MINGW-packages)
- [About MinGW-w64, MinGW, MSYS, MSYS2 and Qt 5](http://wiki.qt.io/MinGW-64-bit)
- [MinGW-builds based Qt( v5.2.1) binary builds(pre-built)](https://sourceforge.net/projects/mingwbuilds/files/external-binary-packages/Qt-Builds/)