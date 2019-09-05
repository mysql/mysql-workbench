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

#include <stdio.h>
#include "grt.h"

#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"


using namespace grt;
using namespace bec;
using namespace std;

namespace {

$ModuleEnvironment() {};

$TestData {
  bec::NodeIds map;
};

$describe("TreeModel") {
  $it("Test of constructors", []() {
    bec::NodeId n1;
    $expect(n1.is_valid()).toBeFalse();

    bec::NodeId n2(3); // NodeId(const int)
    $expect(n2.is_valid()).toBeTrue();

    bec::NodeId n3("1.2.3"); // NodeId(const std::string)
    $expect(n3.is_valid()).toBeTrue();
    $expect(n3.depth() == 3).toBeTrue();

    bec::NodeId n4("1:2:3"); // NodeId(const std::string)
    $expect(n4.is_valid()).toBeTrue();
    $expect(n4.depth()).toBe(3U);

    bec::NodeId n5(n3);
    $expect(n5.is_valid()).toBeTrue();
    $expect(n5.depth()).toBe(3U);
  });

  $it("Equal node test", []() {
    bec::NodeId n1("1.2.3");
    bec::NodeId n2(n1);

    $expect(n1 == n2).toBeTrue();

    bec::NodeId n3("1:2");
    bec::NodeId n4(n2);
    bec::NodeId n5(n3);
    $expect((n3 == n5)).toBeTrue();
    $expect((n3 == n4)).toBeFalse();

    bec::NodeId n6("1:2:3");
    $expect((n2 == n6)).toBeTrue();
    $expect((n1 == n6)).toBeTrue();
  });

  $it("Exceptions test", []() {
    bool exception_caught = false;
    try {
      bec::NodeId n1("1,2,3");
    } catch (std::runtime_error &) {
      exception_caught = true;
    }
    $expect(exception_caught).toBeTrue();

    exception_caught = false;
    try {
      bec::NodeId n1("aaaa");
    } catch (std::runtime_error &) {
      exception_caught = true;
    }
    $expect(exception_caught).toBeTrue();

    exception_caught = false;
    try {
      bec::NodeId n1("1.2.#.\0");
    } catch (std::runtime_error &) {
      exception_caught = true;
    }
    $expect(exception_caught).toBeTrue();

    bec::NodeId n2("");
    $expect(n2.is_valid()).toBeFalse();

    bec::NodeId n3("..::...");
    $expect(n3.is_valid()).toBeFalse();
  });

  $it("Assign test", []() {
    bec::NodeId n1("1:2:3");
    bec::NodeId n2("4.5.6.7.8.8");

    $expect(n1 == n2).toBeFalse();

    n2 = n1;

    $expect(n1 == n2).toBeTrue();
  });

  $it("Node depth() test", []() {
    bec::NodeId n1("1:2:3");

    $expect(n1.depth()).toBe(3U);

    n1 = n1.parent();
    $expect(n1.depth()).toBe(2U);
    $expect(n1.toString()).toBe("1.2");
  });

  $it("Operator[] test", []() {
    bec::NodeId n1("23.56.78.1.43");
    const std::size_t test[] = {23, 56, 78, 1, 43};

    for (unsigned int i = 0U; i < sizeof(test) / sizeof(*test); i++) {
      char buf[64];
      snprintf(buf, sizeof(buf) / sizeof(*buf), "NodeId::operator[] test%i", i);
      $expect(n1[i] == test[i]).toBeTrue();
    }
  });

  $it("Node back() test", []() {
    bec::NodeId n1("23.56.78.1.43");
    $expect(n1.back()).toBe(43U);
  });

  $it("Node next() test", []() {
    bec::NodeId n1("23.56.78.1.43");
    n1.next();
    $expect(n1.back()).toBe(44U);
  });

  $it("Node append() test", []() {
    bec::NodeId n1("23.56.78.1.43");

    n1.append(1111);
    $expect(n1.back()).toBe(1111U);
  });

  $it("Parent node test", [this]() {
    bec::NodeId node("1:2:3");
    bec::NodeId::uid uid1 = data->map.map_node_id(node);

    bec::NodeId node2("1.2.3.5");
    node2 = node2.parent();
    bec::NodeId::uid uid2 = data->map.map_node_id(node2);

    $expect(uid1 == uid2).toBeTrue();
  });

  $it("Map node id test 1", [this]() {
    bec::NodeId node("1:2:3");
    bec::NodeId::uid uid1 = data->map.map_node_id(node);

    bec::NodeId node2(data->map.map_node_id(uid1));
    $expect(node == node2).toBeTrue();
  });

  $it("Map node id test 2", [this]() {
    bec::NodeId n1("1.1");
    bec::NodeId::uid uid1 = data->map.map_node_id(n1);
    n1.next();
    $expect(n1.back()).toEqual(2U);

    bec::NodeId n2(data->map.map_node_id(uid1));
    for (int i = 0; i < 2; i++) {
      $expect(n2[i]).toEqual(1U);
    }
  });

  $it("Sorting nodes test", []() {
    std::vector<bec::NodeId> test;
    for (std::size_t i = 1; i < 20; i++)
      test.push_back(bec::NodeId(i));
    std::sort(test.begin(), test.end());

    for (std::size_t i = 1, j = 0; i < test.size(); i++, j++)
      $expect(i == test[j][0]).toBeTrue();
  });

}
}
