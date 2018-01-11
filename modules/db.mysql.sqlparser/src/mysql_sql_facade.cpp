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

#include <stack>

#include <glib.h>
#include <boost/signals2.hpp>

#include "mysql_sql_facade.h"
#include "mysql_sql_parser.h"
#include "mysql_sql_specifics.h"
#include "mysql_sql_normalizer.h"
#include "mysql_sql_inserts_loader.h"
#include "mysql_sql_schema_rename.h"
#include "mysql_sql_syntax_check.h"
#include "mysql_sql_semantic_check.h"
#include "mysql_invalid_sql_parser.h"
#include "mysql_sql_script_splitter.h"
#include "mysql_sql_statement_decomposer.h"
#include "mysql_sql_schema_rename.h"
#include "base/string_utilities.h"

#include "myx_statement_parser.h"
#include "mysql_sql_parser_fe.h"
#include "myx_sql_tree_item.h"

using namespace bec;

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::splitSqlScript(const std::string &sql, std::list<std::string> &statements) {
  return Mysql_sql_script_splitter::create()->process(sql, statements);
}

//--------------------------------------------------------------------------------------------------

static const unsigned char *skipLeadingWhitespace(const unsigned char *head, const unsigned char *tail) {
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//--------------------------------------------------------------------------------------------------

bool isLineBreak(const unsigned char *head, const unsigned char *line_break) {
  if (*line_break == '\0')
    return false;

  while (*head != '\0' && *line_break != '\0' && *head == *line_break) {
    head++;
    line_break++;
  }
  return *line_break == '\0';
}

//--------------------------------------------------------------------------------------------------

/**
 * A statement splitter to take a list of sql statements and split them into individual statements,
 * return their position and length in the original string (instead the copied strings).
 */
int MysqlSqlFacadeImpl::splitSqlScript(const char *sql, std::size_t length, const std::string &initial_delimiter,
                                       std::vector<std::pair<std::size_t, std::size_t> > &ranges,
                                       const std::string &line_break) {
  _stop = false;
  std::string delimiter = initial_delimiter.empty() ? ";" : initial_delimiter;
  const unsigned char *delimiter_head = (unsigned char *)delimiter.c_str();

  const unsigned char keyword[] = "delimiter";

  const unsigned char *head = (unsigned char *)sql;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *new_line = (unsigned char *)line_break.c_str();
  bool have_content = false; // Set when anything else but comments were found for the current statement.

  while (!_stop && tail < end) {
    switch (*tail) {
      case '/': // Possible multi line comment or hidden (conditional) command.
        if (*(tail + 1) == '*') {
          tail += 2;
          bool is_hidden_command = (*tail == '!');
          while (true) {
            while (tail < end && *tail != '*')
              tail++;
            if (tail == end) // Unfinished comment.
              break;
            else {
              if (*++tail == '/') {
                tail++; // Skip the slash too.
                break;
              }
            }
          }

          if (!is_hidden_command && !have_content)
            head = tail; // Skip over the comment.
        } else
          tail++;

        break;

      case '-': // Possible single line comment.
      {
        const unsigned char *end_char = tail + 2;
        if (*(tail + 1) == '-' && (*end_char == ' ' || *end_char == '\t' || isLineBreak(end_char, new_line))) {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && !isLineBreak(tail, new_line))
            tail++;
          if (!have_content)
            head = tail;
        } else
          tail++;

        break;
      }

      case '#': // MySQL single line comment.
        while (tail < end && !isLineBreak(tail, new_line))
          tail++;
        if (!have_content)
          head = tail;
        break;

      case '"':
      case '\'':
      case '`': // Quoted string/id. Skip this in a local loop.
      {
        have_content = true;
        char quote = *tail++;
        while (tail < end && *tail != quote) {
          // Skip any escaped character too.
          if (*tail == '\\')
            tail++;
          tail++;
        }
        if (*tail == quote)
          tail++; // Skip trailing quote char to if one was there.

        break;
      }

      case 'd':
      case 'D': {
        have_content = true;

        // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
        // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
        unsigned char previous = tail > (unsigned char *)sql ? *(tail - 1) : 0;
        bool is_identifier_char = previous >= 0x80 || (previous >= '0' && previous <= '9') ||
                                  ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z') || previous == '$' ||
                                  previous == '_';
        if (tail == (unsigned char *)sql || !is_identifier_char) {
          const unsigned char *run = tail + 1;
          const unsigned char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ') {
            // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
            tail = run++;
            while (run < end && !isLineBreak(run, new_line))
              run++;
            delimiter = base::trim(std::string((char *)tail, run - tail));
            delimiter_head = (unsigned char *)delimiter.c_str();

            // Skip over the delimiter statement and any following line breaks.
            while (isLineBreak(run, new_line))
              run++;
            tail = run;
            head = tail;
          } else
            tail++;
        } else
          tail++;

        break;
      }

      default:
        if (*tail > ' ')
          have_content = true;
        tail++;
        break;
    }

    if (*tail == *delimiter_head) {
      // Found possible start of the delimiter. Check if it really is.
      size_t count = delimiter.size();
      if (count == 1) {
        // Most common case. Trim the statement and check if it is not empty before adding the range.
        head = skipLeadingWhitespace(head, tail);
        if (head < tail)
          ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
        head = ++tail;
        have_content = false;
      } else {
        const unsigned char *run = tail + 1;
        const unsigned char *del = delimiter_head + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0) {
          // Multi char delimiter is complete. Tail still points to the start of the delimiter.
          // Run points to the first character after the delimiter.
          head = skipLeadingWhitespace(head, tail);
          if (head < tail)
            ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
          tail = run;
          head = run;
          have_content = false;
        }
      }
    }
  }

  // Add remaining text to the range list.
  head = skipLeadingWhitespace(head, tail);
  if (head < tail)
    ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));

  return 0;
}

