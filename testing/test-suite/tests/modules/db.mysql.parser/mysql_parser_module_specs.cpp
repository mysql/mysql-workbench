/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "casmine.h"
#include "wb_test_helpers.h"

#include "grt.h"
#include "grtsqlparser/mysql_parser_services.h"

using namespace parsers;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  MySQLParserServices::Ref services;
  MySQLParserContext::Ref context;
};

// Contains tests for the parser module implementing the ANTLR based parser services.
// Many of the APIs are also used in other tests.
$describe("Parser module") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester(false));
    data->tester->initializeRuntime();

    data->services = MySQLParserServices::get();
    GrtVersionRef version(grt::Initialized);
    version->majorNumber(5);
    version->minorNumber(7);
    version->releaseNumber(10);
    data->context = MySQLParserServices::get()->createParserContext(data->tester->getRdbms()->characterSets(), version, "", true);
  });

  $afterAll([this]() {
    data->context.reset();
  });

  // Tests for parseStatement.
  // Each test spec tests a group of statements from the grammar, as listed in the top rule.
  // Not all query types are implemented in parseStatement. So most of the functions are empty atm.
  $it("alterStatement", []() {
    $pending("requires implementation");
  });

  $it("createStatement", []() {
    $pending("requires implementation");
  });

  $it("dropStatement", []() {
    $pending("requires implementation");
  });

  $it("renameTableStatement", []() {
    $pending("requires implementation");
  });

  $it("truncateTableStatement", []() {
    $pending("requires implementation");
  });

  $it("callStatement", []() {
    $pending("requires implementation");
  });

  $it("deleteStatement", []() {
    $pending("requires implementation");
  });

  $it("doStatement", []() {
    $pending("requires implementation");
  });

  $it("handlerStatement", []() {
    $pending("requires implementation");
  });

  $it("insertStatement", []() {
    $pending("requires implementation");
  });

  $it("loadStatement", []() {
    $pending("requires implementation");
  });

  $it("replaceStatement", []() {
    $pending("requires implementation");
  });

  $it("selectStatement", []() {
    $pending("requires implementation");
  });

  $it("updateStatement", []() {
    $pending("requires implementation");
  });

  $it("partitioning", []() {
    $pending("requires implementation");
  });

  $it("transactionOrLockingStatement", []() {
    $pending("requires implementation");
  });

  $it("replicationStatement", []() {
    $pending("requires implementation");
  });

  $it("preparedStatement", []() {
    $pending("requires implementation");
  });

  $it("accountManagementStatement", [this]() {
    // ----- Grant statements.
    grt::DictRef result = data->services->parseStatement(data->context, "grant all privileges on table a to current_user");
    grt::StringListRef privileges = grt::StringListRef::cast_from(result["privileges"]);
    $expect(privileges.is_valid()).toBeTrue("95.1");
    $expect(privileges.get_index("all privileges")).toBe(0U, "95.2");
    $expect(*grt::StringRef::cast_from(result["target"])).toBe("table a", "95.3");

    grt::DictRef users = grt::DictRef::cast_from(result["users"]);
    $expect(users.is_valid()).toBeTrue("95.4");
    $expect(users.has_key("current_user")).toBeTrue("95.5");
    grt::DictRef user = grt::DictRef::cast_from(users["current_user"]);
    $expect(user.has_key("user")).toBeTrue("95.6");
    $expect(*grt::StringRef::cast_from(user["user"])).toBe("current_user", "95.7");

    result = data->services->parseStatement(data->context, "grant all privileges on table *.* to CURRENT_USER() identified by password 'blah'");
    privileges = grt::StringListRef::cast_from(result["privileges"]);
    $expect(privileges.is_valid()).toBeTrue("95.8");
    $expect(*grt::StringRef::cast_from(privileges[0])).toBe("all privileges", "95.9");
    $expect(*grt::StringRef::cast_from(result["target"])).toBe("table *.*", "95.10");

    users = grt::DictRef::cast_from(result["users"]);
    $expect(users.is_valid()).toBeTrue("95.11");
    user = grt::DictRef::cast_from(users["CURRENT_USER"]);
    $expect(*grt::StringRef::cast_from(user["user"])).toBe("CURRENT_USER", "95.12");
    $expect(*grt::StringRef::cast_from(user["id_method"])).toBe("PASSWORD", "95.13");
    $expect(*grt::StringRef::cast_from(user["id_string"])).toBe("blah", "95.14");

    result = data->services->parseStatement(data->context, "grant all privileges on x.* to mike identified with 'blah' by 'blubb'");
    privileges = grt::StringListRef::cast_from(result["privileges"]);
    $expect(privileges.is_valid()).toBeTrue("95.15");
    $expect(*grt::StringRef::cast_from(privileges[0])).toBe("all privileges", "95.1");
    $expect(*grt::StringRef::cast_from(result["target"])).toBe("x.*", "95.17");

    users = grt::DictRef::cast_from(result["users"]);
    $expect(users.is_valid()).toBeTrue("95.18");
    user = grt::DictRef::cast_from(users["mike"]);
    $expect(user.is_valid()).toBeTrue("95.1");
    $expect(*grt::StringRef::cast_from(user["user"])).toBe("mike", "95.20");
    $expect(*grt::StringRef::cast_from(user["id_method"])).toBe("blah", "95.21");
    $expect(*grt::StringRef::cast_from(user["id_string"])).toBe("blubb", "95.22");

    result = data->services->parseStatement(data->context, "grant all privileges on function x.y to mike\t@\nhome");
    privileges = grt::StringListRef::cast_from(result["privileges"]);
    $expect(privileges.is_valid()).toBeTrue("95.23");
    $expect(*grt::StringRef::cast_from(privileges[0])).toBe("all privileges", "95.24");
    $expect(*grt::StringRef::cast_from(result["target"])).toBe("function x.y", "95.25");

    users = grt::DictRef::cast_from(result["users"]);
    $expect(users.is_valid()).toBeTrue("95.26");
    user = grt::DictRef::cast_from(users["mike"]);
    $expect(user.is_valid()).toBeTrue("95.27");
    $expect(*grt::StringRef::cast_from(user["user"])).toBe("mike", "95.28");
    $expect(user["id_method"].is_valid()).toBeFalse("95.29");
    $expect(user["id_string"].is_valid()).toBeFalse("95.30");
    $expect(*grt::StringRef::cast_from(user["host"])).toBe("home", "95.31");

    result = data->services->parseStatement(data->context, "grant select on pizza to me require X509 with grant option");
    privileges = grt::StringListRef::cast_from(result["privileges"]);
    $expect(privileges.is_valid()).toBeTrue("95.32");
    $expect(*grt::StringRef::cast_from(privileges[0])).toBe("select", "95.33");
    $expect(*grt::StringRef::cast_from(result["target"])).toBe("pizza", "95.34");

    users = grt::DictRef::cast_from(result["users"]);
    $expect(users.is_valid()).toBeTrue("95.35");
    user = grt::DictRef::cast_from(users["me"]);
    $expect(user.is_valid()).toBeTrue("95.36");
    $expect(*grt::StringRef::cast_from(user["user"])).toBe("me", "95.37");
    $expect(user.count()).toBe(1U, "95.38");

    // Everything possible in a grant statements.
    std::string sql = base::wstring_to_string(
      L"grant insert (a), insert (b), insert(c), update(a), "
      L"alter routine, create routine, "
      L"create tablespace, create\t\t\t temporary      tables, create user, create view, delete, drop, event, "
      L"execute, file, grant option, index, insert, insert (a, b, c, d, ⌚️, ♨️), lock tables, process, proxy, "
      L"references (a, b, c, d, ⌚️, ♨️), reload, replication client, select, select (a, b, c, d, ⌚️, ♨️), "
      L"show databases, show view, shutdown, super, trigger, update, update (a, b, c, d, ⌚️, ♨️), usage "
      L"on *.* to current_user, CURRENT_USER() identified by password 'blah', mike identified with 'blah' by 'blubb', "
      L"mike@home require cipher 'abc' and cipher 'xyz' issuer 'a' subject 'b' and issuer '⌚️' with "
      L"grant option max_queries_per_hour 1 max_updates_per_hour 2 max_connections_per_hour 3 "
      L"max_user_connections 4 max_queries_per_hour 111 max_queries_per_hour 111 max_queries_per_hour 222");
    result = data->services->parseStatement(data->context, sql);
    privileges = grt::StringListRef::cast_from(result["privileges"]);
    $expect(privileges.is_valid()).toBeTrue("95.39");
    $expect(*grt::StringRef::cast_from(privileges[0])).toBe("insert (a)", "95.40");
    $expect(*grt::StringRef::cast_from(privileges[7])).toBe("create\t\t\t temporary      tables", "95.41");
    $expect(*grt::StringRef::cast_from(privileges[11])).toBe("drop", "95.42");
    $expect(*grt::StringRef::cast_from(privileges[15])).toBe("grant option", "95.43");
    $expect(*grt::StringRef::cast_from(privileges[18])).toBe(base::wstring_to_string(L"insert (a, b, c, d, ⌚️, ♨️)"), "95.44");
    $expect(*grt::StringRef::cast_from(privileges[27])).toBe("show databases", "95.45");

    $expect(*grt::StringRef::cast_from(result["target"])).toBe("*.*", "95.46");

    users = grt::DictRef::cast_from(result["users"]);
    $expect(users.is_valid()).toBeTrue("95.48");
    $expect(users.count()).toBe(3U, "95.49");
    $expect(users["current_user"].is_valid()).toBeTrue("95.40");
    $expect(users["CURRENT_USER"].is_valid()).toBeTrue("95.50");
    $expect(users["mike"].is_valid()).toBeTrue("95.51");

    user = grt::DictRef::cast_from(users["CURRENT_USER"]);
    $expect(*grt::StringRef::cast_from(user["user"])).toBe("CURRENT_USER", "95.52");
    $expect(user.count()).toBe(3U, "95.53");

    grt::DictRef options = grt::DictRef::cast_from(result["options"]);
    $expect(options.is_valid()).toBeTrue("95.54");
    $expect(options.count()).toBe(5U, "95.55");
    $expect(*grt::StringRef::cast_from(options["grant"])).toBe("", "95.56");
    $expect(*grt::StringRef::cast_from(options["max_queries_per_hour"])).toBe("222", "95.57");

    grt::DictRef requirements = grt::DictRef::cast_from(result["requirements"]);
    $expect(requirements.is_valid()).toBeTrue("95.58");
    $expect(requirements.count()).toBe(3U, "95.59");
    $expect(*grt::StringRef::cast_from(requirements["cipher"])).toBe("xyz", "95.60");
    $expect(*grt::StringRef::cast_from(requirements["issuer"])).toBe(base::wstring_to_string(L"⌚️"), "95.61");
  });

  $it("table_administrationStatement", []() {
    $pending("requires implementation");
  });

  $it("install_uninstall_statment", []() {
    $pending("requires implementation");
  });


  $it("setStatement", []() {
    $pending("requires implementation");
  });


  $it("showStatement", []() {
    $pending("requires implementation");
  });


  $it("other_administrativeStatement", []() {
    $pending("requires implementation");
  });


  $it("utilityStatement", []() {
    $pending("requires implementation");
  });

}

}
