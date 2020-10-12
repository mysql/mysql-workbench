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

#include "mysql_sql_schema_rename.h"
#include "grtsqlparser/module_utils.h"
#include "mysql_sql_parser_fe.h"
#include "myx_statement_parser.h"
#include "base/string_utilities.h"

#include <sstream>

using namespace grt;
using namespace bec;
using namespace base;
using namespace boost::placeholders;

Mysql_sql_schema_rename::Null_state_keeper::~Null_state_keeper() {
  _sql_parser->_old_schema_name = std::string();
  _sql_parser->_new_schema_name = std::string();
  _sql_parser->_schema_names_offsets = std::list<int>();
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Mysql_sql_schema_rename::Mysql_sql_schema_rename() {
  NULL_STATE_KEEPER // reset all members to null-values
}

int Mysql_sql_schema_rename::rename_schema_references(std::string &sql, const std::string &old_schema_name,
                                                      const std::string &new_schema_name) {
  NULL_STATE_KEEPER

  if (old_schema_name.empty())
    return pr_invalid;

  _old_schema_name = old_schema_name;
  _new_schema_name = new_schema_name;

  _messages_enabled = false;

  _process_sql_statement = boost::bind(&Mysql_sql_schema_rename::process_sql_statement, this, _1);
  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.ignore_dml = false;

  rename_schema_references(sql, sql_parser_fe, 1);

  return pr_processed;
}

int Mysql_sql_schema_rename::rename_schema_references(db_CatalogRef catalog, const std::string &old_schema_name,
                                                      const std::string &new_schema_name) {
  NULL_STATE_KEEPER

  if (old_schema_name.empty())
    return pr_invalid;

  _catalog = db_mysql_CatalogRef::cast_from(catalog);
  _old_schema_name = old_schema_name;
  _new_schema_name = new_schema_name;

  std::string rename_action_details =
    strfmt(_("updating references to schema: `%s` -> `%s`"), old_schema_name.c_str(), new_schema_name.c_str());

  {
    std::string msg = strfmt(_("Started %s."), rename_action_details.c_str());
    add_log_message(msg, 0);
  }

  _process_sql_statement = boost::bind(&Mysql_sql_schema_rename::process_sql_statement, this, _1);
  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.ignore_dml = false;

  ListRef<db_mysql_Schema> schemata = _catalog->schemata();
  for (size_t n = 0, count = schemata.count(); n < count; ++n) {
    _active_schema = schemata.get(n);

    // views
    rename_schema_references<db_mysql_View>(_active_schema->views(), &db_mysql_View::sqlDefinition,
                                            &db_mysql_View::sqlDefinition, 0, sql_parser_fe);

    // routines
    rename_schema_references<db_mysql_Routine>(_active_schema->routines(), &db_mysql_Routine::sqlDefinition,
                                               &db_mysql_Routine::sqlDefinition, 1, sql_parser_fe);

    // tables + triggers
    {
      grt::ListRef<db_mysql_Table> tables = _active_schema->tables();

      // tables
      for (size_t n = 0, count = tables.count(); n < count; ++n) {
        // triggers
        rename_schema_references<db_mysql_Trigger>(tables.get(n)->triggers(), &db_mysql_Trigger::sqlDefinition,
                                                   &db_mysql_Trigger::sqlDefinition, 1, sql_parser_fe);
      }
    }
  }

  {
    std::string msg = strfmt(_("Finished %s."), rename_action_details.c_str()) + " " +
                      strfmt(_("Totally processed statements: successful (%i), errors (%i), warnings (%i)."),
                             (int)_processed_obj_count, (int)_err_count, (int)_warn_count);
    add_log_message(msg, 0);
  }

  return pr_processed;
}

template <typename T>
void Mysql_sql_schema_rename::rename_schema_references(grt::ListRef<T> obj_list,
                                                       grt::StringRef (T::*sql_text_prop_r)() const,
                                                       void (T::*sql_text_prop_w)(const grt::StringRef &),
                                                       int delim_wrapping, Mysql_sql_parser_fe &sql_parser_fe) {
  for (size_t n = 0, count = obj_list.count(); n < count; ++n) {
    grt::Ref<T> db_obj = obj_list.get(n);
    std::string sql_text = (db_obj.content().*sql_text_prop_r)();

    if (rename_schema_references(sql_text, sql_parser_fe, delim_wrapping)) {
      (db_obj.content().*sql_text_prop_w)(sql_text);

      {
        std::string log_msg;
        log_msg.append(db_obj.get_metaclass()->get_attribute("caption"))
          .append(" ")
          .append(db_obj->name())
          .append(" updated with regard to new schema name.");
        ++_processed_obj_count;
        add_log_message(log_msg, 0);
      }
    }
  }
}

int Mysql_sql_schema_rename::process_sql_statement(const SqlAstNode *tree) {
  // on parsing error log error message
  if (!tree) {
    report_sql_error(_err_tok_lineno, true, _err_tok_line_pos, _err_tok_len, _err_msg, 2);
    return -1;
  }

  process_sql_statement_item(tree);

  return 0; // success
}

void Mysql_sql_schema_rename::process_sql_statement_item(const SqlAstNode *item) {
  {
    // try all rules with dot notation

    if (item->name_equals(sql::_sp_name))
      process_schema_reference_candidate(item, 1);

    // else if (item->name_equals(sql::_opt_component))
    // irrelevant (doesn't refer schema)

    else if (item->name_equals(sql::_function_call_generic))
      process_schema_reference_candidate(item, 1);

    else if (item->name_equals(sql::_table_wild_one))
      process_schema_reference_candidate(item, 1);

    // else if (item->name_equals(sql::_opt_wild))
    // irrelevant (doesn't refer schema)

    else if (item->name_equals(sql::_table_wild))
      process_schema_reference_candidate(item, 2);

    else if (item->name_equals(sql::_simple_ident_q))
      process_schema_reference_candidate(item, 2);

    else if (item->name_equals(sql::_field_ident))
      process_schema_reference_candidate(item, 2);

    else if (item->name_equals(sql::_table_ident))
      process_schema_reference_candidate(item, 1);

    // else if (item->name_equals(sql::_internal_variable_name))
    // irrelevant (doesn't refer schema)

    else if (item->name_equals(sql::_grant_ident))
      process_schema_reference_candidate(item, 1);
  }

  {
    const SqlAstNode::SubItemList *subitems = item->subitems();
    SqlAstNode::SubItemList::const_iterator it = subitems->begin();
    SqlAstNode::SubItemList::const_iterator it_end = subitems->end();
    for (; it != it_end; ++it) {
      const SqlAstNode *subitem = *it;
      if (subitem->subitems()->size())
        process_sql_statement_item(subitem);
    }
  }
}

void Mysql_sql_schema_rename::process_schema_reference_candidate(const SqlAstNode *item, int dot_count) {
  const SqlAstNode *subitem = NULL;
  switch (dot_count) {
    case 1:
      subitem = item->subseq(sql::_ident, sql::_46); // 46 == ascii('.')
      break;
    case 2:
      subitem = item->subseq(sql::_ident, sql::_46, sql::_ident, sql::_46); // 46 == ascii('.')
      break;
  }
  if (subitem && (subitem = item->subseq(sql::_ident)) &&
      are_strings_eq(subitem->value(), _old_schema_name, _case_sensitive_identifiers))
    _schema_names_offsets.push_back(_splitter->statement_boffset() + subitem->stmt_boffset());
}

bool Mysql_sql_schema_rename::rename_schema_references(std::string &sql_text, Mysql_sql_parser_fe &sql_parser_fe,
                                                       int delim_wrapping) {
  if (sql_text.empty())
    return false;

  const std::string begin_delim1 = "DELIMITER " + _non_std_sql_delimiter + EOL;
  const std::string begin_delim2 = begin_delim1 + "CREATE PROCEDURE proc()" + EOL;
  const std::string end_delim = EOL + _non_std_sql_delimiter + EOL + "DELIMITER ;" + EOL + EOL;

  std::string begin_delim;

  switch (delim_wrapping) {
    case 1:
      begin_delim = begin_delim1;
      break;
    case 2:
      begin_delim = begin_delim2;
      break;
  }

  if (delim_wrapping) {
    sql_text.reserve(sql_text.size() + begin_delim.size() + end_delim.size());
    sql_text.insert(0, begin_delim).append(end_delim);
  }

  (void)parse_sql_script(sql_parser_fe, sql_text.c_str());

  rename_schema_references(sql_text);
  if (delim_wrapping) {
    sql_text.erase(sql_text.size() - end_delim.size(), end_delim.size());
    sql_text.erase(0, begin_delim.size());
  }

  return true;
}

bool Mysql_sql_schema_rename::rename_schema_references(std::string &sql_text) {
  if (_schema_names_offsets.empty())
    return false;

  if (_old_schema_name.size() < _new_schema_name.size())
    sql_text.reserve(sql_text.size() +
                     (_new_schema_name.size() - _old_schema_name.size()) * _schema_names_offsets.size());
  bool cutting_schema_name = _new_schema_name.empty();
  for (std::list<int>::reverse_iterator i = _schema_names_offsets.rbegin(); i != _schema_names_offsets.rend(); ++i) {
    std::string::size_type schema_ref_begin = *i;
    std::string::size_type schema_ref_end = schema_ref_begin + _old_schema_name.size();
    std::string::size_type sql_text_end = sql_text.size();
    if (cutting_schema_name) {
      // in case of cutting schema name extend text region with wrapping quotes and dot symbol preceding schema object
      // name.
      // "`" symbol is not part of ident in lexer, so some tricky code here to guess if it was quoted or not.
      if ((schema_ref_begin > 0) && ('`' == sql_text[schema_ref_begin - 1])) {
        --schema_ref_begin;
        ++schema_ref_end;
      }
      if ((schema_ref_end < sql_text_end) && ('.' == sql_text[schema_ref_end]))
        ++schema_ref_end;
    }
    sql_text.replace(schema_ref_begin, schema_ref_end - schema_ref_begin, _new_schema_name);
  }
  _schema_names_offsets.clear();
  return true;
}
