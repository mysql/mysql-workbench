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

#include "sql_semantic_check.h"

Sql_semantic_check::Sql_semantic_check() {
}

void Sql_semantic_check::reset_context_objects() {
  _context_schema = db_SchemaRef();
  _context_table = db_TableRef();
  _context_trigger = db_TriggerRef();
  _context_view = db_ViewRef();
  _context_routine = db_RoutineRef();
  _context_routine_group = db_RoutineGroupRef();
}

void Sql_semantic_check::context_object(db_SchemaRef obj) {
  _context_schema = obj;
}

void Sql_semantic_check::context_object(db_TableRef obj) {
  _context_table = obj;
}

void Sql_semantic_check::context_object(db_TriggerRef obj) {
  _context_trigger = obj;
}

void Sql_semantic_check::context_object(db_ViewRef obj) {
  _context_view = obj;
}

void Sql_semantic_check::context_object(db_RoutineRef obj) {
  _context_routine = obj;
}

void Sql_semantic_check::context_object(db_RoutineGroupRef obj) {
  _context_routine_group = obj;
}
