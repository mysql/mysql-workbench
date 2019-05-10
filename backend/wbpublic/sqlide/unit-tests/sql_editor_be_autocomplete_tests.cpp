/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "code-completion/mysql-code-completion.h"
#include "mysql/MySQLRecognizerCommon.h"
#include "mysql/MySQLLexer.h"
#include "mysql/MySQLParser.h"
#include "mysql/MySQLParserBaseListener.h"


#include "connection_helpers.h"
#include "base/file_utilities.h"
#include "base/symbol-info.h"

#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/sql_editor_be.h"

#include "grtsqlparser/mysql_parser_services.h"

#include "tut_mysql_versions.h"

using namespace base;
using namespace bec;
using namespace wb;
using namespace parsers;
using namespace antlr4;

namespace ph = std::placeholders;

struct ac_test_entry {
  int version_first; //  First supported version for this entry
  int version_last;  //  Last supported version for this entry

  std::string query;
  std::string typed_part;
  int line;
  int offset;

  bool check_entries;
  std::string entries;
  int parts;
};

BEGIN_TEST_DATA_CLASS(sql_editor_be_autocomplete_tests)
protected:
  WBTester *_tester;
  MySQLEditor::Ref _sql_editor;

  parsers::MySQLParserContext::Ref _autocomplete_context;
  SymbolTable _mainSymbols;
  SymbolTable _dbObjects;
  SymbolTable _runtimeFunctions;

  long version;

public:
TEST_DATA_CONSTRUCTOR(sql_editor_be_autocomplete_tests) {
  _tester = new WBTester();
  populate_grt(*_tester);
}

END_TEST_DATA_CLASS;

TEST_MODULE(sql_editor_be_autocomplete_tests, "SQL code completion tests");

// Create a mockup symbol table with all database objects we support.
void createDBObjects(SymbolTable &symbolTable) {
  auto sakila = symbolTable.addNewSymbol<SchemaSymbol>(nullptr, "sakila");
  symbolTable.addNewSymbol<SchemaSymbol>(nullptr, "sakila_test");
  symbolTable.addNewSymbol<SchemaSymbol>(nullptr, "mysql");
  symbolTable.addNewSymbol<SchemaSymbol>(nullptr, "information_schema");
  symbolTable.addNewSymbol<SchemaSymbol>(nullptr, "sys");
  symbolTable.addNewSymbol<SchemaSymbol>(nullptr, "test");

  symbolTable.addNewSymbol<TableSymbol>(sakila, "actor");
  symbolTable.addNewSymbol<TableSymbol>(sakila, "address");
  symbolTable.addNewSymbol<TableSymbol>(sakila, "category");
  symbolTable.addNewSymbol<TableSymbol>(sakila, "city");
  symbolTable.addNewSymbol<TableSymbol>(sakila, "country");
  symbolTable.addNewSymbol<TableSymbol>(sakila, "custom");
  auto film = symbolTable.addNewSymbol<TableSymbol>(sakila, "film");
  symbolTable.addNewSymbol<ColumnSymbol>(film, "film_id", FundamentalType::INTEGER_TYPE);
  symbolTable.addNewSymbol<ColumnSymbol>(film, "title", FundamentalType::STRING_TYPE);
  symbolTable.addNewSymbol<ColumnSymbol>(film, "description", FundamentalType::STRING_TYPE);
  symbolTable.addNewSymbol<ColumnSymbol>(film, "release_year", FundamentalType::DATE_TYPE);
  symbolTable.addNewSymbol<ColumnSymbol>(film, "language_id", FundamentalType::INTEGER_TYPE);
  symbolTable.addNewSymbol<ColumnSymbol>(film, "original_language_id", FundamentalType::INTEGER_TYPE);
  symbolTable.addNewSymbol<IndexSymbol>(film, "PRIMARY");
  symbolTable.addNewSymbol<IndexSymbol>(film, "idx_title");
  symbolTable.addNewSymbol<ForeignKeySymbol>(film, "fk_film_language");
  symbolTable.addNewSymbol<ForeignKeySymbol>(film, "fk_film_language_original");

  symbolTable.addNewSymbol<ViewSymbol>(sakila, "actor_info");
  symbolTable.addNewSymbol<ViewSymbol>(sakila, "custom_list");
  symbolTable.addNewSymbol<ViewSymbol>(sakila, "nicer_but_slower_film_list");
  symbolTable.addNewSymbol<ViewSymbol>(sakila, "sales_by_film_category");
  symbolTable.addNewSymbol<ViewSymbol>(sakila, "sales_by_store");

  symbolTable.addNewSymbol<EventSymbol>(sakila, "event1");

  symbolTable.addNewSymbol<StoredRoutineSymbol>(sakila, "film_in_stock", nullptr);
  symbolTable.addNewSymbol<StoredRoutineSymbol>(sakila, "film_not_in_stock", nullptr);
  symbolTable.addNewSymbol<StoredRoutineSymbol>(sakila, "rewards_report", nullptr);

  symbolTable.addNewSymbol<StoredRoutineSymbol>(sakila, "get_customer_balance", FundamentalType::FLOAT_TYPE);
  symbolTable.addNewSymbol<StoredRoutineSymbol>(sakila, "inventory_held_by_customer", FundamentalType::INTEGER_TYPE);
  symbolTable.addNewSymbol<StoredRoutineSymbol>(sakila, "inventory_in_stock", FundamentalType::INTEGER_TYPE);

  // Triggers are schema local objects, even though bound to a specific table.
  symbolTable.addNewSymbol<TriggerSymbol>(sakila, "ins_film");
  symbolTable.addNewSymbol<TriggerSymbol>(sakila, "del_film");

  // Other schema or global entities.
  symbolTable.addNewSymbol<UdfSymbol>(sakila, "udf1");
  symbolTable.addNewSymbol<UdfSymbol>(sakila, "udf2");
  symbolTable.addNewSymbol<UdfSymbol>(sakila, "udf3");

  symbolTable.addNewSymbol<EngineSymbol>(nullptr, "innodb");
  symbolTable.addNewSymbol<EngineSymbol>(nullptr, "myisam");
  symbolTable.addNewSymbol<EngineSymbol>(nullptr, "blackhole");

  symbolTable.addNewSymbol<LogfileGroupSymbol>(nullptr, "group1");
  symbolTable.addNewSymbol<LogfileGroupSymbol>(nullptr, "group2");
  symbolTable.addNewSymbol<LogfileGroupSymbol>(nullptr, "group3");

  symbolTable.addNewSymbol<TableSpaceSymbol>(nullptr, "space1");
  symbolTable.addNewSymbol<TableSpaceSymbol>(nullptr, "space2");
  symbolTable.addNewSymbol<TableSpaceSymbol>(nullptr, "space3");

  symbolTable.addNewSymbol<SystemVariableSymbol>(nullptr, "@@var1");
  symbolTable.addNewSymbol<SystemVariableSymbol>(nullptr, "@@var2");
  symbolTable.addNewSymbol<SystemVariableSymbol>(nullptr, "@@var3");

  symbolTable.addNewSymbol<UserVariableSymbol>(nullptr, "@var11", FundamentalType::INTEGER_TYPE);
  symbolTable.addNewSymbol<UserVariableSymbol>(nullptr, "@var22", FundamentalType::INTEGER_TYPE);
  symbolTable.addNewSymbol<UserVariableSymbol>(nullptr, "@var33", FundamentalType::INTEGER_TYPE);

  symbolTable.addNewSymbol<CharsetSymbol>(nullptr, "utf8");
  symbolTable.addNewSymbol<CharsetSymbol>(nullptr, "latin1");
  symbolTable.addNewSymbol<CharsetSymbol>(nullptr, "big5");

  symbolTable.addNewSymbol<CollationSymbol>(nullptr, "utf8_bin");
  symbolTable.addNewSymbol<CollationSymbol>(nullptr, "latin1_general_cs");
  symbolTable.addNewSymbol<CollationSymbol>(nullptr, "big5_chinese_ci");
}

