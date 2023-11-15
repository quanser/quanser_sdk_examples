@echo off

rem Outputs the QUARC root folder
call :root "%QUARC_DIR%"
goto :EOF

:root

echo %~s1

