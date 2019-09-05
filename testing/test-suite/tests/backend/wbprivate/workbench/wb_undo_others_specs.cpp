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

#include "grtdb/db_object_helpers.h"
#include "model/wb_history_tree.h"
#include "model/wb_context_model.h"
#include "grtpp_undo_manager.h"
#include "grtpp_util.h"

#include "casmine.h"
#include "wb_test_helpers.h"
#include "grt_test_helpers.h"

using namespace bec;
using namespace wb;
using namespace grt;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  UndoManager *um = nullptr;
  OverviewBE *overview = nullptr;
  size_t lastUndoStackSize = 0;
  size_t lastRedoStackSize = 0;

  #include "wb_undo_helpers.h"

  void checkOverviewObject(const std::string &what, const NodeId &base_node, const std::string &list_path,
                           size_t initial_count = 0) {
    resetUndoAccounting();

    // Checks Overview object handling by adding a node, renaming it and then deleting it
    // with undo/redo for each operation

    NodeId add_node(base_node);
    add_node.append(0);

    NodeId added_node(base_node);
    added_node.append((int)initial_count + 1);

    std::string name;
    {
      grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->getPmodel(), list_path));
      $expect(list.count()).toEqual(initial_count, list_path + " " + what + " initial count");
    }

    // Add diagram
    overview->get_field(add_node, OverviewBE::Label, name);
    $expect(name).toEqual("Add " + what, what + " add node");
    overview->activate_node(add_node);

    checkOnlyOneUndoAdded();

    // check that it was added
    overview->refresh_node(base_node, true);
    $expect(overview->count_children(base_node)).toEqual(initial_count + 2, what + " node count");
    {
      grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->getPmodel(), list_path));
      $expect(list.count()).toEqual(initial_count + 1, list_path + " " + what + " count");
    }
    // check undo add
    checkUndo();
    overview->refresh_node(base_node, true);
    $expect(overview->count_children(base_node)).toEqual(initial_count + 1, what + " node count");
    {
      grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->getPmodel(), list_path));
      $expect(list.count()).toEqual(initial_count, list_path + " " + what + " count after undo");
    }

    // check redo add
    checkRedo();
    overview->refresh_node(base_node, true);
    $expect(overview->count_children(base_node)).toEqual(initial_count + 2, "diagram node count");
    {
      grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->getPmodel(), list_path));
      $expect(list.count()).toEqual(initial_count + 1, list_path + " " + what + " count after redo");
    }

    // check Renaming
    std::string old_name;
    overview->get_field(added_node, OverviewBE::Label, old_name);

    overview->set_field(added_node, OverviewBE::Label, "new name");
    overview->refresh_node(added_node, false);
    checkOnlyOneUndoAdded();

    overview->get_field(added_node, OverviewBE::Label, name);
    $expect(name).toEqual("new name", "rename " + what);

    checkUndo();
    overview->refresh_node(added_node, false);
    overview->get_field(added_node, OverviewBE::Label, name);
    $expect(name).toEqual(old_name, "undo rename " + what);

    checkRedo();
    overview->refresh_node(added_node, false);
    overview->get_field(added_node, OverviewBE::Label, name);
    $expect(name).toEqual("new name", "redo rename " + what);

    // Delete
    overview->request_delete_object(added_node);
    checkOnlyOneUndoAdded();

    // check delete
    overview->refresh_node(base_node, true);
    $expect(overview->count_children(base_node)).toEqual(initial_count + 1, what + " node count");
    {
      grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->getPmodel(), list_path));
      $expect(list.count()).toEqual(initial_count, list_path + " " + what + " count after delete");
    }

    // check undo delete
    checkUndo();
    overview->refresh_node(base_node, true);
    $expect(overview->count_children(base_node)).toEqual(initial_count + 2, what + " node count");
    {
      grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->getPmodel(), list_path));
      $expect(list.count()).toEqual(initial_count + 1, list_path + " " + what + " count after undo");
    }

    // check redo delete
    checkRedo();
    overview->refresh_node(base_node, true);
    $expect(overview->count_children(base_node)).toEqual(initial_count + 1, what + " node count");
    {
      grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->getPmodel(), list_path));
      $expect(list.count()).toEqual(initial_count, list_path + " " + what + " count after redo");
    }

    checkUndo(); // final undo delete
    checkUndo(); // final undo rename
    checkUndo(); // final undo add
  }
};

