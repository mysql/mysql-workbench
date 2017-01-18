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

#include <stdio.h>
#include "grt.h"

#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"

#include "wb_helpers.h"

using namespace grt;
using namespace bec;
using namespace std;

BEGIN_TEST_DATA_CLASS(tree_model)
public:
db_mgmt_RdbmsRef rdbms;
bec::NodeIds map;

TEST_DATA_CONSTRUCTOR(tree_model)
//, map(0)
{
}

TEST_DATA_DESTRUCTOR(tree_model) {
}
END_TEST_DATA_CLASS

TEST_MODULE(tree_model, "TreeModel");

TEST_FUNCTION(1) {
  // map = new bec::NodeIds;
}

TEST_FUNCTION(2) {
  // Test of constructors
  bec::NodeId n1;
  ensure("NodeId default ctor", n1.is_valid() == false);

  bec::NodeId n2(3); // NodeId(const int)
  ensure("NodeId(const int) test", n2.is_valid() == true);

  bec::NodeId n3("1.2.3"); // NodeId(const std::string)
  ensure("NodeId(const std::string) test1", n3.is_valid() == true);
  ensure("NodeId(const std::string) test2", n3.depth() == 3);

  bec::NodeId n4("1:2:3"); // NodeId(const std::string)
  ensure("NodeId(const std::string) test1", n4.is_valid() == true);
  ensure("NodeId(const std::string) test2", n4.depth() == 3);

  bec::NodeId n5(n3);
  ensure("NodeId(const NodeId&) test1", n5.is_valid() == true);
  ensure("NodeId(const NodeId&) test2", n5.depth() == 3);
}

TEST_FUNCTION(3) {
  bec::NodeId n1("1.2.3");
  bec::NodeId n2(n1);

  ensure("NodeId::op== test1", n1 == n2);

  bec::NodeId n3("1:2");
  bec::NodeId n4(n2);
  bec::NodeId n5(n3);
  ensure("NodeId::op== test2", (n3 == n5));
  ensure("NodeId::op== test3", !(n3 == n4));

  bec::NodeId n6("1:2:3");
  ensure("NodeId::op== test4", (n2 == n6));
  ensure("NodeId::op== test5", (n1 == n6));
}

TEST_FUNCTION(4) {
  bool exception_caught = false;
  try {
    bec::NodeId n1("1,2,3");
  } catch (std::runtime_error) {
    exception_caught = true;
  }
  ensure("Wrong args to ctors, test1", exception_caught);

  exception_caught = false;
  try {
    bec::NodeId n1("aaaa");
  } catch (std::runtime_error) {
    exception_caught = true;
  }
  ensure("Wrong args to ctors, test2", exception_caught);

  exception_caught = false;
  try {
    bec::NodeId n1("1.2.#.\0");
  } catch (std::runtime_error) {
    exception_caught = true;
  }
  ensure("Wrong args to ctors, test3", exception_caught);

  bec::NodeId n2("");
  ensure("Wrong args to ctors, test4", n2.is_valid() == false);

  bec::NodeId n3("..::...");
  ensure("Wrong args to ctors, test4", n3.is_valid() == false);
}

TEST_FUNCTION(5) {
  bec::NodeId n1("1:2:3");
  bec::NodeId n2("4.5.6.7.8.8");

  ensure("Assignment op test1", !(n1 == n2));

  n2 = n1;

  ensure("Assignment op test2", n1 == n2);
}

TEST_FUNCTION(6) {
  bec::NodeId n1("1:2:3");

  ensure("NodeId::depth()", n1.depth() == 3);

  n1 = n1.parent();
  ensure("NodeId::parent() test1", n1.depth() == 2);
  ensure("NodeId::parent() test2", n1.toString() == "1.2");
}

TEST_FUNCTION(7) {
  bec::NodeId n1("23.56.78.1.43");
  const std::size_t test[] = {23, 56, 78, 1, 43};

  for (unsigned int i = 0U; i < sizeof(test) / sizeof(*test); i++) {
    char buf[64];
    snprintf(buf, sizeof(buf) / sizeof(*buf), "NodeId::operator[] test%i", i);
    ensure(buf, n1[i] == test[i]);
  }
}

TEST_FUNCTION(8) {
  bec::NodeId n1("23.56.78.1.43");
  ensure("NodeId::back() test", n1.back() == 43);
}

TEST_FUNCTION(9) {
  bec::NodeId n1("23.56.78.1.43");
  n1.next();
  ensure("NodeId::back() test", n1.back() == 44);
}

TEST_FUNCTION(10) {
  bec::NodeId n1("23.56.78.1.43");

  n1.append(1111);
  ensure("NodeId::back() test", n1.back() == 1111);
}

//==============================================================================
// Tests of NodeIds (mapping)
//==============================================================================
TEST_FUNCTION(11) {
  bec::NodeId node("1:2:3");
  bec::NodeId::uid uid1 = map.map_node_id(node);

  bec::NodeId node2("1.2.3.5");
  node2 = node2.parent();
  bec::NodeId::uid uid2 = map.map_node_id(node2);

  ensure("NodeIds test1", uid1 == uid2);
}

TEST_FUNCTION(12) {
  bec::NodeId node("1:2:3");
  bec::NodeId::uid uid1 = map.map_node_id(node);

  bec::NodeId node2(map.map_node_id(uid1));
  ensure("NodeIds uid to NodeId map", node == node2);
}

TEST_FUNCTION(13) {
  bec::NodeId n1("1.1");
  bec::NodeId::uid uid1 = map.map_node_id(n1);
  n1.next();
  ensure("NodeId::next", n1.back() == 2);

  bec::NodeId n2(map.map_node_id(uid1));
  for (int i = 0; i < 2; i++) {
    ensure("NodeId::next", n2[i] == 1);
  }
}

TEST_FUNCTION(14) {
  std::vector<bec::NodeId> test;
  for (std::size_t i = 1; i < 20; i++)
    test.push_back(bec::NodeId(i));
  std::sort(test.begin(), test.end());

  for (std::size_t i = 1, j = 0; i < test.size(); i++, j++)
    ensure("NodeId: equals", i == test[j][0]);
}

END_TESTS
