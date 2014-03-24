#include "stdafx.h"

#include "grtdb/db_object_helpers.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.h"

#include "grtpp.h"


using namespace grt;


#include "db_frw_eng_be.h"


Db_frw_eng::Db_frw_eng(bec::GRTManager *grtm)
  : Db_plugin(), DbMySQLValidationPage(grtm), _export(grtm)
{
  {
    workbench_DocumentRef doc= workbench_DocumentRef::cast_from(grtm->get_grt()->get("/wb/doc"));
    Db_frw_eng::grtm(grtm, false);
  }

  _catalog= db_mysql_CatalogRef::cast_from(_grtm->get_grt()->get("/wb/doc/physicalModels/0/catalog"));
}


void Db_frw_eng::start_apply_script_to_db()
{
  sql_script(_sql_script);
  Db_plugin::exec_task();
}


void Db_frw_eng::setup_grt_string_list_models_from_catalog(
  bec::GrtStringListModel **users_model,
  bec::GrtStringListModel **users_exc_model,
  bec::GrtStringListModel **tables_model,
  bec::GrtStringListModel **tables_exc_model,
  bec::GrtStringListModel **views_model,
  bec::GrtStringListModel **views_exc_model,
  bec::GrtStringListModel **routines_model,
  bec::GrtStringListModel **routines_exc_model,
  bec::GrtStringListModel **triggers_model,
  bec::GrtStringListModel **triggers_exc_model)
{
  _export.setup_grt_string_list_models_from_catalog(
    users_model,
    users_exc_model,
    tables_model,
    tables_exc_model,
    views_model,
    views_exc_model,
    routines_model,
    routines_exc_model,
    triggers_model,
    triggers_exc_model);
}

