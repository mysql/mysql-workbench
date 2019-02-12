@echo off

rem -------------------------------------------------------------------------------
rem Check parameter
if [%1] == [] goto Usage
if [%2] == [] goto Usage
if [%3] == [] goto Usage

rem -------------------------------------------------------------------------------
rem Script start
echo Preparing output directory...

rem -------------------------------------------------------------------------------
rem Set directory variables

set LIBRARY_DIR=%1library
echo Library directory: %LIBRARY_DIR%

set RES_DIR=%1res
echo Resources directory: %RES_DIR%

set IMAGES_DIR=%1images
echo Images directory: %IMAGES_DIR%

set SCRIPTS_DIR=%1scripts
echo Scripts directory: %SCRIPTS_DIR%

if %2 == Debug ( set ADDITIONAL_LIBFOLDER=debug\)
set EXT_LIB_DIR=%WB_3DPARTY_PATH%\%ADDITIONAL_LIBFOLDER%Lib
echo Windows resource directory: %EXT_LIB_DIR%

set EXT_BIN_DIR=%WB_3DPARTY_PATH%\bin
echo External binary directory: %EXT_BIN_DIR%

set TARGET_DIR=%1bin\%3\%2
echo Target directory: %TARGET_DIR%

set PYTHON_COMMON_DIR=%WB_3DPARTY_PATH%\Python\lib
echo Python common library directory: %PYTHON_COMMON_DIR%

set PYTHON_DIR=%WB_3DPARTY_PATH%\Python
echo Python directory: %PYTHON_DIR%

set PYTHON_LIB_DIR=%WB_3DPARTY_PATH%\Python\Libs
echo Python library directory: %PYTHON_LIB_DIR%

set PYTHON_DLLS_DIR=%WB_3DPARTY_PATH%\Python\Dlls
echo Python dlls directory: %PYTHON_DLLS_DIR%

rem -------------------------------------------------------------------------------
rem Copy the files to the target directory

echo Copy Struct files ...
if not exist %TARGET_DIR%\structs mkdir %TARGET_DIR%\structs
xcopy /i /s /y /d %RES_DIR%\grt\structs*.xml %TARGET_DIR%\structs\. 1> nul 2> nul

echo Copy image files ...
if not exist %TARGET_DIR%\images\grt\structs mkdir %TARGET_DIR%\images\grt\structs
if not exist %TARGET_DIR%\images\icons mkdir %TARGET_DIR%\images\icons
if not exist %TARGET_DIR%\images\cursors mkdir %TARGET_DIR%\images\cursors
if not exist %TARGET_DIR%\images\ui mkdir %TARGET_DIR%\images\ui
if not exist %TARGET_DIR%\images\home mkdir %TARGET_DIR%\images\home
if not exist %TARGET_DIR%\images\sql mkdir %TARGET_DIR%\images\sql
xcopy /i /s /y /d %IMAGES_DIR%\grt\*.png %TARGET_DIR%\images\grt\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\grt\structs\*.png %TARGET_DIR%\images\grt\structs\. 1> nul 2> nul
xcopy /i /y /d %IMAGES_DIR%\icons\*.png %TARGET_DIR%\images\icons\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\toolbar\*.png %TARGET_DIR%\images\icons\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\changeset\*.png %TARGET_DIR%\images\icons\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\admin\*.png %TARGET_DIR%\images\icons\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\migration\*.png %TARGET_DIR%\images\icons\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\cursors\*.png %TARGET_DIR%\images\cursors\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\icons\MySQLWorkbench.ico %TARGET_DIR%\images\icons\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\icons\MySQLWorkbenchDoc.ico %TARGET_DIR%\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\ui\*.png %TARGET_DIR%\images\ui\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\ui\*.xpm %TARGET_DIR%\images\ui\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\home\*.png %TARGET_DIR%\images\home\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\sql\*.png %TARGET_DIR%\images\sql\. 1> nul 2> nul

echo Copy Resource files ..
if not exist %TARGET_DIR%\data mkdir %TARGET_DIR%\data
xcopy /i /s /y /d %RES_DIR%\grtdata\*.xml %TARGET_DIR%\data\. 1> nul 2> nul
xcopy /i /s /y /d %RES_DIR%\wbdata\*.xml %TARGET_DIR%\data\. 1> nul 2> nul
xcopy /i /s /y /d %RES_DIR%\wbdata\data.db %TARGET_DIR%\data\. 1> nul 2> nul

if not exist %TARGET_DIR%\mysql.profiles mkdir %TARGET_DIR%\mysql.profiles
copy %RES_DIR%\mysql.profiles\*.xml %TARGET_DIR%\mysql.profiles\. 1> nul 2> nul

if not exist %TARGET_DIR%\snippets mkdir %TARGET_DIR%\snippets
copy %RES_DIR%\snippets\*.txt %TARGET_DIR%\snippets\. 1> nul 2> nul

if not exist %TARGET_DIR%\script_templates mkdir %TARGET_DIR%\script_templates
copy %RES_DIR%\scripts\script_templates\*.txt %TARGET_DIR%\script_templates\. 1> nul 2> nul

