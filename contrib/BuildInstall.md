# About Qt

- [About MinGW-w64, MinGW, MSYS, MSYS2 and Qt 5](http://wiki.qt.io/MinGW-64-bit)
- [MinGW-builds based Qt( v5.2.1) binary builds(pre-built)](https://sourceforge.net/projects/mingwbuilds/files/external-binary-packages/Qt-Builds/)
- [MSYS2 based Qt binary builds(pre-built)](https://github.com/Alexpux/MINGW-packages)

# Host(Linux), Target(Linux)
# Host(Macos), Target(Macos)
# Host(Linux), Target(Windows)

[mingw_w64_packages_on_ubuntu_url]: https://launchpad.net/ubuntu/+source/mingw-w64

- [Mingw-w64 packages on Ubuntu][mingw_w64_packages_on_ubuntu_url]

# Host(Windows), Target(Windows)
- install [Msys2](http://www.msys2.org/)
- if you are chinese, you may want to change the source to get fast download, then try the following:
  - add `Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/msys/$arch` to the head of file: `/etc/pacman.d/mirrorlist.msys`
  - add `Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/mingw/i686` to the head of file: `/etc/pacman.d/mirrorlist.mingw32`
  - add `Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/mingw/x86_64` to the head of file: `/etc/pacman.d/mirrorlist.mingw64`
  - You can find other mirros that have Msys2 if you want!
    See [here](http://blog.csdn.net/qiuzhiqian1990/article/details/56671839) to get more about this!

  - Here are some [materials](contrib/misc-doc/misc-doc-cross-compiler-toolchain.md) about cross compiler
    toolchains that maybe help.
# Host(Ubuntu)

# Host(Debian)

[mingw_w64_packages_on_debian_url]: https://packages.debian.org/sid/mingw-w64
- [Mingw-w64 packages on Debian][mingw_w64_packages_on_debian_url]