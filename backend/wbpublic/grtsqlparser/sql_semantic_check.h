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

#pragma once

#include "wbpublic_public_interface.h"
#include "sql_syntax_check.h"

/**
 * Defines interface to check semantic of provided SQL statement/script.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_semantic_check : virtual public Sql_syntax_check {
public:
  typedef std::shared_ptr<Sql_semantic_check> Ref;

protected:
  Sql_semantic_check();

public:
  void reset_context_objects();
  void context_object(db_SchemaRef obj);
  void context_object(db_TableRef obj);
  void context_object(db_TriggerRef obj);
  void context_object(db_ViewRef obj);
  void context_object(db_RoutineRef obj);
  void context_object(db_RoutineGroupRef obj);

protected:
  db_SchemaRef _context_schema;
  db_TableRef _context_table;
  db_TriggerRef _context_trigger;
  db_ViewRef _context_view;
  db_RoutineRef _context_routine;
  db_RoutineGroupRef _context_routine_group;
};
