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

#include "grt/grt_value_inspector.h"
#include "wb_test_helpers.h"

using namespace grt;
using namespace bec;

#include "wb_test_helpers.h"
#include "grt_test_helpers.h"

#include "grt_values_test_data.h"
#include "structs.test.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

$TestData {
  std::string dataDir = casmine::CasmineContext::get()->tmpDataDir();
  std::string outputDir = casmine::CasmineContext::get()->outputDir();
};

static void expectFilesEqual(const std::string &test, const std::string file, const std::string reffile) {
  std::string line, refline;
  std::ifstream ref(reffile);
  std::ifstream f(file);

  $expect(ref.is_open()).toBeTrue();
  $expect(f.is_open()).toBeTrue();

  while (!ref.eof() && !f.eof()) {
    getline(ref, refline);
    getline(f, line);

    $expect(line).toBe(refline);
  }

  $expect(f.eof() && ref.eof()).toBeTrue();
}

$describe("GRT Inspector Value") {
  $beforeAll([&]() {
    grt::GRT::get()->load_metaclasses(casmine::CasmineContext::get()->tmpDataDir() + "/structs.test.xml");
    grt::GRT::get()->end_loading_metaclasses();
  });

  $afterAll([&]() {
    WorkbenchTester::reinitGRT();
  });

  $it("Test inspection of list", [this]() {
    bool flag;
    // create a test list
    BaseListRef list(create_list_with_varied_data());

    $expect((int)list.count()).toEqual(10);
    $expect(list.type()).toBe(ListType);

    ValueInspectorBE *vinsp = ValueInspectorBE::create(list, false, false);

    // test listing
    size_t c = vinsp->count();
    $expect(c).toBe(10U);

    std::string name, value;
    Type type;
    NodeId node;

    g_mkdir_with_parents("output", 0700);

    std::vector<ssize_t> columns;
    columns.push_back(ValueInspectorBE::Name);
    columns.push_back(ValueInspectorBE::Value);
    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test1.txt", vinsp, columns, true);
    expectFilesEqual("list contents", data->outputDir + "/grt_inspector_value_test1.txt", data->dataDir + "/be/grt_inspector_value_test1.txt");

    try {
      node = vinsp->get_node(10);
      flag = vinsp->get_field(node, ValueInspectorBE::Name, name);
      $expect(flag).toBeFalse();
    } catch (...) {
    }

    // test get random item

    node = vinsp->get_child(vinsp->get_root(), 6);

    flag = vinsp->get_field(node, ValueInspectorBE::Name, name);
    $expect(flag).toBeTrue();
    flag = vinsp->get_field(node, ValueInspectorBE::Value, value);
    $expect(flag).toBeTrue();
    type = vinsp->get_field_type(node, ValueInspectorBE::Value);
    $expect(flag).toBeTrue();
    $expect(name).toBe("[7]");
    $expect(value).toBe("{item1 = 1, item2 = 2.200000, item3 = test}");
    $expect(type).toBe(DictType);

    NodeId nd;
    $expect(nd.is_valid()).toBeFalse();

    node = vinsp->get_node(-1);
    $expect(node.is_valid()).toBeFalse();

    node = vinsp->get_node(11);
    $expect(node.is_valid()).toBeFalse();

    // test change int value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
    $expect(flag).toBeTrue();

    flag = vinsp->get_field(0, ValueInspectorBE::Value, value);
    $expect(flag).toBeTrue();
    $expect(value).toBe("112211");
    type = vinsp->get_field_type(0, ValueInspectorBE::Value);
    $expect(type).toBe(IntegerType);

    flag = vinsp->set_field(9, ValueInspectorBE::Value, (ssize_t)8888);
    $expect(flag).toBeTrue();

    // test change string value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
    $expect(flag).toBeTrue();

    flag = vinsp->set_field(9, ValueInspectorBE::Value, "world");
    $expect(flag).toBeTrue();

    // test change double value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, 1.234);
    $expect(flag).toBeTrue();

    flag = vinsp->set_field(8, ValueInspectorBE::Value, 5.1);
    $expect(flag).toBeTrue();

    // test change int value from string
    flag = vinsp->set_convert_field(1, ValueInspectorBE::Value, "112233");
    $expect(flag).toBeTrue();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test1.1.txt", vinsp, columns, true);
    expectFilesEqual("list change", data->outputDir + "/grt_inspector_value_test1.1.txt",
                      data->dataDir + "/be/grt_inspector_value_test1.1.txt");

    // item count still ok?

    $expect((int)list.count()).toEqual(10);

    // test add new value
    NodeId nkey;
    flag = vinsp->add_item(nkey);
    $expect(flag).toBeTrue();
    $expect(nkey[0]).toBe(10U);
    flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "new value");
    $expect(flag).toBeTrue();

    $expect(list.count()).toBe(11U);

    // test delete value
    flag = vinsp->delete_item(3);
    $expect(flag).toBeTrue();

    $expect(list.count()).toBe(10U);

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test1.2.txt", vinsp, columns, true);
    expectFilesEqual("list delete", data->outputDir + "/grt_inspector_value_test1.2.txt",
                      data->dataDir + "/be/grt_inspector_value_test1.2.txt");

    flag = vinsp->delete_item(10);
    $expect(flag).toBeFalse();

    delete vinsp;
  });

  $it("Test inspection of string typed list", [this]() {
    bool flag;
    std::vector<ssize_t> columns;
    columns.push_back(ValueInspectorBE::Name);
    columns.push_back(ValueInspectorBE::Value);

    // create a test list
    BaseListRef list(create_string_list(10));

    $expect(list.count()).toBe(10U);
    $expect(list.type()).toBe(ListType);
    $expect(list.content_type()).toBe(StringType);

    ValueInspectorBE *vinsp = ValueInspectorBE::create(list, false, false);

    // test listing
    size_t c = vinsp->count();
    $expect(c).toBe(10U);

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test2.txt", vinsp, columns, true);
    expectFilesEqual("typed list check", data->outputDir + "/grt_inspector_value_test2.txt",
                      data->dataDir + "/be/grt_inspector_value_test2.txt");

    std::string name, value;

    NodeId node = vinsp->get_node(-1);
    $expect(node.is_valid()).toBeFalse();

    flag = vinsp->get_field(11, ValueInspectorBE::Name, name);
    $expect(flag).toBeFalse();

    flag = vinsp->get_field(11, ValueInspectorBE::Value, value);
    $expect(flag).toBeFalse();

    // test change int value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
    $expect(flag).toBeFalse();

    // test change string value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
    $expect(flag).toBeTrue();

    // test change double value
    flag = vinsp->set_field(1, ValueInspectorBE::Value, 1.234);
    $expect(flag).toBeFalse();

    // test change int value from string
    flag = vinsp->set_convert_field(2, ValueInspectorBE::Value, "112233");
    $expect(flag).toBeTrue();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test2.1.txt", vinsp, columns, true);
    expectFilesEqual("typed list setting", data->outputDir + "/grt_inspector_value_test2.1.txt",
                      data->dataDir + "/be/grt_inspector_value_test2.1.txt");

    // item count still ok?
    delete vinsp;

    $expect(list.count()).toEqual(10U);
  });

  $it("Test inspection of int typed list", [this]() {
    bool flag;
    std::vector<ssize_t> columns;
    columns.push_back(ValueInspectorBE::Name);
    columns.push_back(ValueInspectorBE::Value);

    // create a test list
    BaseListRef list(create_int_list(10));

    $expect(list.count()).toBe(10U);
    $expect(list.type()).toBe(ListType);
    $expect(list.content_type()).toBe(IntegerType);

    ValueInspectorBE *vinsp = ValueInspectorBE::create(list, false, false);

    // test listing
    size_t c = vinsp->count();
    $expect(c).toBe(10U);

    std::string name, value;

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test3.txt", vinsp, columns, true);
    expectFilesEqual("int typed list", data->outputDir + "/grt_inspector_value_test3.txt", data->dataDir + "/be/grt_inspector_value_test3.txt");

    NodeId node;
    node = vinsp->get_node(-1);
    $expect(node.is_valid()).toBeFalse();

    node = vinsp->get_node(10);
    $expect(node.is_valid()).toBeFalse();

    node = vinsp->get_node(11);
    $expect(node.is_valid()).toBeFalse();

    // node= vinsp->get_child(4, 0);
    // $expect(node.is_valid()).toBeFalse();

    // node= vinsp->get_child(1, 1);
    // $expect(node.is_valid()).toBeFalse();

    flag = vinsp->set_field(0, ValueInspectorBE::Name, (ssize_t)123);
    $expect(flag).toBeFalse();

    // test change int value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
    $expect(flag).toBeTrue();

    // test change string value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
    $expect(flag).toBeFalse();

    // test change double value
    flag = vinsp->set_field(1, ValueInspectorBE::Value, 1.234);
    $expect(flag).toBeFalse();

    // test change int value from string
    flag = vinsp->set_convert_field(2, ValueInspectorBE::Value, "112233");
    $expect(flag).toBeTrue();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test3.1.txt", vinsp, columns, true);
    expectFilesEqual("int typed list set", data->outputDir + "/grt_inspector_value_test3.1.txt",
                      data->dataDir + "/be/grt_inspector_value_test3.1.txt");

    delete vinsp;

    $expect(list.count()).toEqual(10U);
  });

  // Test Dicts
  // ----------

  $it("Test inspection of dict", [this]() {
    bool flag;
    std::vector<ssize_t> columns;
    columns.push_back(ValueInspectorBE::Name);
    columns.push_back(ValueInspectorBE::Value);

    // create a test dict
    DictRef dict(create_dict_with_varied_data());

    $expect(dict.count()).toBe(6U);
    $expect(dict.type()).toBe(DictType);

    ValueInspectorBE *vinsp = ValueInspectorBE::create(dict, false, false);

    // test listing
    size_t c = vinsp->count();
    $expect(c).toBe(6U);

    std::string name, value;

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test10.txt", vinsp, columns, true);
    expectFilesEqual("dict check", data->outputDir + "/grt_inspector_value_test10.txt", data->dataDir + "/be/grt_inspector_value_test10.txt");

    NodeId node;
    node = vinsp->get_node(-1);
    $expect(node.is_valid()).toBeFalse();

    //  node= vinsp->get_child(1, 0);
    //  $expect(node.is_valid()).toBeFalse();

    // test change int value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
    $expect(flag).toBeTrue();

    flag = vinsp->set_field(4, ValueInspectorBE::Value, (ssize_t)8888);
    $expect(flag).toBeTrue();

    // test change string value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
    $expect(flag).toBeTrue();

    // test change double value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, 1.234);
    $expect(flag).toBeTrue();

    // test change int value from string
    flag = vinsp->set_convert_field(1, ValueInspectorBE::Value, "112233");
    $expect(flag).toBeTrue();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test10.1.txt", vinsp, columns, true);
    expectFilesEqual("dict check set", data->outputDir + "/grt_inspector_value_test10.1.txt",
                      data->dataDir + "/be/grt_inspector_value_test10.1.txt");

    // item count still ok?

    $expect(dict.count()).toBe(6U);

    // test add new value
    NodeId nkey;
    flag = vinsp->add_item(nkey);
    $expect(flag).toBeTrue();
    $expect(nkey[0]).toBe(6U);

    flag = vinsp->set_field(nkey, ValueInspectorBE::Name, "newk");
    $expect(flag).toBeTrue();
    flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "new value");
    $expect(flag).toBeTrue();

    $expect(dict.count()).toBe(7U);

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test10.2.txt", vinsp, columns, true);
    expectFilesEqual("dict check add", data->outputDir + "/grt_inspector_value_test10.2.txt",
                      data->dataDir + "/be/grt_inspector_value_test10.2.txt");
    // test delete value
    flag = vinsp->delete_item(3);
    $expect(flag).toBeTrue();

    $expect(dict.count()).toBe(6U);

    flag = vinsp->delete_item(8);
    $expect(flag).toBeFalse();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test10.3.txt", vinsp, columns, true);
    expectFilesEqual("dict check del", data->outputDir + "/grt_inspector_value_test10.3.txt",
                      data->dataDir + "/be/grt_inspector_value_test10.3.txt");

    delete vinsp;

    $expect(dict.count()).toBe(6U);
  });

  $it("Test inspection of typed dict", [this]() {
    bool flag;
    std::vector<ssize_t> columns;
    columns.push_back(ValueInspectorBE::Name);
    columns.push_back(ValueInspectorBE::Value);

    // create a test dict
    DictRef dict(create_dict_with_int_data());

    $expect(dict.count()).toBe(9U);
    $expect(dict.type()).toBe(DictType);
    $expect(dict.content_type()).toBe(IntegerType);

    ValueInspectorBE *vinsp = ValueInspectorBE::create(dict, false, false);

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test11.txt", vinsp, columns, true);
    expectFilesEqual("object check", data->outputDir + "/grt_inspector_value_test11.txt", data->dataDir + "/be/grt_inspector_value_test11.txt");

    // test listing
    size_t c = vinsp->count();
    $expect(c).toBe(9U);

    std::string name, value;
    NodeId node;

    // test get random item
    node = vinsp->get_node(9);
    $expect(node.is_valid()).toBeFalse();

    node = vinsp->get_node(-1);
    $expect(node.is_valid()).toBeFalse();

    // test change int value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
    $expect(flag).toBeTrue();

    flag = vinsp->set_field(4, ValueInspectorBE::Value, (ssize_t)8888);
    $expect(flag).toBeTrue();

    // test change string value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, "hello");
    $expect(flag).toBeFalse();

    // test change double value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, 1.234);
    $expect(flag).toBeFalse();

    // test change int value from string
    flag = vinsp->set_convert_field(1, ValueInspectorBE::Value, "112233");
    $expect(flag).toBeTrue();
    // in this case, the dict is untyped, so it should just appear as string

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test11.1.txt", vinsp, columns, true);
    expectFilesEqual("object check set", data->outputDir + "/grt_inspector_value_test11.1.txt",
                      data->dataDir + "/be/grt_inspector_value_test11.1.txt");

    // item count still ok?
    $expect((int)dict.count()).toEqual(9);

    // test add new value
    NodeId nkey;
    flag = vinsp->add_item(nkey);
    $expect(flag).toBeTrue();
    $expect(nkey[0]).toBe(9U);

    flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "new value");
    $expect(flag).toBeFalse();

    flag = vinsp->set_field(nkey, ValueInspectorBE::Value, "1234");
    $expect(flag).toBeFalse();

    flag = vinsp->set_field(nkey, ValueInspectorBE::Name, "anewk");
    $expect(flag).toBeTrue();

    flag = vinsp->set_field(nkey, ValueInspectorBE::Value, (ssize_t)1234);
    $expect(flag).toBeTrue();

    $expect(dict.count()).toEqual(10U);

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test11.2.txt", vinsp, columns, true);
    expectFilesEqual("object check add", data->outputDir + "/grt_inspector_value_test11.2.txt",
                      data->dataDir + "/be/grt_inspector_value_test11.2.txt");

    flag = vinsp->add_item(nkey);
    $expect(flag).toBeTrue();

    flag = vinsp->add_item(nkey);
    $expect(flag).toBeFalse();

    vinsp->refresh();

    flag = vinsp->add_item(nkey);
    $expect(flag).toBeTrue();

    flag = vinsp->add_item(nkey);
    $expect(flag).toBeFalse();

    flag = vinsp->set_convert_field(nkey, ValueInspectorBE::Name, "aaa");
    flag = vinsp->set_convert_field(nkey, ValueInspectorBE::Value, "22");
    $expect(flag).toBeTrue();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test11.3.txt", vinsp, columns, true);
    expectFilesEqual("object check add", data->outputDir + "/grt_inspector_value_test11.3.txt",
                      data->dataDir + "/be/grt_inspector_value_test11.3.txt");

    // test delete value
    flag = vinsp->delete_item(3);
    $expect(flag).toBeTrue();

    $expect(dict.count()).toBe(10U);

    flag = vinsp->delete_item(10);
    $expect(flag).toBeFalse();

    flag = vinsp->delete_item(9);
    $expect(flag).toBeTrue();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test11.4.txt", vinsp, columns, true);
    expectFilesEqual("object check del", data->outputDir + "/grt_inspector_value_test11.4.txt",
                      data->dataDir + "/be/grt_inspector_value_test11.4.txt");

    delete vinsp;

    $expect(dict.count()).toBe(9U);
  });

  // Test Objects
  // ------------

  $it("Test inspection of object (ungrouped)", [this]() {
    $pending("require investigation of exception");
    std::vector<ssize_t> columns;
    columns.push_back(ValueInspectorBE::Name);
    columns.push_back(ValueInspectorBE::Value);

    test_BookRef book(grt::Initialized);

    $expect(book.is_valid()).toBeTrue();

    ValueInspectorBE *vinsp = ValueInspectorBE::create(book, false, false);
    bool flag;

    // test listing
    size_t c = vinsp->count();
    $expect(c).toBe(6U);

    std::string name, value;

    NodeId node;
    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test20.txt", vinsp, columns, true);
    expectFilesEqual("object check", data->outputDir + "/grt_inspector_value_test20.txt", data->dataDir + "/be/grt_inspector_value_test20.txt");

    node = vinsp->get_node(9);
    $expect(node.is_valid()).toBeFalse();

    node = vinsp->get_node(-1);
    $expect(node.is_valid()).toBeFalse();

    // test bad change int value
    flag = vinsp->set_field(5, ValueInspectorBE::Value, (ssize_t)112211);
    $expect(flag).toBeFalse();

    flag = vinsp->set_field(5, ValueInspectorBE::Value, "The Illiad");
    $expect(flag).toBeTrue();

    flag = vinsp->get_field(5, ValueInspectorBE::Value, value);
    $expect(flag).toBeTrue();
    $expect(value).toBe("The Illiad");

    // test change int value
    flag = vinsp->set_field(2, ValueInspectorBE::Value, (ssize_t)123);
    $expect(flag).toBeTrue();

    // test change double value
    flag = vinsp->set_field(3, ValueInspectorBE::Value, 1.234);
    $expect(flag).toBeTrue();

    // test change int value from string
    flag = vinsp->set_convert_field(2, ValueInspectorBE::Value, "112233");
    $expect(flag).toBeTrue();
    // in this case, the dict is untyped, so it should just appear as string

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test20.1.txt", vinsp, columns, true);
    expectFilesEqual("object check set", data->outputDir + "/grt_inspector_value_test20.1.txt",
                      data->dataDir + "/be/grt_inspector_value_test20.1.txt");

    // test add new value
    NodeId nkey;
    flag = vinsp->add_item(nkey);
    $expect(flag).toBeFalse();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test20.2.txt", vinsp, columns, true);
    expectFilesEqual("object check add", data->outputDir + "/grt_inspector_value_test20.2.txt",
                      data->dataDir + "/be/grt_inspector_value_test20.2.txt");

    // test delete value
    flag = vinsp->delete_item(3);
    $expect(flag).toBeFalse();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test20.3.txt", vinsp, columns, true);
    expectFilesEqual("object check del", data->outputDir + "/grt_inspector_value_test20.3.txt",
                      data->dataDir + "/be/grt_inspector_value_test20.3.txt");

    delete vinsp;
  });

  $it("Test inspection of object (grouped)", [this]() {
    $pending("require investigation of exception");
    std::vector<ssize_t> columns;
    columns.push_back(ValueInspectorBE::Name);
    columns.push_back(ValueInspectorBE::Value);

    test_BookRef book(grt::Initialized);
    $expect(book.is_valid()).toBeTrue();

    ValueInspectorBE *vinsp = ValueInspectorBE::create(book, true, true);

    // test listing
    size_t c = vinsp->count();
    $expect(c).toBe(3U);

    std::string name, value;
    bool flag;

    NodeId node, gnode;

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test21.txt", vinsp, columns, true);
    expectFilesEqual("grouped object check", data->outputDir + "/grt_inspector_value_test21.txt",
                      data->dataDir + "/be/grt_inspector_value_test21.txt");

    try {
      node = vinsp->get_child(NodeId(1), 3);
      $expect(node.is_valid()).toBeFalse();
    } catch (std::range_error &) {
      // expected
    }

    // test bad change int value
    flag = vinsp->set_field(0, ValueInspectorBE::Value, (ssize_t)112211);
    $expect(flag).toBeFalse();

    node = vinsp->get_child(0, 0);

    flag = vinsp->set_field(node, ValueInspectorBE::Value, "The Illiad");
    $expect(flag).toBeTrue();

    // test change int value
    node = vinsp->get_child(1, 1);
    flag = vinsp->set_field(node, ValueInspectorBE::Value, (ssize_t)123);
    $expect(flag).toBeTrue();

    // test change double value
    node = vinsp->get_child(1, 2);
    flag = vinsp->set_field(node, ValueInspectorBE::Value, 1.234);
    $expect(flag).toBeTrue();

    // test change int value from string
    node = vinsp->get_child(1, 1);
    flag = vinsp->set_convert_field(node, ValueInspectorBE::Value, "112233");
    $expect(flag).toBeTrue();

    // test change group name
    node = vinsp->get_node(1);
    flag = vinsp->set_convert_field(node, ValueInspectorBE::Value, "HELLO");
    $expect(flag).toBeFalse();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test21.1.txt", vinsp, columns, true);
    expectFilesEqual("grouped object check set", data->outputDir + "/grt_inspector_value_test21.1.txt",
                      data->dataDir + "/be/grt_inspector_value_test21.1.txt");

    // test add new value
    NodeId nkey;
    flag = vinsp->add_item(nkey);
    $expect(flag).toBeFalse();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test21.2.txt", vinsp, columns, true);
    expectFilesEqual("grouped object check add", data->outputDir + "/grt_inspector_value_test21.2.txt",
                      data->dataDir + "/be/grt_inspector_value_test21.2.txt");

    // test delete value
    flag = vinsp->delete_item(3);
    $expect(flag).toBeFalse();

    casmine::dumpTreeModel(data->outputDir + "/grt_inspector_value_test21.3.txt", vinsp, columns, true);
    expectFilesEqual("grouped object check del", data->outputDir + "/grt_inspector_value_test21.3.txt",
                      data->dataDir + "/be/grt_inspector_value_test21.3.txt");

    delete vinsp;
  });
}
}
