/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "stdafx.h"

#include "grt/grt_manager.h"
#include "grts/structs.db.h"
#include "grts/structs.workbench.physical.h"
#include "sql_import_be.h"
#include "grtsqlparser/sql_facade.h"
#include "base/string_utilities.h"

void Sql_import::grtm(bec::GRTManager *grtm)
{
  if (grtm)
  {
    grt::GRT *grt= grtm->get_grt();
    _options= grt::DictRef(grt);
    _doc= workbench_DocumentRef::cast_from(grt->get("/wb/doc"));

    // init some options based on global defaults
    // FE will query them to init controls state
    {
      grt::DictRef options= grt::DictRef::cast_from(grtm->get_grt()->get("/wb/options/options"));
      const char *option_names[]= { "SqlIdentifiersCS" };
      for (size_t n= 0, count= sizeof(option_names)/sizeof(option_names[0]); n < count; ++n)
        _options.set(option_names[n], options.get(option_names[n]));
    }
  }
  else
  {
    _doc= workbench_DocumentRef();
  }
}


db_CatalogRef Sql_import::target_catalog()
{
  return _doc->physicalModels().get(0)->catalog();
}


boost::function<grt::ValueRef (grt::GRT*)> Sql_import::get_task_slot()
{
  return boost::bind(&Sql_import::parse_sql_script, this, _1, target_catalog(), sql_script());
}


boost::function<grt::ValueRef (grt::GRT*)> Sql_import::get_autoplace_task_slot()
{
  return boost::bind(&Sql_import::autoplace_grt, this, _1);
}


grt::StringRef Sql_import::parse_sql_script(grt::GRT *grt, db_CatalogRef catalog, const std::string &sql_script)
{
  SqlFacade::Ref sql_parser= SqlFacade::instance_for_rdbms(
    db_mgmt_RdbmsRef::cast_from(catalog->owner().get_member("rdbms")));

  grt::ListRef<GrtObject> created_objects(grt);
  _options.set("created_objects", created_objects);

  parse_sql_script(sql_parser, catalog, sql_script, _options);

  return grt::StringRef("The SQL script was parsed");
}


grt::ListRef<GrtObject> Sql_import::get_created_objects()
{
  return grt::ListRef<GrtObject>::cast_from(_options.get("created_objects"));
}


void Sql_import::parse_sql_script(SqlFacade::Ref sql_parser, db_CatalogRef &catalog, const std::string &sql_script, grt::DictRef &options)
{
  grt::AutoUndo undo(_doc.get_grt());
  options.set("sql_script_codeset", grt::StringRef(_sql_script_codeset));
  sql_parser->parseSqlScriptFileEx(catalog, sql_script, options);
  undo.end(_("Reverse Engineer from SQL Script"));
}


grt::ValueRef Sql_import::autoplace_grt(grt::GRT *grt)
{
  db_CatalogRef catalog= target_catalog();

  workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(catalog->owner()));

  grt::ListRef<db_DatabaseObject> dbobjects(grt);
  grt::ListRef<GrtObject> objects(grt::ListRef<GrtObject>::cast_from(_options.get("created_objects")));

  for (grt::ListRef<GrtObject>::const_iterator iter= objects.begin();
    iter != objects.end(); ++iter)
  {
    if ((*iter).is_instance<db_DatabaseObject>())
      dbobjects.insert(db_DatabaseObjectRef::cast_from(*iter));
  }

  if (dbobjects.count() > 0)
  {
    grt::Module *module= grt->get_module("WbModel");

    grt::BaseListRef args(grt);

    args.ginsert(model);
    args.ginsert(objects);

    module->call_function("createDiagramWithObjects", args);
  }

  return grt::ValueRef();
}
