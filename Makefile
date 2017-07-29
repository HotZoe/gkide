filter-true = $(strip $(filter-out 1 on ON true TRUE, $1))
filter-false = $(strip $(filter-out 0 off OFF false FALSE, $1))

ifeq ($(V),1)
    Q=
else
    Q=@
endif

# copy 'contrib/local.mk.eg' to 'local.mk'
-include local.mk

CMAKE_PROG := cmake
MAKE_PROG  := $(MAKE)
BUILD_CMD  := $(MAKE)

# Build env is windows cmd shell
windows_cmd_shell := OFF

CMAKE_GENERATOR_NAME := "Unix Makefiles"

ifneq (, $(shell ninja --version 2> nul))
    ifneq (, $(VERBOSE))
        BUILD_CMD := ninja -v
    else
        BUILD_CMD := ninja
    endif
endif

# check for windows cmd shell
ifneq (%OS%, $(shell echo %OS%))
    MSYS_SHELL_PATH := $(shell echo %GKIDE_MSYS2_USR_BIN_DIR%\sh.exe)
    windows_cmd_shell := ON
    CMAKE_GENERATOR_NAME := "MinGW Makefiles"

    ifneq (,$(findstring mingw32,$(shell echo %MINGW_TOOLCHAIN_DIR%)))
        TARGET_ARCH_32 := ON
        TARGET_ARCH_64 := OFF
    endif

    ifneq (,$(findstring mingw64,$(shell echo %MINGW_TOOLCHAIN_DIR%)))
        TARGET_ARCH_32 := OFF
        TARGET_ARCH_64 := ON
    endif
endif

# check for classic unix shell
ifeq (OFF,$(windows_cmd_shell))
    ifneq (,$(shell uname -s | grep '^MSYS_NT-*'))
        $(error Do not use MSYS shell as windows build environment, see 'contrib/BuildInstall.md')
    endif

    ifneq (,$(shell uname -s | grep '^MINGW[3|6][2|4]_NT*'))
        $(error Do not use MinGW shell as windows build environment, see 'contrib/BuildInstall.md')
    endif

    ifneq (,$(shell uname -s | grep '^CYGWIN_NT*'))
        $(error Do not use Cygwin shell as windows build environment, see 'contrib/BuildInstall.md')
    endif
endif

# CMake build flags
DEPS_CMAKE_BUILD_FLAGS  := -DCMAKE_BUILD_TYPE=Release
GKIDE_CMAKE_BUILD_FLAGS := -DCMAKE_BUILD_TYPE=Release

# If not set by hand, then auto detected, same as the host
TARGET_ARCH_32 ?= OFF
TARGET_ARCH_64 ?= OFF

ifeq (,$(call filter-true,$(TARGET_ARCH_32)))
    DEPS_CMAKE_BUILD_FLAGS  += -DTARGET_ARCH_32=ON
    GKIDE_CMAKE_BUILD_FLAGS += -DTARGET_ARCH_32=ON
endif

ifeq (,$(call filter-true,$(TARGET_ARCH_64)))
    DEPS_CMAKE_BUILD_FLAGS  += -DTARGET_ARCH_64=ON
    GKIDE_CMAKE_BUILD_FLAGS += -DTARGET_ARCH_64=ON
endif

# By default, disable all testing
NVIM_TESTING_ENABLE ?= OFF
SNAIL_TESTING_ENABLE ?= OFF

ifeq (ON,$(TESTING_ENABLE))
    NVIM_TESTING_ENABLE  := ON
    SNAIL_TESTING_ENABLE := ON
endif

ifeq (nvim,$(TESTING_ENABLE))
    NVIM_TESTING_ENABLE  := ON
endif

ifeq (snail,$(TESTING_ENABLE))
    SNAIL_TESTING_ENABLE := ON
endif

