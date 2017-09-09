;= @echo off
;= rem Call DOSKEY and use this file as the macrofile
;= %SystemRoot%\system32\doskey /listsize=1000 /macrofile=%0%
;= rem In batch mode, jump to the end of the file
;= goto:eof
;= Add aliases below here

e.=explorer .

pwd=cd
clear=cls
cmderr=cd /d "%CMDER_ROOT%"
history=cat "%CMDER_ROOT%\config\.history"

ls=%GKIDE_PROG_LS% $*
m4=%GKIDE_PROG_M4% $*

cat=%GKIDE_PROG_CAT% $*
cut=%GKIDE_PROG_CUT% $*
dot=%GKIDE_PROG_DOT% $*
git=%GKIDE_PROG_GIT% $*
ldd=%GKIDE_PROG_LDD% $*
sed=%GKIDE_PROG_SED% $*

echo=%GKIDE_PROG_ECHO% $*
find=%GKIDE_PROG_FIND% $*
grep=%GKIDE_PROG_GREP% $*
head=%GKIDE_PROG_HEAD% $*
make=%GKIDE_PROG_MINGW32_MAKE% $*

gperf=%GKIDE_PROG_GPERF% $*
which=%GKIDE_PROG_WHICH% $*
unzip=%GKIDE_PROG_UNZIP% $*
