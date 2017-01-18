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

#include "grt/grt_value_inspector.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;

#include "grt_test_utility.h"

#include "grt_values_test_data.h"
#include "structs.test.h"

BEGIN_TEST_DATA_CLASS(be_inspector_value)
public:
END_TEST_DATA_CLASS;

TEST_MODULE(be_inspector_value, "grt value inspector (ValueInspectorBE) backend");

// Test Lists
// ----------

TEST_FUNCTION(1) {
  bool flag;
  // test inspection of list

  // create a test list
  BaseListRef list(create_list_with_varied_data());

  ensure_equals("initial list size", (int)list.count(), 10);
  ensure_equals("list type", list.type(), ListType);

  ValueInspectorBE *vinsp = ValueInspectorBE::create(list, false, false);

  // test listing
  size_t c = vinsp->count();
  ensure_equals("begin listing", c, 10U);

  std::string name, value;
  Type type;
  NodeId node;

  g_mkdir_with_parents("output", 0700);

  std::vector<ssize_t> columns;
  columns.push_back(ValueInspectorBE::Name);
  columns.push_back(ValueInspectorBE::Value);
  dump_tree_model("output/grt_inspector_value_test1.txt", vinsp, columns, true);
  ensure_files_equal("list contents", "output/grt_inspector_value_test1.txt", "data/be/grt_inspector_value_test1.txt");

  try {
    node = vinsp->get_node(10);
    flag = vinsp->get_field(node, ValueInspectorBE::Name, name);
    ensure("last+1 item", !flag);
  } catch (...) {
  }

  // test get random item

  node = vinsp->get_child(vinsp->get_root(), 6);

  flag = vinsp->get_field(node, ValueInspectorBE::Name, name);
  ensure("get", flag);
  flag = vinsp->get_field(node, ValueInspectorBE::Value, value);
  ensure("get", flag);
  type = vinsp->get_field_type(node, ValueInspectorBE::Value);
  ensure("get", flag);
  ensure_equals("7) value", name, "[7]");
  ensure_equals("7) value", value, "{item1 = 1, item2 = 2.200000, item3 = test}");
  ensure_equals("7) type", type, DictType);

  NodeId nd;
  ensure("bad get", !nd.is_valid());

  node = vinsp->get_node(-1);
  ensure("bad get", !node.is_valid());

  node = vinsp->get_node(11);
  ensure("bad get", !node.is_valid());

  // test change int value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
  ensure("set 1", flag);

  flag = vinsp->get_field(0, ValueInspectorBE::Value, value);
  ensure("get/set 1", flag);
  ensure_equals("get/set 1", value, "112211");
  type = vinsp->get_field_type(0, ValueInspectorBE::Value);
  ensure_equals("get/set 1", type, IntegerType);

  flag = vinsp->set_field(9, ValueInspectorBE::Value, (ssize_t)8888);
  ensure("set 9", flag);

  // test change string value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
  ensure("set 1", flag);

  flag = vinsp->set_field(9, ValueInspectorBE::Value, "world");
  ensure("set 10", flag);

  // test change double value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, 1.234);
  ensure("set 1", flag);

  flag = vinsp->set_field(8, ValueInspectorBE::Value, 5.1);
  ensure("set 9", flag);

  // test change int value from string
  flag = vinsp->set_convert_field(1, ValueInspectorBE::Value, "112233");
  ensure("set 2", flag);

  dump_tree_model("output/grt_inspector_value_test1.1.txt", vinsp, columns, true);
  ensure_files_equal("list change", "output/grt_inspector_value_test1.1.txt",
                     "data/be/grt_inspector_value_test1.1.txt");

  // item count still ok?

  ensure_equals("item count after changes", (int)list.count(), 10);

  // test add new value
  NodeId nkey;
  flag = vinsp->add_item(nkey);
  ensure("add ok", flag);
  ensure_equals("add", nkey[0], 10U);
  flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "new value");
  ensure("set item", flag);

  ensure_equals("new count", list.count(), 11U);

  // test delete value
  flag = vinsp->delete_item(3);
  ensure("del", flag);

  ensure_equals("count", list.count(), 10U);

  dump_tree_model("output/grt_inspector_value_test1.2.txt", vinsp, columns, true);
  ensure_files_equal("list delete", "output/grt_inspector_value_test1.2.txt",
                     "data/be/grt_inspector_value_test1.2.txt");

  flag = vinsp->delete_item(10);
  ensure("bad del", !flag);

  delete vinsp;
}

