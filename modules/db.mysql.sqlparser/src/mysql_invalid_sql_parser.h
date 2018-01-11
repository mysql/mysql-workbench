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

#include "mysql_sql_parser.h"
#include "grtsqlparser/invalid_sql_parser.h"
#include <stdexcept>

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class Mysql_invalid_sql_parser : protected Mysql_sql_parser, public Invalid_sql_parser {
public:
  typedef std::shared_ptr<Mysql_invalid_sql_parser> Ref;
  static Ref create() {
    return Ref(new Mysql_invalid_sql_parser());
  }
  virtual ~Mysql_invalid_sql_parser() {
  }

protected:
  Mysql_invalid_sql_parser();

public:
  int parse_inserts(db_TableRef table, const std::string &sql);
  int parse_triggers(db_TableRef table, const std::string &sql);
  int parse_trigger(db_TriggerRef trigger, const std::string &sql);
  int parse_routines(db_RoutineGroupRef routine_group, const std::string &sql);
  int parse_routine(db_RoutineRef routine, const std::string &sql);
  int parse_view(db_ViewRef view, const std::string &sql);

protected:
  int parse_invalid_sql_script(const std::string &sql);
  int process_sql_statement(const SqlAstNode *tree);
  Parse_result process_create_trigger_statement(const SqlAstNode *tree);
  Parse_result process_create_routine_statement(const SqlAstNode *tree) {
    return Mysql_sql_parser::process_create_routine_statement(tree);
  }
  Parse_result process_create_view_statement(const SqlAstNode *tree) {
    return Mysql_sql_parser::process_create_view_statement(tree);
  }

  void create_stub_routine(db_DatabaseDdlObjectRef &obj);
  void create_stub_group_routine(db_DatabaseDdlObjectRef &obj);
  void remove_stub_group_routine(db_DatabaseDdlObjectRef &obj);
  void shape_group_routine(db_mysql_RoutineRef &obj);
  void create_stub_trigger(db_DatabaseDdlObjectRef &obj);
  void create_stub_view(db_DatabaseDdlObjectRef &obj);
  void shape_trigger(db_mysql_TriggerRef &obj);

  void setup_stub_obj(db_DatabaseDdlObjectRef obj, bool set_name);
  std::string stub_obj_name();

  virtual GrtNamedObjectRef get_active_object() {
    return _active_obj;
  };

  // data members
  typedef boost::function<void(db_DatabaseDdlObjectRef &)> Proces_stub_object;
  Proces_stub_object _create_stub_object;
  Proces_stub_object _remove_stub_object;
  db_DatabaseObjectRef _active_grand_obj;              // container for objects to parse
  db_DatabaseDdlObjectRef _active_obj;                 // object to parse
  grt::ListRef<db_DatabaseDdlObject> _active_obj_list; // default container of objects
  grt::ListRef<db_DatabaseDdlObject>
    _active_obj_list2; // container of objects (narrowed contatiner for objects being contained in 2 collections)
  std::string _stub_name;
  int _stub_num;
  int _next_group_routine_seqno;
  int _next_trigger_seqno;
  bool _leading_use_found;

  class Null_state_keeper : Mysql_sql_parser::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_invalid_sql_parser *sql_parser)
      : Mysql_sql_parser::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    ~Null_state_keeper();

  private:
    Mysql_invalid_sql_parser *_sql_parser;
  };
};
