/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

  grt::ListRef<db_mysql_StorageEngine> MYSQLMODULEDBMYSQL_PUBLIC_FUNC get_known_engines();
}
