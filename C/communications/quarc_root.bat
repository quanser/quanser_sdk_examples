@echo off

rem Outputs the QUARC root folder
call :root "%QSDK_DIR%"
goto :EOF

:root

echo %~s1

