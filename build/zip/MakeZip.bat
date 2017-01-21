@echo off

if "%1"=="" goto printUsage
if "%2"=="" goto printUsage
if "%3"=="" goto printUsage

if "%3"=="win32" (
  set ARCH=x86
  set FILENAME_ARCH=win32
)
if "%3"=="win64" (
  set ARCH=x64
  set FILENAME_ARCH=winx64
)

rem Set edition specific variables
if "%1"=="commercial" set BIN_DIR=..\..\bin\%ARCH%\Release
if "%1"=="commercial" set LICENSE_TYPE=commercial
if "%1"=="commercial" set SETUP_TYPE=commercial
if "%1"=="commercial" set SETUP_TYPE_UC=

if "%1"=="community" set BIN_DIR=..\..\bin\%ARCH%\Release_OSS
if "%1"=="community" set LICENSE_TYPE=community
if "%1"=="community" set SETUP_TYPE=community
if "%1"=="community" set SETUP_TYPE_UC= CE

rem Set version variables
set VERSION_DETAIL=%2
for %%A in ("%VERSION_DETAIL%") do set VERSION_MAIN=%%~nA

rem Set other variables
set DIST_DIR=.\distribution
set UTIL_PATH=..\..\..\mysql-win-res\bin
set PYTHON_EXE_PATH=..\..\..\..\mysql-win-res\bin\python\python.exe
set PYTHONPATH=..\..\..\..\mysql-win-res\lib\Python
set OUTPUT_FILENAME=mysql-workbench-%SETUP_TYPE%-%VERSION_DETAIL%-%FILENAME_ARCH%.zip
set OUTPUT_DIRNAME="MySQL Workbench %VERSION_DETAIL%%SETUP_TYPE_UC% (%FILENAME_ARCH%)"
set TMP_DIR=.\temp

set path=%path%;%PYTHON_EXE_PATH%

if not exist %BIN_DIR% goto ERROR1
if not exist %DIST_DIR% mkdir %DIST_DIR%
if not exist %TMP_DIR% mkdir %TMP_DIR%


rem -------------------------------------------------------------------------------------
echo Copy Editions specific files...
echo .
if "%1"=="commercial" copy ..\..\LICENSE.mysql %BIN_DIR%\. 1> nul
if "%1"=="community" copy ..\res\COPYING %BIN_DIR%\. 1> nul


rem -------------------------------------------------------------------------------------
echo Make Zip script started...
echo .

echo Copy files ...
if not exist %TMP_DIR%\%OUTPUT_DIRNAME% mkdir %TMP_DIR%\%OUTPUT_DIRNAME%
xcopy %BIN_DIR%\*.* %TMP_DIR%\%OUTPUT_DIRNAME%\. /S /Y /Q
echo .

rem -------------------------------------------------------------------------------------
echo Remove obsolete files ...
pushd %TMP_DIR%
pushd %OUTPUT_DIRNAME%
del /Q *.exp
del /Q *.lib
del /Q *.pdb
del /Q *.ilk
del /Q *.metagen
del /Q *vshost.exe
del /Q *vshost.exe.manifest
del /Q license.txt
erase /S /Q Makefile.am
erase /S /Q *.pyc
rmdir /S /Q python\lib\sqlite3\test
rem for CE remove reporting dirs
if "%1"=="community" rmdir /S /Q modules\data\wb_model_reporting
popd
popd
echo .


echo Build zip file...
pushd %TMP_DIR%
rem precompile Python sources
%PYTHON_EXE_PATH% -mcompileall %OUTPUT_DIRNAME%
if %ERRORLEVEL% == 1 goto ERROR5
  
..\%UTIL_PATH%\zip -q -9 -r %OUTPUT_FILENAME% %OUTPUT_DIRNAME%
if %ERRORLEVEL% == 1 goto ERROR6
popd
echo .

rem move generated file
move %TMP_DIR%\%OUTPUT_FILENAME% %DIST_DIR%\%OUTPUT_FILENAME%

rem remove temp dir
rmdir /S /Q %TMP_DIR%

pushd %DIST_DIR%
echo Make .md5 sum ...
..\%UTIL_PATH%\md5sum %OUTPUT_FILENAME% > %OUTPUT_FILENAME%.md5
popd
echo .


echo Build was successful. You can find the generated files in the %DIST_DIR% directory.

EXIT /B 0


:ERROR1
echo Error: %BIN_DIR% folder not found. Check the path and make sure it contains a valid build.
EXIT /B 1

:ERROR2
echo Error: set_shell_vars.cmd cannot be generated
EXIT /B 1

:ERROR3
echo Error: The directory %SOURCE_DIR% has to contain the files from the .zip distribution
EXIT /B 1

:ERROR4
echo Error: Building the setup-files failed. Error messages should have been provided above.
EXIT /B 1

:ERROR5
echo Error: Compiling python code.
EXIT /B 1

:ERROR6
echo Error: Failed to zip.
EXIT /B 1

:printUsage
echo MakeZip Version 2.0.0
echo Usage:
echo .
echo %0 EDITION VERSION ARCH
echo .
echo   EDITION can be commercial or community
echo   VERSION has to be a 3 number version code
echo   ARCH is either win32 or win64
echo .
echo   Examples:
echo   %0 community 6.1.4 win64
EXIT /B 1
