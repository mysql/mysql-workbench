/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "sql_facade.h"
#include "base/string_utilities.h"

using namespace grt;

//--------------------------------------------------------------------------------------------------
/*
SqlFacade::Ref SqlFacade::instance_for_db_obj(db_DatabaseObjectRef db_obj)
{
  db_mgmt_RdbmsRef rdbms;

  if (db_obj.is_instance(db_Table(db_obj.get_grt()).get_metaclass())
    || db_obj.is_instance(db_View(db_obj.get_grt()).get_metaclass())
    || db_obj.is_instance(db_Routine(db_obj.get_grt()).get_metaclass())
    || db_obj.is_instance(db_RoutineGroup(db_obj.get_grt()).get_metaclass()))
  {
    rdbms = db_mgmt_RdbmsRef::cast_from(db_obj->owner()->owner()->owner()->get_member("rdbms"));
  }
  else if (db_obj.is_instance(db_Schema(db_obj.get_grt()).get_metaclass()))
  {
    rdbms = db_mgmt_RdbmsRef::cast_from(db_obj->owner()->owner()->get_member("rdbms"));
  }

  return instance_for_rdbms(rdbms);
}
*/
//--------------------------------------------------------------------------------------------------

SqlFacade::Ref SqlFacade::instance_for_rdbms(db_mgmt_RdbmsRef rdbms) {
  return instance_for_rdbms_name(rdbms->name());
}

//--------------------------------------------------------------------------------------------------

SqlFacade::Ref SqlFacade::instance_for_rdbms_name(const std::string &name) {
  const char *def_module_name = "SqlFacade";
  std::string module_name = name + def_module_name;
  SqlFacade::Ref module = dynamic_cast<SqlFacade::Ref>(grt::GRT::get()->get_module(module_name));
  if (!module)
    throw std::runtime_error(base::strfmt("Can't get '%s' module.", module_name.c_str()));
  return module;
}

//--------------------------------------------------------------------------------------------------
