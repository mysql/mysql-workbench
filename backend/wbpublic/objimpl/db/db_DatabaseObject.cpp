/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.db.h>

#include "base/string_utilities.h"
#include "base/util_functions.h"

#include <grtpp_undo_manager.h>

#include "grt/common.h"

//================================================================================
// db_DatabaseObject

void db_DatabaseObject::init() {
}

db_DatabaseObject::~db_DatabaseObject() {
}

void db_DatabaseObject::lastChangeDate(const grt::StringRef &value) {
  grt::ValueRef ovalue(_lastChangeDate);

  _lastChangeDate = value;

  member_changed("lastChangeDate", ovalue, value);

  if (_owner.is_valid() && _owner.is_instance(db_Schema::static_class_name())) {
    db_SchemaRef schema(db_SchemaRef::cast_from(_owner));
    (*schema->signal_refreshDisplay())(this);
  }
}

void db_DatabaseObject::name(const grt::StringRef &value) {
  grt::StringRef oname(_name);

  grt::AutoUndo undo(!is_global());

  _name = value;
  if (_owner.is_valid()) // don't update if the object is still being loaded
    _lastChangeDate = base::fmttime(0, DATETIME_FMT);

  member_changed("name", oname, value);

  undo.end(base::strfmt(_("Rename '%s' to '%s'"), oname.c_str(), value.c_str()));

  if (_owner.is_valid() && _owner.is_instance(db_Schema::static_class_name())) {
    db_SchemaRef schema(db_SchemaRef::cast_from(_owner));
    (*schema->signal_refreshDisplay())(this);
  }
}

void db_DatabaseObject::owner(const grt::Ref<GrtNamedObject> &value) {
  grt::ValueRef ovalue(_owner);
  _owner = value;
  member_changed("owner", ovalue, value);
}
