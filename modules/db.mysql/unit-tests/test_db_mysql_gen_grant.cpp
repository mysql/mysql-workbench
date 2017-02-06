/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "testgrt.h"
#include "grt_test_utility.h"
#include "synthetic_mysql_model.h"
#include "../src/db_mysql_diffsqlgen_grant.h"
#include "wb_helpers.h"

using namespace tut;

BEGIN_TEST_DATA_CLASS(test_db_mysql_gen_grant)
public:
TEST_DATA_CONSTRUCTOR(test_db_mysql_gen_grant) {
}

END_TEST_DATA_CLASS

TEST_MODULE(test_db_mysql_gen_grant, "test_db_mysql_gen_grant");

TEST_FUNCTION(1) {
  grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
  grt::GRT::get()->end_loading_metaclasses();
}

TEST_FUNCTION(10) {
  SynteticMySQLModel model;
  model.catalog->users().remove_all();
  model.catalog->roles().remove_all();

  xRole role("Admin", model);
  xUser user("monty", model);

  add_privilege(model, role, model.table, "SELECT");

  assign_role(user, role);

  std::list<std::string> actual;
  gen_grant_sql((db_CatalogRef)model.catalog, actual);
  std::string expect[] = {"GRANT SELECT ON TABLE `test_schema`.`t1` TO 'monty'"};

  assure_containers_equal(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
}

TEST_FUNCTION(11) {
  SynteticMySQLModel model;
  model.catalog->users().remove_all();
  model.catalog->roles().remove_all();

  xRole adminRole("Admin", model);
  xRole userRole("User", model);

  add_privilege(model, adminRole, model.table, "INSERT");
  add_privilege(model, userRole, model.table, "SELECT");

  xUser user1("monty", model);
  xUser user2("scott", model);

  assign_role(user1, adminRole);
  assign_role(user1, userRole);

  assign_role(user2, userRole);

  std::list<std::string> actual;
  gen_grant_sql((db_CatalogRef)model.catalog, actual);
  std::string expect[] = {
    "GRANT INSERT ON TABLE `test_schema`.`t1` TO 'monty'", "GRANT SELECT ON TABLE `test_schema`.`t1` TO 'monty'",
    "GRANT SELECT ON TABLE `test_schema`.`t1` TO 'scott'",
  };

  assure_containers_equal(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
}

TEST_FUNCTION(12) { // test when no databaseObject assigned: use databaseObjectName instead
  SynteticMySQLModel model;
  model.catalog->users().remove_all();
  model.catalog->roles().remove_all();

  xRole role("Admin", model);
  xUser user("monty", model);

  add_privilege(model, role, "TABLE", "dummy_obj", "SELECT");

  assign_role(user, role);

  std::list<std::string> actual;
  gen_grant_sql((db_CatalogRef)model.catalog, actual);
  std::string expect[] = {"GRANT SELECT ON TABLE dummy_obj TO 'monty'"};

  assure_containers_equal(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
}

TEST_FUNCTION(13) { // test parent role
  SynteticMySQLModel model;
  model.catalog->users().remove_all();
  model.catalog->roles().remove_all();

  xRole role("Admin", model);
  xRole roleBase("Deleter", model);

  role->parentRole(roleBase);
  roleBase->childRoles().insert(role);

  xUser user("monty", model);

  add_privilege(model, role, model.table, "SELECT");
  add_privilege(model, roleBase, model.table, "DELETE");

  // note: only one role (and one privilege) assigned here but should derive one more (see expect[])
  assign_role(user, role);

  std::list<std::string> actual;
  gen_grant_sql((db_CatalogRef)model.catalog, actual);
  std::string expect[] = {
    "GRANT DELETE ON TABLE `test_schema`.`t1` TO 'monty'", "GRANT SELECT ON TABLE `test_schema`.`t1` TO 'monty'",
  };

  assure_containers_equal(actual.begin(), actual.end(), expect, expect + UPPER_BOUND(expect));
}

END_TESTS
