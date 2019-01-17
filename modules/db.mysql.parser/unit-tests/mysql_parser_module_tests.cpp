/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_helpers.h"

#include "grt.h"
#include "grtsqlparser/mysql_parser_services.h"

using namespace parsers;

// Contains tests for the parser module implementing the ANTLR based parser services.
// Many of the APIs are also used in other tests.

BEGIN_TEST_DATA_CLASS(mysql_parser_module_tests)
protected:
  WBTester *_tester;
  MySQLParserServices::Ref _services;
  MySQLParserContext::Ref _context;

  TEST_DATA_CONSTRUCTOR(mysql_parser_module_tests)
  {
    _tester = new WBTester();
    populate_grt(*_tester);

    _services = MySQLParserServices::get();
    GrtVersionRef version(grt::Initialized);
    version->majorNumber(5);
    version->minorNumber(7);
    version->releaseNumber(10);
    _context = MySQLParserServices::get()->createParserContext(_tester->get_rdbms()->characterSets(), version, "", true);
  }

END_TEST_DATA_CLASS

TEST_MODULE(mysql_parser_module_tests, "parser module tests");

// Tests for parseStatement.
// Each test function tests a group of statements from the grammar, as listed in the top rule.
// Not all query types are implemented in parseStatement. So most of the functions are empty atm.

// alterStatement
TEST_FUNCTION(5)
{
}

// createStatement
// dropStatement
// renameTableStatement
// truncateTableStatement
// callStatement
// deleteStatement
// doStatement
// handlerStatement
// insertStatement
// loadStatement
// replaceStatement
// selectStatement
// updateStatement
// partitioning
// transactionOrLockingStatement
// replicationStatement
// preparedStatement

