#================================================================================
# MySQL Interface Generation Script
#================================================================================
# This script will generate all header files.
# In order to make this work Workbench must be built completely, including genobj + genwrap targets.
# ================================================================================

BINPATH="../DerivedData/MySQLWorkbench/Build/Products/Debug/MySQLWorkbench.app/Contents/MacOS"

echo --------------------------------------------------------------------------------
echo
echo Generating new wrappers...
echo

$BINPATH/genobj ../res/grt/structs.xml ../res/grt grts ../backend/wbpublic/objimpl
$BINPATH/genobj ../res/grt/structs.app.xml ../res/grt/ grts ../backend/wbpublic/objimpl/app
$BINPATH/genobj ../res/grt/structs.db.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db
$BINPATH/genobj ../res/grt/structs.db.mgmt.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mgmt
$BINPATH/genobj ../res/grt/structs.db.migration.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.migration
$BINPATH/genobj ../res/grt/structs.db.mssql.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mssql
$BINPATH/genobj ../res/grt/structs.db.mysql.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.mysql
$BINPATH/genobj ../res/grt/structs.db.oracle.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.oracle
$BINPATH/genobj ../res/grt/structs.db.query.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.query
$BINPATH/genobj ../res/grt/structs.db.ng.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.ng
$BINPATH/genobj ../res/grt/structs.db.sybase.xml ../res/grt/ grts ../backend/wbpublic/objimpl/db.sybase
$BINPATH/genobj ../res/grt/structs.eer.xml ../res/grt/ grts ../backend/wbpublic/objimpl/eer
$BINPATH/genobj ../res/grt/structs.model.xml ../res/grt/ grts ../backend/wbpublic/objimpl/model
$BINPATH/genobj ../res/grt/structs.workbench.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench
$BINPATH/genobj ../res/grt/structs.workbench.logical.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.logical
$BINPATH/genobj ../res/grt/structs.workbench.model.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.model
$BINPATH/genobj ../res/grt/structs.workbench.physical.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.physical
$BINPATH/genobj ../res/grt/structs.workbench.model.reporting.xml ../res/grt/ grts ../backend/wbpublic/objimpl/workbench.model.reporting
$BINPATH/genobj ../res/grt/structs.ui.xml ../res/grt/ grts ../backend/wbpublic/objimpl/ui
$BINPATH/genobj ../res/grt/structs.wrapper.xml ../res/grt/ grts ../backend/wbpublic/objimpl/wrapper

echo
echo --------------------------------------------------------------------------------
echo
echo Generating interface files...
echo

$BINPATH/genwrap interfaces ../modules/interfaces/plugin.h grti/plugin.h
$BINPATH/genwrap interfaces ../modules/interfaces/wb_model_reporting.h grti/wb_model_reporting.h
$BINPATH/genwrap interfaces ../modules/interfaces/wbvalidation.h grti/wbvalidation.h

echo
echo ================================================================================
