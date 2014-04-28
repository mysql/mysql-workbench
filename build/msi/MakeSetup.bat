@echo off

if "%1"=="" goto printUsage
if "%2"=="" goto printUsage
if "%3"=="" goto printUsage

if "%3"=="win32" set RELEASE_ARCH=x86
if "%3"=="win64" set RELEASE_ARCH=x64

if "%RELEASE_ARCH%" == "" goto arch_error

rem Set edition specific variables
if "%1"=="commercial" (
  set BIN_DIR=..\..\bin\%RELEASE_ARCH%\Release
  set LICENSE_TYPE=commercial
  set SETUP_TYPE=commercial
)

if "%1"=="community" (
  set BIN_DIR=..\..\bin\%RELEASE_ARCH%\Release_OSS
  set LICENSE_TYPE=community
  set SETUP_TYPE=community
)

if "%1"=="debug" (
  set BIN_DIR=..\..\bin\%RELEASE_ARCH%\Debug
  set LICENSE_TYPE=debug
  set SETUP_TYPE=debug
)

rem Set version variables
set VERSION_DETAIL=%2
for %%A in ("%VERSION_DETAIL%") do set VERSION_MAIN=%%~nA

if "%4"=="" goto no_sign
set BUILD_SIGNED=1
:no_sign

rem Set other variables
set DIST_DIR=.\distribution
set UTIL_PATH=..\..\..\mysql-win-res\bin
set OUTPUT_FILENAME=mysql-workbench-%SETUP_TYPE%-%VERSION_DETAIL%-%3.msi
set OUTPUT_FILENAME_UNSIGNED=mysql-workbench-%SETUP_TYPE%-%VERSION_DETAIL%-%3-unsigned.msi

if not exist %BIN_DIR% goto ERROR
if not exist %DIST_DIR% mkdir %DIST_DIR%


rem -------------------------------------------------------------------------------------
echo Copy Editions specific files...
echo .
if "%1"=="commercial" copy ..\..\LICENSE.mysql %BIN_DIR%\. 1> nul
if "%1"=="community" copy ..\..\COPYING %BIN_DIR%\. 1> nul

rem -------------------------------------------------------------------------------------
echo Make Setup script started...
echo .

echo Clean old object files ...
rem Cleaning is necessary because *.wixobj files must be remade if the license
rem type changes.
nmake /NOLOGO -f Makefile.mak clean
echo .

rem -------------------------------------------------------------------------------------
echo Copying WiX source files ...
copy source\mysql_workbench.xml mysql_workbench.xml
copy source\mysql_workbench_fragment.xml mysql_workbench_fragment.xml
echo .


echo Build MSI file...
nmake /NOLOGO -f Makefile.mak LICENSE_TYPE=%LICENSE_TYPE% SETUP_TYPE=%SETUP_TYPE% ARCHITECTURE=%RELEASE_ARCH% all VERSION_MAIN=%VERSION_MAIN% VERSION_DETAIL=%VERSION_DETAIL%
if errorlevel 1 goto ERROR4
echo .
echo MSI file build successfully.
echo .

rem http://stcss.us.oracle.com/codesign/faces/index.jsp
if "%BUILD_SIGNED%"=="" goto no_sign
rename mysql_workbench.msi %OUTPUT_FILENAME%
if exist %DIST_DIR%\%OUTPUT_FILENAME% del %DIST_DIR%\%OUTPUT_FILENAME% 1> nul 2> nul
copy /y %OUTPUT_FILENAME% %DIST_DIR%\%OUTPUT_FILENAME_UNSIGNED%
java -Xmx1024m -jar %UTIL_PATH%\Client.jar -user %4 -pass %5 -file_to_sign %OUTPUT_FILENAME% -signed_location %DIST_DIR%
if not exist %DIST_DIR%\%OUTPUT_FILENAME% goto sign_error
goto make_md5
:no_sign
rem move generated file
move mysql_workbench.msi %DIST_DIR%\%OUTPUT_FILENAME%

:make_md5
pushd %DIST_DIR%
echo Make .md5 sum ...
..\%UTIL_PATH%\md5sum %OUTPUT_FILENAME% > %OUTPUT_FILENAME%.md5
echo .
popd

echo Clean object files...
call MakeClean.bat
echo .

echo Build was successful. You can find the generated files in the %DIST_DIR% directory.

EXIT /B 0


:ERROR
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

:sign_error
echo Error: Signig the setup-files failed. Error messages should have been provided above.
EXIT /B 1

:arch_error
echo Error: the given architecture (%3) is not valid. It must either be win32 or win64.
EXIT /B 1

:printUsage
echo MakeSetup Version 2.1
echo Usage:
echo .
echo %0 EDITION VERSION ARCHITECTURE [SIGN_USER SIGN_PASS]
echo .
echo   EDITION can be commercial, community or debug (commercial only)
echo   VERSION has to be a 3 number version code
echo   ARCHITECTURE has to be either win32 or win64
echo .
echo   Examples:
echo   %0 community 6.2.0 win64 
echo   %0 commercial 6.2.0 win32 user pass
echo .
echo To sign msi provide login and password for Oracle Corporate Code Signing tool

EXIT /B 1