/**
 * Setup required objects.
 */
TEST_FUNCTION(5) {
  GrtVersionRef version = _tester->get_rdbms()->version();

  // Copy a current version of the code editor configuration file to the test data folder.
  gchar *contents;
  gsize length;
  GError *error = NULL;
  if (g_file_get_contents("../../res/wbdata/code_editor.xml", &contents, &length, &error)) {
    ensure("Could not write editor configuration to target file",
           g_file_set_contents("data/code_editor.xml", contents, length, &error) == TRUE);
    g_free(contents);
  } else
    fail("Could not copy code editor configuration");

  parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();
  parsers::MySQLParserContext::Ref context =
    services->createParserContext(_tester->get_rdbms()->characterSets(), version, "", false);

  _autocomplete_context = services->createParserContext(_tester->get_rdbms()->characterSets(), version, "", false);

  _sql_editor = MySQLEditor::create(context, _autocomplete_context, { &_mainSymbols });
  _sql_editor->set_current_schema("sakila");

  createDBObjects(_dbObjects);
  _mainSymbols.addDependencies({ &_dbObjects, functionSymbolsForVersion(MySQLVersion::MySQL57) });
}

// Testing proper symbol retrieval in symbol tables.
TEST_FUNCTION(10) {
  auto tables = _mainSymbols.getSymbolsOfType<TableSymbol>();
  ensure_equals("Test 10.1", tables.size(), 0U);

  auto schemas = _mainSymbols.getSymbolsOfType<SchemaSymbol>();
  ensure_equals("Test 10.2", schemas.size(), 6U);

  auto iterator = std::find_if(schemas.begin(), schemas.end(), [](Symbol *symbol) {
    return symbol->name == "sakila";
  });
  ensure("Test 10.3", iterator != schemas.end());
  SchemaSymbol *schema = *iterator;
  ensure("Test 10.4", schema != nullptr);

  tables = _mainSymbols.getSymbolsOfType<TableSymbol>(schema);
  ensure_equals("Test 10.5", tables.size(), 7U);
  auto views = _mainSymbols.getSymbolsOfType<ViewSymbol>(schema);
  ensure_equals("Test 10.6", views.size(), 5U);
  auto routines = _mainSymbols.getSymbolsOfType<RoutineSymbol>(schema);
  ensure_equals("Test 10.7", routines.size(), 6U);
  auto udfs = _mainSymbols.getSymbolsOfType<UdfSymbol>(schema);
  ensure_equals("Test 10.8", udfs.size(), 3U);

  auto systemFunctions = _mainSymbols.getSymbolsOfType<RoutineSymbol>(); // System functions.
  ensure_equals("Test 10.9", systemFunctions.size(), 293U);
}

