@echo off
echo ================================================================================
echo MySQL Interface Generation Script
echo ================================================================================
echo This script will build the tools solution (genobj, genwrap) and generate all header files
echo It is necessary to have a good build of WB in the solution bin folder (we are using debug x64 here).
echo ================================================================================
echo .

echo Starting GRT interface generation.
echo .

rem Set search path to our debug folder to be able to find all required dlls for the wrapper.
rem Note: This requires that WB was successfully build at least once, otherwise the folder does not
rem       yet have all the dlls. We use the x64 debug build here, as the architecture doesn't make a difference.
set PATH=../bin/x64/Debug;%PATH%

echo --------------------------------------------------------------------------------
echo.
echo Building generation tools with original source files...
pushd ..\tools\
devenv tools.sln /build "Debug|x64"
popd
if errorlevel 1 goto error

echo.
echo Generating new wrappers...
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.xml ../res/grt grts ../backend/wbpublic/objimpl
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.app.xml ../res/grt/ grts ../backend/wbpublic/objimpl/app
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.mgmt.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mgmt
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.migration.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.migration
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.mssql.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mssql
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.mysql.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mysql
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.oracle.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.oracle
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.query.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.query
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.ng.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.ng
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.db.sybase.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.sybase
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.eer.xml ../res/grt/ grts ../backend/wbpublic/objimpl/eer
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.model.xml ../res/grt/ grts ../backend/wbpublic/objimpl/model
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.workbench.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.workbench.logical.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.logical
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.workbench.model.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.model
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.workbench.physical.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.physical
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.workbench.model.reporting.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.model.reporting
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.ui.xml ../res/grt/ grts ../backend/wbpublic/objimpl/ui
..\tools\bin\x64\Debug\genobj.exe ../res/grt/structs.wrapper.xml ../res/grt/ grts ../backend/wbpublic/objimpl/wrapper

echo.
echo Building generation tools with new wrappers...
pushd ..\tools\
devenv tools.sln /build "Debug|x64"
popd
if errorlevel 1 goto error

echo --------------------------------------------------------------------------------
echo.
echo Generating interface files...

..\tools\bin\x64\Debug\genwrap.exe interfaces ../modules/interfaces/plugin.h grti/plugin.h
..\tools\bin\x64\Debug\genwrap.exe interfaces ../modules/interfaces/wb_model_reporting.h grti/wb_model_reporting.h
..\tools\bin\x64\Debug\genwrap.exe interfaces ../modules/interfaces/wbvalidation.h grti/wbvalidation.h

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
