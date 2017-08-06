# Quick Reference for Native & Cross Build of CMake ENV

|  ENV\\(Host,Target)  |(Linux,Linux) | (Linux,Windows) | (Windows,Windows) | (MacOS,MacOS) |
|:--------------------:|:------------:|:---------------:|:-----------------:|:-------------:|
|        MINGW         |    false     |      true       |       true        |     false     |
| CMAKE_CROSSCOMPILING |    false     |      true       |       false       |     false     |
|        UNIX          |    true      |      false      |       false       |     true      |
|        WIN32         |    false     |      true       |       true        |     false     |
|        APPLE         |    false     |      false      |       false       |     true      |
|   CMAKE_SYSTEM_NAME  |   "Linux"    |    "Windows"    |     "Windows"     |    "Darwin"   |
|    CMAKE_HOST_UNIX   |    true      |      true       |       false       |     true      |
|    CMAKE_HOST_WIN32  |    false     |      false      |       true        |     false     |
|    CMAKE_HOST_APPLE  |    false     |      false      |       false       |     true      |
|CMAKE_HOST_SYSTEM_NAME|   "Linux"    |     "Linux"     |     "Windows"     |    "Darwin"   |

- All dependencies libraries for `nvim` are always using static linking.
- static/shared Qt5 are both supported for snail.
  - To build **Release** or **MinSizeRel** type of `snail` using static Qt5 is recommend.
  - To build `snail` using static Qt5 will get a little bigger output, and depends none Qt shared libraries.
  - To build `snail` using shared Qt5 will get much smaller output, but depends on many Qt shared libraries.

## Host(Macos), Target(Macos)

- not supported yet!

## Host(Windows), Target(Windows)
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
  - do local configuration, [template](local.mk.eg) is `contrib/local.mk.eg`
  - open `tools/cmder_mini/Cmder.exe` shell, then run: `$ make`

## Host(Linux), Target(Windows)
[mingw_w64_packages_on_ubuntu_url]: https://launchpad.net/ubuntu/+source/mingw-w64
[mingw_w64_packages_on_debian_url]: https://packages.debian.org/sid/mingw-w64

- [Mingw-w64 packages on Ubuntu][mingw_w64_packages_on_ubuntu_url]
- [Mingw-w64 packages on Debian][mingw_w64_packages_on_debian_url]

- not supported yet!

## Host(Linux), Target(Linux)

### Debian & Ubuntu
- build pass, to write the build tips

# About Qt & MSYS2

- [MSYS2 based Qt binary builds(pre-built)](https://github.com/Alexpux/MINGW-packages)
- [About MinGW-w64, MinGW, MSYS, MSYS2 and Qt 5](http://wiki.qt.io/MinGW-64-bit)
- [MinGW-builds based Qt( v5.2.1) binary builds(pre-built)](https://sourceforge.net/projects/mingwbuilds/files/external-binary-packages/Qt-Builds/)

# Generated The Details Build Log
- run: `$ make V=1 VERBOSE=1 | tee build.log`
- run: `$ mingw32-make V=1 VERBOSE=1 | tee build.log`