@echo off

if "%1"=="" goto print_usage

if "%1"=="Debug" set BUILD=Debug
if "%1"=="Release" set BUILD=Release
if "%1"=="Release_OSS" set BUILD=Release_OSS

if "%BUILD%" == "" goto build_error

if not "%2"=="" set config=%2

set app=..\..\bin\x64\%BUILD%\WBTests.exe

title MySQL Workbench Tests

pushd test-suite
echo Running test application: %app%

if not exist %app% (
	echo Could not find the test application "%app%"
) else ( 
	if "%config%" == "" ( %app% ) else ( %app% --config %config% )
)

popd
exit /B

:print_usage
echo Usage:
echo.
echo %0 BUILD CONFIG
echo.
echo   BUILD has to be either Debug, Release or Release_OSS
echo   CONFIG is optional and allows to specify a json config file
echo.
exit /B 1

:build_error
echo Missing a valid build parameter
echo.
exit /B 1
