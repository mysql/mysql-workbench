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

#ifndef _INVALID_SQL_PARSER_H_
#define _INVALID_SQL_PARSER_H_

#include "wbpublic_public_interface.h"
#include "sql_parser_base.h"

/** Defines interface to transform single DDL statement into GRT object with special processing of syntax errors in
 * mind.
 * In case of syntax error instance of derived class creates placeholder GRT object containing provided SQL and marks it
 * as invalid.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Invalid_sql_parser : virtual public Sql_parser_base {
public:
  typedef std::shared_ptr<Invalid_sql_parser> Ref;

protected:
  Invalid_sql_parser() {
  }

public:
  virtual int parse_inserts(db_TableRef table, const std::string &sql) = 0;
  virtual int parse_triggers(db_TableRef table, const std::string &sql) = 0;
  virtual int parse_trigger(db_TriggerRef trigger, const std::string &sql) = 0;
  virtual int parse_routines(db_RoutineGroupRef routine_group, const std::string &sql) = 0;
  virtual int parse_routine(db_RoutineRef routine, const std::string &sql) = 0;
  virtual int parse_view(db_ViewRef view, const std::string &sql) = 0;
};

#endif // _INVALID_SQL_PARSER_H_
