/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "grtdb/db_object_helpers.h"
#include "model/wb_history_tree.h"
#include "model/wb_context_model.h"
#include "grt_test_utility.h"
#include "grtpp_undo_manager.h"
#include "grtpp_util.h"

using namespace bec;
using namespace wb;
using namespace grt;

BEGIN_TEST_DATA_CLASS(wb_undo_others)
public:
WBTester *tester;
UndoManager *um;
OverviewBE *overview;
size_t last_undo_stack_height;
size_t last_redo_stack_height;

TEST_DATA_CONSTRUCTOR(wb_undo_others) {
  tester = new WBTester;
  tester->create_new_document();
  um = grt::GRT::get()->get_undo_manager();
  overview = 0;
  last_undo_stack_height = 0;
  last_redo_stack_height = 0;
}

#include "wb_undo_methods.h"

void check_overview_object(const std::string &what, const NodeId &base_node, const std::string &list_path,
                           size_t initial_count = 0) {
  reset_undo_accounting();

  // Checks Overview object handling by adding a node, renaming it and then deleting it
  // with undo/redo for each operation

  NodeId add_node(base_node);
  add_node.append(0);

  NodeId added_node(base_node);
  added_node.append((int)initial_count + 1);

  std::string name;
  {
    grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->get_pmodel(), list_path));
    ensure_equals(list_path + " " + what + " initial count", list.count(), initial_count);
  }
  // Add diagram
  overview->get_field(add_node, OverviewBE::Label, name);
  ensure_equals(what + " add node", name, "Add " + what);
  overview->activate_node(add_node);

  check_only_one_undo_added();

  // check that it was added
  overview->refresh_node(base_node, true);
  ensure_equals(what + " node count", overview->count_children(base_node), initial_count + 2);
  {
    grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->get_pmodel(), list_path));
    ensure_equals(list_path + " " + what + " count", list.count(), initial_count + 1U);
  }
  // check undo add
  check_undo();
  overview->refresh_node(base_node, true);
  ensure_equals(what + " node count", overview->count_children(base_node), initial_count + 1);
  {
    grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->get_pmodel(), list_path));
    ensure_equals(list_path + " " + what + " count after undo", list.count(), initial_count);
  }

  // check redo add
  check_redo();
  overview->refresh_node(base_node, true);
  ensure_equals("diagram node count", overview->count_children(base_node), initial_count + 2);
  {
    grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->get_pmodel(), list_path));
    ensure_equals(list_path + " " + what + " count after redo", list.count(), initial_count + 1U);
  }

  // check Renaming
  std::string old_name;
  overview->get_field(added_node, OverviewBE::Label, old_name);

  overview->set_field(added_node, OverviewBE::Label, "new name");
  overview->refresh_node(added_node, false);
  check_only_one_undo_added();

  overview->get_field(added_node, OverviewBE::Label, name);
  ensure_equals("rename " + what, name, "new name");

  check_undo();
  overview->refresh_node(added_node, false);
  overview->get_field(added_node, OverviewBE::Label, name);
  ensure_equals("undo rename " + what, name, old_name);

  check_redo();
  overview->refresh_node(added_node, false);
  overview->get_field(added_node, OverviewBE::Label, name);
  ensure_equals("redo rename " + what, name, "new name");

  // Delete
  overview->request_delete_object(added_node);
  check_only_one_undo_added();

  // check delete
  overview->refresh_node(base_node, true);
  ensure_equals(what + " node count", overview->count_children(base_node), initial_count + 1);
  {
    grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->get_pmodel(), list_path));
    ensure_equals(list_path + " " + what + " count after delete", list.count(), initial_count);
  }

  // check undo delete
  check_undo();
  overview->refresh_node(base_node, true);
  ensure_equals(what + " node count", overview->count_children(base_node), initial_count + 2);
  {
    grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->get_pmodel(), list_path));
    ensure_equals(list_path + " " + what + " count after undo", list.count(), initial_count + 1U);
  }

  // check redo delete
  check_redo();
  overview->refresh_node(base_node, true);
  ensure_equals(what + " node count", overview->count_children(base_node), initial_count + 1);
  {
    grt::BaseListRef list = grt::BaseListRef::cast_from(get_value_by_path(tester->get_pmodel(), list_path));
    ensure_equals(list_path + " " + what + " count after redo", list.count(), initial_count);
  }

  check_undo(); // final undo delete

  check_undo(); // final undo rename

  check_undo(); // final undo add
}

END_TEST_DATA_CLASS;

