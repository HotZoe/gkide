# Quick Reference for Native Build of CMake ENV

|  ENV\\(Host,Target)  |(Linux,Linux) | (Windows,Windows) | (MacOS,MacOS) |
|:--------------------:|:------------:|:-----------------:|:-------------:|
|        MINGW         |    false     |       true        |     false     |
| CMAKE_CROSSCOMPILING |    false     |       false       |     false     |
|        UNIX          |    true      |       false       |     true      |
|        WIN32         |    false     |       true        |     false     |
|        APPLE         |    false     |       false       |     true      |
|   CMAKE_SYSTEM_NAME  |   "Linux"    |     "Windows"     |    "Darwin"   |
|    CMAKE_HOST_UNIX   |    true      |       false       |     true      |
|    CMAKE_HOST_WIN32  |    false     |       true        |     false     |
|    CMAKE_HOST_APPLE  |    false     |       false       |     true      |
|CMAKE_HOST_SYSTEM_NAME|   "Linux"    |     "Windows"     |    "Darwin"   |

# [Macos](macos.md)
  - macOS 10.10, 10.11, 10.12
  - clang, xcode
  - 32-bit, 64-bit

# [Windows](windows.md)
  - windows 7, 8, 10
  - mingw-w64-gcc
  - 32-bit, 64-bit

# Linux(32-bit, 64-bit)
  - [Debian](debian.md)
  - [Ubuntu](debian.md)

# Generated The Details Build Log
- run: `$ make V=1 VERBOSE=1 | tee build.log`
- run: `$ mingw32-make V=1 VERBOSE=1 | tee build.log`