if not exist %TARGET_DIR%\sys mkdir %TARGET_DIR%\sys
xcopy /i /s /y /d %RES_DIR%\scripts\sys %TARGET_DIR%\sys 1> nul 2> nul 

echo Copy Scripting Libraries...
xcopy /i /s /y /d %RES_DIR%\scripts\vbs\*.vbs %TARGET_DIR%\
xcopy /i /s /y /d %RES_DIR%\scripts\python\*.py %TARGET_DIR%\
xcopy /i /s /y /d %RES_DIR%\scripts\snippets\shell_snippets.* %TARGET_DIR%\
xcopy /i /s /y /d %LIBRARY_DIR%\sshtunnel\sshtunnel.py %TARGET_DIR%\
xcopy /i /s /y /d %RES_DIR%\scripts\shell\*.vbs %TARGET_DIR%\

if not exist %TARGET_DIR%\firewall\ mkdir %TARGET_DIR%\firewall
xcopy /i /s /y /d %RES_DIR%\scripts\firewall\* %TARGET_DIR%\firewall 1> nul 2> nul

echo Copy python workbench files
if not exist %TARGET_DIR%\workbench mkdir %TARGET_DIR%\workbench
xcopy /i /s /y /d %LIBRARY_DIR%\python\workbench\*.py %TARGET_DIR%\workbench

echo Copy python/mforms
xcopy /i /s /y /d %LIBRARY_DIR%\forms\swig\mforms.py %TARGET_DIR%\
xcopy /i /s /y /d %LIBRARY_DIR%\forms\swig\cairo.py %TARGET_DIR%\

echo Copy executables ...

xcopy /i /s /y /d %EXT_BIN_DIR%\mysqldump.exe %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_BIN_DIR%\mysql.exe %TARGET_DIR%\.

xcopy /i /s /y /d %EXT_BIN_DIR%\ogrinfo.exe %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_BIN_DIR%\ogr2ogr.exe %TARGET_DIR%\.


rem Do not remove
rem Python executable needed by MSI Custom Action (to precompile Python files) and maybe some externally executed scripts...
rem xcopy /i /s /y /d %EXT_BIN_DIR%\python*.exe %TARGET_DIR%\.
if %2 == Debug ( set DEBUG_PREFIX=_d)
if not %2 == Debug ( set EXCLUDE_CMD=/xf *_d.* )
robocopy %PYTHON_DIR% %TARGET_DIR% python27%DEBUG_PREFIX%.dll %EXCLUDE_CMD%
robocopy %PYTHON_DIR% %TARGET_DIR% python%DEBUG_PREFIX%.exe %EXCLUDE_CMD%
robocopy %PYTHON_DIR% %TARGET_DIR%\python\site-packages pyodbc%DEBUG_PREFIX%.pyd %EXCLUDE_CMD%

rem =========== Python ============================

echo * Python libraries ...
if not exist %TARGET_DIR%\python mkdir %TARGET_DIR%\python 1> nul 2> nul
if not exist %TARGET_DIR%\python\lib mkdir %TARGET_DIR%\python\lib 1> nul 2> nul
if not exist %TARGET_DIR%\python\DLLs mkdir %TARGET_DIR%\python\DLLs 1> nul 2> nul
if not exist %TARGET_DIR%\python\site-packages mkdir %TARGET_DIR%\python\site-packages 1> nul 2> nul

rem the shared python files
xcopy /i /y /d %PYTHON_COMMON_DIR%\*.py %TARGET_DIR%\python\lib 1> nul 2> nul

xcopy /i /s /y /d %PYTHON_COMMON_DIR%\multiprocessing %TARGET_DIR%\python\lib\multiprocessing 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\email %TARGET_DIR%\python\lib\email 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\encodings %TARGET_DIR%\python\lib\encodings 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\logging %TARGET_DIR%\python\lib\logging 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\json %TARGET_DIR%\python\lib\json 1> nul 2> nul
rem xcopy /i /s /y /d %PYTHON_COMMON_DIR%\unittest %TARGET_DIR%\python\lib\unittest 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\ctypes %TARGET_DIR%\python\lib\ctypes 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\sqlite3 %TARGET_DIR%\python\lib\sqlite3 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\xml %TARGET_DIR%\python\lib\xml 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\importlib %TARGET_DIR%\python\lib\importlib 1> nul 2> nul

rem xcopy /i /y /d %PYTHON_DLLS_DIR%\*.pyd %TARGET_DIR%\python\DLLs 1> nul 2> nul
robocopy %PYTHON_DLLS_DIR% %TARGET_DIR%\python\DLLs *%DEBUG_PREFIX%.pyd %EXCLUDE_CMD% _ctypes_test*.pyd
robocopy %PYTHON_DLLS_DIR% %TARGET_DIR%\python\DLLs *%DEBUG_PREFIX%.pyd %EXCLUDE_CMD% _ctypes_test*.pyd
robocopy %PYTHON_DIR%\ %TARGET_DIR%\python\DLLs *sqlite3*%DEBUG_PREFIX%.dll %EXCLUDE_CMD%

