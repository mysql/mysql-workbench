/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <grtpp_util.h>

//================================================================================
// db_RoutineGroup

static void routine_group_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                                       db_RoutineGroup *group) {
  (*group->signal_contentChanged())();
}

void db_RoutineGroup::init() {
  // No need in disconnet management since signal it part of object
  _list_changed_signal.connect(
    std::bind(&routine_group_list_changed, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, this));
}

db_RoutineGroup::~db_RoutineGroup() {
}
