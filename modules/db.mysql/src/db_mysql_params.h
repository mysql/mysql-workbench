/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "db_mysql_public_interface.h"

#include <string>
#include "grts/structs.db.mysql.h"

namespace dbmysql {

  template <class P, class T>
  bool get_parent(P& parent, const T& object) {
    GrtObjectRef owner = object->owner();
    if (!owner.is_valid())
      return false;

    if (!P::can_wrap(owner))
      return get_parent(parent, owner);

    parent = P::cast_from(owner);
    return true;
  }

  inline std::string full_name(db_DatabaseObjectRef obj, db_SchemaRef schema = db_SchemaRef()) {
    std::string res = '`' + *obj->name() + '`';
    if (get_parent(schema, obj))
      return '`' + *schema->name() + "`." + res;

    return res;
  }

  enum EngineId {
    eetMySAM = 0,
    eetInnoDB,
    eetFalcon,
    eetMerge,
    eetMemory,
    eetExample,
    eetFederated,
    eetArchive,
    eetCsv,
    eetBlackhole,
    eetOTHER
  };

  EngineId MYSQLMODULEDBMYSQL_PUBLIC_FUNC engine_id_by_name(const char* name);

  std::string MYSQLMODULEDBMYSQL_PUBLIC_FUNC engine_name_by_id(EngineId id);

  db_mysql_StorageEngineRef MYSQLMODULEDBMYSQL_PUBLIC_FUNC engine_by_name(const char* name);

  db_mysql_StorageEngineRef MYSQLMODULEDBMYSQL_PUBLIC_FUNC engine_by_id(EngineId id);

  grt::ListRef<db_mysql_StorageEngine> MYSQLMODULEDBMYSQL_PUBLIC_FUNC get_known_engines();

  bool MYSQLMODULEDBMYSQL_PUBLIC_FUNC check_valid_characters(const char* str);
}
