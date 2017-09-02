:: use this file to run your own startup commands
:: use  in front of the command to prevent printing the command

:: call "%GIT_INSTALL_ROOT%/cmd/start-ssh-agent.cmd"
:: set "PATH=%CMDER_ROOT%\vendor\whatever;%PATH%"

@echo off

rem Change to the project root directory
set GKIDE_ROOT_DIR=%CMDER_ROOT:~0,-17%
cd /d "%GKIDE_ROOT_DIR%"

for /F "tokens=1,2* delims=^= " %%i in (%GKIDE_ROOT_DIR%\mingw-w64) do (
    REM if %%i == # ( echo "skip" ) else ( set %%i=%%j)
    if not %%i == # set %%i=%%j)

set CC=gcc
set GKIDE_PROG_M4=%GKIDE_MSYS2_USR_BIN_DIR%\m4.exe
set GKIDE_PROG_LS=%GKIDE_MSYS2_USR_BIN_DIR%\ls.exe
set GKIDE_PROG_CAT=%GKIDE_MSYS2_USR_BIN_DIR%\cat.exe
set GKIDE_PROG_SED=%GKIDE_MSYS2_USR_BIN_DIR%\sed.exe
set GKIDE_PROG_CUT=%GKIDE_MSYS2_USR_BIN_DIR%\cut.exe
set GKIDE_PROG_LDD=%GKIDE_MSYS2_USR_BIN_DIR%\ldd.exe
set GKIDE_PROG_GREP=%GKIDE_MSYS2_USR_BIN_DIR%\grep.exe
set GKIDE_PROG_ECHO=%GKIDE_MSYS2_USR_BIN_DIR%\echo.exe
set GKIDE_PROG_HEAD=%GKIDE_MSYS2_USR_BIN_DIR%\head.exe
set GKIDE_PROG_FIND=%GKIDE_MSYS2_USR_BIN_DIR%\find.exe
set GKIDE_PROG_MKDIR=%GKIDE_MSYS2_USR_BIN_DIR%\mkdir.exe
set GKIDE_PROG_GPERF=%GKIDE_MSYS2_USR_BIN_DIR%\gperf.exe
set GKIDE_PROG_WHICH=%GKIDE_MSYS2_USR_BIN_DIR%\which.exe
set GKIDE_PROG_UNZIP=%GKIDE_MSYS2_USR_BIN_DIR%\unzip.exe
set GKIDE_PROG_INSTALL=%GKIDE_MSYS2_USR_BIN_DIR%\install.exe

set GKIDE_PROG_GIT=%GKIDE_MSYS2_GIT_BIN_DIR%\git.exe

set GKIDE_MINGW_BIN_DIR=%GKIDE_MINGW_TOOLCHAIN_DIR%\bin
set GKIDE_MINGW_INC_DIR=%GKIDE_MINGW_TOOLCHAIN_DIR%\include
set GKIDE_MINGW_LIB_DIR=%GKIDE_MINGW_TOOLCHAIN_DIR%\lib

set GKIDE_PROG_MINGW32_MAKE=%GKIDE_MINGW_BIN_DIR%\mingw32-make.exe

%GKIDE_PROG_ECHO% -e "Check Env Vars  : \033[33mset GKIDE\033[0m"
%GKIDE_PROG_ECHO% -e "Convenient Alias: \033[33mmake\033[0m <= mingw32-make"
%GKIDE_PROG_ECHO% -e "MINGW Toolchain : \033[33m%GKIDE_MINGW_TOOLCHAIN_DIR%\033[0m"
%GKIDE_PROG_ECHO% -e "MSYS2 Toolchain : \033[33m%GKIDE_MSYS2_USR_BIN_DIR:~,-4%\\\\bin\033[0m"

echo %PATH% > %GKIDE_ROOT_DIR%\cmder_env_path_1
%GKIDE_PROG_SED% -e 's/;/;\n/g' %GKIDE_ROOT_DIR%\cmder_env_path_1 > %GKIDE_ROOT_DIR%\cmder_env_path_2

rem Clean the ENV PATH
set PATH=

rem Add Local build path for convenient
echo %GKIDE_ROOT_DIR%\build\bin; >> %GKIDE_ROOT_DIR%\cmder_env_path_3

rem Add MinGW
echo %GKIDE_MINGW_BIN_DIR%; >> %GKIDE_ROOT_DIR%\cmder_env_path_3

rem Add NodeJS install path to ENV path
echo %GKIDE_NODEJS_INSTALL_DIR%; >> %GKIDE_ROOT_DIR%\cmder_env_path_3

%GKIDE_PROG_GREP% -e "AppData\\\\Roaming\\\\npm" %GKIDE_ROOT_DIR%\cmder_env_path_2 > %GKIDE_ROOT_DIR%\cmder_env_path_1
for /F "delims=;" %%i in (%GKIDE_ROOT_DIR%\cmder_env_path_1) do (
    if not $%%i$ == $""$ echo %%i; >> %GKIDE_ROOT_DIR%\cmder_env_path_3)

rem Add cmder path to ENV path
%GKIDE_PROG_GREP% 'cmder_mini' %GKIDE_ROOT_DIR%\cmder_env_path_2 > %GKIDE_ROOT_DIR%\cmder_env_path_1
for /F "delims=;" %%i in (%GKIDE_ROOT_DIR%\cmder_env_path_1) do (
    if not $%%i$ == $""$ echo %%i; >> %GKIDE_ROOT_DIR%\cmder_env_path_3)

rem Add windows standard path to ENV path
echo C:\Windows; >> cmder_env_path_3

%GKIDE_PROG_GREP% -e "[S|s]ystem32" %GKIDE_ROOT_DIR%\cmder_env_path_2 > %GKIDE_ROOT_DIR%\cmder_env_path_1
for /F "delims=;" %%i in (%GKIDE_ROOT_DIR%\cmder_env_path_1) do (
    if not $%%i$ == $""$ echo %%i; >> %GKIDE_ROOT_DIR%\cmder_env_path_3)

rem remove the unwanted spaces
%GKIDE_PROG_SED% -i 's/ *; *$/;/' %GKIDE_ROOT_DIR%\cmder_env_path_3
%GKIDE_PROG_SED% ':a;N;$!ba;s/\n/ /g' %GKIDE_ROOT_DIR%\cmder_env_path_3 > %GKIDE_ROOT_DIR%\cmder_env_path_4
%GKIDE_PROG_SED% -i 's/; /;/g' %GKIDE_ROOT_DIR%\cmder_env_path_4

rem set the final build ENV path
for /F %%i in (%GKIDE_ROOT_DIR%\cmder_env_path_4) do set PATH=%%i

del %GKIDE_ROOT_DIR%\cmder_env_path_1
del %GKIDE_ROOT_DIR%\cmder_env_path_2
del %GKIDE_ROOT_DIR%\cmder_env_path_3
del %GKIDE_ROOT_DIR%\cmder_env_path_4