TEST_FUNCTION(2) {
  bool flag;
  std::vector<ssize_t> columns;
  columns.push_back(ValueInspectorBE::Name);
  columns.push_back(ValueInspectorBE::Value);

  // test inspection of string typed list

  // create a test list
  BaseListRef list(create_string_list(10));

  ensure_equals("initial list size", list.count(), 10U);
  ensure_equals("list type", list.type(), ListType);
  ensure_equals("list content type", list.content_type(), StringType);

  ValueInspectorBE *vinsp = ValueInspectorBE::create(list, false, false);

  // test listing
  size_t c = vinsp->count();
  ensure_equals("begin listing", c, 10U);

  dump_tree_model("output/grt_inspector_value_test2.txt", vinsp, columns, true);
  ensure_files_equal("typed list check", "output/grt_inspector_value_test2.txt",
                     "data/be/grt_inspector_value_test2.txt");

  std::string name, value;

  NodeId node = vinsp->get_node(-1);
  ensure("bad get", !node.is_valid());

  flag = vinsp->get_field(11, ValueInspectorBE::Name, name);
  ensure("bad get", !flag);

  flag = vinsp->get_field(11, ValueInspectorBE::Value, value);
  ensure("bad get", !flag);

  // test change int value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
  ensure("set 1", !flag);

  // test change string value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
  ensure("set 1", flag);

  // test change double value
  flag = vinsp->set_field(1, ValueInspectorBE::Value, 1.234);
  ensure("set 2", !flag);

  // test change int value from string
  flag = vinsp->set_convert_field(2, ValueInspectorBE::Value, "112233");
  ensure("set 3", flag);

  dump_tree_model("output/grt_inspector_value_test2.1.txt", vinsp, columns, true);
  ensure_files_equal("typed list setting", "output/grt_inspector_value_test2.1.txt",
                     "data/be/grt_inspector_value_test2.1.txt");

  // item count still ok?
  delete vinsp;

  ensure_equals("item count after changes", (int)list.count(), 10);
}

TEST_FUNCTION(3) {
  bool flag;
  std::vector<ssize_t> columns;
  columns.push_back(ValueInspectorBE::Name);
  columns.push_back(ValueInspectorBE::Value);

  // test inspection of int typed list

  // create a test list
  BaseListRef list(create_int_list(10));

  ensure_equals("initial list size", list.count(), 10U);
  ensure_equals("list type", list.type(), ListType);
  ensure_equals("list content type", list.content_type(), IntegerType);

  ValueInspectorBE *vinsp = ValueInspectorBE::create(list, false, false);

  // test listing
  size_t c = vinsp->count();
  ensure_equals("begin listing", c, 10U);

  std::string name, value;

  dump_tree_model("output/grt_inspector_value_test3.txt", vinsp, columns, true);
  ensure_files_equal("int typed list", "output/grt_inspector_value_test3.txt", "data/be/grt_inspector_value_test3.txt");

  NodeId node;
  node = vinsp->get_node(-1);
  ensure("bad get", !node.is_valid());

  node = vinsp->get_node(10);
  ensure("bad get", !node.is_valid());

  node = vinsp->get_node(11);
  ensure("bad get", !node.is_valid());

  // node= vinsp->get_child(4, 0);
  // ensure("bad get", !node.is_valid());

  // node= vinsp->get_child(1, 1);
  // ensure("bad get", !node.is_valid());

  flag = vinsp->set_field(0, ValueInspectorBE::Name, (ssize_t)123);
  ensure("bad set", !flag);

  // test change int value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
  ensure("set 1", flag);

  // test change string value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
  ensure("set 1", !flag);

  // test change double value
  flag = vinsp->set_field(1, ValueInspectorBE::Value, 1.234);
  ensure("set 2", !flag);

  // test change int value from string
  flag = vinsp->set_convert_field(2, ValueInspectorBE::Value, "112233");
  ensure("set 3", flag);

  dump_tree_model("output/grt_inspector_value_test3.1.txt", vinsp, columns, true);
  ensure_files_equal("int typed list set", "output/grt_inspector_value_test3.1.txt",
                     "data/be/grt_inspector_value_test3.1.txt");

  delete vinsp;

  ensure_equals("item count after changes", (int)list.count(), 10);
}

// Test Dicts
// ----------