// For undo tests we use a single document loaded at the beginning of the group
// Each test must do the test and undo everything so that the state of the document
// never actually changes.
// At the end of the test group, the document is compared to the saved one to check
// for unexpected changes (ie, anything but stuff like timestamps)

TEST_MODULE(wb_undo_others, "undo tests for Workbench");

// setup
TEST_FUNCTION(1) {
  bool flag = tester->wb->open_document("data/workbench/undo_test_model1.mwb");
  ensure("open_document", flag);

  overview = wb::WBContextUI::get()->get_physical_overview();
  wb::WBContextUI::get()->set_active_form(overview);

  ensure_equals("schemas", tester->get_catalog()->schemata().count(), 1U);

  db_SchemaRef schema(tester->get_catalog()->schemata()[0]);

  // make sure the loaded model contains expected number of things
  ensure_equals("tables", schema->tables().count(), 4U);
  ensure_equals("views", schema->views().count(), 1U);
  ensure_equals("groups", schema->routineGroups().count(), 1U);

  ensure_equals("diagrams", tester->get_pmodel()->diagrams().count(), 1U);
  model_DiagramRef view(tester->get_pmodel()->diagrams()[0]);

  ensure_equals("figures", view->figures().count(), 5U);

  ensure_equals("undo stack is empty", um->get_undo_stack().size(), 0U);
}

// Overview
//----------------------------------------------------------------------------------------
// Test Adding, Renaming and Deleting objects

TEST_FUNCTION(10) // Diagram
{
  check_overview_object("Diagram", NodeId("0"), "/diagrams", 1);
  ensure_equals("undo stack size", um->get_undo_stack().size(), 0U);
}

TEST_FUNCTION(12) // Schema
{
  std::string s;

  ensure_equals("schema count", overview->count_children(NodeId("1")), 1U);

  overview->request_add_object(NodeId("1"));
  check_only_one_undo_added();

  overview->refresh_node(NodeId("1"), true);
  ensure_equals("schema count", overview->count_children(NodeId("1")), 2U);

  check_undo();
  overview->refresh_node(NodeId("1"), true);
  ensure_equals("schema count", overview->count_children(NodeId("1")), 1U);

  check_redo();
  overview->refresh_node(NodeId("1"), true);
  ensure_equals("schema count", overview->count_children(NodeId("1")), 2U);

  overview->activate_node(NodeId("1.1"));

  overview->refresh_node(NodeId("1"), true);
  overview->get_field(NodeId("1.1"), 0, s);
  ensure_equals("overview original name", s, "new_schema1");

  /* cant rename schema directly atm
  bool flag= overview->set_field(NodeId("1.1"), 0, "sakila");
  ensure("rename", flag);
  check_only_one_undo_added();

  overview->refresh_node(NodeId("1"), true);
  overview->get_field(NodeId("1.1"), 0, s);
  ensure_equals("overview rename", s, "sakila");

  check_undo();
  overview->refresh_node(NodeId("1"), true);
  overview->get_field(NodeId("1.1"), 0, s);
  ensure_equals("overview original name", s, "new_schema1");

  check_redo();
  overview->set_field(NodeId("1.1"), 0, "sakila");
  check_only_one_undo_added();
*/
  overview->request_delete_object(NodeId("1.1"));
  check_only_one_undo_added();

  overview->refresh_node(NodeId("1"), true);
  ensure_equals("schema count", overview->count_children(NodeId("1")), 1U);

  check_undo();
  overview->refresh_node(NodeId("1"), true);
  ensure_equals("schema count", overview->count_children(NodeId("1")), 2U);

  check_redo();
  overview->refresh_node(NodeId("1"), true);
  ensure_equals("schema count", overview->count_children(NodeId("1")), 1U);

  check_undo();

  check_undo(); // final undo schema add

  ensure_equals("undo stack size", um->get_undo_stack().size(), 0U);
}

TEST_FUNCTION(14) // Table
{
  check_overview_object("Table", NodeId("1.0.0"), "/catalog/schemata/0/tables", 4U);
}

TEST_FUNCTION(16) //  View
{
  check_overview_object("View", NodeId("1.0.1"), "/catalog/schemata/0/views", 1U);
}

TEST_FUNCTION(18) //  Routine
{
  check_overview_object("Routine", NodeId("1.0.2"), "/catalog/schemata/0/routines", 0U);
}

TEST_FUNCTION(20) //  Routine Group
{
  check_overview_object("Group", NodeId("1.0.3"), "/catalog/schemata/0/routineGroups", 1U);
}

TEST_FUNCTION(22) //  User
{
  check_overview_object("User", NodeId("2.0"), "/catalog/users");
}

TEST_FUNCTION(24) //  Role
{
  check_overview_object("Role", NodeId("2.1"), "/catalog/roles");
}

