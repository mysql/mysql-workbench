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
