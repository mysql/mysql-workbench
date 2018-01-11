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

#include <glib.h>
#include <boost/signals2.hpp>

#include "mysql_invalid_sql_parser.h"
#include "mysql_sql_parser_fe.h"
#include "mysql_sql_parser_utils.h"
#include "grtsqlparser/module_utils.h"
#include "base/string_utilities.h"
#include <sstream>
#include <boost/lambda/bind.hpp>

using namespace grt;
using namespace base;

Mysql_invalid_sql_parser::Null_state_keeper::~Null_state_keeper() {
  _sql_parser->_next_group_routine_seqno = 0;
  _sql_parser->_next_trigger_seqno = 0;
  _sql_parser->_stub_num = 0;
  _sql_parser->_leading_use_found = false;
  _sql_parser->_stub_name = std::string();
  _sql_parser->_active_obj_list = grt::ListRef<db_DatabaseDdlObject>();
  _sql_parser->_active_obj_list2 = grt::ListRef<db_DatabaseDdlObject>();
  _sql_parser->_active_obj = db_DatabaseDdlObjectRef();
  _sql_parser->_active_grand_obj = db_DatabaseObjectRef();
  boost::function<bool()> f = boost::lambda::constant(false);
  _sql_parser->_create_stub_object = boost::bind(f);
  _sql_parser->_remove_stub_object = boost::bind(f);
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Mysql_invalid_sql_parser::Mysql_invalid_sql_parser() : _leading_use_found(false) {
  NULL_STATE_KEEPER
}

int Mysql_invalid_sql_parser::parse_inserts(db_TableRef table, const std::string &sql) {
  NULL_STATE_KEEPER

  return pr_processed; //!
}

Sql_parser_base::Parse_result Mysql_invalid_sql_parser::process_create_trigger_statement(const SqlAstNode *tree) {
  Parse_result result = Mysql_sql_parser::process_create_trigger_statement(tree);
  if (result == pr_irrelevant) {
    ++_stub_num;

    // try to reuse existing stub
    db_DatabaseDdlObjectRef obj =
      find_named_object_in_list(_active_obj_list, stub_obj_name(), _case_sensitive_identifiers);

    if (obj.is_valid())
      setup_stub_obj(obj, false);
    else {
      _create_stub_object(obj);
      if (!_active_obj.is_valid())
        _active_obj_list.insert(obj);
    }
    obj->modelOnly(1);
    db_TableRef table = db_TableRef::cast_from(_active_grand_obj);
    table->customData().set("triggerInvalid", grt::IntegerRef(1));
    _created_objects.insert(obj);
    return pr_invalid;
  }
  return result;
}

int Mysql_invalid_sql_parser::parse_triggers(db_TableRef table, const std::string &sql) {
  NULL_STATE_KEEPER

  _active_grand_obj = table;
  _active_obj_list = ListRef<db_DatabaseDdlObject>::cast_from(table->triggers());
  _stub_name = "SYNTAX_ERROR_";
  _process_specific_create_statement =
    boost::bind(&Mysql_invalid_sql_parser::process_create_trigger_statement, this, _1);
  _create_stub_object = boost::bind(&Mysql_invalid_sql_parser::create_stub_trigger, this, _1);
  _shape_trigger = boost::bind(&Mysql_invalid_sql_parser::shape_trigger, this, _1);

  _triggers_owner_table = db_mysql_TableRef::cast_from(table);

  return parse_invalid_sql_script(sql);
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper to determine if an explicit DELIMITER statement in the sql string is needed.
 */
bool needs_delimiter_for_trigger(const std::string &sql) {
  std::vector<std::pair<size_t, size_t> > statement_ranges;
  SqlFacade::Ref facade = SqlFacade::instance_for_rdbms_name("Mysql");
  facade->splitSqlScript(sql.c_str(), sql.size(), ";", statement_ranges);

  // If the splitter only returns one statement (USE statements don't count) then we don't
  // need a delimiter (because either the code is so simple it does really not need any or
  // there is already a delimiter statement).
  if (statement_ranges.size() < 2)
    return false;

  for (size_t i = 0; i < statement_ranges.size(); ++i) {
    std::string statement = base::trim_left(sql.substr(statement_ranges[i].first, statement_ranges[i].second));
    if (base::tolower(statement).find("use ") == 0)
      continue;

    if (i == statement_ranges.size() - 1)
      return false;
    else
      return true;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

int Mysql_invalid_sql_parser::parse_trigger(db_TriggerRef trigger, const std::string &sql) {
  NULL_STATE_KEEPER

  _active_grand_obj = db_mysql_TableRef::cast_from(trigger->owner());
  _active_obj = trigger;
  _active_obj_list =
    ListRef<db_DatabaseDdlObject>::cast_from(db_mysql_TableRef::cast_from(_active_obj->owner())->triggers());
  _stub_name = "SYNTAX_ERROR_";
  _process_specific_create_statement =
    boost::bind(&Mysql_invalid_sql_parser::process_create_trigger_statement, this, _1);
  _create_stub_object = boost::bind(&Mysql_invalid_sql_parser::create_stub_trigger, this, _1);
  _shape_trigger = boost::bind(&Mysql_invalid_sql_parser::shape_trigger, this, _1);

  _triggers_owner_table = db_mysql_TableRef::cast_from(trigger->owner());

  // XXX: refactor all this out of the parser and let the table editor do the necessary jobs.

  // We can end up here with either no, one or even multiple USE and DELIMITER
  // statements before the actual CREATE. Simply adding another DELIMITER statement won't work as
  // it might break following USE statements. So we scan the input sql to see if we
  // can find a DELIMITER statement. And only if there's none we add one and continue parsing.
  if (!needs_delimiter_for_trigger(sql))
    return parse_invalid_sql_script(sql);

  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name("Mysql");
  Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
  std::string non_std_sql_delimiter = sql_specifics->non_std_sql_delimiter();

  std::string actual_sql = "DELIMITER " + non_std_sql_delimiter + "\n\nUSE `" +
                           *_triggers_owner_table->owner()->name() + "`" + non_std_sql_delimiter + "\n" + sql;
  return parse_invalid_sql_script(actual_sql);
}

//--------------------------------------------------------------------------------------------------

int Mysql_invalid_sql_parser::parse_routines(db_RoutineGroupRef routine_group, const std::string &sql) {
  NULL_STATE_KEEPER

  _active_grand_obj = routine_group;
  _active_obj_list =
    ListRef<db_DatabaseDdlObject>::cast_from(db_mysql_SchemaRef::cast_from(_active_grand_obj->owner())->routines());
  _active_obj_list2 = ListRef<db_DatabaseDdlObject>::cast_from(routine_group->routines());
  _stub_name = (*routine_group->name()).append("_SYNTAX_ERROR_");
  _process_specific_create_statement =
    boost::bind(&Mysql_invalid_sql_parser::process_create_routine_statement, this, _1);
  _create_stub_object = boost::bind(&Mysql_invalid_sql_parser::create_stub_group_routine, this, _1);
  _remove_stub_object = boost::bind(&Mysql_invalid_sql_parser::remove_stub_group_routine, this, _1);
  _shape_routine = boost::bind(&Mysql_invalid_sql_parser::shape_group_routine, this, _1);

  // We need to store old case sensitivy setting, to not break the rest parser.
  bool old_case_opt = _case_sensitive_identifiers;
  _case_sensitive_identifiers = false; //! identifiers within a mysql routine body are always case-insensitive?
  int res = parse_invalid_sql_script(sql);
  _case_sensitive_identifiers = old_case_opt;
  return res;
}

int Mysql_invalid_sql_parser::parse_routine(db_RoutineRef routine, const std::string &sql) {
  NULL_STATE_KEEPER

  _active_obj = routine;
  _active_grand_obj = _active_obj;
  _active_obj_list =
    ListRef<db_DatabaseDdlObject>::cast_from(db_mysql_SchemaRef::cast_from(_active_obj->owner())->routines());
  _stub_name = "SYNTAX_ERROR_";
  _process_specific_create_statement =
    boost::bind(&Mysql_invalid_sql_parser::process_create_routine_statement, this, _1);
  _create_stub_object = boost::bind(&Mysql_invalid_sql_parser::create_stub_routine, this, _1);

  // We need to store old case sensitivy setting, to not break the rest parser.
  bool old_case_opt = _case_sensitive_identifiers;
  _case_sensitive_identifiers = false; //! identifiers within a mysql routine body are always case-insensitive?
  int res = parse_invalid_sql_script(sql);
  _case_sensitive_identifiers = old_case_opt;
  return res;
}

int Mysql_invalid_sql_parser::parse_view(db_ViewRef view, const std::string &sql) {
  NULL_STATE_KEEPER

  _active_obj = view;
  _active_grand_obj = _active_obj;
  _active_obj_list =
    ListRef<db_DatabaseDdlObject>::cast_from(db_mysql_SchemaRef::cast_from(_active_obj->owner())->views());
  _stub_name = "SYNTAX_ERROR_";
  _process_specific_create_statement = boost::bind(&Mysql_invalid_sql_parser::process_create_view_statement, this, _1);
  _create_stub_object = boost::bind(&Mysql_invalid_sql_parser::create_stub_view, this, _1);

  _sql_script_preamble = "DELIMITER " + _non_std_sql_delimiter + EOL;
  std::string sql_ = _sql_script_preamble + sql;

  return parse_invalid_sql_script(sql_);
}

int Mysql_invalid_sql_parser::parse_invalid_sql_script(const std::string &sql) {
  set_options(DictRef());

  if (!_active_obj_list2.is_valid())
    _active_obj_list2 = _active_obj_list;

  if (db_TriggerRef::can_wrap(_active_grand_obj))
    _active_schema = db_mysql_SchemaRef::cast_from(_active_grand_obj->owner()->owner());
  else
    _active_schema = db_mysql_SchemaRef::cast_from(_active_grand_obj->owner());
  _catalog = db_mysql_CatalogRef(grt::Initialized);
  _catalog->schemata().insert(_active_schema);

  // take simple datatypes & other major attributes from given catalog
  // simple datatypes may be needed for parsing routine parameters & return value types
  {
    db_mysql_CatalogRef given_catalog = db_mysql_CatalogRef::cast_from(_active_schema->owner());
    _catalog->version(given_catalog->version());
    _catalog->defaultCharacterSetName(given_catalog->defaultCharacterSetName());
    _catalog->defaultCollationName(given_catalog->defaultCollationName());
    replace_contents(_catalog->simpleDatatypes(), given_catalog->simpleDatatypes());
  }

  _created_objects = grt::ListRef<GrtObject>(true);
  _reuse_existing_objects = true;
  _stick_to_active_schema = true;
  _set_old_names = false;
  _messages_enabled = false;
  _strip_sql = false;

  build_datatype_cache();

  _process_sql_statement = boost::bind(&Mysql_invalid_sql_parser::process_sql_statement, this, _1);

  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.ignore_dml = false;

  // Consider using const char* for the sql instead if memory usage is an issue.
  int res = Mysql_sql_parser_base::parse_sql_script(sql_parser_fe, sql.c_str());

  // remove from initial list objects that were not created during parsing
  // for parsing single objects (_active_obj.is_valid()) this is not appropriate
  if (_active_obj_list2.is_valid() && !_active_obj.is_valid()) {
    for (size_t n = _active_obj_list2.count(); n > 0; --n) {
      db_DatabaseDdlObjectRef obj = _active_obj_list2.get(n - 1);
      if (!find_named_object_in_list(_created_objects, obj->name(), _case_sensitive_identifiers).is_valid()) {
        _active_obj_list.remove_value(obj);

        /*
          objects of some types are registered in several lists
          (e.g. routine in db_Schema::routines & db_RoutineGroup::routines)
          this call allows to make additional cleanup for such objects
        */
        _remove_stub_object(obj);
      }
    }
  }

  return res;
}

int Mysql_invalid_sql_parser::process_sql_statement(const SqlAstNode *tree) {
  int err = Mysql_sql_parser::process_sql_statement(tree);
  if (0 != err) {
    ++_stub_num;

    // try to reuse existing stub
    db_DatabaseDdlObjectRef obj =
      find_named_object_in_list(_active_obj_list, stub_obj_name(), _case_sensitive_identifiers);

    if (obj.is_valid())
      setup_stub_obj(obj, false);
    else {
      _create_stub_object(obj);
      if (!_active_obj.is_valid())
        _active_obj_list.insert(obj);
    }

    _created_objects.insert(obj);
  } else {
    if (_last_parse_result != pr_processed) {
      if (_leading_use_found) {
        if (db_TableRef::can_wrap(_active_grand_obj)) {
          db_TableRef table = db_TableRef::cast_from(_active_grand_obj);
          table->customData().set("triggerInvalid", grt::IntegerRef(1));
        }
      } else
        _leading_use_found = true;
    }
  }

  return err;
}

void Mysql_invalid_sql_parser::create_stub_view(db_DatabaseDdlObjectRef &obj) {
  obj = db_mysql_ViewRef::cast_from(_active_obj);
  obj->sqlDefinition(strip_sql_statement(sql_statement(), _strip_sql));
}

void Mysql_invalid_sql_parser::create_stub_routine(db_DatabaseDdlObjectRef &obj) {
  obj = db_mysql_RoutineRef::cast_from(_active_obj);
  obj->sqlDefinition(strip_sql_statement(sql_statement(), _strip_sql));
}

void Mysql_invalid_sql_parser::create_stub_group_routine(db_DatabaseDdlObjectRef &obj) {
  db_mysql_RoutineRef routine(grt::Initialized);
  routine->owner(_active_schema);
  setup_stub_obj(routine, true);
  routine->routineType("<stub>");

  _active_obj_list2.insert(routine);

  obj = routine;
}

void Mysql_invalid_sql_parser::remove_stub_group_routine(db_DatabaseDdlObjectRef &obj) {
  _active_obj_list2.remove_value(obj);
}

void Mysql_invalid_sql_parser::shape_group_routine(db_mysql_RoutineRef &obj) {
  if (!find_named_object_in_list(_active_obj_list2, obj->name(), _case_sensitive_identifiers).is_valid())
    _active_obj_list2.insert(obj);
  obj->sequenceNumber(_next_group_routine_seqno++);
}

void Mysql_invalid_sql_parser::create_stub_trigger(db_DatabaseDdlObjectRef &obj) {
  if (_active_obj.is_valid()) {
    obj = db_mysql_TriggerRef::cast_from(_active_obj);
    obj->sqlDefinition(strip_sql_statement(sql_statement(), _strip_sql));
  } else {
    db_mysql_TriggerRef trigger(grt::Initialized);
    trigger->owner(_active_grand_obj);
    setup_stub_obj(trigger, true);

    obj = trigger;
  }
}

void Mysql_invalid_sql_parser::shape_trigger(db_mysql_TriggerRef &obj) {
  // obj->sequenceNumber(_next_trigger_seqno++);
}

void Mysql_invalid_sql_parser::setup_stub_obj(db_DatabaseDdlObjectRef obj, bool set_name) {
  // First set the sql then the name. Both trigger a UI refresh (<sigh>) but if the name comes first
  // wrong sql is loaded.
  obj->sqlDefinition(strip_sql_statement(sql_statement(), _strip_sql));

  if (set_name)
    obj->name(stub_obj_name());
  /*
  if (db_mysql_TriggerRef::can_wrap(obj))
    db_mysql_TriggerRef::cast_from(obj)->sequenceNumber(_next_trigger_seqno++);
  else */ if (db_mysql_RoutineRef::can_wrap(obj) && db_RoutineGroupRef::can_wrap(_active_grand_obj))
    db_mysql_RoutineRef::cast_from(obj)->sequenceNumber(_next_group_routine_seqno++);
}

std::string Mysql_invalid_sql_parser::stub_obj_name() {
  std::ostringstream oss;
  oss << _stub_name << _stub_num;
  return oss.str();
}