// accountManagementStatement
TEST_FUNCTION(95)
{
  // ----- Grant statements.
  grt::DictRef result = _services->parseStatement(_context, "grant all privileges on table a to current_user");
  grt::StringListRef privileges = grt::StringListRef::cast_from(result["privileges"]);
  ensure("95.1", privileges.is_valid());
  ensure("95.2", privileges.get_index("all privileges") == 0);
  ensure_equals("95.3", *grt::StringRef::cast_from(result["target"]), "table a");

  grt::DictRef users = grt::DictRef::cast_from(result["users"]);
  ensure("95.4", users.is_valid());
  ensure("95.5", users.has_key("current_user"));
  grt::DictRef user = grt::DictRef::cast_from(users["current_user"]);
  ensure("95.6", user.has_key("user"));
  ensure_equals("95.7", *grt::StringRef::cast_from(user["user"]), "current_user");

  result = _services->parseStatement(_context, "grant all privileges on table *.* to CURRENT_USER() identified by password 'blah'");
  privileges = grt::StringListRef::cast_from(result["privileges"]);
  ensure("95.8", privileges.is_valid());
  ensure_equals("95.9", *grt::StringRef::cast_from(privileges[0]), "all privileges");
  ensure_equals("95.10", *grt::StringRef::cast_from(result["target"]), "table *.*");

  users = grt::DictRef::cast_from(result["users"]);
  ensure("95.11", users.is_valid());
  user = grt::DictRef::cast_from(users["CURRENT_USER"]);
  ensure_equals("95.12", *grt::StringRef::cast_from(user["user"]), "CURRENT_USER");
  ensure_equals("95.13", *grt::StringRef::cast_from(user["id_method"]), "PASSWORD");
  ensure_equals("95.14", *grt::StringRef::cast_from(user["id_string"]), "blah");

  result = _services->parseStatement(_context, "grant all privileges on x.* to mike identified with 'blah' by 'blubb'");
  privileges = grt::StringListRef::cast_from(result["privileges"]);
  ensure("95.15", privileges.is_valid());
  ensure_equals("95.16", *grt::StringRef::cast_from(privileges[0]), "all privileges");
  ensure_equals("95.17", *grt::StringRef::cast_from(result["target"]), "x.*");

  users = grt::DictRef::cast_from(result["users"]);
  ensure("95.18", users.is_valid());
  user = grt::DictRef::cast_from(users["mike"]);
  ensure("95.19", user.is_valid());
  ensure_equals("95.20", *grt::StringRef::cast_from(user["user"]), "mike");
  ensure_equals("95.21", *grt::StringRef::cast_from(user["id_method"]), "blah");
  ensure_equals("95.22", *grt::StringRef::cast_from(user["id_string"]), "blubb");

  result = _services->parseStatement(_context, "grant all privileges on function x.y to mike\t@\nhome");
  privileges = grt::StringListRef::cast_from(result["privileges"]);
  ensure("95.23", privileges.is_valid());
  ensure_equals("95.24", *grt::StringRef::cast_from(privileges[0]), "all privileges");
  ensure_equals("95.25", *grt::StringRef::cast_from(result["target"]), "function x.y");

  users = grt::DictRef::cast_from(result["users"]);
  ensure("95.26", users.is_valid());
  user = grt::DictRef::cast_from(users["mike"]);
  ensure("95.27", user.is_valid());
  ensure_equals("95.28", *grt::StringRef::cast_from(user["user"]), "mike");
  ensure("95.29", !user["id_method"].is_valid());
  ensure("95.30", !user["id_string"].is_valid());
  ensure_equals("95.31", *grt::StringRef::cast_from(user["host"]), "home");

  result = _services->parseStatement(_context, "grant select on pizza to me require X509 with grant option");
  privileges = grt::StringListRef::cast_from(result["privileges"]);
  ensure("95.32", privileges.is_valid());
  ensure_equals("95.33", *grt::StringRef::cast_from(privileges[0]), "select");
  ensure_equals("95.34", *grt::StringRef::cast_from(result["target"]), "pizza");

  users = grt::DictRef::cast_from(result["users"]);
  ensure("95.35", users.is_valid());
  user = grt::DictRef::cast_from(users["me"]);
  ensure("95.36", user.is_valid());
  ensure_equals("95.37", *grt::StringRef::cast_from(user["user"]), "me");
  ensure_equals("95.38", user.count(), 1UL);

  // Everything possible in a grant statements.
  std::string sql = base::wstring_to_string(L"grant insert (a), insert (b), insert(c), update(a), "
    L"alter routine, create routine, "
    L"create tablespace, create\t\t\t temporary      tables, create user, create view, delete, drop, event, "
    L"execute, file, grant option, index, insert, insert (a, b, c, d, ⌚️, ♨️), lock tables, process, proxy, "
    L"references (a, b, c, d, ⌚️, ♨️), reload, replication client, select, select (a, b, c, d, ⌚️, ♨️), "
    L"show databases, show view, shutdown, super, trigger, update, update (a, b, c, d, ⌚️, ♨️), usage "
    L"on *.* to current_user, CURRENT_USER() identified by password 'blah', mike identified with 'blah' by 'blubb', "
    L"mike@home require cipher 'abc' and cipher 'xyz' issuer 'a' subject 'b' and issuer '⌚️' with "
    L"grant option max_queries_per_hour 1 max_updates_per_hour 2 max_connections_per_hour 3 "
    L"max_user_connections 4 max_queries_per_hour 111 max_queries_per_hour 111 max_queries_per_hour 222");
  result = _services->parseStatement(_context, sql);
  privileges = grt::StringListRef::cast_from(result["privileges"]);
  ensure("95.39", privileges.is_valid());
  ensure_equals("95.40", *grt::StringRef::cast_from(privileges[0]), "insert (a)");
  ensure_equals("95.41", *grt::StringRef::cast_from(privileges[7]), "create\t\t\t temporary      tables");
  ensure_equals("95.42", *grt::StringRef::cast_from(privileges[11]), "drop");
  ensure_equals("95.43", *grt::StringRef::cast_from(privileges[15]), "grant option");
  ensure_equals("95.44", *grt::StringRef::cast_from(privileges[18]), base::wstring_to_string(L"insert (a, b, c, d, ⌚️, ♨️)"));
  ensure_equals("95.45", *grt::StringRef::cast_from(privileges[27]), "show databases");

  ensure_equals("95.46", *grt::StringRef::cast_from(result["target"]), "*.*");

  users = grt::DictRef::cast_from(result["users"]);
  ensure("95.48", users.is_valid());
  ensure_equals("95.49", users.count(), 3UL);
  ensure("95.40", users["current_user"].is_valid());
  ensure("95.50", users["CURRENT_USER"].is_valid());
  ensure("95.51", users["mike"].is_valid());

  user = grt::DictRef::cast_from(users["CURRENT_USER"]);
  ensure_equals("95.52", *grt::StringRef::cast_from(user["user"]), "CURRENT_USER");
  ensure_equals("95.53", user.count(), 3UL);

  grt::DictRef options = grt::DictRef::cast_from(result["options"]);
  ensure("95.54", options.is_valid());
  ensure_equals("95.55", options.count(), 5UL);
  ensure_equals("95.56", *grt::StringRef::cast_from(options["grant"]), "");
  ensure_equals("95.57", *grt::StringRef::cast_from(options["max_queries_per_hour"]), "222");

  grt::DictRef requirements = grt::DictRef::cast_from(result["requirements"]);
  ensure("95.58", requirements.is_valid());
  ensure_equals("95.59", requirements.count(), 3UL);
  ensure_equals("95.60", *grt::StringRef::cast_from(requirements["cipher"]), "xyz");
  ensure_equals("95.61", *grt::StringRef::cast_from(requirements["issuer"]), base::wstring_to_string(L"⌚️"));

}

// table_administrationStatement
// install_uninstall_statment
// setStatement
// showStatement
// other_administrativeStatement
// utilityStatement

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99)
{
  _context.reset();
  delete _tester;
}

END_TESTS
