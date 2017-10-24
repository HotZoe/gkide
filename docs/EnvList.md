# GKIDE Environment Variable Name List

## GKIDE_SYS_HOME
- automatic detection and setting at runtime
- if set up fails, then **nvim** programme will exit
- once set up, can not changed during the current runtime
- full path, dependent on the current running **nvim** programme
- the default and desired gkide install directory layout is:
  - .../xxx/bin: **system** executable programme directory, contains, e.g., **nvim**, **snail**
  - .../xxx/etc: **system** configuration directory
  - .../xxx/doc: **system** documentation directory
  - .../xxx/plg: **system** runtime plugins directory
  - .../xxx/loc: **system** miscellaneous directory

## GKIDE_USR_HOME
- automatic detection and setting at runtime
- full path, dependent on:
  - if already set and points to exist directory, then just use it
  - if already set but points to directory that not exist, then try to create
  - if not set, then use the default value to set it, create directory as needed, which is:
    - **$HOME/.gkide** for linux
    - **$HOMEDRIVE\\$HOMEPATH\\.gkide** for windows
- if set up fails, then **nvim** programme will exit
- once set up, can not changed during the current runtime
- the default and desired gkide user directory layout is:
  - .../xxx/bin: **user** executable programme directory
  - .../xxx/etc: **user** configuration directory
  - .../xxx/plg: **user** runtime plugins directory

## GKIDE_DYN_HOME
- full path, can changing at runtime, set or not
- the default and desired gkide dynamic directory layout is:
  - .../xxx/bin: **dynamic** executable programme directory
  - .../xxx/etc: **dynamic** configuration directory
  - .../xxx/plg: **dynamic** runtime plugins directory

## GKIDE_NVIM_LANGUAGE
**nvim** will initialize message translation (using **gettext**) on startup, and will check:
- if set, then use it, which should points to the local directory
- if not set, then use the default value, which is **$GKIDE_SYS_HOME/loc/language**

## GKIDE_NVIM_LOGGINGS
**nvim** will write proper runtime logging messages to logfile, and the logfile initialized on
on startup by:
- if set, then use it, which should points to the logfile
  - the logfile will automatic create if not exist
  - the path directory contains the logfile must exist
- if not set, then use the default value, which is **$GKIDE_USR_HOME/nvim.log**

## GKIDE_SNAIL_LOGGINGS
**snail** will write proper runtime logging messages to logfile.

## GKIDE_SNAIL_LOGLEVEL
if enable **snail** logging, the logging level can be set by this env-variable

## GKIDE_SNAIL_NVIMEXEC
The **nvim** executable file full path used by **snail**. The env-value overwrite '--nvim' command
line arguments

## GKIDE_SNAIL_PLGSPATH
Extra runtime plugin directory used by **nvim**.
