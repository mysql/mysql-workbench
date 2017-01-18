/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MODULE_DB_MYSQL_SHARED_CODE_H_
#define _MODULE_DB_MYSQL_SHARED_CODE_H_

#include "base/string_utilities.h"

inline std::string get_qualified_schema_object_name(const GrtNamedObjectRef object) {
  if (object.is_instance("db.Catalog"))
    return std::string("`").append(object->name().c_str()).append("`");
  else if (object.is_instance("db.Trigger"))
    return std::string("`")
      .append(object->owner()->owner()->name().c_str())
      .append("`.`")
      .append(object->name().c_str())
      .append("`");
  else if (object.is_instance("db.Index"))
    return std::string("`")
      .append(object->owner()->owner()->name().c_str())
      .append("`.`")
      .append(object->owner()->name().c_str())
      .append("`.`")
      .append(object->name().c_str())
      .append("`");
  else if (object.is_instance("db.User"))
    return std::string("`").append(object->name()).append("`");

  return std::string("`")
    .append(object->owner()->name().c_str())
    .append("`.`")
    .append(object->name().c_str())
    .append("`");
}

inline std::string get_object_old_name(GrtNamedObjectRef object) {
  if (strlen(object->oldName().c_str()) > 0 && (!db_mysql_SchemaRef::can_wrap(object)))
    return std::string(object->oldName().c_str());
  return std::string(object->name().c_str());
}

inline std::string get_object_old_name(GrtObjectRef object) {
  if (GrtNamedObjectRef::can_wrap(object) && (!db_mysql_SchemaRef::can_wrap(object)))
    return get_object_old_name(GrtNamedObjectRef::cast_from(object));
  return std::string(object->name().c_str());
}

inline std::string get_qualified_schema_object_old_name(GrtNamedObjectRef object) {
  if (object.is_instance("db.Catalog"))
    return std::string("`").append(get_object_old_name(object)).append("`");
  else if (object.is_instance("db.Trigger"))
    return std::string("`")
      .append(get_object_old_name(object->owner()->owner()))
      .append("`.`")
      .append(get_object_old_name(object))
      .append("`");
  else if (object.is_instance("db.Index"))
    return std::string("`")
      .append(get_object_old_name(object->owner()->owner()))
      .append("`.`")
      .append(get_object_old_name(object->owner()))
      .append("`.`")
      .append(get_object_old_name(object))
      .append("`");
  else if (object.is_instance("db.User"))
    return std::string("`").append(get_object_old_name(object)).append("`");

  return std::string("`")
    .append(get_object_old_name(object->owner()))
    .append("`.`")
    .append(get_object_old_name(object))
    .append("`");
}

inline std::string get_full_object_name_for_key(GrtNamedObjectRef object, const bool case_sensitive) {
  const std::string result =
    std::string(object.class_name())
      .append("::")
      .append(get_qualified_schema_object_old_name(object).append("::").append(object->name()));
  return case_sensitive ? result : base::toupper(result);
}

inline std::string get_old_object_name_for_key(GrtNamedObjectRef object, const bool case_sensitive) {
  std::string old_name = object->oldName().empty() ? object->name() : object->oldName();

  const std::string result = std::string(object.class_name())
                               .append("::")
                               .append(get_qualified_schema_object_old_name(object).append("::").append(old_name));
  return case_sensitive ? result : base::toupper(result);
}

#endif