rem site packages that are release type independent
rem xcopy /i /s /y /d %PYTHON_COMMON_DIR%\site-packages\paramiko %TARGET_DIR%\python\site-packages\paramiko 1> nul 2> nul
rem xcopy /i /s /y /d %PYTHON_COMMON_DIR%\site-packages\ecdsa %TARGET_DIR%\python\site-packages\ecdsa 1> nul 2> nul

rem site packages for debug/release types
robocopy %PYTHON_DIR%\pysqlite2 %TARGET_DIR%\python\site-packages\pysqlite2 *%DEBUG_PREFIX%.pyd %EXCLUDE_CMD%
xcopy /i /s /y /d %PYTHON_DIR%\pysqlite2\*.py %TARGET_DIR%\python\site-packages\pysqlite2 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_LIB_DIR%\site-packages\*.pyd %TARGET_DIR%\python\site-packages\ 1> nul 2> nul

rem =======================================

if %2 == Debug ( set DEBUG_PREFIX=d)

echo * SSL library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\libeay32.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\ssleay32.dll %TARGET_DIR%\.

echo * glib libraries ...
xcopy /i /s /y /d %EXT_LIB_DIR%\glib.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\gmodule.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\gobject.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\gthread.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\intl.dll %TARGET_DIR%\.

echo * libxml2 libraries ...
xcopy /i /s /y /d %EXT_LIB_DIR%\libxml2.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\iconv.dll %TARGET_DIR%\.

echo * zlib + libzip libraries ...
xcopy /i /s /y /d %EXT_LIB_DIR%\zlib%DEBUG_PREFIX%1.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\zip.dll %TARGET_DIR%\.

echo * ANTLR4 runtime lib ...
xcopy /i /s /y /d %EXT_LIB_DIR%\antlr4-runtime.dll %TARGET_DIR%\.

echo * cairo library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\libcairo.dll %TARGET_DIR%\. 1> nul 2> nul

echo * png library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\libpng16%DEBUG_PREFIX%.dll %TARGET_DIR%\. 1> nul 2> nul

echo * cppconn library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\mysqlcppconn-7-vs14.dll %TARGET_DIR%\. 1> nul 2> nul

echo * pcre library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\pcre%DEBUG_PREFIX%.dll %TARGET_DIR%\. 1> nul 2> nul
xcopy /i /s /y /d %EXT_LIB_DIR%\pcrecpp%DEBUG_PREFIX%.dll %TARGET_DIR%\. 1> nul 2> nul

echo * sqlite library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\sqlite3.dll %TARGET_DIR%\. 1> nul 2> nul

echo * vsqlite++ library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\vsqlite++.dll %TARGET_DIR%\. 1> nul 2> nul

echo * gdal library + tools ...
xcopy /i /s /y /d %EXT_LIB_DIR%\gdal.dll %TARGET_DIR%\. 1> nul 2> nul
xcopy /i /s /y /d %EXT_LIB_DIR%\*.exe %TARGET_DIR%\. 1> nul 2> nul

echo * ssh library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\ssh.dll %TARGET_DIR%\.

echo * Templates
if not exist %TARGET_DIR%\modules\data\sqlide mkdir %TARGET_DIR%\modules\data\sqlide
xcopy /i /s /y /d %RES_DIR%\sqlidedata\templates\*.* %TARGET_DIR%\modules\data\sqlide\. 1> nul 2> nul

echo * Context Help
xcopy /i /s /y /d %RES_DIR%\sqlidedata\context-help\*.* %TARGET_DIR%\modules\data\sqlide\. 1> nul 2> nul

echo * Copy Sample Files
if not exist %TARGET_DIR%\extras mkdir %TARGET_DIR%\extras
xcopy /i /y /d %1samples\models\* %TARGET_DIR%\extras 1> nul 2> nul

echo * README
if %2 == Release_OSS xcopy /i /y /d %1README.md %TARGET_DIR%
if not %2 == Release_OSS xcopy /i /y /d %1README-commercial.md %TARGET_DIR%

echo * License
if %2 == Release_OSS xcopy /i /y /d %1license.txt %TARGET_DIR%
if not %2 == Release_OSS xcopy /i /y /d %1license-commercial.txt %TARGET_DIR%

rem -------------------------------------------------------------------------------
rem Call sub-scripts

call %1\modules\PrepareOutputDir.cmd %1 %2 %3

rem -------------------------------------------------------------------------------
rem Work is done
echo Output directory preparation complete.

rem Make sure to reset error level
set ERRORLEVEL=0

goto EndOfScript

:Usage

echo This script sets up the output directory so that applications can be started from there and find
echo all directories and files as in the final distribution. The script takes 3 parameters, the
echo SolutionDirectory and ConfigurationName.
echo Use an ABSOLUTE PATH to the solution directory and end it with a backslash!
echo .
echo Usage:
echo   %0 SolutionDirectory ConfigurationName Architecture
echo .
echo Example:
echo   %0 "C:\Documents and Settings\mysqldev\My Documents\work\mysql-workbench\" Debug x64
echo .

:EndOfScript
