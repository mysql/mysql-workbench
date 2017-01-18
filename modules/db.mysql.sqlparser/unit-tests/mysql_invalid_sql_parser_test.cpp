/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grtsqlparser/sql_facade.h"
#include "wb_helpers.h"

// This file contains unit tests for the yacc based invalid sql parser.

//--------------------------------------------------------------------------------------------------

BEGIN_TEST_DATA_CLASS(mysql_invalid_sql_parser_test)
protected:
WBTester *_tester;
SqlFacade::Ref _facade;
std::string _specifics_delimiter;
std::string _user_delimiter;

TEST_DATA_CONSTRUCTOR(mysql_invalid_sql_parser_test) {
  _tester = new WBTester();
  populate_grt(*_tester);

  _facade = SqlFacade::instance_for_rdbms_name("Mysql");
  Sql_specifics::Ref sql_specifics = _facade->sqlSpecifics();
  _specifics_delimiter = sql_specifics->non_std_sql_delimiter();
  if (_specifics_delimiter == ";;")
    _user_delimiter = "%%";
  else
    _user_delimiter = ";;";
}

END_TEST_DATA_CLASS

TEST_MODULE(mysql_invalid_sql_parser_test, "MySQL invalid sql parser test suite (yacc)");

//--------------------------------------------------------------------------------------------------

TEST_FUNCTION(5) {
  std::string trigger_sql =
    "CREATE TRIGGER `ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN\n"
    "  INSERT INTO film_text (film_id, title, description)\n"
    "  VALUES (new.film_id, new.title, new.description);\n"
    "END";

  Invalid_sql_parser::Ref parser = _facade->invalidSqlParser();

  db_mysql_CatalogRef catalog(grt::Initialized);
  db_mysql_SchemaRef schema(grt::Initialized);
  schema->name("sakila");
  catalog->schemata().insert(schema);
  schema->owner(catalog);

  db_mysql_TableRef table(grt::Initialized);
  table->name("film");
  schema->tables().insert(table);
  table->owner(schema);

  db_mysql_TriggerRef trigger(grt::Initialized);
  table->triggers().insert(trigger);
  trigger->owner(table);

  parser->parse_trigger(trigger, trigger_sql);

  // The parsing process returns one line break before the actual sql definition, even if no delimiter
  // etc. was given. This is due to the way query separation works there.
  // So we exclude the first char from the result in our comparisons.
  std::string result = (*trigger->sqlDefinition()).substr(1);
  ensure_equals("5.1 Trigger SQL differs", result, trigger_sql);

  std::string sql = "use test;\n" + trigger_sql;
  parser->parse_trigger(trigger, trigger_sql);
  result = (*trigger->sqlDefinition()).substr(1);
  ensure_equals("5.2 Trigger SQL differs", result, trigger_sql);

  sql = "DELIMITER ;\n" + trigger_sql;
  parser->parse_trigger(trigger, trigger_sql);
  result = (*trigger->sqlDefinition()).substr(1);
  ensure_equals("5.3 Trigger SQL differs", result, trigger_sql);

  sql = "DELIMITER " + _specifics_delimiter + "\n" + trigger_sql;
  parser->parse_trigger(trigger, trigger_sql);
  result = (*trigger->sqlDefinition()).substr(1);
  ensure_equals("5.4 Trigger SQL differs", result, trigger_sql);

  sql = "DELIMITER " + _specifics_delimiter + "\nuse test" + _specifics_delimiter + "\n" + trigger_sql;
  parser->parse_trigger(trigger, trigger_sql);
  result = (*trigger->sqlDefinition()).substr(1);
  ensure_equals("5.5 Trigger SQL differs", result, trigger_sql);

  sql = "DELIMITER " + _specifics_delimiter + "\nDELIMITER " + _user_delimiter + "\nDELIMITER ;\nDELIMITER " +
        _user_delimiter + "\nDELIMITER " + _specifics_delimiter + "\nDELIMITER " + _specifics_delimiter + "\nuse test" +
        _specifics_delimiter + "\n" + trigger_sql;
  parser->parse_trigger(trigger, trigger_sql);
  result = (*trigger->sqlDefinition()).substr(1);
  ensure_equals("5.6 Trigger SQL differs", result, trigger_sql);

  sql = "DELIMITER " + _user_delimiter + "\nuse test" + _specifics_delimiter + "\n\n\n\n" + trigger_sql;
  parser->parse_trigger(trigger, trigger_sql);
  result = (*trigger->sqlDefinition()).substr(1);
  ensure_equals("5.7 Trigger SQL differs", result, trigger_sql);
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete _tester;
}

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------
