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

#include <grtpp_util.h>

//================================================================================
// db_Trigger

void db_Trigger::init() {
}

db_Trigger::~db_Trigger() {
}

void db_Trigger::event(const grt::StringRef &value) {
  grt::ValueRef ovalue(_event);

  if (_owner.is_valid() && _event != value)
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("trigger");

  _event = value;
  member_changed("event", ovalue, value);
}

void db_Trigger::name(const grt::StringRef &value) {
  grt::ValueRef ovalue(_name);

  if (_owner.is_valid() && _name != value)
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("trigger");

  _name = value;
  member_changed("name", ovalue, value);
}

void db_Trigger::timing(const grt::StringRef &value) {
  grt::ValueRef ovalue(_timing);

  if (_owner.is_valid() && _timing != value)
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("trigger");

  _timing = value;
  member_changed("timing", ovalue, value);
}
