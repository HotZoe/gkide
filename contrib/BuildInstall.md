[mingw_w64_packages_on_ubuntu_url]: https://launchpad.net/ubuntu/+source/mingw-w64
[mingw_w64_packages_on_debian_url]: https://packages.debian.org/sid/mingw-w64

# About Qt & MSYS2

- [MSYS2 based Qt binary builds(pre-built)](https://github.com/Alexpux/MINGW-packages)
- [About MinGW-w64, MinGW, MSYS, MSYS2 and Qt 5](http://wiki.qt.io/MinGW-64-bit)
- [MinGW-builds based Qt( v5.2.1) binary builds(pre-built)](https://sourceforge.net/projects/mingwbuilds/files/external-binary-packages/Qt-Builds/)


# Host(Macos), Target(Macos)
# Host(Linux), Target(Linux)
# Host(Linux), Target(Windows)

- [Mingw-w64 packages on Ubuntu][mingw_w64_packages_on_ubuntu_url]
- [Mingw-w64 packages on Debian][mingw_w64_packages_on_debian_url]

# Host(Windows), Target(Windows)
  - install [Msys2](http://www.msys2.org/)
  - in **MSYS** shell, run:
    ```
    $ pacman -S gperf unzip git

    # for 32-bit build
    $ pacman -S mingw-w64-i686-gcc mingw-w64-i686-libtool mingw-w64-i686-make \
                mingw-w64-i686-pkg-config mingw-w64-i686-unibilium mingw-w64-i686-cmake \
                mingw-w64-i686-perl mingw-w64-i686-python2

    # for 64-bit build
    $ pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-libtool mingw-w64-x86_64-make \
                mingw-w64-x86_64-pkg-config mingw-w64-x86_64-unibilium mingw-w64-x86_64-cmake \
                mingw-w64-x86_64-perl mingw-w64-x86_64-python2
    ```
  - in **MinGW32** shell for 32-bit build OR **MinGW64** shell for 64-bit build, run:
  ```
  $ mingw32-make
  ```

# Local Makefile Configurations
Local configuration [template](contrib/local.mk.eg) is: `contrib/local.mk.eg`

# Generated The Details Build Log
- run: `make VERBOSE=1 | tee build.log`
- run: `mingw32-make VERBOSE=1 | tee build.log`