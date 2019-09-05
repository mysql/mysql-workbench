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

#include "grt_test_helpers.h"
#include "db_mysql_diffsqlgen_grant.h"

#include "casmine.h"
#include "model_mockup.h"
#include "wb_test_helpers.h"

namespace {

$ModuleEnvironment() {};

template <class _InIt1, class _InIt2>
  inline void expectContainersEqual(_InIt1 _First1, _InIt1 _Last1, _InIt2 _First2, _InIt2 _Last2) {
    $expect(std::distance(_First1, _Last1)).toBe(std::distance(_First2, _Last2));

    _InIt1 iter1 = _First1;
    _InIt2 iter2 = _First2;
    for (; iter1 != _Last1; iter1++, iter2++)
      $expect(*iter1).toBe(*iter2);
  }

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};


$describe("DB MySQL gen grant") {

  $beforeAll([&]() {
    data->tester.reset(new WorkbenchTester());
  });

  $afterAll([&]() {

  });

  $it("Grant select role", []() {
    casmine::SyntheticMySQLModel model;
    model.catalog->users().remove_all();
    model.catalog->roles().remove_all();

    casmine::xRole role("Admin", model);
    casmine::xUser user("monty", model);

    casmine::addPrivilege(model, role, model.table, "SELECT");

    casmine::assignRole(user, role);

    std::list<std::string> actual;
    gen_grant_sql((db_CatalogRef)model.catalog, actual);
    std::string expect[] = {"GRANT SELECT ON TABLE `test_schema`.`t1` TO 'monty'"};

    expectContainersEqual(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
  });

  $it("Grant insert", []() {
    casmine::SyntheticMySQLModel model;
    model.catalog->users().remove_all();
    model.catalog->roles().remove_all();

    casmine::xRole adminRole("Admin", model);
    casmine::xRole userRole("User", model);

    casmine::addPrivilege(model, adminRole, model.table, "INSERT");
    casmine::addPrivilege(model, userRole, model.table, "SELECT");

    casmine::xUser user1("monty", model);
    casmine::xUser user2("scott", model);

    casmine::assignRole(user1, adminRole);
    casmine::assignRole(user1, userRole);

    casmine::assignRole(user2, userRole);

    std::list<std::string> actual;
    gen_grant_sql((db_CatalogRef)model.catalog, actual);
    std::string expect[] = {
      "GRANT INSERT ON TABLE `test_schema`.`t1` TO 'monty'", "GRANT SELECT ON TABLE `test_schema`.`t1` TO 'monty'",
      "GRANT SELECT ON TABLE `test_schema`.`t1` TO 'scott'",
    };

    expectContainersEqual(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
  });

  $it("Test when no databaseObject assigned: use databaseObjectName instead", []() {
    casmine::SyntheticMySQLModel model;
    model.catalog->users().remove_all();
    model.catalog->roles().remove_all();

    casmine::xRole role("Admin", model);
    casmine::xUser user("monty", model);

    casmine::addPrivilege(model, role, "TABLE", "dummy_obj", "SELECT");

    casmine::assignRole(user, role);

    std::list<std::string> actual;
    gen_grant_sql((db_CatalogRef)model.catalog, actual);
    std::string expect[] = {"GRANT SELECT ON TABLE dummy_obj TO 'monty'"};

    expectContainersEqual(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
  });

  $it("Test parent role", []() {
    casmine::SyntheticMySQLModel model;
    model.catalog->users().remove_all();
    model.catalog->roles().remove_all();

    casmine::xRole role("Admin", model);
    casmine::xRole roleBase("Deleter", model);

    role->parentRole(roleBase);
    roleBase->childRoles().insert(role);

    casmine::xUser user("monty", model);

    casmine::addPrivilege(model, role, model.table, "SELECT");
    casmine::addPrivilege(model, roleBase, model.table, "DELETE");

    // note: only one role (and one privilege) assigned here but should derive one more (see expect[])
    casmine::assignRole(user, role);

    std::list<std::string> actual;
    gen_grant_sql((db_CatalogRef)model.catalog, actual);
    std::string expect[] = {
      "GRANT DELETE ON TABLE `test_schema`.`t1` TO 'monty'", "GRANT SELECT ON TABLE `test_schema`.`t1` TO 'monty'",
    };

    expectContainersEqual(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
  });

}
}
