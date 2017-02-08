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

set EXT_LIB_DIR=%1..\mysql-win-res\lib\%3
echo Windows resource directory: %EXT_LIB_DIR%

set EXT_BIN_DIR=%1..\mysql-win-res\redist-bin\%3\%2
echo External binary directory: %EXT_BIN_DIR%

set EXT_DATA_DIR=%1..\mysql-win-res\data
echo External data directory: %EXT_DATA_DIR%

set EXT_SRC_DIR=%1..\mysql-win-res\source
echo External source directory: %EXT_SRC_DIR%

set TARGET_DIR=%1bin\%3\%2
echo Target directory: %TARGET_DIR%

set PYTHON_COMMON_DIR=%1..\mysql-win-res\lib\Python
echo Python common library directory: %PYTHON_COMMON_DIR%

set PYTHON_LIB_DIR=%EXT_LIB_DIR%\python\%2
echo Python library directory: %PYTHON_LIB_DIR%

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
xcopy /i /s /y /d %EXT_DATA_DIR%\cursors\*.cur %TARGET_DIR%\images\cursors\. 1> nul 2> nul
xcopy /i /s /y /d %EXT_DATA_DIR%\icons\MySQLWorkbench.ico %TARGET_DIR%\images\icons\. 1> nul 2> nul
xcopy /i /s /y /d %EXT_DATA_DIR%\icons\MySQLWorkbenchDoc.ico %TARGET_DIR%\. 1> nul 2> nul
xcopy /i /s /y /d %EXT_DATA_DIR%\icons\MySQLWBPlugin.ico %TARGET_DIR%\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\ui\*.png %TARGET_DIR%\images\ui\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\ui\*.xpm %TARGET_DIR%\images\ui\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\home\*.png %TARGET_DIR%\images\home\. 1> nul 2> nul
xcopy /i /s /y /d %IMAGES_DIR%\sql\*.png %TARGET_DIR%\images\sql\. 1> nul 2> nul

echo Copy Resource files ..
if not exist %TARGET_DIR%\data mkdir %TARGET_DIR%\data
xcopy /i /s /y /d %RES_DIR%\grtdata\*.xml %TARGET_DIR%\data\. 1> nul 2> nul
xcopy /i /s /y /d %RES_DIR%\wbdata\*.xml %TARGET_DIR%\data\. 1> nul 2> nul
xcopy /i /s /y /d %RES_DIR%\wbdata\data.db %TARGET_DIR%\data\. 1> nul 2> nul

echo Copy parser grammar + support files
xcopy /i /s /y /d %LIBRARY_DIR%\parsers\MySQL.tokens %TARGET_DIR%\data 1> nul 2> nul
xcopy /i /s /y /d %LIBRARY_DIR%\parsers\MySQLSimpleParser.tokens %TARGET_DIR%\data 1> nul 2> nul
xcopy /i /s /y /d %LIBRARY_DIR%\parsers\grammars\MySQL.g %TARGET_DIR%\data 1> nul 2> nul
xcopy /i /s /y /d %LIBRARY_DIR%\parsers\grammars\MySQLSimpleParser.g %TARGET_DIR%\data 1> nul 2> nul

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

rem Do not remove
rem Python executable needed by MSI Custom Action (to precompile Python files) and maybe some externally executed scripts...
xcopy /i /s /y /d %EXT_BIN_DIR%\python*.exe %TARGET_DIR%\.


echo * MySQL client library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\mysql\%2\libmysql*.dll %TARGET_DIR%\.
rem xcopy /i /s /y /d %EXT_LIB_DIR%\mysql\%2\libmysql*.pdb %TARGET_DIR%\. 1> nul 2> nul

echo * MySQL cdbc driver ...
rem copy %EXT_LIB_DIR%\cppconn\mysql\%2\mysqlcppconn.dll %TARGET_DIR%\. 1> nul 2> nul