//--------------------------------------------------------------------------------------------------

// A splitter using the grt (probably for python).
grt::BaseListRef MysqlSqlFacadeImpl::getSqlStatementRanges(const std::string &sql) {
  grt::BaseListRef list(true);
  std::list<std::pair<std::size_t, std::size_t> > ranges;
  Mysql_sql_script_splitter::create()->process(sql.c_str(), ranges);

  for (std::list<std::pair<size_t, size_t> >::const_iterator i = ranges.begin(); i != ranges.end(); ++i) {
    grt::BaseListRef item(true);
    item.ginsert(grt::IntegerRef((long)i->first));
    item.ginsert(grt::IntegerRef((long)i->second));
    list.ginsert(item);
  }
  return list;
}

//--------------------------------------------------------------------------------------------------

Sql_parser::Ref MysqlSqlFacadeImpl::sqlParser() {
  return Mysql_sql_parser::create();
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseSqlScriptString(db_CatalogRef catalog, const std::string sql) {
  return parseSqlScriptStringEx(catalog, sql, DictRef());
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseSqlScriptStringEx(db_CatalogRef catalog, const std::string sql,
                                               const grt::DictRef options) {
  return Mysql_sql_parser::create()->parse_sql_script(db_mysql_CatalogRef::cast_from(catalog), sql, options);
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseSqlScriptFile(db_CatalogRef catalog, const std::string filename) {
  return parseSqlScriptFileEx(catalog, filename, DictRef());
}

int MysqlSqlFacadeImpl::parseSqlScriptFileEx(db_CatalogRef catalog, const std::string filename,
                                             const grt::DictRef options) {
  return Mysql_sql_parser::create()->parse_sql_script_file(db_mysql_CatalogRef::cast_from(catalog), filename, options);
}

Invalid_sql_parser::Ref MysqlSqlFacadeImpl::invalidSqlParser() {
  return Mysql_invalid_sql_parser::create();
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseInserts(db_TableRef table, const std::string &sql) {
  return Mysql_invalid_sql_parser::create()->parse_inserts(db_mysql_TableRef::cast_from(table), sql);
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseTrigger(db_TriggerRef trigger, const std::string &sql) {
  return Mysql_invalid_sql_parser::create()->parse_trigger(trigger, sql);
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseRoutine(db_RoutineRef routine, const std::string &sql) {
  return Mysql_invalid_sql_parser::create()->parse_routine(db_mysql_RoutineRef::cast_from(routine), sql);
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseRoutines(db_RoutineGroupRef routineGroup, const std::string &sql) {
  return Mysql_invalid_sql_parser::create()->parse_routines(db_mysql_RoutineGroupRef::cast_from(routineGroup), sql);
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::parseView(db_ViewRef view, const std::string &sql) {
  return Mysql_invalid_sql_parser::create()->parse_view(db_mysql_ViewRef::cast_from(view), sql);
}

//--------------------------------------------------------------------------------------------------

Sql_syntax_check::Ref MysqlSqlFacadeImpl::sqlSyntaxCheck() {
  return Mysql_sql_syntax_check::create();
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::checkSqlSyntax(const std::string &sql) {
  return Mysql_sql_syntax_check::create()->check_sql(sql.c_str());
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::checkTriggerSyntax(const std::string &sql) {
  return Mysql_sql_syntax_check::create()->check_trigger(sql.c_str());
}

int MysqlSqlFacadeImpl::checkViewSyntax(const std::string &sql) {
  return Mysql_sql_syntax_check::create()->check_view(sql.c_str());
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::checkRoutineSyntax(const std::string &sql) {
  return Mysql_sql_syntax_check::create()->check_routine(sql.c_str());
}

//--------------------------------------------------------------------------------------------------

Sql_semantic_check::Ref MysqlSqlFacadeImpl::sqlSemanticCheck() {
  return Mysql_sql_semantic_check::create();
}

//--------------------------------------------------------------------------------------------------

Sql_specifics::Ref MysqlSqlFacadeImpl::sqlSpecifics() {
  return Mysql_sql_specifics::create();
}

//--------------------------------------------------------------------------------------------------

Sql_normalizer::Ref MysqlSqlFacadeImpl::sqlNormalizer() {
  return Mysql_sql_normalizer::create();
}

//--------------------------------------------------------------------------------------------------

std::string MysqlSqlFacadeImpl::normalizeSqlStatement(const std::string sql, const std::string schema_name) {
  return Mysql_sql_normalizer::create()->normalize(sql, schema_name);
}

//--------------------------------------------------------------------------------------------------

std::string MysqlSqlFacadeImpl::removeInterTokenSpaces(const std::string sql) {
  return Mysql_sql_normalizer::create()->remove_inter_token_spaces(sql);
}

//--------------------------------------------------------------------------------------------------

Sql_inserts_loader::Ref MysqlSqlFacadeImpl::sqlInsertsLoader() {
  return Mysql_sql_inserts_loader::create();
}

//--------------------------------------------------------------------------------------------------

Sql_schema_rename::Ref MysqlSqlFacadeImpl::sqlSchemaRenamer() {
  return Mysql_sql_schema_rename::create();
}

//--------------------------------------------------------------------------------------------------

int MysqlSqlFacadeImpl::renameSchemaReferences(db_CatalogRef catalog, const std::string old_schema_name,
                                               const std::string new_schema_name) {
  return Mysql_sql_schema_rename::create()->rename_schema_references(catalog, old_schema_name, new_schema_name);
}

//--------------------------------------------------------------------------------------------------

Sql_statement_decomposer::Ref MysqlSqlFacadeImpl::sqlStatementDecomposer(grt::DictRef db_opts) {
  return Mysql_sql_statement_decomposer::create(db_opts);
}

//--------------------------------------------------------------------------------------------------

grt::StringListRef MysqlSqlFacadeImpl::splitSqlStatements(const std::string &sql) {
  grt::StringListRef list(grt::Initialized);
  std::list<std::string> statements;

  splitSqlScript(sql, statements);

  for (std::list<std::string>::const_iterator i = statements.begin(); i != statements.end(); ++i)
    list.insert(*i);

  return list;
}

//--------------------------------------------------------------------------------------------------

static grt::BaseListRef process_ast_node(int base_offset, const SqlAstNode &item) {
  grt::BaseListRef tuple(true);
  sql::symbol item_name = item.name();
  tuple.ginsert(grt::StringRef(item_name ? sql::symbol_names[item_name] : ""));

  bool has_value = false;
  if (!item.value().empty()) {
    tuple.ginsert(grt::StringRef(item.value()));
    has_value = true;
  } else
    tuple.ginsert(grt::StringRef());

  {
    SqlAstNode::SubItemList *subitems = item.subitems();
    grt::BaseListRef children(true);
    if (subitems) {
      for (SqlAstNode::SubItemList::const_iterator i = subitems->begin(), i_end = subitems->end(); i != i_end; ++i)
        children.ginsert(process_ast_node(base_offset, **i));
    }
    tuple.ginsert(children);
  }

  if (has_value) {
    tuple.ginsert(grt::IntegerRef(base_offset));
    tuple.ginsert(grt::IntegerRef(item.stmt_boffset()));
    tuple.ginsert(grt::IntegerRef(item.stmt_eoffset()));
  } else {
    tuple.ginsert(grt::ValueRef());
    tuple.ginsert(grt::ValueRef());
    tuple.ginsert(grt::ValueRef());
  }

  return tuple;
}

//--------------------------------------------------------------------------------------------------

static int parse_callback(void *user_data, const MyxStatementParser *splitter, const char *sql, const SqlAstNode *tree,
                          int stmt_begin_lineno, int stmt_begin_line_pos, int stmt_end_lineno, int stmt_end_line_pos,
                          int err_tok_lineno, int err_tok_line_pos, int err_tok_len, const std::string &err_msg) {
  grt::BaseListRef result = *(grt::BaseListRef *)user_data;

  if (err_msg.empty())
    result.ginsert(process_ast_node(splitter->statement_boffset(), *tree));
  else
    result.ginsert(grt::StringRef(err_msg));

  return 0;
}

//--------------------------------------------------------------------------------------------------

grt::BaseListRef MysqlSqlFacadeImpl::parseAstFromSqlScript(const std::string &sql) {
  Mysql_sql_parser_fe parser(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  grt::BaseListRef result(true);

  parser.is_ast_generation_enabled = true;
  parser.ignore_dml = false;

  parser.parse_sql_script(sql.c_str(), parse_callback, (void *)&result);

  return result;
}

//--------------------------------------------------------------------------------------------------

grt::BaseListRef MysqlSqlFacadeImpl::getItemFromPath(const std::string &path, const grt::BaseListRef source) {
  if (!source.is_valid())
    return grt::BaseListRef();
  bool valid = true;
  grt::BaseListRef current_item = source;
  grt::BaseListRef current_child;

  std::vector<std::string> path_array = base::split(path, ",");

  for (size_t index = 0; valid && index < path_array.size(); index++) {
    bool found = false;

    for (size_t item_index = 0; item_index < current_item->count() && !found; item_index++) {
      /* Gets the #th child from the list */
      current_child = grt::BaseListRef::cast_from(current_item->get(item_index));

      /* Validates the next item name */
      grt::StringRef item_name = grt::StringRef::cast_from(current_child->get(0));

      found = (item_name == path_array[index]);
    }

    if (!found)
      valid = false;
    else if (index < path.size() && current_child.count() > 2)
      current_item = grt::BaseListRef::cast_from(current_child->get(2));
  }

  return valid ? current_item : grt::BaseListRef();
}

//--------------------------------------------------------------------------------------------------

bool MysqlSqlFacadeImpl::parseSelectStatementForEdit(const std::string &sql, std::string &schema_name,
                                                     std::string &table_name, String_tuple_list &column_names) {
  bool ret_val = false;

  /* gets the AST tree for the given statement */
  grt::BaseListRef result = parseAstFromSqlScript(sql);

  if (result.is_valid()) {
    /* Ensures only a single statement has been processed */
    /* Correct results are given inside a list, errors are reported as strings */
    if (result->count() == 1 && result[0].type() == ListType) {
      std::string uninon_path =
        "verb_clause,statement,select,select_init,select_init2,union_clause,union_list,select_init";
      grt::BaseListRef select_union = getItemFromPath(uninon_path, result);
      if (select_union.is_valid())
        return ret_val;

      /* Retrieves the node containing for a SELECT statement */
      std::string select_path = "verb_clause,statement,select,select_init,select_init2,select_part2";
      grt::BaseListRef select_item = getItemFromPath(select_path, result);

      if (select_item.is_valid()) {
        /* Retrieves the information about the queried tables */
        std::string tables_path = "select_into,select_from,join_table_list,derived_table_list";
        grt::BaseListRef table_list = getItemFromPath(tables_path, select_item);

        if (select_item.is_valid() && table_list.is_valid()) {
          /* Validates only one table has been queried */
          if (table_list->count() == 1) {
            /* Gets the table/schema information from the queried table */
            std::string table_path = "esc_table_ref,table_ref,table_factor,table_ident";

            grt::BaseListRef table = getItemFromPath(table_path, table_list);
            if (table.is_valid()) {
              grt::BaseListRef table_name_node = grt::BaseListRef();
              grt::BaseListRef schema_name_node = grt::BaseListRef();

              switch (table->count()) {
                case 1:
                  table_name_node = grt::BaseListRef::cast_from(table->get(0));
                  break;
                case 2:
                  table_name_node = grt::BaseListRef::cast_from(table->get(1));
                  break;
                case 3:
                  schema_name_node = grt::BaseListRef::cast_from(table->get(0));
                  table_name_node = grt::BaseListRef::cast_from(table->get(2));
                  break;
              }

              schema_name = schema_name_node.is_valid() && schema_name_node.count() > 1
                              ? grt::StringRef::extract_from(schema_name_node->get(1))
                              : "";
              table_name = table_name_node.is_valid() && table_name_node.count() > 1
                             ? grt::StringRef::extract_from(table_name_node->get(1))
                             : "";

              // Retrieves the column information
              grt::BaseListRef column_list = getItemFromPath("select_item_list", select_item);

              if (column_list.is_valid()) {
                ret_val = true;

                // Process the queried columns

                {
                  for (size_t column_index = 0; ret_val && column_index < column_list->count(); column_index++) {
                    grt::BaseListRef next_column = grt::BaseListRef::cast_from(
                      column_list->get(column_index++)); // Note column_index is being increased to skip the "," item

                    // Validation occurs only on non * columns
                    if (next_column[1].toString() == "*")
                      column_names.push_back(std::make_pair("*", "*"));
                    else {
                      grt::BaseListRef generic_column_node =
                        getItemFromPath("expr,bool_pri,predicate,bit_expr,simple_expr",
                                        grt::BaseListRef::cast_from(next_column->get(2)));
                      // Tries to get the column as a simple identifier node
                      grt::BaseListRef column_name_node = grt::BaseListRef();

                      if (generic_column_node.is_valid()) {
                        grt::BaseListRef column_node = getItemFromPath("simple_ident", generic_column_node);

                        if (column_node.is_valid()) {
                          grt::BaseListRef q_column_node = getItemFromPath("simple_ident_q", column_node);
                          if (q_column_node.is_valid())
                            column_name_node =
                              grt::BaseListRef::cast_from(q_column_node->get(q_column_node->count() - 1));
                          else
                            column_name_node = grt::BaseListRef::cast_from(column_node->get(0));

                          std::string column_name = column_name_node.is_valid() && column_name_node.count() > 1
                                                      ? grt::StringRef::extract_from(column_name_node->get(1))
                                                      : "";
                          std::string alias_name = "";

                          if (next_column->count() > 2) {
                            grt::BaseListRef alias_ident_node =
                              getItemFromPath("select_alias", grt::BaseListRef::cast_from(next_column->get(2)));
                            grt::BaseListRef alias_name_node =
                              alias_ident_node.is_valid()
                                ? grt::BaseListRef::cast_from(alias_ident_node->get(alias_ident_node->count() - 1))
                                : grt::BaseListRef();
                            alias_name = alias_name_node.is_valid() && alias_name_node.count() > 1
                                           ? grt::StringRef::extract_from(alias_name_node->get(1))
                                           : column_name;
                          }

                          column_names.push_back(std::make_pair(column_name, alias_name));
                        } else
                          ret_val = false;
                      } else {
                        ret_val = false;
                        if (next_column->count() > 2) {
                          grt::BaseListRef wildcard_column_node =
                            getItemFromPath("table_wild", grt::BaseListRef::cast_from(next_column->get(2)));
                          if (wildcard_column_node.is_valid()) {
                            column_name_node =
                              grt::BaseListRef::cast_from(wildcard_column_node->get(wildcard_column_node->count() - 1));
                            std::string column_name = column_name_node.is_valid() && column_name_node.count() > 1
                                                        ? grt::StringRef::extract_from(column_name_node->get(1))
                                                        : "";
                            column_names.push_back(std::make_pair(column_name, column_name));
                            ret_val = true;
                          }
                        }
                      }
                    }

                    // These lines were intended to support creating columns from literals, as they are not
                    // editable, whenever a non real column is found, the whole method should return false
                    // Code will be let in place in case editable columns are allowed in the future
                    /*else
                     {
                     column_node = getItemFromPath("literal", generic_column_node);

                     // Ensures it is a single literal node
                     if (column_node.is_valid() && column_node->count() == 1)
                     {
                     // Gets the literal node, no matter which
                     column_name_node = grt::BaseListRef::cast_from(column_node->get(0));

                     // gets the literal type as for text_literals need to go one level deeper
                     if (grt::StringRef::extract_from(column_name_node->get(0)) == "text_literal")
                     {
                     column_name_node = grt::BaseListRef::cast_from(column_name_node->get(2));
                     column_name_node = grt::BaseListRef::cast_from(column_name_node->get(0));
                     }
                     }
                     }*/
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Resets the data to be returned in anything caused it to fail
  if (!ret_val) {
    schema_name = "";
    table_name = "";
    column_names.clear();
  }

  return ret_val;
}

std::string MysqlSqlFacadeImpl::getTypeDescription(grt::BaseListRef type_node,
                                                   std::vector<std::string> *additional_type_data_paths) {
  // Sets the data type name..
  grt::BaseListRef temp_node = grt::BaseListRef::cast_from(type_node->get(0));
  std::string description = grt::StringRef::extract_from(temp_node->get(1));

  if (additional_type_data_paths) {
    grt::BaseListRef path_node;
    for (size_t path_index = 0; path_index < additional_type_data_paths->size(); path_index++) {
      path_node = getItemFromPath(additional_type_data_paths->at(path_index), type_node);
      if (path_node.is_valid()) {
        for (size_t index = 0; index < path_node->count(); index++) {
          temp_node = grt::BaseListRef::cast_from(path_node->get(index));
          description += grt::StringRef::extract_from(temp_node->get(1));
        }
      }
    }
  }

  return description;
}

bool MysqlSqlFacadeImpl::parseRoutineDetails(const std::string &sql, std::string &type, std::string &name,
                                             String_tuple_list &parameters, std::string &return_value,
                                             std::string &comments) {
  bool ret_val = false;
  grt::BaseListRef temp_node;
  std::string param_name, param_type;

  // gets the AST tree for the given ddl (which is expected to be a
  // Stored procedure or function ddl
  grt::BaseListRef result = parseAstFromSqlScript(sql);

  if (result.is_valid()) {
    // Ensures only a single statement has been processed
    // Correct results are given inside a list, errors are reported as strings
    if (result->count() == 1 && result[0].type() == ListType) {
      // Retrieves the parent node common to both procedures and functions
      std::string select_path = "verb_clause,statement,create,view_or_trigger_or_sp_or_event,definer_tail";
      grt::BaseListRef select_item = getItemFromPath(select_path, result);

      // Attempts to retrieve a function node
      grt::BaseListRef routine_item = getItemFromPath("sf_tail", select_item);
      type = "FUNCTION";
      std::string param_list = "sp_fdparam_list,sp_fdparams";
      std::string param = "sp_fdparam";

      // If failed then attempts with a procedure node
      if (!routine_item.is_valid()) {
        routine_item = getItemFromPath("sp_tail", select_item);
        type = "PROCEDURE";
        param_list = "sp_pdparam_list,sp_pdparams";
        param = "sp_pdparam";
      }

      // If we've got a valid routine node...
      if (routine_item.is_valid()) {
        // Retrieves the name of the routine...
        temp_node = getItemFromPath("sp_name", routine_item);
        temp_node = grt::BaseListRef::cast_from(temp_node->get(0));
        name = grt::StringRef::extract_from(temp_node->get(1));

        // These paths inside each parameter compplete the type definitions
        // They contain information like precision (x,y), length (x) and others
        std::vector<std::string> additional_type_data_paths;
        additional_type_data_paths.push_back("field_length");
        additional_type_data_paths.push_back("opt_field_length,field_length");
        additional_type_data_paths.push_back("opt_precision,precision");
        additional_type_data_paths.push_back("type_datetime_precision");
        additional_type_data_paths.push_back("opt_bin_mod");
        additional_type_data_paths.push_back("float_options,precision");
        additional_type_data_paths.push_back("float_options,field_length");
        additional_type_data_paths.push_back("field_options,field_opt_list,field_option");

        // Gets the node containing all the parameter definition...
        grt::BaseListRef parameters_node = getItemFromPath(param_list, routine_item);
        if (parameters_node.is_valid()) {
          for (size_t index = 0; index < parameters_node->count(); index++) {
            // Each item is either a parameter definition...or a comma
            grt::BaseListRef parameters_item = grt::BaseListRef::cast_from(parameters_node->get(index));
            std::string node_type = grt::StringRef::extract_from(parameters_item->get(0));

            // Ignores command and only processes parameter definition nodes
            if (node_type == param) {
              grt::BaseListRef parameter = grt::BaseListRef::cast_from(parameters_item->get(2));

              int name_node = 0;

              param_type = "";
              // In the case of the procedures retrieves the parameter type (IN, OUT, INOUT)
              if (type == "PROCEDURE") {
                temp_node = grt::BaseListRef::cast_from(parameter->get(0));
                param_type = "[" + grt::StringRef::extract_from(temp_node->get(1)) + "] ";
                name_node++;
              }

              // Gets the parameter name
              temp_node = grt::BaseListRef::cast_from(parameter->get(name_node));
              param_name = grt::StringRef::extract_from(temp_node->get(1));

              // Gets the type node
              grt::BaseListRef type_node = getItemFromPath("type_with_opt_collate,type", parameter);

              // Retrieves the full type definition...
              param_type += getTypeDescription(type_node, &additional_type_data_paths);
              parameters.push_back(std::make_pair(param_name, param_type));
            }
          }
        }

        // In the case of functions retrieves the return type
        if (type == "FUNCTION") {
          temp_node = getItemFromPath("type_with_opt_collate,type", routine_item);
          return_value = getTypeDescription(temp_node, &additional_type_data_paths);
        }

        // Finally searches for comments defined on the routine
        comments = "";
        temp_node = getItemFromPath("sp_c_chistics", routine_item);
        if (temp_node.is_valid()) {
          for (size_t index = 0; index < temp_node->count() && !comments.length(); index++) {
            grt::BaseListRef item_node = grt::BaseListRef::cast_from(temp_node->get(index));
            item_node = getItemFromPath("sp_c_chistic,sp_chistic", temp_node);

            if (item_node.is_valid()) {
              grt::BaseListRef sub_item = grt::BaseListRef::cast_from(item_node->get(0));

              if (grt::StringRef::extract_from(sub_item->get(0)) == "COMMENT_SYM") {
                sub_item = grt::BaseListRef::cast_from(item_node->get(1));
                comments = grt::StringRef::extract_from(sub_item->get(1));
              }
            }
          }
        }

        ret_val = true;
      }
    }
  }

  return ret_val;
}

//--------------------------------------------------------------------------------------------------
static std::string symbol_from_node(grt::ValueRef node) {
  return grt::StringRef::cast_from(grt::BaseListRef::cast_from(node)[0]);
}

static std::string value_from_node(grt::ValueRef node) {
  return grt::StringRef::cast_from(grt::BaseListRef::cast_from(node)[1]);
}

static grt::BaseListRef children_from_node(grt::ValueRef node) {
  return grt::BaseListRef::cast_from(grt::BaseListRef::cast_from(node)[2]);
}

static bool extract_schema_object_idents(grt::BaseListRef node, std::string &schema_name, std::string &object_name,
                                         int offset = 0) {
  int c = (int)node.count();
  if (c > offset + 0 && symbol_from_node(node[offset + 0]) == "ident") {
    if (c > offset + 2 && symbol_from_node(node[offset + 1]) == "46" && symbol_from_node(node[offset + 2]) == "ident") {
      schema_name = value_from_node(node[offset + 0]);
      object_name = value_from_node(node[offset + 2]);
    } else {
      object_name = value_from_node(node[offset + 0]);
      return true;
    }
  }
  return false;
}

bool MysqlSqlFacadeImpl::parseDropStatement(const std::string &sql, std::string &object_type,
                                            std::vector<std::pair<std::string, std::string> > &object_names) {
  bool ret_val = false;

  /* gets the AST tree for the given statement */
  grt::BaseListRef result = parseAstFromSqlScript(sql);

  if (result.is_valid()) {
    /* Ensures only a single statement has been processed */
    /* Correct results are given inside a list, errors are reported as strings */
    if (result->count() == 1 && result[0].type() == ListType) {
      /* Retrieves the node containing for a DROP statement */
      std::string drop_path = "verb_clause,statement,drop";
      grt::BaseListRef drop_item = getItemFromPath(drop_path, result);
      if (drop_item.is_valid() && drop_item.count() > 0) {
        // item 0 = DROP
        std::string type = symbol_from_node(drop_item[1]);
        if (type == "DATABASE") {
          object_type = "db.Schema";
          for (size_t i = 2; i < drop_item.count(); i++) {
            if (symbol_from_node(drop_item[i]) == "ident") {
              object_names.push_back(std::make_pair(value_from_node(drop_item[i]), ""));
              ret_val = true;
              break;
            }
          }
        } else if (type == "table_or_tables") {
          grt::BaseListRef table_list = getItemFromPath("table_list", drop_item);
          object_type = "db.Table";
          for (size_t i = 0; i < table_list.count(); i++) {
            if (symbol_from_node(table_list[i]) == "table_name") {
              std::string schema_name, table_name;
              extract_schema_object_idents(getItemFromPath("table_ident", children_from_node(table_list[i])),
                                           schema_name, table_name);
              object_names.push_back(std::make_pair(schema_name, table_name));
              ret_val = true;
            }
          }
        } else if (type == "VIEW_SYM") {
          grt::BaseListRef table_list = getItemFromPath("table_list", drop_item);
          object_type = "db.View";
          for (size_t i = 0; i < table_list.count(); i++) {
            if (symbol_from_node(table_list[i]) == "table_name") {
              std::string schema_name, table_name;
              extract_schema_object_idents(getItemFromPath("table_ident", children_from_node(table_list[i])),
                                           schema_name, table_name);
              object_names.push_back(std::make_pair(schema_name, table_name));
              ret_val = true;
            }
          }
        } else if (type == "PROCEDURE_SYM") {
          grt::BaseListRef args = children_from_node(drop_item);
          object_type = "db.StoredProcedure";
          std::string schema_name, routine_name;
          extract_schema_object_idents(getItemFromPath("sp_name", drop_item), schema_name, routine_name);
          object_names.push_back(std::make_pair(schema_name, routine_name));
          ret_val = true;
        } else if (type == "FUNCTION_SYM") {
          object_type = "db.Function";
          std::string schema_name, routine_name;
          extract_schema_object_idents(drop_item, schema_name, routine_name, 2);
          object_names.push_back(std::make_pair(schema_name, routine_name));
          ret_val = true;
        } else if (type == "TRIGGER_SYM") {
          grt::BaseListRef args = children_from_node(drop_item);
          object_type = "db.Trigger";
          std::string schema_name, routine_name;
          extract_schema_object_idents(getItemFromPath("sp_name", drop_item), schema_name, routine_name);
          object_names.push_back(std::make_pair(schema_name, routine_name));
          ret_val = true;
        } else if (type == "INDEX_SYM") {
          object_type = "db.Index";

          if (drop_item.count() > 3) {
            std::string index_name = value_from_node(drop_item[2]);
            std::string schema_name, table_name;
            if (extract_schema_object_idents(getItemFromPath("table_ident", children_from_node(drop_item[4])),
                                             schema_name, table_name)) {
              object_names.push_back(std::make_pair(schema_name, table_name + "." + index_name));

              ret_val = true;
            }
          }
        }
      }
    }
  }
  return ret_val;
}

/**
 * Signals any ongoing process to stop. This must be called from a different thread than from where
 * the processing was started to make it work.
 */
void MysqlSqlFacadeImpl::stop_processing() {
  _stop = true;
}
