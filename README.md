# GKIDE

[![Conventional Commits](https://img.shields.io/badge/Conventional%20Commits-1.0.0-yellow.svg)](https://conventionalcommits.org)
[![Travis Build Status](https://www.travis-ci.org/gkide/gkide.svg?branch=master)](https://www.travis-ci.org/gkide/gkide)
[![Coverage Status](https://coveralls.io/repos/github/gkide/gkide/badge.svg)](https://coveralls.io/github/gkide/gkide)
[![Stories in Ready](https://badge.waffle.io/gkide/gkide.svg?label=ready&title=Ready)](http://waffle.io/gkide/gkide)


GKIDE is an elegant and sophisticated code editor based on vim/nvim.

# What is GKIDE exactly?

...

# Layout

    ├─ cmake/         CMake Custom Build Scripts for GKIDE
    ├─ contrib/       About contribution and local settings
    ├─ deps/          CMake subproject to build dependencies
	├─ license/       About the license and related things
    ├─ scripts/       Helpfully scripts
    ├─ source/config  Configuration for nvim & snail
    ├─ source/nvim    Source code of nvim
    ├─ source/snail   Source code of snail
    ├─ test/nvim      Testing code about nvim
    └─ test/snail     Testing code about snail

# Build & Install

- step 1: install **Qt5**
- step 2: run `$ make env-check` to check the build environment
- step 3: fix the missings of the build environment and do config
- step 4: run `$ make` to build
- step 5: run `$ make install` to install

See the [contrib/build/build.md](contrib/build/build.md) for details.

# License

- license/LICENSE.nvim: LICENSE from [neovim][neovim_url]
- license/LICENSE.neovim-qt: LICENSE from [neovim-qt][neovim_qt_url]

[neovim_url]: https://github.com/neovim/neovim
[neovim_qt_url]: https://github.com/equalsraf/neovim-qt