echo * glib libraries ...
xcopy /i /s /y /d %EXT_LIB_DIR%\glib\glib.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\glib\gmodule.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\glib\gobject.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\glib\gthread.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\glib\libintl-8.dll %TARGET_DIR%\.

echo * libxml2 libraries ...
xcopy /i /s /y /d %EXT_LIB_DIR%\libxml\libxml2.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\libxml\libiconv.dll %TARGET_DIR%\.

echo * zlib + libzip libraries ...
xcopy /i /s /y /d %EXT_LIB_DIR%\zlib\%2\zlib.dll %TARGET_DIR%\.
xcopy /i /s /y /d %EXT_LIB_DIR%\libzip\%2\libzip.dll %TARGET_DIR%\.

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

xcopy /i /y /d %PYTHON_LIB_DIR%\*.dll %TARGET_DIR%\. 1> nul 2> nul
xcopy /i /y /d %PYTHON_LIB_DIR%\DLLs\*.pyd %TARGET_DIR%\python\DLLs 1> nul 2> nul

rem site packages that are release type independent
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\site-packages\paramiko %TARGET_DIR%\python\site-packages\paramiko 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_COMMON_DIR%\site-packages\ecdsa %TARGET_DIR%\python\site-packages\ecdsa 1> nul 2> nul

rem site packages for debug/release types
xcopy /i /s /y /d %PYTHON_LIB_DIR%\site-packages\Crypto %TARGET_DIR%\python\site-packages\Crypto 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_LIB_DIR%\site-packages\pysqlite2 %TARGET_DIR%\python\site-packages\pysqlite2 1> nul 2> nul
xcopy /i /s /y /d %PYTHON_LIB_DIR%\site-packages\*.pyd %TARGET_DIR%\python\site-packages\ 1> nul 2> nul

rem Cleanup stuff that should never or only under certain circumstances stay in the target folder.
rem if "%2"=="Release" del %TARGET_DIR%\python\*_d.* /S
rem del %TARGET_DIR%\python\*85.dll %TARGET_DIR%\python\_tkinter.*  1> nul 2> nul

rem =======================================

echo * cairo library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\cairo\*.dll %TARGET_DIR%\. 1> nul 2> nul

echo * png library ...
xcopy /i /s /y /d %EXT_LIB_DIR%\libpng\libpng.dll %TARGET_DIR%\.

echo * ctemplate library ...
copy %EXT_LIB_DIR%\ctemplate\%2\libctemplate.dll %TARGET_DIR%\.

echo * cppconn library ...
copy %EXT_LIB_DIR%\mysqlcppconn\%2\mysqlcppconn.dll %TARGET_DIR%\.

echo * pcre library ...
copy %EXT_LIB_DIR%\pcre\%2\pcre.dll %TARGET_DIR%\.

echo * sqlite library ...
copy %EXT_LIB_DIR%\sqlite\%2\sqlite3.dll %TARGET_DIR%\.

echo * vsqlite++ library ...
copy "%EXT_LIB_DIR%\vsqlite++\%2\vsqlite++.dll" %TARGET_DIR%\.

echo * gdal library + tools ...
copy %EXT_LIB_DIR%\gdal\%2\gdal.dll %TARGET_DIR%\.
copy %EXT_LIB_DIR%\gdal\%2\*.exe %TARGET_DIR%\.

echo * Templates
if not exist %TARGET_DIR%\modules\data\sqlide mkdir %TARGET_DIR%\modules\data\sqlide
xcopy /i /s /y /d %RES_DIR%\sqlidedata\templates\*.* %TARGET_DIR%\modules\data\sqlide\. 1> nul 2> nul

echo * Copy Sample Files
if not exist %TARGET_DIR%\extras mkdir %TARGET_DIR%\extras
xcopy /i /y /d %1samples\models\* %TARGET_DIR%\extras 1> nul 2> nul

echo * README files
xcopy /i /y /d %1README %TARGET_DIR%

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
