filter-true = $(strip $(filter-out 1 on ON true TRUE, $1))
filter-false = $(strip $(filter-out 0 off OFF false FALSE, $1))

ifeq ($(V),1)
    Q=
else
    Q=@
endif

SNAIL_HOME_PAGE_URL  := https://github/gkide/snail
SNAIL_ONLINE_DOC_URL := https://github.io/gkide/snail

# copy 'contrib/local.mk.eg' to 'local.mk'
-include local.mk

ARCH_32_ENABLE       ?= OFF
ARCH_64_ENABLE       ?= OFF
CMAKE_GENERATOR_NAME ?= "Unix Makefiles"

NVIM_TESTING_ENABLE  ?= ON
SNAIL_TESTING_ENABLE ?= ON

ifeq (OFF,$(TESTING_ENABLE))
    NVIM_TESTING_ENABLE  := OFF
    SNAIL_TESTING_ENABLE := OFF
endif

ifeq (nvim,$(TESTING_ENABLE))
    SNAIL_TESTING_ENABLE := OFF
endif

ifeq (snail,$(TESTING_ENABLE))
    NVIM_TESTING_ENABLE  := OFF
endif

DEPS_CMAKE_BUILD_FLAGS :=

ifeq (,$(call filter-true,$(ARCH_32_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS += -DARCH_32_ENABLE=ON
endif

ifeq (,$(call filter-true,$(ARCH_64_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS += -DARCH_64_ENABLE=ON
endif

ifeq (,$(call filter-true,$(NVIM_TESTING_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS += -DNVIM_TESTING_ENABLE=ON
endif

ifeq (,$(call filter-true,$(SNAIL_TESTING_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS += -DSNAIL_TESTING_ENABLE=ON
endif

ifneq (,$(DEPS_CMAKE_EXTRA_FLAGS))
    DEPS_CMAKE_BUILD_FLAGS += $(DEPS_CMAKE_EXTRA_FLAGS)
endif

SNAIL_CMAKE_BUILD_TYPE  ?= Debug
SNAIL_CMAKE_BUILD_FLAGS := -DCMAKE_BUILD_TYPE=$(SNAIL_CMAKE_BUILD_TYPE)

ifeq (,$(call filter-true,$(ARCH_32_ENABLE)))
    SNAIL_CMAKE_BUILD_FLAGS += -DARCH_32_ENABLE=ON
endif

ifeq (,$(call filter-true,$(ARCH_64_ENABLE)))
    SNAIL_CMAKE_BUILD_FLAGS += -DARCH_64_ENABLE=ON
endif

ifneq (Dev,$(SNAIL_CMAKE_BUILD_TYPE))
    SNAIL_CMAKE_BUILD_FLAGS += -Wno-dev
endif

ifeq (,$(call filter-true,$(NVIM_TESTING_ENABLE)))
    SNAIL_CMAKE_BUILD_FLAGS += -DNVIM_TESTING_ENABLE=ON
endif

ifeq (,$(call filter-true,$(SNAIL_TESTING_ENABLE)))
    SNAIL_CMAKE_BUILD_FLAGS += -DSNAIL_TESTING_ENABLE=ON
endif

ifneq (,$(SNAIL_CMAKE_EXTRA_FLAGS))
    SNAIL_CMAKE_BUILD_FLAGS += $(SNAIL_CMAKE_EXTRA_FLAGS)
endif

.PHONY: deps cmake nvim snail all

all: nvim snail

cmake:
	$(Q)touch CMakeLists.txt
	$(Q)make build/.run-cmake

snail: build/.run-cmake
	+make -C build snail

nvim: build/.run-cmake
	+make -C build nvim

build/.run-cmake: | deps
	$(Q)cd build && cmake -G $(CMAKE_GENERATOR_NAME) $(SNAIL_CMAKE_BUILD_FLAGS) ..
	$(Q)touch $@

deps: | build/.run-deps-cmake
	+make -C deps/build

build/.run-deps-cmake:
	$(Q)if [ ! -d deps/build ]; then mkdir deps/build; fi
	$(Q)if [ ! -d deps/downloads ]; then mkdir deps/downloads; fi
	$(Q)cd deps/build && cmake -G $(CMAKE_GENERATOR_NAME) $(DEPS_CMAKE_BUILD_FLAGS) ..
	$(Q)if [ ! -d build ]; then mkdir build; fi
	$(Q)touch $@

.PHONY: check-nvim
.PHONY: run-nvim-unit-test
.PHONY: run-nvim-functional-test

run-nvim-functional-test: | nvim
	+make -C build run-nvim-functional-test

run-nvim-unit-test: | nvim

check-nvim: run-nvim-functional-test run-nvim-unit-test

.PHONY: check-snail
.PHONY: run-snail-libs-test
.PHONY: run-snail-shared-test
.PHONY: run-snail-plugins-test

run-snail-libs-test: | snail
	+make -C build run-snail-libs-test

check-snail: run-snail-libs-test

.PHONY: check

check: check-nvim check-snail

.PHONY: clean distclean depsclean

clean:
	$(Q)test -d build && make -C build clean || true

distclean:
	$(Q)rm -rf build

depsclean: distclean
	$(Q)rm -rf deps/build

