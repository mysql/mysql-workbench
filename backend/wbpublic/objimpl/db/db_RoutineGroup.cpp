/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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
