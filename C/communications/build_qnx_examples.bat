@echo off

rem This DOS script builds all the QNX examples. It takes three arguments:
rem
rem   %1 = configuration (debug or release)
rem   %2 = platform (ppc_be, ppc_be_spe or x86)
rem   %3 = operation (build, rebuild, clean)
rem
rem All of the arguments are optional. The defaults are debug, x86, build respectively.

rem This adds QNX utilities to the PATH 
PATH=%QNX_HOST%/usr/bin;%PATH%

set configuration=debug
set platform=x86
set operation=build

:loop
if "%1"=="" goto build

if /I "%1"=="debug"      set configuration=debug
if /I "%1"=="release"    set configuration=release
if /I "%1"=="ppc_be"     set platform=ppc_be
if /I "%1"=="ppc_be_spe" set platform=ppc_be_spe
if /I "%1"=="x86"        set platform=x86
if /I "%1"=="build"      set operation=build
if /I "%1"=="rebuild"    set operation=rebuild
if /I "%1"=="clean"      set operation=clean

shift
goto :loop

:build

rem Determine the variant to make based on the configuration
set variant_list=EXCLUDE_VARIANTLIST^=g
if /I "%configuration%"=="debug" set variant_list=VARIANTLIST^=g

rem Determine the make flags based on the operation
set flags=all
if "%operation%"=="rebuild" set flags=-B clean all
if "%operation%"=="clean"   set flags=clean

rem Invoke make in each example directory
for /d %%f in (*.*) do (
    if exist %%f\Makefile echo Building %%f... && call :build %%f "%QSDK_DIR%"
)

goto :EOF

:build

"%QNX_HOST%\usr\bin\make" -C %~ps1\%~nx1 CONFIGURATION=%configuration% PLATFORM=%platform% %variant_list% EXTRA_CCFLAGS= MAKEFLAGS=-I%QNX_TARGET%/usr/include QRDIR=%QNX_TARGET%/usr/include/ QSDK_DIR=%~s2 %flags%