ifeq (,$(call filter-true,$(NVIM_TESTING_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS  += -DNVIM_TESTING_ENABLE=ON
    GKIDE_CMAKE_BUILD_FLAGS += -DNVIM_TESTING_ENABLE=ON
endif

ifeq (,$(call filter-true,$(SNAIL_TESTING_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS  += -DSNAIL_TESTING_ENABLE=ON
    GKIDE_CMAKE_BUILD_FLAGS += -DSNAIL_TESTING_ENABLE=ON
endif

ifneq (,$(DEPS_CMAKE_EXTRA_FLAGS))
    DEPS_CMAKE_BUILD_FLAGS += $(DEPS_CMAKE_EXTRA_FLAGS)
endif

ifneq (,$(GKIDE_CMAKE_EXTRA_FLAGS))
    GKIDE_CMAKE_BUILD_FLAGS += $(GKIDE_CMAKE_EXTRA_FLAGS)
endif

# Do not show cmake warnings for none 'Dev' build
ifneq (Dev,$(GKIDE_CMAKE_BUILD_TYPE))
    DEPS_CMAKE_BUILD_FLAGS  += -Wno-dev
    GKIDE_CMAKE_BUILD_FLAGS += -Wno-dev
endif

# check local Qt5 install prefix
ifeq (,$(QT5_INSTALL_PREFIX))
    ifneq (env-check,$(MAKECMDGOALS))
        $(error Qt5 install perfix not set, see contrib/local.mk.eg)
    endif
else
    GKIDE_CMAKE_BUILD_FLAGS += -DCMAKE_PREFIX_PATH=$(QT5_INSTALL_PREFIX)
endif

.PHONY: deps cmake nvim snail all

all: nvim snail

cmake:
ifeq (OFF,$(windows_cmd_shell))
	$(Q)touch CMakeLists.txt
endif
	$(Q)$(BUILD_CMD) build/.run-cmake

snail: build/.run-cmake
	+$(BUILD_CMD) -C build snail

nvim: build/.run-cmake
	+$(BUILD_CMD) -C build nvim

build/.run-cmake: | deps
	$(Q)cd build && $(CMAKE_PROG) -G $(CMAKE_GENERATOR_NAME) $(GKIDE_CMAKE_BUILD_FLAGS) ..
ifeq (ON,$(windows_cmd_shell))
	$(Q)echo ok > $@
else
	$(Q)touch $@
endif

deps: | build/.run-deps-cmake
	+$(BUILD_CMD) -C deps/build

build/.run-deps-cmake:
ifeq (ON,$(windows_cmd_shell))
	$(Q)cd deps && if not exist build mkdir build
	$(Q)cd deps && if not exist downloads mkdir downloads
	$(Q)if not exist build mkdir build
	$(Q)echo ok > $@
else
	$(Q)if [ ! -d deps/build ]; then mkdir deps/build; fi
	$(Q)if [ ! -d deps/downloads ]; then mkdir deps/downloads; fi
	$(Q)if [ ! -d build ]; then mkdir build; fi
	$(Q)touch $@
endif
	$(Q)cd deps/build && $(CMAKE_PROG) -G $(CMAKE_GENERATOR_NAME) $(DEPS_CMAKE_BUILD_FLAGS) ..

.PHONY: check-nvim
.PHONY: run-nvim-unit-test
.PHONY: run-nvim-functional-test

run-nvim-functional-test: | nvim
	+$(BUILD_CMD) -C build run-nvim-functional-test

run-nvim-unit-test: | nvim

check-nvim: run-nvim-functional-test run-nvim-unit-test

.PHONY: check-snail
.PHONY: run-snail-libs-test
.PHONY: run-snail-shared-test
.PHONY: run-snail-plugins-test

run-snail-libs-test: | snail
	+$(BUILD_CMD) -C build run-snail-libs-test

check-snail: run-snail-libs-test

.PHONY: check

check: check-nvim check-snail

.PHONY: clean gkideclean distclean

clean:
ifeq (ON,$(windows_cmd_shell))
	$(Q)if exist build $(BUILD_CMD) -C build clean
else
	$(Q)test -d build && $(BUILD_CMD) -C build clean || true
endif

gkideclean:
ifeq (ON,$(windows_cmd_shell))
	$(Q)if exist build rmdir /S /Q build
else
	$(Q)rm -rf build
endif

distclean: gkideclean
ifeq (ON,$(windows_cmd_shell))
	$(Q)cd deps && if exist build rmdir /S /Q build
else
	$(Q)rm -rf deps/build
endif

.PHONY: env-check

env-check:
	$(Q)$(MSYS_SHELL_PATH) scripts/envcheck/env-check.sh $(QT5_INSTALL_PREFIX) $(MSYS_SHELL_PATH)