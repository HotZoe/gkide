filter-true = $(strip $(filter-out 1 on ON true TRUE, $1))
filter-false = $(strip $(filter-out 0 off OFF false FALSE, $1))

# the dot program
ifeq (, $(DOT_PROG))
    ifeq (command line, $(origin DOT_PROG))
        $(error Command line argument 'DOT_PROG' is empty)
    endif

    DOT_PROG := dot
endif

# the dot output top path
ifeq (, $(DOT_OUTPUT_DIR))
    ifeq (command line, $(origin DOT_OUTPUT_DIR))
        $(error Command line argument 'DOT_OUTPUT_DIR' is empty)
    endif

    DOT_OUTPUT_DIR := build
endif

# the output file format, default is png
ifeq (, $(DOT_OUTPUT_FORMAT))
    ifeq (command line, $(origin DOT_OUTPUT_FORMAT))
        $(error Command line 'DOT_OUTPUT_FORMAT' is empty)
    endif

    DOT_OUTPUT_FORMAT := png
endif

# ON  for windows cmd shell
# OFF for unix shell
WINDOWS_CMD_SHELL ?= OFF

# the Root DIRectroy File NameS only, which is 'graphviz/*.dot' files
INPUTS_RDIR_FNS = $(basename $(wildcard *.dot))

# the SubDIRectroy of 'graphviz/' File NameS, 1 level deep only
INPUTS_SDIR_FNS = $(basename $(wildcard */*.dot))
# remove the trailling /
INPUTS_SUB_DIRS = $(subst /,,$(dir $(INPUTS_SDIR_FNS)))

# All the target files: the file suffix is '$(DOT_OUTPUT_FORMAT)'
INPUTS_NAMES = $(INPUTS_RDIR_FNS) $(INPUTS_SDIR_FNS)

TARGETS = $(foreach item,$(INPUTS_NAMES),$(item).$(DOT_OUTPUT_FORMAT))

# the 'index.dot' fies use the default layout algorithm, which is 'dot'
all: $(TARGETS)

%.$(DOT_OUTPUT_FORMAT): %.dot mk-sub-dirs
	$(Q)$(DOT_PROG) $(DOT_ARGS) -T$(DOT_OUTPUT_FORMAT) -o $(DOT_OUTPUT_DIR)/$@ $<
	$(Q)echo Generating $(DOT_OUTPUT_DIR)/$@

mk-sub-dirs:
ifeq (ON,$(WINDOWS_CMD_SHELL))
	@cd $(subst /,\,$(DOT_OUTPUT_DIR)) && for %%i in ($(INPUTS_SUB_DIRS)) do if not exist %%i mkdir %%i
else
	@for subdir in $(INPUTS_SUB_DIRS); do mkdir $(DOT_OUTPUT_DIR)/$(subdir); done
endif