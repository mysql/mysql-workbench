#include "stdafx.h"

#include "grtdb/db_object_helpers.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.workbench.h"


#include "grtpp.h"


using namespace grt;


#include "db_mysql_sql_sync.h"

#include "diff/diffchange.h"

DbMySQLSync::DbMySQLSync(bec::GRTManager *grtm)
  : Db_plugin(), DbMySQLValidationPage(grtm)
{
  DbMySQLSync::grtm(grtm, false);
  _catalog= db_mysql_CatalogRef::cast_from(_grtm->get_grt()->get("/wb/doc/physicalModels/0/catalog"));
}


void DbMySQLSync::set_option(const std::string& name, const std::string& value)
{
  if(name.compare("InputFileName") == 0)
    _input_filename= value;
  else if(name.compare("OutputFileName") == 0)
    _output_filename= value;
  else if(name.compare("ScriptToApply") == 0)
    _script_to_apply= value;
}


void DbMySQLSync::start_apply_script_to_db()
{
  sql_script(_script_to_apply);
  Db_plugin::exec_task();
}

