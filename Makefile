filter-false = $(strip $(filter-out 0 off OFF false FALSE, $1))
filter-true  = $(strip $(filter-out 1 on ON true TRUE    , $1))

SNAIL_HOMEPAGE_URL   := https://github/gkide/snail
SNAIL_ONLINE_DOC_URL := https://github.io/gkide/snail

# copy 'contrib/local.mk.eg' to 'local.mk'
-include local.mk

ARCH_32_ENABLE       ?= OFF
ARCH_64_ENABLE       ?= OFF
CMAKE_GENERATOR_NAME ?= "Unix Makefiles"

DEPS_CMAKE_BUILD_FLAGS :=

ifeq (,$(call filter-true,$(ARCH_32_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS += -DARCH_32_ENABLE=ON
endif

ifeq (,$(call filter-true,$(ARCH_64_ENABLE)))
    DEPS_CMAKE_BUILD_FLAGS += -DARCH_64_ENABLE=ON
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

ifneq (,$(SNAIL_CMAKE_EXTRA_FLAGS))
    SNAIL_CMAKE_BUILD_FLAGS += $(SNAIL_CMAKE_EXTRA_FLAGS)
endif


ifeq ($(V),1)
    Q=
else
    Q=@
endif

all: snail

cmake:
	$(Q)touch CMakeLists.txt
	$(Q)make build/.run-cmake

snail: build/.run-cmake deps
	$(Q)make -C build

build/.run-cmake: | deps
	$(Q)cd build && cmake -G $(CMAKE_GENERATOR_NAME) $(SNAIL_CMAKE_BUILD_FLAGS) ..
	$(Q)touch $@

deps: | build/.run-deps-cmake
	$(Q)make -C deps/build

build/.run-deps-cmake:
	$(Q)if [ ! -d deps/build ]; then mkdir deps/build; fi
	$(Q)if [ ! -d deps/downloads ]; then mkdir deps/downloads; fi
	$(Q)cd deps/build && cmake -G $(CMAKE_GENERATOR_NAME) $(DEPS_CMAKE_BUILD_FLAGS) ..
	$(Q)if [ ! -d build ]; then mkdir build; fi
	$(Q)touch $@

clean:
	$(Q)test -d build && make -C build clean || true

distclean:
	$(Q)rm -rf build

depsclean: distclean
	$(Q)rm -rf deps/build

.PHONY: deps cmake
.PHONY: snail
.PHONY: clean distclean depsclean