$describe("General Undo/Redo") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->createNewDocument();
    data->um = grt::GRT::get()->get_undo_manager();

    bool flag = data->tester->wb->open_document(casmine::CasmineContext::get()->tmpDataDir() + "/workbench/undo_test_model1.mwb");
    $expect(flag).toBeTrue("open_document");

    data->overview = wb::WBContextUI::get()->get_physical_overview();
    wb::WBContextUI::get()->set_active_form(data->overview);

    $expect(data->tester->getCatalog()->schemata().count()).toEqual(1U, "schemas");

    db_SchemaRef schema(data->tester->getCatalog()->schemata()[0]);

    // make sure the loaded model contains expected number of things
    $expect(schema->tables().count()).toEqual(4U, "tables");
    $expect(schema->views().count()).toEqual(1U, "views");
    $expect(schema->routineGroups().count()).toEqual(1U, "groups");

    $expect(data->tester->getPmodel()->diagrams().count()).toEqual(1U, "diagrams");
    model_DiagramRef view(data->tester->getPmodel()->diagrams()[0]);

    $expect(view->figures().count()).toEqual(5U, "figures");

    $expect(data->um->get_undo_stack().size()).toEqual(0U, "undo stack is empty");
  });

  // For undo tests we use a single document loaded at the beginning of the group
  // Each test must do the test and undo everything so that the state of the document
  // never actually changes.
  // At the end of the test group, the document is compared to the saved one to check
  // for unexpected changes (ie, anything but stuff like timestamps)

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: Diagram", [this]() {
    data->checkOverviewObject("Diagram", NodeId("0"), "/diagrams", 1);
    $expect(data->um->get_undo_stack().size()).toEqual(0U, "undo stack size");
  });

  $it("Overview manipulations: Schema", [this]() {
    std::string s;

    $expect(data->overview->count_children(NodeId("1"))).toEqual(1U, "schema count");

    data->overview->request_add_object(NodeId("1"));
    data->checkOnlyOneUndoAdded();

    data->overview->refresh_node(NodeId("1"), true);
    $expect(data->overview->count_children(NodeId("1"))).toEqual(2U, "schema count");

    data->checkUndo();
    data->overview->refresh_node(NodeId("1"), true);
    $expect(data->overview->count_children(NodeId("1"))).toEqual(1U, "schema count");

    data->checkRedo();
    data->overview->refresh_node(NodeId("1"), true);
    $expect(data->overview->count_children(NodeId("1"))).toEqual(2U, "schema count");

    data->overview->activate_node(NodeId("1.1"));

    data->overview->refresh_node(NodeId("1"), true);
    data->overview->get_field(NodeId("1.1"), 0, s);
    $expect(s).toEqual("new_schema1", "overview original name");

    /* cant rename schema directly atm
     bool flag= overview->set_field(NodeId("1.1"), 0, "sakila");
     $expect("rename", flag);
     data->checkOnlyOneUndoAdded();

     overview->refresh_node(NodeId("1"), true);
     overview->get_field(NodeId("1.1"), 0, s);
     $expect("overview rename", s, "sakila");

     data->checkUndo();
     overview->refresh_node(NodeId("1"), true);
     overview->get_field(NodeId("1.1"), 0, s);
     $expect("overview original name", s, "new_schema1");

     data->checkRedo();
     overview->set_field(NodeId("1.1"), 0, "sakila");
     data->checkOnlyOneUndoAdded();
     */
    data->overview->request_delete_object(NodeId("1.1"));
    data->checkOnlyOneUndoAdded();

    data->overview->refresh_node(NodeId("1"), true);
    $expect(data->overview->count_children(NodeId("1"))).toEqual(1U, "schema count");

    data->checkUndo();
    data->overview->refresh_node(NodeId("1"), true);
    $expect(data->overview->count_children(NodeId("1"))).toEqual(2U, "schema count");

    data->checkRedo();
    data->overview->refresh_node(NodeId("1"), true);
    $expect(data->overview->count_children(NodeId("1"))).toEqual(1U, "schema count");

    data->checkUndo();
    data->checkUndo(); // final undo schema add

    $expect(data->um->get_undo_stack().size()).toEqual(0U, "undo stack size");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: Table", [this]() {
    data->checkOverviewObject("Table", NodeId("1.0.0"), "/catalog/schemata/0/tables", 4);
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: View", [this]() {
    data->checkOverviewObject("View", NodeId("1.0.1"), "/catalog/schemata/0/views", 1);
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: Routine", [this]() {
    data->checkOverviewObject("Routine", NodeId("1.0.2"), "/catalog/schemata/0/routines", 0);
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: Group", [this]() {
    data->checkOverviewObject("Group", NodeId("1.0.3"), "/catalog/schemata/0/routineGroups", 1);
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: User", [this]() {
    data->checkOverviewObject("User", NodeId("2.0"), "/catalog/users");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: Role", [this]() {
    data->checkOverviewObject("Role", NodeId("2.1"), "/catalog/roles");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: Script", [this]() {
    data->checkOverviewObject("Script", NodeId("3"), "/scripts");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Overview manipulations: Note", [this]() {
    data->checkOverviewObject("Note", NodeId("4"), "/notes");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Sidebar", [this]() {
    data->tester->wb->close_document();

    // reinitialize
    bool flag = data->tester->wb->open_document(casmine::CasmineContext::get()->tmpDataDir() + "/workbench/undo_test_model1.mwb");
    $expect(flag).toBeTrue("open_document");

    data->overview = wb::WBContextUI::get()->get_physical_overview();
    wb::WBContextUI::get()->set_active_form(data->overview);

    bec::NodeId node(0);
    node.append(1);
    data->overview->activate_node(node);
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Property", [this]() {
    ModelDiagramForm *view = data->tester->wb->get_model_context()->get_diagram_form_for_diagram_id(data->tester->getPview().id());
    $expect(view).Not.toBeNull("viewform");

    model_FigureRef table(find_named_object_in_list(data->tester->getPview()->figures(), "table2"));

    data->tester->getPview()->selectObject(table);

    $expect(view->get_selection().count()).toEqual(1U, "selection");

    std::vector<std::string> items;

    ValueInspectorBE *insp = wb::WBContextUI::get()->create_inspector_for_selection(view, items);
    $expect(insp).Not.toBeNull("prop inspector created");
    $expect(items.size()).toEqual(1U, "items");
    $expect(items[0]).toEqual("table2: Table", "item0");

    $expect(*table->name()).toEqual("table2", "table name");

    std::string s;

    insp->get_field(NodeId(7), ValueInspectorBE::Name, s);
    $expect(s).toEqual("name", "node for name");

    bool flag = insp->set_field(NodeId(7), ValueInspectorBE::Value, "hello");
    $expect(flag).toBeTrue("rename value");
    data->checkOnlyOneUndoAdded();

    $expect(*table->name()).toEqual("hello", "table renamed");
    data->checkUndo();
    $expect(*table->name()).toEqual("table2", "table renamed back");
    data->checkRedo();
    $expect(*table->name()).toEqual("hello", "table renamed");

    data->checkUndo();

    delete insp;
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Description", [this]() {
    // select table in overview
    data->overview->refresh_node(NodeId("1.0.0"), true);
    data->overview->begin_selection_marking();
    data->overview->select_node(NodeId("1.0.0.1"));
    data->overview->end_selection_marking();

    std::vector<std::string> items;
    grt::ListRef<GrtObject> new_object_list;
    std::string description, old_description;

    old_description = wb::WBContextUI::get()->get_description_for_selection(new_object_list, items);

    $expect(items.size()).toEqual(1U, "selection count");

    wb::WBContextUI::get()->set_description_for_selection(new_object_list, "test description");
    data->checkOnlyOneUndoAdded();

    $expect(*data->tester->getCatalog()->schemata()[0]->tables()[0]->comment()).toEqual("test description", "description");

    data->checkUndo();

    $expect(*data->tester->getCatalog()->schemata()[0]->tables()[0]->comment()).toEqual("", "description");

    data->checkRedo();

    $expect(*data->tester->getCatalog()->schemata()[0]->tables()[0]->comment()).toEqual("test description", "description");

    // undo change description
    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Configuration: general settings", []() {
    $pending("not implemented");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Configuration: model settings", []() {
    $pending("not implemented");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Configuration: diagram settings", []() {
    $pending("not implemented");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Configuration: page settings", []()
  {
    $pending("not implemented");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Plugin execution", []() {
    $pending("something wrong with blocked UI events at this point... maybe should split the test");
    /*
     model_DiagramRef mview(tester->get_pview());

     //  wb::WBContextUI::get()->set_active_form(tester->tester->get_model_context()->get_diagram_form_for_diagram_id(tester->getPmodel()->diagrams()[0].id()));

     $expect("grid", mview->options().get_int("ShowGrid", -42), -42);

     wb::WBContextUI::get()->get_command_ui()->activate_command("plugin:tester->edit.toggleGrid");
     data->checkOnlyOneUndoAdded();

     $expect("grid", mview->options().get_int("ShowGrid", -42), 0);
     data->checkUndo();

     $expect("grid", mview->options().get_int("ShowGrid", -42), -42);
     data->checkRedo();

     $expect("grid", mview->options().get_int("ShowGrid", -42), 0);

     data->checkUndo();
     */
  });

}

}