TEST_FUNCTION(10) {
  bool flag;
  std::vector<ssize_t> columns;
  columns.push_back(ValueInspectorBE::Name);
  columns.push_back(ValueInspectorBE::Value);

  // test inspection of dict

  // create a test dict
  DictRef dict(create_dict_with_varied_data());

  ensure_equals("initial dict size", dict.count(), 6U);
  ensure_equals("dict type", dict.type(), DictType);

  ValueInspectorBE *vinsp = ValueInspectorBE::create(dict, false, false);

  // test listing
  size_t c = vinsp->count();
  ensure_equals("begin listing", c, 6U);

  std::string name, value;

  dump_tree_model("output/grt_inspector_value_test10.txt", vinsp, columns, true);
  ensure_files_equal("dict check", "output/grt_inspector_value_test10.txt", "data/be/grt_inspector_value_test10.txt");

  NodeId node;
  node = vinsp->get_node(-1);
  ensure("bad get", !node.is_valid());

  //  node= vinsp->get_child(1, 0);
  //  ensure("bad get", !node.is_valid());

  // test change int value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
  ensure("set 1", flag);

  flag = vinsp->set_field(4, ValueInspectorBE::Value, (ssize_t)8888);
  ensure("set k5", flag);

  // test change string value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
  ensure("set k1", flag);

  // test change double value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, 1.234);
  ensure("set k1", flag);

  // test change int value from string
  flag = vinsp->set_convert_field(1, ValueInspectorBE::Value, "112233");
  ensure("set k2", flag);

  dump_tree_model("output/grt_inspector_value_test10.1.txt", vinsp, columns, true);
  ensure_files_equal("dict check set", "output/grt_inspector_value_test10.1.txt",
                     "data/be/grt_inspector_value_test10.1.txt");

  // item count still ok?

  ensure_equals("item count after changes", dict.count(), 6U);

  // test add new value
  NodeId nkey;
  flag = vinsp->add_item(nkey);
  ensure("add ok", flag);
  ensure_equals("add", nkey[0], 6U);

  flag = vinsp->set_field(nkey, ValueInspectorBE::Name, "newk");
  ensure("set item name", flag);
  flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "new value");
  ensure("set item", flag);

  ensure_equals("new count", dict.count(), 7U);

  dump_tree_model("output/grt_inspector_value_test10.2.txt", vinsp, columns, true);
  ensure_files_equal("dict check add", "output/grt_inspector_value_test10.2.txt",
                     "data/be/grt_inspector_value_test10.2.txt");
  // test delete value
  flag = vinsp->delete_item(3);
  ensure("del", flag);

  ensure_equals("count", dict.count(), 6U);

  // ensure_equals("get/del value", value, "[ 1, 2, 3, 4, 5 ]");

  flag = vinsp->delete_item(8);
  ensure("bad del", !flag);

  dump_tree_model("output/grt_inspector_value_test10.3.txt", vinsp, columns, true);
  ensure_files_equal("dict check del", "output/grt_inspector_value_test10.3.txt",
                     "data/be/grt_inspector_value_test10.3.txt");

  delete vinsp;

  ensure_equals("final count", dict.count(), 6U);
}

