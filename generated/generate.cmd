@echo off
echo ================================================================================
echo MySQL Interface Generation Script
echo ================================================================================
echo This script will build the tools solution (genobj, genwrap) and generate all header files
echo It is necessary to have a good build of WB in the bin/debug folder.
echo ================================================================================
echo .

echo Starting GRT interface generation.
echo .

rem Set search path to our debug folder to be able to find all required dlls for the wrapper.
rem Note: This requires that WB was successfully build at least once, otherwise the folder does not
rem       yet have all the dlls.
set PATH=../bin/debug;%PATH%

echo --------------------------------------------------------------------------------
echo.
echo Building generation tools with original source files...
pushd ..\tools\
devenv tools.sln /build "Debug|Win32"
popd
if errorlevel 1 goto error

echo.
echo Generating new wrappers...
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.xml ../res/grt grts ../backend/wbpublic/objimpl
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.app.xml ../res/grt/ grts ../backend/wbpublic/objimpl/app
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.mgmt.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mgmt
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.migration.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.migration
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.mssql.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mssql
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.mysql.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mysql
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.oracle.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.oracle
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.query.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.query
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.db.sybase.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.sybase
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.eer.xml ../res/grt/ grts ../backend/wbpublic/objimpl/eer
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.model.xml ../res/grt/ grts ../backend/wbpublic/objimpl/model
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.workbench.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.workbench.logical.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.logical
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.workbench.model.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.model
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.workbench.physical.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.physical
..\tools\genobj\Debug\genobj.exe ../res/grt/structs.workbench.model.reporting.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.model.reporting

echo.
echo Building generation tools with new wrappers...
pushd ..\tools\
devenv tools.sln /build "Debug|Win32"
popd
if errorlevel 1 goto error

echo --------------------------------------------------------------------------------
echo.
echo Generating interface files...

..\tools\genwrap\Debug\genwrap.exe interfaces ../modules/interfaces/plugin.h grti/plugin.h
..\tools\genwrap\Debug\genwrap.exe interfaces ../modules/interfaces/wb_model_reporting.h grti/wb_model_reporting.h
..\tools\genwrap\Debug\genwrap.exe interfaces ../modules/interfaces/wbvalidation.h grti/wbvalidation.h

goto finish

:error
echo --------------------------------------------------------------------------------
echo ##### The script aborted due to an error #####
pause
goto end

:finish
echo --------------------------------------------------------------------------------
echo Successfully finished the script.

:end
echo ================================================================================
