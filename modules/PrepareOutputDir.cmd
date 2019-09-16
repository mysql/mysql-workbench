@echo off

rem -------------------------------------------------------------------------------
rem Check parameter
if [%1] == [] goto Usage
if [%2] == [] goto Usage
if [%3] == [] goto Usage

rem -------------------------------------------------------------------------------
rem Script start
echo Preparing output modules directory...

rem -------------------------------------------------------------------------------
rem Set directory variables
set MODULES_DIR=%1modules
echo External libraries directory: %MODULES_DIR%

set TARGET_DIR=%1\bin\%3\%2
echo Target directory: %TARGET_DIR%

rem -------------------------------------------------------------------------------
rem Copy the files to the target directory

echo Copy Module Resource files ..
if not exist %TARGET_DIR%\modules\data mkdir %TARGET_DIR%\modules\data

rem sqlide data files
copy %MODULES_DIR%\..\backend\wbpublic\sqlide\res\*.* %TARGET_DIR%\modules\data\. 1> nul 2> nul
if not exist %TARGET_DIR%\modules\data\sqlide mkdir %TARGET_DIR%\modules\data\sqlide

rem db.mysql.sqlide data files
copy %MODULES_DIR%\db.mysql.sqlide\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul

rem db.mysql data files
copy %MODULES_DIR%\db.mysql\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.mssql\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.generic\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.sybase\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.postgresql\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.sql92\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.sqlanywhere\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.sqlite\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul
copy %MODULES_DIR%\db.msaccess\res\*.xml %TARGET_DIR%\modules\data\. 1> nul 2> nul

rem catalog diff reporting templates
if not exist %TARGET_DIR%\modules\data\db_mysql_catalog_reporting mkdir %TARGET_DIR%\modules\data\db_mysql_catalog_reporting
xcopy %MODULES_DIR%\db.mysql\res\db_mysql_catalog_reporting\* %TARGET_DIR%\modules\data\db_mysql_catalog_reporting /Y /E 1> nul 2> nul


rem create temp file for xcopy exclude TODO: this is obviously not needed and can go when switching to robocopy.
echo .svn >> _xcopy_exclude.txt

rem schema reporting templates
if not exist %TARGET_DIR%\modules\data\wb_model_reporting mkdir %TARGET_DIR%\modules\data\wb_model_reporting
xcopy %MODULES_DIR%\wb.model\res\wb_model_reporting\* %TARGET_DIR%\modules\data\wb_model_reporting /Y /E /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul

rem copy non-binary modules
xcopy %MODULES_DIR%\db.sybase\* %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\wb.utils\* %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\wb.sqlide\* %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.mysql\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.mssql\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.generic\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.sql92\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.postgresql\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.sqlanywhere\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.sqlite\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\db.msaccess\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul

rem copy WBA extension modules
echo Copy WBA Extension files ..
if not %2 == Release_OSS xcopy %MODULES_DIR%\..\plugins\wb.admin\internal %TARGET_DIR%\modules\ /S /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul

rem copy non-binary modules
xcopy %MODULES_DIR%\..\plugins\wb.admin\frontend\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\wb.admin\backend\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\migration\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\migration\frontend\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\migration\backend\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\migration\dbcopy\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\wb.bugreport\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\wb.query.analysis\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\wb.updater\backend\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul
xcopy %MODULES_DIR%\..\plugins\wb.sqlide\*.py %TARGET_DIR%\modules\ /Y /EXCLUDE:_xcopy_exclude.txt 1> nul 2> nul

rem remove temp file
del _xcopy_exclude.txt

rem -------------------------------------------------------------------------------
rem Work is done
echo Modules directory preparation complete.

rem Make sure to reset error level
set ERRORLEVEL=0

goto EndOfScript

:Usage

echo This script sets up the output directory so that applications can be started from there and find
echo all directories and files as in the final distribution. The script takes 2 parameters, the 
echo SolutionDirectory and the ConfigurationName
echo .
echo Usage: 
echo   %0 SolutionDirectory ConfigurationName Architecture
echo .
echo Example:
echo   %0 "C:\Documents and Settings\mysqldev\My Documents\work\mysql-workbench" Debug x64
echo .

:EndOfScript