TEST_FUNCTION(11) {
  bool flag;
  std::vector<ssize_t> columns;
  columns.push_back(ValueInspectorBE::Name);
  columns.push_back(ValueInspectorBE::Value);

  // test inspection of typed dict

  // create a test dict
  DictRef dict(create_dict_with_int_data());

  ensure_equals("initial dict size", dict.count(), 9U);
  ensure_equals("dict type", dict.type(), DictType);
  ensure_equals("dict content type", dict.content_type(), IntegerType);

  ValueInspectorBE *vinsp = ValueInspectorBE::create(dict, false, false);

  dump_tree_model("output/grt_inspector_value_test11.txt", vinsp, columns, true);
  ensure_files_equal("object check", "output/grt_inspector_value_test11.txt", "data/be/grt_inspector_value_test11.txt");

  // test listing
  size_t c = vinsp->count();
  ensure_equals("begin listing", c, 9U);

  std::string name, value;
  NodeId node;

  // test get random item
  node = vinsp->get_node(9);
  ensure("bad get", !node.is_valid());

  node = vinsp->get_node(-1);
  ensure("bad get", !node.is_valid());

  // test change int value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
  ensure("set 1", flag);

  flag = vinsp->set_field(4, ValueInspectorBE::Value, (ssize_t)8888);
  ensure("set k5", flag);

  // test change string value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
  ensure("set k1", !flag);

  // test change double value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, 1.234);
  ensure("set k1", !flag);

  // test change int value from string
  flag = vinsp->set_convert_field(1, ValueInspectorBE::Value, "112233");
  ensure("set k2", flag);
  // in this case, the dict is untyped, so it should just appear as string

  dump_tree_model("output/grt_inspector_value_test11.1.txt", vinsp, columns, true);
  ensure_files_equal("object check set", "output/grt_inspector_value_test11.1.txt",
                     "data/be/grt_inspector_value_test11.1.txt");

  // item count still ok?
  ensure_equals("item count after changes", (int)dict.count(), 9);

  // test add new value
  NodeId nkey;
  flag = vinsp->add_item(nkey);
  ensure("add ok", flag);
  ensure_equals("add", nkey[0], 9U);

  flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "new value");
  ensure("set item", !flag);

  flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "1234");
  ensure("set item", !flag);

  flag = vinsp->set_field(nkey, ValueInspectorBE::Name, "anewk");
  ensure("set item", flag);

  flag = vinsp->set_field(nkey, ValueInspectorBE::Value, (ssize_t)1234);
  ensure("set item", flag);

  ensure_equals("new count", (int)dict.count(), 10);

  dump_tree_model("output/grt_inspector_value_test11.2.txt", vinsp, columns, true);
  ensure_files_equal("object check add", "output/grt_inspector_value_test11.2.txt",
                     "data/be/grt_inspector_value_test11.2.txt");

  flag = vinsp->add_item(nkey);
  ensure("add", flag);

  flag = vinsp->add_item(nkey);
  ensure("add ng", !flag);

  vinsp->refresh();

  flag = vinsp->add_item(nkey);
  ensure("add", flag);

  flag = vinsp->add_item(nkey);
  ensure("add again", !flag);

  flag = vinsp->set_convert_field(nkey, ValueInspectorBE::Name, "aaa");
  flag = vinsp->set_convert_field(nkey, ValueInspectorBE::Value, "22");
  ensure("set item", flag);

  dump_tree_model("output/grt_inspector_value_test11.3.txt", vinsp, columns, true);
  ensure_files_equal("object check add", "output/grt_inspector_value_test11.3.txt",
                     "data/be/grt_inspector_value_test11.3.txt");

  // test delete value
  flag = vinsp->delete_item(3);
  ensure("del", flag);

  ensure_equals("count", dict.count(), 10U);

  flag = vinsp->delete_item(10);
  ensure("bad del", !flag);

  flag = vinsp->delete_item(9);
  ensure("del last", flag);

  dump_tree_model("output/grt_inspector_value_test11.4.txt", vinsp, columns, true);
  ensure_files_equal("object check del", "output/grt_inspector_value_test11.4.txt",
                     "data/be/grt_inspector_value_test11.4.txt");

  delete vinsp;

  ensure_equals("final count", dict.count(), 9U);
}

// Test Objects
// ------------

TEST_FUNCTION(20) {
  std::vector<ssize_t> columns;
  columns.push_back(ValueInspectorBE::Name);
  columns.push_back(ValueInspectorBE::Value);

  // test inspection of object (ungrouped)

  // Load test meta classes.
  grt::GRT::get()->load_metaclasses("data/structs.test.xml");

  // Consolidate the loaded classes.
  grt::GRT::get()->end_loading_metaclasses();

  test_BookRef book(grt::Initialized);
  ensure("object valid", book.is_valid());

  ValueInspectorBE *vinsp = ValueInspectorBE::create(book, false, false);
  bool flag;

  // test listing
  size_t c = vinsp->count();
  ensure_equals("begin listing", c, 6U);

  std::string name, value;

  NodeId node;
  dump_tree_model("output/grt_inspector_value_test20.txt", vinsp, columns, true);
  ensure_files_equal("object check", "output/grt_inspector_value_test20.txt", "data/be/grt_inspector_value_test20.txt");

  node = vinsp->get_node(9);
  ensure("bad get", !node.is_valid());

  node = vinsp->get_node(-1);
  ensure("bad get", !node.is_valid());

  // test bad change int value
  flag = vinsp->set_field(5, ValueInspectorBE::Value, (ssize_t)112211);
  ensure("set invalid 1", !flag);

  flag = vinsp->set_field(5, ValueInspectorBE::Value, "The Illiad");
  ensure("set title", flag);

  flag = vinsp->get_field(5, ValueInspectorBE::Value, value);
  ensure("get/set title", flag);
  ensure_equals("get/set title", value, "The Illiad");

  // test change int value
  flag = vinsp->set_field(2, ValueInspectorBE::Value, (ssize_t)123);
  ensure("set pages", flag);

  // test change double value
  flag = vinsp->set_field(3, ValueInspectorBE::Value, 1.234);
  ensure("set price", flag);

  // test change int value from string
  flag = vinsp->set_convert_field(2, ValueInspectorBE::Value, "112233");
  ensure("set convert pages", flag);
  // in this case, the dict is untyped, so it should just appear as string

  dump_tree_model("output/grt_inspector_value_test20.1.txt", vinsp, columns, true);
  ensure_files_equal("object check set", "output/grt_inspector_value_test20.1.txt",
                     "data/be/grt_inspector_value_test20.1.txt");

  // test add new value
  NodeId nkey;
  flag = vinsp->add_item(nkey);
  ensure("add ok", !flag);

  dump_tree_model("output/grt_inspector_value_test20.2.txt", vinsp, columns, true);
  ensure_files_equal("object check add", "output/grt_inspector_value_test20.2.txt",
                     "data/be/grt_inspector_value_test20.2.txt");

  // test delete value
  flag = vinsp->delete_item(3);
  ensure("del", !flag);

  dump_tree_model("output/grt_inspector_value_test20.3.txt", vinsp, columns, true);
  ensure_files_equal("object check del", "output/grt_inspector_value_test20.3.txt",
                     "data/be/grt_inspector_value_test20.3.txt");

  delete vinsp;
}