class ErrorListener : public BaseErrorListener {
public:
  size_t errorCount = 0;
  virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                           const std::string &msg, std::exception_ptr e) override {
    ++errorCount;
  }
};

// Testing code completion core for correct candidate collections.
TEST_FUNCTION(20) {
  ANTLRInputStream input(
    "CREATE TABLE `partition_test` (\n"
    "`id` int(10) NOT NULL AUTO_INCREMENT,\n"
    "`team` int(10) NOT NULL DEFAULT '0',\n"
    "`created` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',\n"
    "PRIMARY KEY (`id`,`team`,`created`)\n"
    ") ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8\n"
    "/*!50100 PARTITION BY RANGE (team)\n"
    "(PARTITION t1 VALUES LESS THAN (1) ENGINE = MyISAM,\n"
    "PARTITION t2 VALUES LESS THAN (2) ENGINE = MyISAM,\n"
    "PARTITION t3 VALUES LESS THAN (3) ENGINE = MyISAM,\n"
    "PARTITION t4 VALUES LESS THAN (4) ENGINE = MyISAM,\n"
    "PARTITION t5 VALUES LESS THAN (5) ENGINE = MyISAM,\n"
    "PARTITION t6 VALUES LESS THAN (6) ENGINE = MyISAM,\n"
    "PARTITION t7 VALUES LESS THAN (7) ENGINE = MyISAM,\n"
    "PARTITION t8 VALUES LESS THAN (8) ENGINE = MyISAM,\n"
    "PARTITION t9 VALUES LESS THAN (9) ENGINE = MyISAM,\n"
    "PARTITION t10 VALUES LESS THAN (10) ENGINE = MyISAM,\n"
    "PARTITION t11 VALUES LESS THAN (11) ENGINE = MyISAM,\n"
    "PARTITION t12 VALUES LESS THAN (12) ENGINE = MyISAM,\n"
    "PARTITION t13 VALUES LESS THAN (13) ENGINE = MyISAM,\n"
    "PARTITION t14 VALUES LESS THAN (14) ENGINE = MyISAM,\n"
    "PARTITION t15 VALUES LESS THAN (15) ENGINE = MyISAM,\n"
    "PARTITION t16 VALUES LESS THAN MAXVALUE ENGINE = MyISAM) */;");
  MySQLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  MySQLParser parser(&tokens);

  lexer.serverVersion = 50717;
  lexer.sqlMode = MySQLBaseLexer::SqlMode::AnsiQuotes;
  parser.serverVersion = 50717;
  parser.sqlMode = MySQLBaseLexer::SqlMode::AnsiQuotes;
  parser.setBuildParseTree(true);

  ErrorListener errorListener;
  parser.removeErrorListeners();
  parser.addErrorListener(&errorListener);
  parser.query();
  ensure_equals("Test 20.1", errorListener.errorCount, 0U);

  auto candidates = getCodeCompletionList(7, 34, "sakila", false, &parser, _mainSymbols);
  ensure_equals("Test 20.2", candidates.size(), 9U);
  ensure_equals("Test 20.3", candidates[0].second, "comment");
  ensure_equals("Test 20.4", candidates[1].second, "data directory");
  ensure_equals("Test 20.5", candidates[2].second, "engine");
  ensure_equals("Test 20.6", candidates[3].second, "index directory");
  ensure_equals("Test 20.7", candidates[4].second, "max_rows");
  ensure_equals("Test 20.8", candidates[5].second, "min_rows");
  ensure_equals("Test 20.9", candidates[6].second, "nodegroup");
  ensure_equals("Test 20.10", candidates[7].second, "storage");
  ensure_equals("Test 20.11", candidates[8].second, "tablespace");

  candidates = getCodeCompletionList(7, 44, "sakila", false, &parser, _mainSymbols);
  ensure_equals("Test 20.12", candidates.size(), 3U);
  ensure_equals("Test 20.13", candidates[0].second, "blackhole");
  ensure_equals("Test 20.14", candidates[1].second, "innodb");
  ensure_equals("Test 20.15", candidates[2].second, "myisam");
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  _sql_editor.reset();
  _autocomplete_context.reset();
  delete _tester;
}

END_TESTS