TEST_FUNCTION(26) //  Script
{
  check_overview_object("Script", NodeId("3"), "/scripts");
}

TEST_FUNCTION(28) //  Note
{
  check_overview_object("Note", NodeId("4"), "/notes");
}

// Sidebar
//----------------------------------------------------------------------------------------

TEST_FUNCTION(29) {
  tester->wb->close_document();

  // reinitialize
  bool flag = tester->wb->open_document("data/workbench/undo_test_model1.mwb");
  ensure("open_document", flag);

  overview = wb::WBContextUI::get()->get_physical_overview();
  wb::WBContextUI::get()->set_active_form(overview);

  bec::NodeId node(0);
  node.append(1);
  overview->activate_node(node);
}

TEST_FUNCTION(30) // Property
{
  ModelDiagramForm *view = tester->wb->get_model_context()->get_diagram_form_for_diagram_id(tester->get_pview().id());
  ensure("viewform", view != 0);

  model_FigureRef table(find_named_object_in_list(tester->get_pview()->figures(), "table2"));

  tester->get_pview()->selectObject(table);

  ensure_equals("selection", view->get_selection().count(), 1U);

  std::vector<std::string> items;

  ValueInspectorBE *insp = wb::WBContextUI::get()->create_inspector_for_selection(view, items);
  ensure("prop inspector created", insp != 0);
  ensure_equals("items", items.size(), 1U);
  ensure_equals("item0", items[0], "table2: Table");

  ensure_equals("table name", *table->name(), "table2");

  std::string s;

  insp->get_field(NodeId(7), ValueInspectorBE::Name, s);
  ensure_equals("node for name", s, "name");

  bool flag = insp->set_field(NodeId(7), ValueInspectorBE::Value, "hello");
  ensure("rename value", flag);
  check_only_one_undo_added();

  ensure_equals("table renamed", *table->name(), "hello");
  check_undo();
  ensure_equals("table renamed back", *table->name(), "table2");
  check_redo();
  ensure_equals("table renamed", *table->name(), "hello");

  check_undo();

  delete insp;
}

TEST_FUNCTION(32) // Description
{
  // select table in overview
  overview->refresh_node(NodeId("1.0.0"), true);
  overview->begin_selection_marking();
  overview->select_node(NodeId("1.0.0.1"));
  overview->end_selection_marking();

  std::vector<std::string> items;
  grt::ListRef<GrtObject> new_object_list;
  std::string description, old_description;

  old_description = wb::WBContextUI::get()->get_description_for_selection(new_object_list, items);

  ensure_equals("selection count", items.size(), 1U);

  wb::WBContextUI::get()->set_description_for_selection(new_object_list, "test description");
  check_only_one_undo_added();

  ensure_equals("description", *tester->get_catalog()->schemata()[0]->tables()[0]->comment(), "test description");

  check_undo();

  ensure_equals("description", *tester->get_catalog()->schemata()[0]->tables()[0]->comment(), "");

  check_redo();

  ensure_equals("description", *tester->get_catalog()->schemata()[0]->tables()[0]->comment(), "test description");

  // undo change description
  check_undo();
}

// Configuration
//----------------------------------------------------------------------------------------
TEST_FUNCTION(40) // General Settings
{
  //	ensure("not implemented", false);
}

TEST_FUNCTION(42) // Model Settings
{
  // ensure("not implemented", false);
}

TEST_FUNCTION(44) // Diagram Settings
{
  //	ensure("not implemented", false);
}

TEST_FUNCTION(46) // Page Settings
{
  //	ensure("not implemented", false);
}

// Plugins
//----------------------------------------------------------------------------------------
TEST_FUNCTION(50) // Plugin Execution
{ 
  // XXX: something wrong with blocked UI events at this point... maybe should split the test
  /*
  model_DiagramRef mview(tester->get_pview());

  //  wb::WBContextUI::get()->set_active_form(tester->tester->get_model_context()->get_diagram_form_for_diagram_id(tester->get_pmodel()->diagrams()[0].id()));

  ensure_equals("grid", mview->options().get_int("ShowGrid", -42), -42);

  wb::WBContextUI::get()->get_command_ui()->activate_command("plugin:tester->edit.toggleGrid");
  check_only_one_undo_added();

  ensure_equals("grid", mview->options().get_int("ShowGrid", -42), 0);
  check_undo();

  ensure_equals("grid", mview->options().get_int("ShowGrid", -42), -42);
  check_redo();

  ensure_equals("grid", mview->options().get_int("ShowGrid", -42), 0);

  check_undo();
  */
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