TEST_FUNCTION(21) {
  std::vector<ssize_t> columns;
  columns.push_back(ValueInspectorBE::Name);
  columns.push_back(ValueInspectorBE::Value);

  // test inspection of object (grouped)

  test_BookRef book(grt::Initialized);
  ensure("object valid", book.is_valid());

  ValueInspectorBE *vinsp = ValueInspectorBE::create(book, true, true);

  // test listing
  size_t c = vinsp->count();
  ensure_equals("top listing", c, 3U);

  std::string name, value;
  bool flag;

  NodeId node, gnode;

  dump_tree_model("output/grt_inspector_value_test21.txt", vinsp, columns, true);
  ensure_files_equal("grouped object check", "output/grt_inspector_value_test21.txt",
                     "data/be/grt_inspector_value_test21.txt");

  try {
    node = vinsp->get_child(NodeId(1), 3);
    ensure("Invalid item", !node.is_valid());
  } catch (std::range_error) {
    // expected
  }

  // test bad change int value
  flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
  ensure("set invalid 1", !flag);

  node = vinsp->get_child(0, 0);

  flag = vinsp->set_field(node, ValueInspectorBE::Value, "The Illiad");
  ensure("set title", flag);

  // test change int value
  node = vinsp->get_child(1, 1);
  flag = vinsp->set_field(node, ValueInspectorBE::Value, (ssize_t)123);
  ensure("set pages", flag);

  // test change double value
  node = vinsp->get_child(1, 2);
  flag = vinsp->set_field(node, ValueInspectorBE::Value, 1.234);
  ensure("set price", flag);

  // test change int value from string
  node = vinsp->get_child(1, 1);
  flag = vinsp->set_convert_field(node, ValueInspectorBE::Value, "112233");
  ensure("set convert pages", flag);

  // test change group name
  node = vinsp->get_node(1);
  flag = vinsp->set_convert_field(node, ValueInspectorBE::Value, "HELLO");
  ensure("set group name", !flag);

  dump_tree_model("output/grt_inspector_value_test21.1.txt", vinsp, columns, true);
  ensure_files_equal("grouped object check set", "output/grt_inspector_value_test21.1.txt",
                     "data/be/grt_inspector_value_test21.1.txt");

  // test add new value
  NodeId nkey;
  flag = vinsp->add_item(nkey);
  ensure("add ok", !flag);

  dump_tree_model("output/grt_inspector_value_test21.2.txt", vinsp, columns, true);
  ensure_files_equal("grouped object check add", "output/grt_inspector_value_test21.2.txt",
                     "data/be/grt_inspector_value_test21.2.txt");

  // test delete value
  flag = vinsp->delete_item(3);
  ensure("del", !flag);

  dump_tree_model("output/grt_inspector_value_test21.3.txt", vinsp, columns, true);
  ensure_files_equal("grouped object check del", "output/grt_inspector_value_test21.3.txt",
                     "data/be/grt_inspector_value_test21.3.txt");

  delete vinsp;
}

END_TESTS
