@echo off & setlocal enableextensions enabledelayedexpansion
::
::===============================================================
:: Windows batch script for generating SerialAPI defines for supported functions based on what exists in library.
:: Parameters:
:: 1. library file
:: 2. output file
if "%1" == "" goto usage
if "%2" == "" goto usage

set path=%KEILPATH%\bin;%TOOLSDIR%\Python;%path%
SET LIBRARYFILE=%1
SET OUTPUTFILE=%2
echo Generating SerialAPI defines for supported functions based on what exists in
echo   %LIBRARYFILE% ...
echo /* Machine generated SerialAPI defines for supported functions based on what exists in library: */!> %OUTPUTFILE%
echo /*   %LIBRARYFILE%   */!>> %OUTPUTFILE%
FOR /F "eol=; tokens=1,2 delims=," %%i in (serialapi-supported-func-list.txt) do (
  call :FunctionInLibrary %%j %LIBRARYFILE% resultOut_
  echo #define %%i!resultOut_! /* %%j */!>> %OUTPUTFILE%
)
endlocal & goto :EOF

::
::===============================================================
:FunctionInLibrary FunctionIn LibraryIn ResultOut
::
setlocal enableextensions
set functionIn_=%1
set libraryIn_=%2
REM TO#03012 - Workaround. The following line is added as a workaround for TO#03012
set functionIn_=%functionIn_:aA=.A%
libx51 LIST %libraryIn_% PUBLICS | findstr /I /R /C:"      .*%functionIn_%$" > NUL
if not ERRORLEVEL 1 (endlocal & set %3=1& goto :EOF) else (endlocal & set %3=0& goto :EOF)

:usage
echo usage: make-supported-functions-include.bat ...\ZW_controller_static_ZW040x.lib ...\serialapi_supported_func.h
endlocal & goto :EOF
