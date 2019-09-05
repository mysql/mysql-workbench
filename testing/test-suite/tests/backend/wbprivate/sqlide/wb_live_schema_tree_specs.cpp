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

#include "stub/stub_mforms.h"
#include "sqlide/wb_live_schema_tree.h"
#include "grt.h"

#include "casmine.h"

using namespace grt;
using namespace wb;

#define SCHEMA 1

#define TABLE 2
#define VIEW 4
#define PROCEDURE 8
#define FUNCTION 16

#define TABLES 32
#define VIEWS 64
#define PROCEDURES 128
#define FUNCTIONS 256

#define COLUMNS 512
#define INDEXES 1024
#define TRIGGERS 2048
#define FKS 4096

#define TABLE_COLUMN 8192
#define INDEX 16384
#define TRIGGER 32768
#define FK 65536

#define VIEW_COLUMN 131072

#define SET_DEF_SCH 1
#define FIL_TO_SCH 2
#define COPY_TC 4
#define SEND_TE 8
#define CREATE 16
#define ALTER 32
#define DROP 64
#define REFRESH 128
#define SEL_ROWS 256
#define EDIT 512

#define SUB_NAME 1
#define SUB_NAME_S 2
#define SUB_NAME_L 4
#define SUB_CREATE 8
#define SUB_SEL_ALL 16
#define SUB_INSERT 32
#define SUB_UPDATE 64
#define SUB_DELETE 128
#define SUB_SEL_COL 256

namespace wb {

  class LiveSchemaTreeTestHelper : public LiveSchemaTree {
  public:
    using LiveSchemaTree::_active_schema;
    using LiveSchemaTree::load_schema_content;
    using LiveSchemaTree::_case_sensitive_identifiers;
    using LiveSchemaTree::identifiers_equal;
    using LiveSchemaTree::is_object_type;
    using LiveSchemaTree::_model_view;
    using LiveSchemaTree::_delegate;
    using LiveSchemaTree::_fetch_delegate;
    using LiveSchemaTree::get_filter_wildcard;
    using LiveSchemaTree::filter_children;
    using LiveSchemaTree::clean_filter;
    using LiveSchemaTree::set_filter;
    using LiveSchemaTree::getFilter;
    using LiveSchemaTree::getBase;
    using LiveSchemaTree::_schema_pattern;
    using LiveSchemaTree::_object_pattern;

    LiveSchemaTreeTestHelper() : LiveSchemaTree(base::MySQLVersion::MySQL57) {}
  };
}

namespace {

$ModuleEnvironment() {};

$TestData {
  class LiveTreeTestDelegate : public LiveSchemaTree::Delegate, public LiveSchemaTree::FetchDelegate {
  public:
    LiveSchemaTree *ptree;
    bool _expect_fetch_schema_list_call;
    bool _expect_fetch_schema_contents_call;
    bool _expect_fetch_object_details_call;
    bool _expect_fetch_data_for_filter;
    bool _expect_plugin_item_call;

    bool _expect_tree_activate_objects;
    bool _expect_tree_create_object;
    bool _expect_tree_alter_objects;
    bool _expect_tree_drop_objects;
    bool _expect_tree_refresh;

    std::string _mock_schema_name;
    std::string _mock_object_name;
    std::string _mock__schema_pattern;
    std::string _mock__object_pattern;
    std::string _mock_str_object_type;
    LiveSchemaTree::ObjectType _mock_object_type;
    short _mock_flags;

    base::StringListPtr _mock_schema_list;
    base::StringListPtr _mock_table_list;
    base::StringListPtr _mock_view_list;
    base::StringListPtr _mock_procedure_list;
    base::StringListPtr _mock_function_list;
    base::StringListPtr _mock_column_list;
    base::StringListPtr _mock_index_list;
    base::StringListPtr _mock_trigger_list;
    base::StringListPtr _mock_fk_list;

    std::vector<LiveSchemaTree::ChangeRecord> _mock_expected_changes;

    bool _mock_call_back_slot;
    bool _mock_call_back_slot_columns;
    bool _mock_call_back_slot_indexes;
    bool _mock_call_back_slot_triggers;
    bool _mock_call_back_slot_foreign_keys;
    bool _mock_just_append;

    std::string _mock_expected_text;
    std::string _mock_expected_action;
    std::string _mock_expected_schema;
    LiveSchemaTree::ObjectType _mock_expected_object_type;
    std::string _mock_expected_object;

    std::string _check_id;

    LiveTreeTestDelegate()
      : _expect_fetch_schema_list_call(false),
      _expect_fetch_schema_contents_call(false),
      _expect_fetch_object_details_call(false),
      _expect_fetch_data_for_filter(false),
      _expect_plugin_item_call(false),
      _expect_tree_activate_objects(false),
      _expect_tree_create_object(false),
      _expect_tree_alter_objects(false),
      _expect_tree_drop_objects(false),
      _expect_tree_refresh(false),
      _mock_call_back_slot_columns(false),
      _mock_call_back_slot_indexes(false),
      _mock_call_back_slot_triggers(false),
      _mock_call_back_slot_foreign_keys(false),
      _mock_just_append(false) {
    }

    virtual ~LiveTreeTestDelegate() {
    }

    void expect_fetch_schema_contents_call() {
      _mock_schema_list = base::StringListPtr(new std::list<std::string>());
      _mock_table_list = base::StringListPtr(new std::list<std::string>());
      _mock_view_list = base::StringListPtr(new std::list<std::string>());
      _mock_procedure_list = base::StringListPtr(new std::list<std::string>());
      _mock_function_list = base::StringListPtr(new std::list<std::string>());
      _mock_column_list = base::StringListPtr(new std::list<std::string>());
      _mock_index_list = base::StringListPtr(new std::list<std::string>());
      _mock_trigger_list = base::StringListPtr(new std::list<std::string>());
      _mock_fk_list = base::StringListPtr(new std::list<std::string>());
      _expect_fetch_schema_contents_call = true;
    }

    virtual std::vector<std::string> fetch_schema_list() {
      $expect(_expect_fetch_schema_list_call).toBeTrue(_check_id + " : Unexpected call to fetch_schema_list");
      _expect_fetch_schema_list_call = false;

      std::vector<std::string> slist;
      slist.assign(_mock_schema_list->begin(), _mock_schema_list->end());
      return slist;
    }

    virtual bool fetch_data_for_filter(const std::string& _schema_pattern, const std::string& _object_pattern,
                                       const LiveSchemaTree::NewSchemaContentArrivedSlot& arrived_slot) {
      $expect(_expect_fetch_data_for_filter).toBeTrue(_check_id + " : Unexpected call to fetch_data_for_filter");
      _expect_fetch_data_for_filter = false;

      $expect(_mock__schema_pattern).toEqual(_schema_pattern, _check_id +
                                             " : Unexpected schema filter on fetch_schema_list");
      $expect(_mock__object_pattern).toEqual(_object_pattern, _check_id +
                                             " : Unexpected object filter on fetch_schema_list");

      return true;
    }

    virtual bool fetch_schema_contents(const std::string& schema_name,
                                       const LiveSchemaTree::NewSchemaContentArrivedSlot& arrived_slot) {
      $expect(_expect_fetch_schema_contents_call).toBeTrue(_check_id + " : Unexpected call to fetch_schema_contents");
      _expect_fetch_schema_contents_call = false;

      $expect(schema_name).toEqual(_mock_schema_name, _check_id +
                                   " : Unexpected schema name on call to fetch_schema_contents");

      if (_mock_call_back_slot)
        arrived_slot(_mock_schema_name, _mock_table_list, _mock_view_list, _mock_procedure_list, _mock_function_list,
                     _mock_just_append);

      return true;
    }

    virtual bool fetch_object_details(const std::string& schema_name, const std::string& obj_name,
                                      LiveSchemaTree::ObjectType obj_type, short flags,
                                      const LiveSchemaTree::NodeChildrenUpdaterSlot& updater_slot) {
      mforms::TreeNodeRef parent;
      LiveSchemaTree::ViewData* pviewdata;

      $expect(_expect_fetch_object_details_call).toBeTrue(_check_id + " : Unexpected call to fetch_object_details");
      _expect_fetch_object_details_call = false;

      $expect(schema_name).toEqual(_mock_schema_name, _check_id +
                                   " : Unexpected schema name on call to fetch_object_details");
      $expect(obj_name).toEqual(_mock_object_name, _check_id +
                                " : Unexpected object name on call to fetch_object_details");
      $expect(obj_type).toEqual(_mock_object_type, _check_id +
                                " : Unexpected object type on call to fetch_object_details");

      mforms::TreeNodeRef node = ptree->get_node_for_object(schema_name, obj_type, obj_name);
      pviewdata = dynamic_cast<LiveSchemaTree::ViewData*>(node->get_data());

      if (_mock_call_back_slot_columns) {
        parent = (obj_type == LiveSchemaTree::View) ? node : node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);

        updater_slot(parent, _mock_column_list,
                     (obj_type == LiveSchemaTree::Table) ? LiveSchemaTree::TableColumn : LiveSchemaTree::ViewColumn,
                     false, false);

        mforms::TreeNodeRef column;
        LiveSchemaTree::ColumnData* pdata;

        for (size_t index = 0; index < _mock_column_list->size(); index++) {
          column = parent->get_child((int)index);
          pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(column->get_data());
          pdata->details = "MOCK LOADED Column : " + column->get_string(0);
        }

        pviewdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA);

        _mock_column_list->clear();
        _mock_call_back_slot_columns = false;
      }

      if (obj_type == LiveSchemaTree::Table) {
        if (_mock_call_back_slot_indexes) {
          parent = node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);

          updater_slot(parent, _mock_index_list, LiveSchemaTree::Index, false, false);

          mforms::TreeNodeRef index_node;
          LiveSchemaTree::IndexData* pdata;

          for (size_t index = 0; index < _mock_index_list->size(); index++) {
            index_node = parent->get_child((int)index);
            pdata = dynamic_cast<LiveSchemaTree::IndexData*>(index_node->get_data());
            pdata->details = "MOCK LOADED Index : " + index_node->get_string(0);
          }

          pviewdata->set_loaded_data(LiveSchemaTree::INDEX_DATA);

          _mock_index_list->clear();
          _mock_call_back_slot_indexes = false;
        }

        if (_mock_call_back_slot_foreign_keys) {
          parent = node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);

          updater_slot(parent, _mock_fk_list, LiveSchemaTree::ForeignKey, false, false);

          mforms::TreeNodeRef fk_node;
          LiveSchemaTree::FKData* pdata;

          for (size_t index = 0; index < _mock_fk_list->size(); index++) {
            fk_node = parent->get_child((int)index);
            pdata = dynamic_cast<LiveSchemaTree::FKData*>(fk_node->get_data());
            pdata->details = "MOCK LOADED Foreign Key : " + fk_node->get_string(0);
          }

          pviewdata->set_loaded_data(LiveSchemaTree::FK_DATA);

          _mock_fk_list->clear();
          _mock_call_back_slot_foreign_keys = false;
        }

        if (_mock_call_back_slot_triggers) {
          parent = node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);

          updater_slot(parent, _mock_trigger_list, LiveSchemaTree::Trigger, false, false);

          mforms::TreeNodeRef trigger_node;
          LiveSchemaTree::TriggerData* pdata;

          for (size_t index = 0; index < _mock_trigger_list->size(); index++) {
            trigger_node = parent->get_child((int)index);
            pdata = dynamic_cast<LiveSchemaTree::TriggerData*>(trigger_node->get_data());
            pdata->details = "MOCK LOADED Trigger : " + trigger_node->get_string(0);
          }

          pviewdata->set_loaded_data(LiveSchemaTree::TRIGGER_DATA);

          _mock_trigger_list->clear();
          _mock_call_back_slot_triggers = false;
        }
      }

      return true;
    }

    virtual bool fetch_routine_details(const std::string& schema_name, const std::string& obj_name,
                                       LiveSchemaTree::ObjectType obj_type) {
      return true;
    }

    virtual void tree_refresh() {
      $expect(_expect_tree_refresh).toBeTrue(_check_id + " : Unexpected call to tree_refresh.");
      _expect_tree_refresh = false;
    }

    virtual bool sidebar_action(const std::string& action) {
      return true;
    }

    void check_expected_changes(const std::string& change, const std::vector<LiveSchemaTree::ChangeRecord>& changes) {
      $expect(changes.size()).toEqual(_mock_expected_changes.size(), _check_id + " : Unexpected number of objects "
                                      + change);

      for (size_t index = 0; index < changes.size(); index++) {
        $expect(changes[index].type).toEqual(_mock_expected_changes[index].type, _check_id +
                                             " : Unexpected object type has been " + change);
        $expect(changes[index].schema).toEqual(_mock_expected_changes[index].schema, _check_id +
                                               " : Unexpected schema has been " + change);
        $expect(changes[index].name).toEqual(_mock_expected_changes[index].name, _check_id +
                                             " : Unexpected object has been " + change);
        $expect(changes[index].detail).toEqual(_mock_expected_changes[index].detail, _check_id +
                                               " : Unexpected sub_object has been " + change);
      }

      _mock_expected_changes.clear();
    }

    virtual void tree_activate_objects(const std::string& action, const std::vector<LiveSchemaTree::ChangeRecord>& changes) {
      $expect(_expect_tree_activate_objects).toBeTrue(_check_id + " : Unexpected call to tree_activate_objects.");
      $expect(action).toEqual(_mock_expected_action, _check_id +
                              " : Unexpected action received on tree_activate_objects.");
      _expect_tree_activate_objects = false;
      $expect(action).toEqual(_mock_expected_action, _check_id + " : Unexpected action has been activated");
      check_expected_changes("activated", changes);
    }

    virtual void tree_alter_objects(const std::vector<LiveSchemaTree::ChangeRecord>& changes) {
      $expect(_expect_tree_alter_objects).toBeTrue(_check_id + " : Unexpected call to tree_alter_objects.");
      _expect_tree_alter_objects = false;
      check_expected_changes("altered", changes);
    }

    virtual void tree_create_object(LiveSchemaTree::ObjectType type, const std::string& schema_name,
                                    const std::string& object_name) {
      $expect(_expect_tree_create_object).toBeTrue(_check_id + " : Unexpected call to tree_create_object.");
      $expect(schema_name).toEqual(_mock_expected_changes[0].schema, _check_id + " : Unexpected schema name.");
      $expect(type).toEqual(_mock_expected_changes[0].type, _check_id + " : Unexpected object type.");
      $expect(object_name).toEqual(_mock_expected_changes[0].name, _check_id + " : Unexpected object name.");

      _mock_expected_changes.erase(_mock_expected_changes.begin());

      _expect_tree_create_object = false;
    }

    virtual void tree_drop_objects(const std::vector<LiveSchemaTree::ChangeRecord>& changes) {
      $expect(_expect_tree_drop_objects).toBeTrue(_check_id + " : Unexpected call to tree_drop_objects.");
      _expect_tree_drop_objects = false;
      check_expected_changes("dropped", changes);
    }

    void check_and_reset(const std::string& check_id) {
      $expect(_expect_fetch_schema_list_call).toBeFalse(check_id + " : Missed call to fetch_schema_list");
      $expect(_expect_fetch_schema_contents_call).toBeFalse(check_id + " : Missed call to fetch_schema_contents");
      $expect(_expect_fetch_object_details_call).toBeFalse(check_id + " : Missed call to fetch_object_details");
      $expect(_mock_expected_changes.size()).toEqual(0U, check_id + " : Missing expected changes.");

      $expect(_expect_tree_refresh).toBeFalse(check_id + " : Missed call to tree_refresh");
      $expect(_expect_tree_activate_objects).toBeFalse(check_id + " : Missed call to tree_activate_objects");
      $expect(_expect_tree_alter_objects).toBeFalse(check_id + " : Missed call to tree_alter_objects");
      $expect(_expect_tree_create_object).toBeFalse(check_id + " : Missed call to tree_create_object");
      $expect(_expect_tree_drop_objects).toBeFalse(check_id + " : Missed call to tree_drop_objects");
      $expect(_expect_plugin_item_call).toBeFalse(check_id + " : Missed call to plugin_item_call");
      $expect(_expect_fetch_data_for_filter).toBeFalse(check_id + " : Missed call to fetch_data_for_filter");

      _expect_fetch_schema_list_call = false;
      _expect_fetch_schema_contents_call = false;
      _expect_fetch_object_details_call = false;
      _expect_plugin_item_call = false;
    }
  };

  mforms::TreeView *pModelView;
  mforms::TreeView *pModelViewFiltered;
  LiveSchemaTreeTestHelper treeTestHelper;
  LiveSchemaTreeTestHelper treeTestHelperFiltered;

  std::shared_ptr<LiveTreeTestDelegate> delegate;
  std::shared_ptr<LiveTreeTestDelegate> delegateFiltered;

  GPatternSpec *schemaPattern = nullptr;
  GPatternSpec *objectPattern = nullptr;

  class DummyLST : public LiveSchemaTree::LSTData {
    virtual LiveSchemaTree::ObjectType get_type() {
      return LiveSchemaTree::Any;
    }

    virtual std::string get_object_name() {
      return "DummyLST";
    }
  };

  void fillBasicSchema(const std::string& check_id) {
    // Fills the tree using the real structure..
    base::StringListPtr schemas(new std::list<std::string>());
    mforms::TreeNodeRef node;

    schemas->push_back("schema1");

    // Fills a schema.
    treeTestHelper.update_schemata(schemas);
    node = treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, "");

    // Fills the schema content.
    delegate->expect_fetch_schema_contents_call();
    delegate->_mock_view_list->push_back("view1");
    delegate->_mock_table_list->push_back("table1");
    delegate->_mock_procedure_list->push_back("procedure1");
    delegate->_mock_function_list->push_back("function1");
    delegate->_mock_call_back_slot = true;
    delegate->_mock_schema_name = "schema1";
    delegate->_check_id = check_id;

    treeTestHelper.load_schema_content(node);

    delegate->check_and_reset(check_id);

    // Fills view column.
    delegate->_mock_schema_name = "schema1";
    delegate->_mock_object_name = "view1";
    delegate->_mock_object_type = LiveSchemaTree::View;
    delegate->_expect_fetch_object_details_call = true;
    delegate->_mock_column_list->push_back("view_column1");
    delegate->_mock_call_back_slot_columns = true;
    delegate->_check_id = check_id;

    treeTestHelper.load_table_details(LiveSchemaTree::View, "schema1", "view1", LiveSchemaTree::COLUMN_DATA);

    delegate->check_and_reset(check_id);

    // Fills table data...
    delegate->_mock_schema_name = "schema1";
    delegate->_mock_object_name = "table1";
    delegate->_mock_object_type = LiveSchemaTree::Table;
    delegate->_expect_fetch_object_details_call = true;
    delegate->_mock_column_list->clear();
    delegate->_mock_index_list->clear();
    delegate->_mock_column_list->push_back("table_column1");
    delegate->_mock_index_list->push_back("index1");
    delegate->_mock_trigger_list->push_back("trigger1");
    delegate->_mock_fk_list->push_back("fk1");
    delegate->_mock_call_back_slot_columns = true;
    delegate->_mock_call_back_slot_indexes = true;
    delegate->_mock_call_back_slot_triggers = true;
    delegate->_mock_call_back_slot_foreign_keys = true;
    delegate->_check_id = check_id;

    treeTestHelper.load_table_details(LiveSchemaTree::Table, "schema1", "table1",
      LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA | LiveSchemaTree::FK_DATA);

    delegate->check_and_reset(check_id);
  }

  void fillSchemaObjectLists() {
    delegate->_mock_view_list->clear();
    delegate->_mock_view_list->push_back("first_view");
    delegate->_mock_view_list->push_back("second_view");
    delegate->_mock_view_list->push_back("secure_view");
    delegate->_mock_view_list->push_back("third");

    delegate->_mock_table_list->clear();
    delegate->_mock_table_list->push_back("customer");
    delegate->_mock_table_list->push_back("client");
    delegate->_mock_table_list->push_back("store");
    delegate->_mock_table_list->push_back("product");

    delegate->_mock_procedure_list->clear();
    delegate->_mock_procedure_list->push_back("get_debths");
    delegate->_mock_procedure_list->push_back("get_payments");
    delegate->_mock_procedure_list->push_back("get_lazy");

    delegate->_mock_function_list->clear();
    delegate->_mock_function_list->push_back("calc_income");
    delegate->_mock_function_list->push_back("calc_debth_list");
    delegate->_mock_function_list->push_back("dummy");
  }

  void fillComplexSchema(const std::string& check_id) {
    // Fills the tree using the real structure.
    base::StringListPtr schemas(new std::list<std::string>());
    mforms::TreeNodeRef node;
    delegate->_check_id = check_id;

    schemas->push_back("test_schema");
    schemas->push_back("basic_schema");
    schemas->push_back("basic_training");
    schemas->push_back("dev_schema");

    // Fills a schema.
    treeTestHelper.update_schemata(schemas);

    // Fills the schema content.
    delegate->expect_fetch_schema_contents_call();
    fillSchemaObjectLists();
    delegate->_mock_call_back_slot = true;
    delegate->_mock_schema_name = "test_schema";
    node = treeTestHelper.get_node_for_object("test_schema", LiveSchemaTree::Schema, "");
    treeTestHelper.load_schema_content(node);

    delegate->expect_fetch_schema_contents_call();
    fillSchemaObjectLists();
    delegate->_mock_call_back_slot = true;
    delegate->_mock_schema_name = "basic_schema";
    node = treeTestHelper.get_node_for_object("basic_schema", LiveSchemaTree::Schema, "");
    treeTestHelper.load_schema_content(node);

    delegate->expect_fetch_schema_contents_call();
    fillSchemaObjectLists();
    delegate->_mock_call_back_slot = true;
    delegate->_mock_schema_name = "basic_training";
    node = treeTestHelper.get_node_for_object("basic_training", LiveSchemaTree::Schema, "");
    treeTestHelper.load_schema_content(node);

    delegate->expect_fetch_schema_contents_call();
    fillSchemaObjectLists();
    delegate->_mock_call_back_slot = true;
    delegate->_mock_schema_name = "dev_schema";
    node = treeTestHelper.get_node_for_object("dev_schema", LiveSchemaTree::Schema, "");
    treeTestHelper.load_schema_content(node);

    delegate->check_and_reset(check_id);

    // Fills view column.
    delegate->_mock_column_list->push_back("view_col1");
    delegate->_mock_column_list->push_back("view_col2");
    delegate->_mock_column_list->push_back("view_col3");
    delegate->_mock_column_list->push_back("view_col4");

    delegate->_mock_schema_name = "test_schema";
    delegate->_mock_object_name = "first_view";
    delegate->_mock_object_type = LiveSchemaTree::View;

    std::list<std::string> view_list;
    view_list.push_back("first_view");
    view_list.push_back("second_view");
    view_list.push_back("secure_view");
    view_list.push_back("third");

    std::list<std::string>::iterator v_index, v_end = view_list.end();
    for (v_index = view_list.begin(); v_index != v_end; v_index++) {
      delegate->_expect_fetch_object_details_call = true;
      delegate->_mock_call_back_slot_columns = true;
      delegate->_mock_object_name = *v_index;
      treeTestHelper.load_table_details(LiveSchemaTree::View, "test_schema", *v_index, LiveSchemaTree::COLUMN_DATA);
      delegate->check_and_reset(check_id);
    }

    // Fills table data.
    delegate->_mock_schema_name = "test_schema";
    delegate->_mock_object_type = LiveSchemaTree::Table;
    delegate->_mock_column_list->clear();
    delegate->_mock_index_list->clear();
    delegate->_mock_column_list->push_back("id");
    delegate->_mock_column_list->push_back("name");
    delegate->_mock_column_list->push_back("relation");
    delegate->_mock_index_list->push_back("primary_key");
    delegate->_mock_index_list->push_back("name_unique");
    delegate->_mock_trigger_list->push_back("a_trigger");
    delegate->_mock_fk_list->push_back("some_fk");
    delegate->_mock_call_back_slot_columns = true;
    delegate->_mock_call_back_slot_indexes = true;
    delegate->_mock_call_back_slot_triggers = true;
    delegate->_mock_call_back_slot_foreign_keys = true;

    std::list<std::string> table_list;
    table_list.push_back("customer");
    table_list.push_back("client");
    table_list.push_back("store");
    table_list.push_back("product");

    std::list<std::string>::iterator t_index, t_end = table_list.end();
    for (t_index = table_list.begin(); t_index != t_end; t_index++) {
      delegate->_mock_object_name = *t_index;
      delegate->_expect_fetch_object_details_call = true;
      delegate->_mock_call_back_slot_columns = true;
      delegate->_mock_call_back_slot_indexes = true;
      delegate->_mock_call_back_slot_triggers = true;
      delegate->_mock_call_back_slot_foreign_keys = true;
      treeTestHelper.load_table_details(LiveSchemaTree::Table, "test_schema", *t_index,
        LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA | LiveSchemaTree::FK_DATA);
      delegate->check_and_reset(check_id);
    }
  }

  void checkGetSchemaNameRecursive(LiveSchemaTree* lst, mforms::TreeNodeRef root) {
    $expect(lst->get_schema_name(root)).toEqual("schema1");

    for (int index = 0; index < root->count(); index++) {
      checkGetSchemaNameRecursive(lst, root->get_child(index));
    }
  }

  void checkNodePathsRecursive(LiveSchemaTree* lst, mforms::TreeNodeRef root) {
    std::vector<std::string> path = lst->get_node_path(root);
    mforms::TreeNodeRef other_node = lst->get_node_from_path(path);

    $expect(root.ptr()).toEqual(other_node.ptr());

    for (int index = 0; index < root->count(); index++) {
      checkNodePathsRecursive(lst, root->get_child(index));
    }
  }

  void setNodes(LiveSchemaTree* lst, std::list<mforms::TreeNodeRef>& nodes, int flags) {
    mforms::TreeNodeRef schema_node = lst->get_node_for_object("schema1", LiveSchemaTree::Schema, "");
    mforms::TreeNodeRef object_node;
    if (SCHEMA & flags)
      nodes.push_back(schema_node);

    if (TABLES & flags)
      nodes.push_back(schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX));

    object_node = schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0);

    if (TABLE & flags)
      nodes.push_back(object_node);

    if (COLUMNS & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX));

    if (TABLE_COLUMN & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->get_child(0));

    if (INDEXES & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX));

    if (INDEX & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->get_child(0));

    if (TRIGGERS & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX));

    if (TRIGGER & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->get_child(0));

    if (FKS & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX));

    if (FK & flags)
      nodes.push_back(object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->get_child(0));

    if (VIEWS & flags)
      nodes.push_back(schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX));

    object_node = schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(0);

    if (VIEW & flags)
      nodes.push_back(object_node);

    if (VIEW_COLUMN & flags)
      nodes.push_back(object_node->get_child(0));

    if (PROCEDURES & flags)
      nodes.push_back(schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX));

    if (FUNCTION & flags)
      nodes.push_back(schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(0));
  }

  bool ensureItemExists(const bec::MenuItemList& items, const std::string& item_caption) {
    bool found = false;

    for (size_t index = 0; !found && index < items.size(); index++) {
      found = (items[index].caption == item_caption);
    }

    return found;
  }

  bool ensureSubItemExists(const std::string& item_caption, const bec::MenuItemList& items,
                           const std::string& subitem_caption) {
    bool found = false;

    for (size_t index = 0; !found && index < items.size(); index++) {
      if (items[index].caption == item_caption)
        found = ensureItemExists(items[index].subitems, subitem_caption);
    }

    return found;
  }

  void ensureMenuItemsExist(const std::string check, const bec::MenuItemList& items, int main_items, int sub_items,
                            const std::string& single, const std::string& multi) {
    std::string custom_caption = single;

    if (SET_DEF_SCH & main_items)
      $expect(ensureItemExists(items, "Set as Default Schema")).toBeTrue(check + ": Expected \"Set as Default Schema\" menu item not found");

    if (FIL_TO_SCH & main_items)
      $expect(ensureItemExists(items, "Filter to This Schema")).toBeTrue(check + ": Expected \"Filter to This Schema\" menu item not found");

    if (COPY_TC & main_items)
      $expect(ensureItemExists(items, "Copy to Clipboard")).toBeTrue(check + ": Expected \"Copy to Clipboard\" menu item not found");

    if (SEND_TE & main_items)
      $expect(ensureItemExists(items, "Send to SQL Editor")).toBeTrue(check + ": Expected \"Send to SQL Editor\" menu item not found");

    if (CREATE & main_items)
      $expect(ensureItemExists(items, "Create " + custom_caption + "...")).toBeTrue(check + ": Expected \"Create " + custom_caption + "...\" menu item not found");

    if (multi.length() > 0)
      custom_caption = multi;

    if (ALTER & main_items)
      $expect(ensureItemExists(items, "Alter " + custom_caption + "...")).toBeTrue(check + ": Expected \"Alter " + custom_caption + "...\" menu item not found");

    if (DROP & main_items)
      $expect(ensureItemExists(items, "Drop " + custom_caption + "...")).toBeTrue(check + ": Expected \"Drop " + custom_caption + "...\" menu item not found");

    if (REFRESH & main_items)
      $expect(ensureItemExists(items, "Refresh All")).toBeTrue(check + ": Expected \"Refresh All\" menu item not found");

    if (SEL_ROWS & main_items)
      $expect(ensureItemExists(items, "Select Rows")).toBeTrue(check + ": Expected \"Select Rows\" menu item not found");

    if (EDIT & main_items)
      $expect(ensureItemExists(items, "Edit Table Data")).toBeTrue(check + ": Expected \"Edit Table Data\" menu item not found");

    if (SUB_NAME & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Name")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Name\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Name")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Name\" menu item not found");
    }

    if (SUB_NAME_S & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Name (short)")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Name (short)\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Name (short)")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Name (short)\" menu item not found");
    }

    if (SUB_NAME_L & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Name (long)")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Name (long)\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Name (long)")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Name (long)\" menu item not found");
    }

    if (SUB_SEL_ALL & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Select All Statement")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Select All Statement\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Select All Statement")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Select All Statement\" menu item not found");
    }

    if (SUB_SEL_COL & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Select Columns Statement")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Select Columns Statement\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Select Columns Statement")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Select Columns Statement\" menu item not found");
    }

    if (SUB_CREATE & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Create Statement")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Create Statement\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Create Statement")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Create Statement\" menu item not found");
    }

    if (SUB_INSERT & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Insert Statement")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Insert Statement\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Insert Statement")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Insert Statement\" menu item not found");
    }

    if (SUB_UPDATE & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Update Statement")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Update Statement\" menu item not");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Update Statement")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Update Statement\" menu item not found");
    }

    if (SUB_DELETE & sub_items) {
      $expect(ensureSubItemExists("Copy to Clipboard", items, "Delete Statement")).toBeTrue(check + ": Expected \"Copy to Clipboard\\Delete Statement\" menu item not found");
      $expect(ensureSubItemExists("Send to SQL Editor", items, "Delete Statement")).toBeTrue(check + ": Expected \"Send to SQL Editor\\Delete Statement\" menu item not found");
    }
  }

  void setChangeRecords(std::vector<LiveSchemaTree::ChangeRecord>& change_records, int flags) {
    if (SCHEMA & flags) {
      LiveSchemaTree::ChangeRecord change = {LiveSchemaTree::Schema, "", "schema1", ""};
      change_records.push_back(change);
    }
    if (TABLE & flags) {
      LiveSchemaTree::ChangeRecord change = {LiveSchemaTree::Table, "schema1", "table1", ""};
      change_records.push_back(change);
    }
    if (VIEW & flags) {
      LiveSchemaTree::ChangeRecord change = {LiveSchemaTree::View, "schema1", "view1", ""};
      change_records.push_back(change);
    }
    if (PROCEDURE & flags) {
      LiveSchemaTree::ChangeRecord change = {LiveSchemaTree::Procedure, "schema1", "procedure1", ""};
      change_records.push_back(change);
    }
    if (FUNCTION & flags) {
      LiveSchemaTree::ChangeRecord change = {LiveSchemaTree::Function, "schema1", "function1", ""};
      change_records.push_back(change);
    }
  }

  void setPatterns(const std::string& filter) {
    std::vector<std::string> filters = base::split(filter, ".", 2);

    if (schemaPattern) {
      g_pattern_spec_free(schemaPattern);
      schemaPattern = nullptr;
    }

    if (objectPattern) {
      g_pattern_spec_free(objectPattern);
      objectPattern = nullptr;
    }

    // Creates the schema/table patterns.
    schemaPattern = g_pattern_spec_new(base::toupper(filters[0]).c_str());
    if (filters.size() > 1)
      objectPattern = g_pattern_spec_new(base::toupper(filters[1]).c_str());
  }

  void verifyFilterResult(const std::string& check, mforms::TreeNodeRef root, const std::vector<std::string>& schemas,
                          const std::vector<std::string>& tables, const std::vector<std::string>& views,
                          const std::vector<std::string>& procedures, const std::vector<std::string>& functions) {
    mforms::TreeNodeRef schema_node_f;
    mforms::TreeNodeRef object_node_f;

    $expect(static_cast<size_t>(root->count())).toEqual(schemas.size(), check + ": Unexpected number of schema nodes after filtering");

    for (int schema_index = 0; schema_index < root->count(); schema_index++) {
      schema_node_f = root->get_child(schema_index);

      $expect(schema_node_f->get_string(0)).toEqual(schemas[schema_index], check + ": Unexpected schema name after filtering");

      $expect(schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count())
        .toEqual(static_cast<int>(tables.size()), check + ": Unexpected number of table nodes after filtering");
      $expect(schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count())
        .toEqual(static_cast<int>(views.size()), check + ": Unexpected number of view nodes after filtering");
      $expect(schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count())
        .toEqual(static_cast<int>(procedures.size()), check + ": Unexpected number of procedure nodes after filtering");
      $expect(schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count())
        .toEqual(static_cast<int>(functions.size()), check + ": Unexpected number of function nodes after filtering");

      for (int table_index = 0; table_index < schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count();
           table_index++) {
        object_node_f = schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(table_index);
        $expect(object_node_f->get_string(0)).toEqual(tables[table_index], check + ": Unexpected table node after filtering");
      }

      for (int view_index = 0; view_index < schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count();
           view_index++) {
        object_node_f = schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(view_index);
        $expect(object_node_f->get_string(0)).toEqual(views[view_index], check + ": Unexpected view node after filtering");
      }

      for (int procedure_index = 0;
           procedure_index < schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count();
           procedure_index++) {
        object_node_f = schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(procedure_index);
        $expect(object_node_f->get_string(0)).toEqual(procedures[procedure_index], check + ": Unexpected procedure node after filtering");
      }

      for (int function_index = 0;
           function_index < schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count(); function_index++) {
        object_node_f = schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(function_index);
        $expect(object_node_f->get_string(0)).toEqual(functions[function_index], check + ": Unexpected function node after filtering");
      }
    }
  }
};

$describe("Live Schema Tree") {

  $beforeAll([this]() {
    data->delegate.reset(new TestData::LiveTreeTestDelegate());
    data->delegateFiltered.reset(new TestData::LiveTreeTestDelegate());

    GRT::get()->set("/wb", DictRef(true));
    GRT::get()->set("/wb/options", DictRef(true));
    GRT::get()->set("/wb/options/options", DictRef(true));
    GRT::get()->set("/wb/options/options/SqlEditor:AutoFetchColumnInfo", IntegerRef(1));

    mforms::stub::init(nullptr);
    data->pModelView =
      new mforms::TreeView(mforms::TreeNoColumns | mforms::TreeNoBorder | mforms::TreeSidebar | mforms::TreeNoHeader);
    data->pModelViewFiltered =
      new mforms::TreeView(mforms::TreeNoColumns | mforms::TreeNoBorder | mforms::TreeSidebar | mforms::TreeNoHeader);

    data->treeTestHelper.set_model_view(data->pModelView);
    data->treeTestHelperFiltered.set_model_view(data->pModelViewFiltered);

    data->treeTestHelper.set_delegate(data->delegate);
    data->treeTestHelper.set_fetch_delegate(data->delegate);

    data->treeTestHelperFiltered.set_delegate(data->delegateFiltered);
    data->treeTestHelperFiltered.set_fetch_delegate(data->delegateFiltered);

    data->delegate->ptree = &data->treeTestHelper;
    data->delegateFiltered->ptree = &data->treeTestHelperFiltered;
  });

  $afterAll([this]() {
    if (data->schemaPattern != nullptr) {
      g_pattern_spec_free(data->schemaPattern);
    }

    if (data->objectPattern != nullptr) {
      g_pattern_spec_free(data->objectPattern);
    }
  });

  $it("General node tests", [this]() {
    mforms::TreeNodeRef test_node_ref = data->pModelView->root_node();
    test_node_ref->set_string(0, "Dummy");

    // Adds a schema node

    // Testing copy and get details at LSTData (data root)
    {
      TestData::DummyLST source, target;
      source.details = "This is a sample";

      $expect(target.details).toEqual("");

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");

      $expect(target.get_details(false, test_node_ref)).toEqual("This is a sample");
      $expect(target.get_details(true, test_node_ref)).toEqual("<b>DummyLST:</b> <font color='#148814'><b>Dummy</b></font><br><br>");
    }

    // Testing a ColumnData node.
    {
      LiveSchemaTree::ColumnData source, target;
      source.details = "This is a sample";
      source.default_value = "A default value";
      source.is_fk = true;
      source.is_id = true;
      source.is_pk = true;

      $expect(target.get_object_name()).toEqual("Column");
      $expect(target.get_type()).toEqual(LiveSchemaTree::TableColumn);

      $expect(target.details).toEqual("");
      $expect(target.default_value).toEqual("");
      $expect(target.is_fk).toBeFalse();
      $expect(target.is_id).toBeFalse();
      $expect(target.is_pk).toBeFalse();

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");
      $expect(target.default_value).toEqual("A default value");
      $expect(target.is_fk).toBeTrue();
      $expect(target.is_id).toBeTrue();
      $expect(target.is_pk).toBeTrue();

      $expect(target.get_details(false, test_node_ref)).toEqual("This is a sample");
      $expect(target.get_details(true, test_node_ref)).toEqual("<b>Column:</b> <font color='#148814'><b>Dummy</b></font><br><br>"
        "<b>Definition:</b><table style=\"border: none; border-collapse: collapse;\">This is a sample</table><br><br>");
    }

    // Testing a ForeignKey node.
    {
      LiveSchemaTree::FKData source, target;
      source.details = "This is a sample to test copy";
      source.delete_rule = 1;
      source.update_rule = 5;
      source.referenced_table = "destino";
      source.from_cols = "one, two";
      source.to_cols = "uno, dos";

      $expect(target.get_object_name()).toEqual("Foreign Key");
      $expect(target.get_type()).toEqual(LiveSchemaTree::ForeignKey);

      $expect(target.details).toEqual("");
      $expect(target.delete_rule).toEqual(0);
      $expect(target.update_rule).toEqual(0);
      $expect(target.referenced_table).toEqual("");
      $expect(target.from_cols).toEqual("");
      $expect(target.to_cols).toEqual("");

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample to test copy");
      $expect(target.delete_rule).toEqual(1);
      $expect(target.update_rule).toEqual(5);
      $expect(target.referenced_table).toEqual("destino");
      $expect(target.from_cols).toEqual("one, two");
      $expect(target.to_cols).toEqual("uno, dos");

      // Clean details to test dynamic generation.
      target.details = "";

      std::string expected =
        "<table style=\"border: none; border-collapse: collapse;\">"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">Target</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>destino (one, two \xE2\x86\x92 uno, "
        "dos)</font></td>"
        "</tr>"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">On Update</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>NO ACTION</font></td>"
        "</tr>"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">On Delete</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>CASCADE</font></td>"
        "</tr>"
        "</table>";

      $expect(target.get_details(false, test_node_ref)).toEqual(expected);
      expected = "<b>Foreign Key:</b> <font color='#148814'><b>Dummy</b></font><br><br><b>Definition:</b><br>" + expected;
      $expect(target.get_details(true, test_node_ref)).toEqual(expected);
    }

    // Testing an Index node.
    {
      LiveSchemaTree::IndexData source, target;
      source.details = "This is a sample to test copy";
      source.columns.push_back("one");
      source.columns.push_back("two");
      source.type = 6;
      source.unique = true;

      $expect(target.get_object_name()).toEqual("Index");
      $expect(target.get_type()).toEqual(LiveSchemaTree::Index);

      $expect(target.details).toEqual("");
      $expect(target.columns.size()).toEqual(0U);
      $expect( target.type).toEqual(0);
      $expect(target.unique).toBeFalse();

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample to test copy");
      $expect(target.columns.size()).toEqual(2U);
      $expect(target.columns[0]).toEqual( "one");
      $expect(target.columns[1]).toEqual("two");
      $expect(target.type).toEqual(6);
      $expect(target.unique).toBeTrue();

      // Clean details to test dynamic generation.
      target.details = "";

      std::string expected =
        "<table style=\"border: none; border-collapse: collapse;\">"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">Type</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>BTREE</font></td>"
        "</tr>"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">Unique</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>Yes</font></td>"
        "</tr>"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">Visible</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>Yes</font></td>"
        "</tr>"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">Columns</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>one</font></td>"
        "</tr>"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\"></td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>two</font></td>"
        "</tr>"
        "</table>";

      $expect(target.get_details(false, test_node_ref)).toEqual(expected);
      expected = "<b>Index:</b> <font color='#148814'><b>Dummy</b></font><br><br><b>Definition:</b><br>" + expected;
      $expect(target.get_details(true, test_node_ref)).toEqual(expected);
    }

    // Testing copy and get_details for a Trigger node.
    {
      LiveSchemaTree::TriggerData source, target;
      source.details = "This is a sample to test copy";
      source.event_manipulation = 11;
      source.timing = 15;

      $expect(target.get_object_name()).toEqual("Trigger");
      $expect(target.get_type()).toEqual(LiveSchemaTree::Trigger);

      $expect(target.details).toEqual("");
      $expect(target.event_manipulation).toEqual(0);
      $expect(target.timing).toEqual(0);

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample to test copy");
      $expect(target.event_manipulation).toEqual(11);
      $expect(target.timing).toEqual(15);

      // Clean details to test dynamic generation.
      target.details = "";

      std::string expected =
        "<table style=\"border: none; border-collapse: collapse;\">"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">Event</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>INSERT</font></td>"
        "</tr>"
        "<tr>"
        "<td style=\"border:none; padding-left: 15px;\">Timing</td>"
        "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>AFTER</font></td>"
        "</tr>"
        "</table>";
      $expect(target.get_details(false, test_node_ref)).toEqual(expected);
      expected = "<b>Trigger:</b> <font color='#148814'><b>Dummy</b></font><br><br><b>Definition:</b><br>" + expected;
      $expect(target.get_details(true, test_node_ref)).toEqual(expected);
    }

    // Testing an Object node
    {
      LiveSchemaTree::ObjectData source, target;
      source.details = "This is a sample";
      source.fetched = true;
      source.fetching = true;

      $expect(target.get_object_name()).toEqual("Object");
      $expect(target.get_type()).toEqual(LiveSchemaTree::Any);

      $expect(target.details).toEqual("");
      $expect(target.fetched).toBeFalse();
      $expect(target.fetching).toBeFalse();

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");
      $expect(target.fetched).toBeTrue();
      $expect(target.fetching).toBeTrue();

      $expect(target.get_details(false, test_node_ref)).toEqual("This is a sample");
      $expect(target.get_details(true, test_node_ref)).toEqual("<b>Object:</b> <font color='#148814'><b>Dummy</b></font><br><br>");
    }

    // Testing a Function node
    // When parameter display was added, the details of a function will be
    // A concatenation between the formatted header, and the details which should be
    // html code with the parameter information
    {
      LiveSchemaTree::FunctionData source, target;
      source.details = "This is a sample";
      source.fetched = true;
      source.fetching = true;

      $expect(target.get_object_name()).toEqual("Function");
      $expect(target.get_type()).toEqual(LiveSchemaTree::Function);

      $expect(target.details).toEqual("");
      $expect(target.fetched).toBeFalse();
      $expect(target.fetching).toBeFalse();

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");
      $expect(target.fetched).toBeTrue();
      $expect(target.fetching).toBeTrue();

      $expect(target.get_details(false, test_node_ref)).toEqual("<b>Function:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
      $expect(target.get_details(true, test_node_ref)).toEqual("<b>Function:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
    }

    // Testing a Procedure node
    // When parameter display was added, the details of a procedure will be
    // A concatenation between the formatted header, and the details which should be
    // html code with the parameter information
    {
      LiveSchemaTree::ProcedureData source, target;
      source.details = "This is a sample";
      source.fetched = true;
      source.fetching = true;

      $expect(target.get_object_name()).toEqual("Procedure");
      $expect(target.get_type()).toEqual(LiveSchemaTree::Procedure);

      $expect(target.details).toEqual("");
      $expect(target.fetched).toBeFalse();
      $expect(target.fetching).toBeFalse();

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");
      $expect(target.fetched).toBeTrue();
      $expect(target.fetching).toBeTrue();

      $expect(target.get_details(false, test_node_ref)).toEqual("<b>Procedure:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
      $expect(target.get_details(true, test_node_ref)).toEqual("<b>Procedure:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
    }

    // Testing a View node
    {
      LiveSchemaTree::ViewData source, target;
      source.details = "This is a sample";
      source.columns_load_error = true;
      source.fetched = true;
      source.fetching = true;
      source._loaded_mask = 1;
      source._loading_mask = 1;

      $expect(target.get_object_name()).toEqual("View");
      $expect(target.get_type()).toEqual(LiveSchemaTree::View);

      $expect(target.details).toEqual("");
      $expect(target.columns_load_error).toBeFalse();
      $expect(target.fetched).toBeFalse();
      $expect(target.fetching).toBeFalse();
      $expect(target._loaded_mask).toEqual(0);
      $expect(target._loading_mask).toEqual(0);

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");
      $expect(target.columns_load_error).toBeTrue();
      $expect(target.fetched).toBeTrue();
      $expect(target.fetching).toBeTrue();
      $expect(target._loaded_mask).toEqual(1);
      $expect(target._loading_mask).toEqual(1);

      // Fills the tree using the real structure..
      base::StringListPtr schemas(new std::list<std::string>());
      mforms::TreeNodeRef schema;
      mforms::TreeNodeRef view;
      LiveSchemaTree::ViewData* pdata;

      schemas->push_back("one");

      data->treeTestHelper.update_schemata(schemas);
      schema = data->treeTestHelper.get_child_node(test_node_ref, "one");

      data->delegate->expect_fetch_schema_contents_call();
      data->delegate->_mock_view_list->push_back("view1");
      data->delegate->_mock_call_back_slot = true;
      data->delegate->_mock_schema_name = "one";
      data->delegate->_check_id = "TF001CHK009";
      data->treeTestHelper.load_schema_content(schema);

      data->delegate->check_and_reset("TF001CHK009");

      data->delegate->_mock_schema_name = "one";
      data->delegate->_mock_object_name = "view1";
      data->delegate->_mock_object_type = LiveSchemaTree::View;
      data->delegate->_expect_fetch_object_details_call = true;
      data->delegate->_mock_column_list->push_back("first_column");
      data->delegate->_mock_column_list->push_back("second_column");
      data->delegate->_mock_call_back_slot_columns = true;

      data->treeTestHelper.load_table_details(LiveSchemaTree::View, "one", "view1", LiveSchemaTree::COLUMN_DATA);

      data->delegate->check_and_reset("TF001CHK009");

      view = data->treeTestHelper.get_node_for_object("one", LiveSchemaTree::View, "view1");
      pdata = dynamic_cast<LiveSchemaTree::ViewData*>(view->get_data());

      $expect(pdata).Not.toBeNull();

      $expect(pdata->get_details(true, view)).toEqual("<b>View:</b> <font color='#148814'><b>view1</b></font><br><br>"
        "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
        "MOCK LOADED Column : first_column"
        "MOCK LOADED Column : second_column"
        "</table><br><br>");

      test_node_ref->remove_children();

      // Testing the flag setting logic.
      LiveSchemaTree::ViewData view_node;

      $expect(view_node.get_loaded_mask()).toEqual(0);
      $expect(view_node.get_loading_mask()).toEqual(0);

      view_node.set_loading_mask(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA |
                                 LiveSchemaTree::TRIGGER_DATA);
      $expect(view_node.get_loaded_mask()).toEqual(0);
      $expect(view_node.get_loading_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA |
              LiveSchemaTree::TRIGGER_DATA);

      view_node.set_loaded_data(LiveSchemaTree::COLUMN_DATA);
      $expect(view_node.get_loaded_mask()).toEqual((short)LiveSchemaTree::COLUMN_DATA);
      $expect(view_node.get_loading_mask()).toEqual(LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA | LiveSchemaTree::TRIGGER_DATA);

      view_node.set_loaded_data(LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loading_mask()).toEqual(LiveSchemaTree::FK_DATA | LiveSchemaTree::TRIGGER_DATA);

      view_node.set_loaded_data(LiveSchemaTree::FK_DATA);
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA);
      $expect(view_node.get_loading_mask()).toEqual((short)LiveSchemaTree::TRIGGER_DATA);

      view_node.set_loaded_data(LiveSchemaTree::TRIGGER_DATA);
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA | LiveSchemaTree::TRIGGER_DATA);
      $expect(view_node.get_loading_mask()).toEqual(0);

      view_node.set_unloaded_data(LiveSchemaTree::TRIGGER_DATA);
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA);
      $expect(view_node.get_loading_mask()).toEqual(0);

      view_node.set_unloaded_data(LiveSchemaTree::FK_DATA);
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loading_mask()).toEqual(0);

      view_node.set_unloaded_data(LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loaded_mask()).toEqual((short)LiveSchemaTree::COLUMN_DATA);
      $expect(view_node.get_loading_mask()).toEqual(0);

      view_node.set_unloaded_data(LiveSchemaTree::COLUMN_DATA);
      $expect(view_node.get_loaded_mask()).toEqual(0);
      $expect(view_node.get_loading_mask()).toEqual(0);

      // Tests the inconsistency scenarios.
      // Loading items get cleaned if they get loaded, tho if other items are to be set as loaded they are done.
      view_node.set_loading_mask(LiveSchemaTree::COLUMN_DATA);
      view_node.set_loaded_data(LiveSchemaTree::INDEX_DATA);
      $expect(view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();
      $expect(view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA)).toBeTrue();
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loading_mask()).toEqual(LiveSchemaTree::COLUMN_DATA);

      view_node.set_loaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      $expect(view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA)).toBeTrue();
      $expect(view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA)).toBeTrue();
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loading_mask()).toEqual(0);

      // In order to set data unloaded, must be at loaded state first.
      view_node.set_loading_mask(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      view_node.set_loaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      view_node.set_unloaded_data(LiveSchemaTree::FK_DATA);
      $expect(view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeTrue();;
      $expect(view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA)).toBeTrue();;
      $expect(view_node.get_loaded_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loading_mask()).toEqual(0);

      view_node.set_unloaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA);
      $expect(view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();
      $expect(view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA)).toBeTrue();
      $expect(view_node.get_loaded_mask()).toEqual((short)LiveSchemaTree::INDEX_DATA);
      $expect(view_node.get_loading_mask()).toEqual(0);
    }

    // Testing a Table node.
    {
      LiveSchemaTree::TableData source, target;
      source.details = "This is a sample";
      source.columns_load_error = true;
      source.fetched = true;
      source.fetching = true;
      source._loaded_mask = 1;
      source._loading_mask = 1;

      $expect(target.get_object_name()).toEqual("Table");
      $expect(target.get_type()).toEqual(LiveSchemaTree::Table);

      $expect(target.details).toEqual("");
      $expect(target.columns_load_error).toBeFalse();
      $expect(target.fetched).toBeFalse();
      $expect(target.fetching).toBeFalse();
      $expect(target._loaded_mask).toEqual(0);
      $expect(target._loading_mask).toEqual(0);

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");
      $expect(target.columns_load_error).toBeTrue();
      $expect(target.fetched).toBeTrue();
      $expect(target.fetching).toBeTrue();
      $expect(target._loaded_mask).toEqual(1);
      $expect(target._loading_mask).toEqual(1);

      // Fills the tree using the real structure..
      base::StringListPtr schemas(new std::list<std::string>());
      mforms::TreeNodeRef schema;
      mforms::TreeNodeRef table;
      LiveSchemaTree::TableData* pdata;

      schemas->push_back("one");

      data->treeTestHelper.update_schemata(schemas);
      schema = data->treeTestHelper.get_child_node(test_node_ref, "one");

      data->delegate->expect_fetch_schema_contents_call();
      data->delegate->_mock_table_list->push_back("table1");
      data->delegate->_mock_call_back_slot = true;
      data->delegate->_mock_schema_name = "one";
      data->delegate->_check_id = "TF001CHK011";
      data->treeTestHelper.load_schema_content(schema);

      data->delegate->check_and_reset("TF001CHK011");

      data->delegate->_mock_schema_name = "one";
      data->delegate->_mock_object_name = "table1";
      data->delegate->_mock_object_type = LiveSchemaTree::Table;
      data->delegate->_expect_fetch_object_details_call = true;
      data->delegate->_mock_fk_list->push_back("fk_1");
      data->delegate->_mock_fk_list->push_back("fk_2");
      data->delegate->_mock_call_back_slot_indexes = true;
      data->delegate->_mock_index_list->push_back("first_column");
      data->delegate->_mock_call_back_slot_triggers = true;
      data->delegate->_mock_trigger_list->push_back("trigger1");
      data->delegate->_mock_call_back_slot_columns = true;
      data->delegate->_mock_call_back_slot_foreign_keys = true;
      data->delegate->_mock_column_list->push_back("first_column");
      data->delegate->_mock_column_list->push_back("second_column");

      data->treeTestHelper.load_table_details(LiveSchemaTree::Table, "one", "table1",
        LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA | LiveSchemaTree::TRIGGER_DATA |
        LiveSchemaTree::INDEX_DATA);

      data->delegate->check_and_reset("TF001CHK011");

      table = data->treeTestHelper.get_node_for_object("one", LiveSchemaTree::Table, "table1");
      pdata = dynamic_cast<LiveSchemaTree::TableData*>(table->get_data());

      $expect(pdata).Not.toBeNull();
      $expect(pdata->get_details(true, table)).toEqual("<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
        "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
        "MOCK LOADED Column : first_column"
        "MOCK LOADED Column : second_column"
        "</table><br><br>"
        "<div><b>Related Tables:</b></div>"
        "MOCK LOADED Foreign Key : fk_1"
        "MOCK LOADED Foreign Key : fk_2");
    }

    // Testing copy and get_details for a Schema node.
    {
      LiveSchemaTree::SchemaData source, target;
      source.details = "This is a sample";
      source.fetched = true;
      source.fetching = true;

      $expect(target.get_object_name()).toEqual("Schema");
      $expect(target.get_type()).toEqual(LiveSchemaTree::Schema);

      $expect(target.details).toEqual("");
      $expect(target.fetched).toBeFalse();
      $expect(target.fetching).toBeFalse();

      target.copy(&source);
      $expect(target.details).toEqual("This is a sample");
      $expect(target.fetched).toBeTrue();
      $expect(target.fetching).toBeTrue();

      $expect(target.get_details(false, test_node_ref)).toEqual("This is a sample");
      $expect(target.get_details(true, test_node_ref)).toEqual("<b>Schema:</b> <font color='#148814'><b>Dummy</b></font><br><br>");
    }
  });

  $it("Setting up nodes, inclusive their icons", [this]() {
    std::string path;
    mforms::TreeNodeRef node = data->pModelView->root_node();

    // Testing SchemaNode.
    {
      LiveSchemaTree::SchemaData* pdata = nullptr;
      LiveSchemaTree::SchemaData* pdata_temp = new LiveSchemaTree::SchemaData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data pointer is passed, it is used
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Schema, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Schema);
      pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Schema);

      pdata->fetching = true;
      data->treeTestHelper.update_node_icon(temp_node);
      path = bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.Schema.loading.side.$.png", bec::Icon16));
      $expect(temp_node->get_string(1)).toEqual(path);

      pdata->fetched = true;
      data->treeTestHelper.update_node_icon(temp_node);
      path = bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.Schema.side.$.png", bec::Icon16));
      $expect(temp_node->get_string(1)).toEqual(path);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing TableNode.
    {
      LiveSchemaTree::TableData* pdata = nullptr;
      LiveSchemaTree::TableData* pdata_temp = new LiveSchemaTree::TableData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data pointer is passed, it is used
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Table, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::TableData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Table);
      pdata = dynamic_cast<LiveSchemaTree::TableData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Table);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing ViewNode.
    {
      LiveSchemaTree::ViewData* pdata = nullptr;
      LiveSchemaTree::ViewData* pdata_temp = new LiveSchemaTree::ViewData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::View, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::ViewData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::View);
      pdata = dynamic_cast<LiveSchemaTree::ViewData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::View);

      // Checking for icon setup.
      pdata->columns_load_error = true;
      data->treeTestHelper.update_node_icon(temp_node);
      path = bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.View.broken.side.$.png", bec::Icon16));
      $expect(temp_node->get_string(1)).toEqual(path);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing ProcedureNode.
    {
      LiveSchemaTree::ProcedureData* pdata = nullptr;
      LiveSchemaTree::ProcedureData* pdata_temp = new LiveSchemaTree::ProcedureData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Procedure, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::ProcedureData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Procedure);
      pdata = dynamic_cast<LiveSchemaTree::ProcedureData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Procedure);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing FunctionNode.
    {
      LiveSchemaTree::FunctionData* pdata = nullptr;
      LiveSchemaTree::FunctionData* pdata_temp = new LiveSchemaTree::FunctionData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Function, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::FunctionData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Function);
      pdata = dynamic_cast<LiveSchemaTree::FunctionData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Function);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing ViewColumnNode.
    {
      LiveSchemaTree::ColumnData* pdata = nullptr;
      LiveSchemaTree::ColumnData* pdata_temp = new LiveSchemaTree::ColumnData(LiveSchemaTree::ViewColumn);
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data it is requested to not create data if not passed.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::ViewColumn, NULL, true);
      $expect(temp_node->get_data()).toBeNull();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::ViewColumn, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::ViewColumn);
      pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::ViewColumn);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing TableColumnNode.
    {
      LiveSchemaTree::ColumnData* pdata = nullptr;
      LiveSchemaTree::ColumnData* pdata_temp = new LiveSchemaTree::ColumnData(LiveSchemaTree::TableColumn);
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::TableColumn, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::TableColumn);
      pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::TableColumn);

      // Checking for icon setup.
      pdata->is_fk = true;
      data->treeTestHelper.update_node_icon(temp_node);
      path = bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.Column.fk.side.$.png", bec::Icon16));
      $expect(temp_node->get_string(1)).toEqual(path);

      pdata->is_pk = true;
      data->treeTestHelper.update_node_icon(temp_node);
      path = bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.Column.pk.side.$.png", bec::Icon16));
      $expect(temp_node->get_string(1)).toEqual(path);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing IndexNode.
    {
      LiveSchemaTree::IndexData* pdata = nullptr;
      LiveSchemaTree::IndexData* pdata_temp = new LiveSchemaTree::IndexData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Index, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::IndexData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Index);
      pdata = dynamic_cast<LiveSchemaTree::IndexData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Index);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing TriggerNode.
    {
      LiveSchemaTree::TriggerData* pdata = nullptr;
      LiveSchemaTree::TriggerData* pdata_temp = new LiveSchemaTree::TriggerData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data it is requested to not create data if not passed.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Trigger, NULL, true);
      $expect(temp_node->get_data()).toBeNull();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Trigger, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::TriggerData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::Trigger);
      pdata = dynamic_cast<LiveSchemaTree::TriggerData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Trigger);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }

    // Testing ForeignKeyNode.
    {
      LiveSchemaTree::FKData* pdata = nullptr;
      LiveSchemaTree::FKData* pdata_temp = new LiveSchemaTree::FKData();
      mforms::TreeNodeRef temp_node = node->add_child();

      // Testing when a data it is requested to not create data if not passed.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::ForeignKey, NULL, true);
      $expect(temp_node->get_data()).toBeNull();

      // Testing when a data pointer is passed, it is used.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::ForeignKey, pdata_temp);
      pdata = dynamic_cast<LiveSchemaTree::FKData*>(temp_node->get_data());
      $expect(pdata).toEqual(pdata_temp);

      // Testing without data so a new instance is created.
      data->treeTestHelper.setup_node(temp_node, LiveSchemaTree::ForeignKey);
      pdata = dynamic_cast<LiveSchemaTree::FKData*>(temp_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect( pdata).Not.toEqual(pdata_temp);
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::ForeignKey);

      node->remove_children();
      delete pdata;
      delete pdata_temp;
    }
  });

  $it("Binary + sequential node search", [this]() {
    // The Binary Search is used for schemas, tables, views and routines for
    // which the assumption is that the list of elements is sorted.
    //
    // The Sequential search is for the rest of the objects which should keep
    // the order in which they were created. If specified, the type parameter will
    // enforce that the searched node is of the specified type. This may not be used
    // as all the nodes containing unsorted nodes have objects of the same type.

    LiveSchemaTree::LSTData* pdata = nullptr;
    mforms::TreeNodeRef node = data->pModelView->root_node();
    mforms::TreeNodeRef child01 = node->add_child();
    mforms::TreeNodeRef child02 = node->add_child();
    mforms::TreeNodeRef child03 = node->add_child();
    mforms::TreeNodeRef child04 = node->add_child();
    mforms::TreeNodeRef child05 = node->add_child();
    mforms::TreeNodeRef found_node;

    // All the nodes are added to the tree root as we are not
    // testing the structure but the search operation.
    data->treeTestHelper.setup_node(child01, LiveSchemaTree::Schema);
    data->treeTestHelper.setup_node(child02, LiveSchemaTree::Table);
    data->treeTestHelper.setup_node(child03, LiveSchemaTree::View);
    data->treeTestHelper.setup_node(child04, LiveSchemaTree::Table);
    data->treeTestHelper.setup_node(child05, LiveSchemaTree::View);

    child01->set_string(0, "uno");
    child02->set_string(0, "uno");
    child03->set_string(0, "uno");
    child04->set_string(0, "dos");
    child05->set_string(0, "dos");

    {
      found_node = data->treeTestHelper.get_child_node(node, "uno", LiveSchemaTree::Schema, false);
      $expect(found_node).toEqual(child01);
      $expect(found_node->get_string(0)).toEqual(child01->get_string(0));
      $expect(found_node->get_data()).toEqual(child01->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Schema);
    }

    {
      found_node = data->treeTestHelper.get_child_node(node, "uno", LiveSchemaTree::Table, false);
      $expect(found_node).toEqual(child02);
      $expect(found_node->get_string(0)).toEqual(child02->get_string(0));
      $expect(found_node->get_data()).toEqual(child02->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Table);
    }

    {
      found_node = data->treeTestHelper.get_child_node(node, "uno", LiveSchemaTree::View, false);
      $expect(found_node).toEqual(child03);
      $expect(found_node->get_string(0)).toEqual(child03->get_string(0));
      $expect(found_node->get_data()).toEqual(child03->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::View);
    }

    {
      found_node = data->treeTestHelper.get_child_node(node, "dos", LiveSchemaTree::Table, false);
      $expect(found_node).toEqual(child04);
      $expect(found_node->get_string(0)).toEqual(child04->get_string(0));
      $expect(found_node->get_data()).toEqual(child04->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Table);
    }

    {
      found_node = data->treeTestHelper.get_child_node(node, "dos", LiveSchemaTree::View, false);
      $expect(found_node).toEqual(child05);
      $expect(found_node->get_string(0)).toEqual(child05->get_string(0));
      $expect(found_node->get_data()).toEqual(child05->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::View);
    }

    {
      found_node = data->treeTestHelper.get_child_node(node, "tres", LiveSchemaTree::View, false);
      $expect(found_node.is_valid()).toBeFalse();
    }

    // Now we create a series of tables.
    std::vector<mforms::TreeNodeRef> tables;
    for (size_t index = 0; index < 50; index++) {
      tables.push_back(child01->add_child());
      data->treeTestHelper.setup_node(tables[index], LiveSchemaTree::Table);
      tables[index]->set_string(0, base::strfmt("Table%02d", (int)(index + 1)));
    }

    {
      found_node = data->treeTestHelper.get_child_node(child01, "Table01", LiveSchemaTree::Table, true);
      $expect(found_node).toEqual(tables[0]);
      $expect(found_node->get_string(0)).toEqual(tables[0]->get_string(0));
      $expect(found_node->get_data()).toEqual(tables[0]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Table);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child01, "Table26", LiveSchemaTree::Table, true);
      $expect(found_node).toEqual(tables[25]);
      $expect(found_node->get_string(0)).toEqual(tables[25]->get_string(0));
      $expect(found_node->get_data()).toEqual(tables[25]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Table);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child01, "Table50", LiveSchemaTree::Table, true);
      $expect(found_node).toEqual(tables[49]);
      $expect(found_node->get_string(0)).toEqual(tables[49]->get_string(0));
      $expect(found_node->get_data()).toEqual(tables[49]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Table);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child01, "Table51", LiveSchemaTree::View, true);
      $expect(found_node.is_valid()).toBeFalse();
    }

    // Now we create a series of procedures and functions.
    std::vector<mforms::TreeNodeRef> procedures;
    for (size_t index = 0; index < 25; index++) {
      procedures.push_back(child02->add_child());
      data->treeTestHelper.setup_node(procedures[index], LiveSchemaTree::Procedure);
      procedures[index]->set_string(0, base::strfmt("Procedure%02d", (int)(index + 1)));
    }

    {
      found_node = data->treeTestHelper.get_child_node(child02, "Procedure01", LiveSchemaTree::Procedure, true);
      $expect(found_node).toEqual(procedures[0]);
      $expect(found_node->get_string(0)).toEqual(procedures[0]->get_string(0));
      $expect(found_node->get_data()).toEqual(procedures[0]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Procedure);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child02, "Procedure13", LiveSchemaTree::Procedure, true);
      $expect(found_node).toEqual(procedures[12]);
      $expect(found_node->get_string(0)).toEqual(procedures[12]->get_string(0));
      $expect(found_node->get_data()).toEqual(procedures[12]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Procedure);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child02, "Procedure25", LiveSchemaTree::Procedure, true);
      $expect(found_node).toEqual(procedures[24]);
      $expect(found_node->get_string(0)).toEqual(procedures[24]->get_string(0));
      $expect(found_node->get_data()).toEqual(procedures[24]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Procedure);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child02, "Procedure26", LiveSchemaTree::Procedure, true);
      $expect(found_node.is_valid()).toBeFalse();
    }

    std::vector<mforms::TreeNodeRef> functions;
    for (size_t index = 0; index < 25; index++) {
      functions.push_back(child03->add_child());
      data->treeTestHelper.setup_node(functions[index], LiveSchemaTree::Function);
      functions[index]->set_string(0, base::strfmt("Function%02d", (int)(index + 1)));
    }

    {
      found_node = data->treeTestHelper.get_child_node(child03, "Function01", LiveSchemaTree::Function, true);
      $expect(found_node).toEqual(functions[0]);
      $expect(found_node->get_string(0)).toEqual(functions[0]->get_string(0));
      $expect(found_node->get_data()).toEqual(functions[0]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Function);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child03, "Function13", LiveSchemaTree::Function, true);
      $expect(found_node).toEqual(functions[12]);
      $expect(found_node->get_string(0)).toEqual(functions[12]->get_string(0));
      $expect(found_node->get_data()).toEqual(functions[12]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Function);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child03, "Function25", LiveSchemaTree::Function, true);
      $expect(found_node).toEqual(functions[24]);
      $expect(found_node->get_string(0)).toEqual(functions[24]->get_string(0));
      $expect(found_node->get_data()).toEqual(functions[24]->get_data());
      pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
      $expect( pdata->get_type()).toEqual(LiveSchemaTree::Function);
    }

    {
      found_node = data->treeTestHelper.get_child_node(child03, "Function26", LiveSchemaTree::Function, true);
      $expect(found_node.is_valid()).toBeFalse();
    }
  });

  $it("Update node children", [this]() {
    mforms::TreeNodeRef node = data->pModelView->root_node();
    mforms::TreeNodeRef node_filtered = data->pModelView->root_node();
    base::StringListPtr children01(new std::list<std::string>());
    base::StringListPtr children02(new std::list<std::string>());
    children01->push_back("actor");
    children01->push_back("address");
    children01->push_back("client");
    children02->push_back("client");
    children02->push_back("film");
    children02->push_back("movie");

    // Clears the node to have a clean start of the test.
    node->remove_children();
    $expect(node->count()).toEqual(0);

    // The first update will add the client nodes into the root.
    $expect(data->treeTestHelper.update_node_children(node, children01, LiveSchemaTree::Schema));
    $expect(node->count()).toEqual(3);
    $expect(data->treeTestHelper.get_child_node(node, "actor"));
    $expect(data->treeTestHelper.get_child_node(node, "address"));
    $expect(data->treeTestHelper.get_child_node(node, "client"));

    // Testing an operation that will result in no changes.
    $expect(data->treeTestHelper.update_node_children(node, children01, LiveSchemaTree::Schema)).toBeFalse();
    $expect(node->count()).toEqual(3);

    // Testing an update removing nodes for unexisting names and appending new nodes.
    $expect(data->treeTestHelper.update_node_children(node, children02, LiveSchemaTree::Schema, true)).toBeTrue();
    $expect(node->count()).toEqual(3);
    $expect(data->treeTestHelper.get_child_node(node, "actor").is_valid()).toBeFalse();
    $expect(data->treeTestHelper.get_child_node(node, "address").is_valid()).toBeFalse();
    $expect(data->treeTestHelper.get_child_node(node, "client").is_valid()).toBeTrue();
    $expect(data->treeTestHelper.get_child_node(node, "film").is_valid()).toBeTrue();
    $expect(data->treeTestHelper.get_child_node(node, "movie").is_valid()).toBeTrue();

    children01->push_back("actor");
    children01->push_back("address");
    children01->push_back("client");

    // Testing an update removing nodes for unexisting names and appending new nodes.
    $expect(data->treeTestHelper.update_node_children(node, children01, LiveSchemaTree::Schema, true, true)).toBeTrue();
    $expect(node->count()).toEqual(5);
    $expect(data->treeTestHelper.get_child_node(node, "actor").is_valid()).toBeTrue();
    $expect(data->treeTestHelper.get_child_node(node, "address").is_valid()).toBeTrue();
    $expect(data->treeTestHelper.get_child_node(node, "client").is_valid()).toBeTrue();
    $expect(data->treeTestHelper.get_child_node(node, "film").is_valid()).toBeTrue();
    $expect(data->treeTestHelper.get_child_node(node, "movie").is_valid()).toBeTrue();

    // Repeat the tests using a filtered tree.
    data->treeTestHelperFiltered.set_base(&data->treeTestHelper);
    data->treeTestHelperFiltered.set_filter("*e*.*");
    data->treeTestHelperFiltered.filter_data();

    node_filtered = data->pModelViewFiltered->root_node();

    $expect(node_filtered->count()).toEqual(3);

    // The first update will add the client nodes into the root.
    children01->push_back("filtered");
    children01->push_back("finally");
    children01->push_back("done");
    $expect(data->treeTestHelperFiltered.update_node_children(node_filtered, children01, LiveSchemaTree::Schema, true, true)).toBeTrue();
    $expect(node->count()).toEqual(8);
    $expect(node_filtered->count()).toEqual(5);
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "address").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "client").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "done").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "filtered").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "movie").is_valid()).toBeTrue();

    // Testing an operation that will result in no changes.
    $expect(data->treeTestHelperFiltered.update_node_children(node_filtered, children01, LiveSchemaTree::Schema, true, true)).toBeFalse();
    $expect(node->count()).toEqual(8);
    $expect(node_filtered->count()).toEqual(5);

    children02->push_back("client");
    children02->push_back("customer");

    // Testing an update removing nodes for unexisting names and appending new nodes.
    $expect(data->treeTestHelperFiltered.update_node_children(node_filtered, children02, LiveSchemaTree::Schema, true, false)).toBeTrue();
    $expect(node->count()).toEqual(4);
    $expect(node_filtered->count()).toEqual(3);
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "client").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "customer").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "movie").is_valid()).toBeTrue();

    children01->push_back("actor");
    children01->push_back("address");
    children01->push_back("client");
    children01->push_back("film");
    children01->push_back("filtered");
    children01->push_back("finally");
    children01->push_back("movie");
    children01->push_back("done");

    // Testing an update removing nodes for unexisting names and appending new nodes.
    $expect(data->treeTestHelperFiltered.update_node_children(node_filtered, children01, LiveSchemaTree::Schema, true, false)).toBeTrue();
    $expect(node->count()).toEqual(8);
    $expect(node_filtered->count()).toEqual(5);
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "address").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "client").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "filtered").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "movie").is_valid()).toBeTrue();
    $expect(data->treeTestHelperFiltered.get_child_node(node_filtered, "done").is_valid()).toBeTrue();

    node->remove_children();
    node_filtered->remove_children();
  });

  $it("Activating a schema", [this]() {
    mforms::TreeNodeRef node = data->pModelView->root_node();
    mforms::TreeNodeRef schema;
    base::StringListPtr schemas(new std::list<std::string>());

    schemas->push_back("one");
    schemas->push_back("two");
    schemas->push_back("three");

    data->treeTestHelper.update_node_children(node, schemas, LiveSchemaTree::Schema, true, true);

    data->treeTestHelperFiltered.set_base(&data->treeTestHelper);
    data->treeTestHelperFiltered.set_filter("*e*");
    data->treeTestHelperFiltered.filter_data();

    data->treeTestHelperFiltered.set_active_schema("one");
    $expect(data->treeTestHelperFiltered._active_schema).toEqual("one");
    $expect(data->treeTestHelper._active_schema).toEqual("one");

    data->treeTestHelperFiltered.set_active_schema("three");
    $expect(data->treeTestHelperFiltered._active_schema).toEqual("three");
    $expect(data->treeTestHelper._active_schema).toEqual("three");

    node->remove_children();
    data->pModelViewFiltered->root_node()->remove_children();
  });

  $it("Updating schema nodes", [this]() {
    mforms::TreeNodeRef node = data->pModelView->root_node();
    mforms::TreeNodeRef schema;
    base::StringListPtr schemas(new std::list<std::string>());
    LiveSchemaTree::SchemaData* pdata = nullptr;

    $expect(node->count()).toEqual(0);

    schemas->push_back("one");
    schemas->push_back("two");
    schemas->push_back("three");

    data->treeTestHelper.update_schemata(schemas);

    $expect(node->count()).toEqual(3);

    schema = data->treeTestHelper.get_child_node(node, "one");
    $expect(schema);
    pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());
    $expect(pdata).Not.toBeNull();
    $expect(pdata->get_type()).toEqual(LiveSchemaTree::Schema);
    pdata = nullptr;

    schema = data->treeTestHelper.get_child_node(node, "two");
    $expect(schema);
    pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());
    $expect(pdata).Not.toBeNull();
    $expect(pdata->get_type()).toEqual(LiveSchemaTree::Schema);
    pdata = nullptr;

    schema = data->treeTestHelper.get_child_node(node, "three");
    $expect(schema);
    pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());
    $expect(pdata).Not.toBeNull();
    $expect(pdata->get_type()).toEqual(LiveSchemaTree::Schema);

    // Simulating schema expansion to $expect a loaded schema triggers a data reload.
    schema->expand();
    pdata->fetched = true;
    data->delegate->expect_fetch_schema_contents_call();
    data->delegate->_mock_call_back_slot = false;
    data->delegate->_mock_schema_name = "three";

    data->treeTestHelper.update_schemata(schemas);

    data->delegate->check_and_reset("TF009CHK004");

    node->remove_children();
  });

  $it("Loading schema content", [this]() {
    mforms::TreeNodeRef node_base = data->pModelView->root_node();
    mforms::TreeNodeRef node = data->pModelViewFiltered->root_node();
    mforms::TreeNodeRef schema;
    mforms::TreeNodeRef schema_base;
    mforms::TreeNodeRef child;
    base::StringListPtr schemas(new std::list<std::string>());
    LiveSchemaTree::SchemaData* pdata = nullptr;

    $expect(node->count()).toEqual(0);

    schemas->push_back("one");
    schemas->push_back("two");
    schemas->push_back("three");

    data->treeTestHelper.update_schemata(schemas);

    data->treeTestHelperFiltered.set_base(&data->treeTestHelper);
    data->treeTestHelperFiltered.set_filter("one");
    data->treeTestHelperFiltered.filter_data();

    schema_base = data->treeTestHelper.get_child_node(node_base, "one");
    schema = data->treeTestHelperFiltered.get_child_node(node, "one");
    pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());

    // Validates the previous state.
    $expect(pdata->fetched).toBeFalse();
    $expect(pdata->fetching).toBeFalse();
    $expect(schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION);

    $expect(schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION);

    // Simulating schema expansion to $expect a loaded schema triggers a data reload
    data->delegateFiltered->expect_fetch_schema_contents_call();
    data->delegateFiltered->_mock_call_back_slot = false;
    data->delegateFiltered->_mock_schema_name = "one";

    data->treeTestHelperFiltered.load_schema_content(schema);

    data->delegateFiltered->check_and_reset("TF010CHK002");

    // Validates the previous state.
    $expect(pdata->fetching).toBeTrue();
    $expect(schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);

    $expect(schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);

    data->pModelView->root_node()->remove_children();
    data->pModelViewFiltered->root_node()->remove_children();
  });

  $it("Receiving schema content", [this]() {
    mforms::TreeNodeRef node_base = data->pModelView->root_node();
    mforms::TreeNodeRef node = data->pModelViewFiltered->root_node();
    mforms::TreeNodeRef schema;
    mforms::TreeNodeRef schema_base;
    mforms::TreeNodeRef child;
    base::StringListPtr schemas(new std::list<std::string>());
    LiveSchemaTree::SchemaData* pdata = nullptr;

    $expect(node->count()).toEqual(0);

    schemas->push_back("one");
    schemas->push_back("two");
    schemas->push_back("three");

    data->treeTestHelper.update_schemata(schemas);

    data->treeTestHelperFiltered.set_base(&data->treeTestHelper);
    data->treeTestHelperFiltered.set_filter("one");
    data->treeTestHelperFiltered.filter_data();

    schema_base = data->treeTestHelper.get_child_node(node_base, "one");
    schema = data->treeTestHelperFiltered.get_child_node(node, "one");
    pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());

    // Validates the previous state.
    $expect(pdata->fetching).toBeFalse();
    $expect(schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION);
    $expect(schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION);

    $expect(schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION);
    $expect(schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION);

    // Simulating schema expansion to ensure a loaded schema triggers a data reload.
    data->delegateFiltered->expect_fetch_schema_contents_call();
    data->delegateFiltered->_mock_call_back_slot = true;
    data->delegateFiltered->_mock_schema_name = "one";
    data->delegateFiltered->_mock_table_list->push_back("table1");
    data->delegateFiltered->_mock_table_list->push_back("table2");
    data->delegateFiltered->_mock_table_list->push_back("table3");
    data->delegateFiltered->_mock_view_list->push_back("view1");
    data->delegateFiltered->_mock_view_list->push_back("view2");
    data->delegateFiltered->_mock_procedure_list->push_back("procedure1");
    data->delegateFiltered->_mock_function_list->push_back("function1");

    data->treeTestHelperFiltered.load_schema_content(schema);

    data->delegateFiltered->check_and_reset("TF011CHK002");

    schema = data->treeTestHelperFiltered.get_child_node(node, "one");

    // Validates the previous state.
    $expect(pdata->fetched).toBeTrue();
    $expect(pdata->fetching).toBeFalse();

    child = schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION);
    $expect(child->count()).toEqual(3);

    child = schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION);
    $expect(child->count()).toEqual(2);

    child = schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION);
    $expect(child->count()).toEqual(1);

    child = schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION);
    $expect(child->count()).toEqual(1);

    child = schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::TABLES_CAPTION);
    $expect(child->count()).toEqual(3);

    child = schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::VIEWS_CAPTION);
    $expect(child->count()).toEqual(2);

    child = schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::PROCEDURES_CAPTION);
    $expect(child->count()).toEqual(1);

    child = schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
    $expect(child->get_string(0)).toEqual(LiveSchemaTree::FUNCTIONS_CAPTION);
    $expect(child->count()).toEqual(1);

    node->remove_children();
    node_base->remove_children();
  });

  $it("Loading table details", [this]() {
    mforms::TreeNodeRef node = data->pModelView->root_node();
    mforms::TreeNodeRef schema;
    mforms::TreeNodeRef table;
    base::StringListPtr schemas(new std::list<std::string>());
    LiveSchemaTree::TableData* pdata = nullptr;

    $expect(node->count()).toEqual(0);

    schemas->push_back("one");

    data->treeTestHelper.update_schemata(schemas);

    schema = data->treeTestHelper.get_child_node(node, "one");

    // Simulating schema expansion to $expect a loaded schema triggers a data reload.
    data->delegate->expect_fetch_schema_contents_call();
    data->delegate->_mock_call_back_slot = true;
    data->delegate->_mock_schema_name = "one";
    data->delegate->_mock_table_list->push_back("table1");
    data->delegate->_mock_table_list->push_back("table2");
    data->delegate->_mock_view_list->push_back("view1");
    data->delegate->_mock_procedure_list->push_back("procedure1");
    data->delegate->_mock_function_list->push_back("function1");
    data->delegate->_check_id = "TF012CHK001";

    data->treeTestHelper.load_schema_content(schema);
    data->delegate->check_and_reset("TF012CHK001");

    // Initial test, nothing has been loaded.
    data->delegate->_expect_fetch_object_details_call = true;
    data->delegate->_mock_flags = LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA;
    data->delegate->_mock_schema_name = "one";
    data->delegate->_mock_object_name = "table1";
    data->delegate->_mock_object_type = LiveSchemaTree::Table;
    data->delegate->_check_id = "TF012CHK002";

    data->treeTestHelper.load_table_details(LiveSchemaTree::Table, "one", "table1",
                                        LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);

    data->delegate->check_and_reset("TF012CHK002");

    // Initial test, same loading requested but as it's already in the process of
    // loading (because of the previous step) no fetch call is done.
    data->delegate->_expect_fetch_object_details_call = false;
    data->delegate->_check_id = "TF012CHK003";
    data->treeTestHelper.load_table_details(LiveSchemaTree::Table, "one", "table1",
                                        LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    data->delegate->check_and_reset("TF012CHK003");

    // Third test, reloading existing data and additional info, causes only additional info.
    // to be requested
    data->delegate->_expect_fetch_object_details_call = true;
    data->delegate->_mock_flags = LiveSchemaTree::TRIGGER_DATA;
    data->delegate->_check_id = "TF012CHK004";
    data->treeTestHelper.load_table_details(LiveSchemaTree::Table, "one", "table1",
                                        LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA);
    data->delegate->check_and_reset("TF012CHK004");

    // Repeat the tests but now marking some information as already loaded.
    table = data->treeTestHelper.get_child_node(schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX), "table2");
    pdata = dynamic_cast<LiveSchemaTree::TableData*>(table->get_data());

    pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA);

    data->delegate->_expect_fetch_object_details_call = true;
    data->delegate->_mock_flags = LiveSchemaTree::INDEX_DATA;
    data->delegate->_mock_schema_name = "one";
    data->delegate->_mock_object_name = "table2";
    data->delegate->_mock_object_type = LiveSchemaTree::Table;
    data->delegate->_check_id = "TF012CHK005";

    data->treeTestHelper.load_table_details(LiveSchemaTree::Table, "one", "table2",
      LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);

    data->delegate->check_and_reset("TF012CHK005");

    data->delegate->_expect_fetch_object_details_call = true;
    data->delegate->_mock_flags = LiveSchemaTree::TRIGGER_DATA | LiveSchemaTree::FK_DATA;
    data->delegate->_mock_schema_name = "one";
    data->delegate->_mock_object_name = "table2";
    data->delegate->_mock_object_type = LiveSchemaTree::Table;
    data->delegate->_check_id = "TF012CHK006";

    data->treeTestHelper.load_table_details(LiveSchemaTree::Table, "one", "table2",
      LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA | LiveSchemaTree::FK_DATA);

    data->delegate->check_and_reset("TF012CHK006");
  });

  $it("Identifier comparisons", [this]() {
    data->treeTestHelper.set_case_sensitive_identifiers(true);

    $expect( data->treeTestHelper._case_sensitive_identifiers).toBeTrue();
    $expect(data->treeTestHelper.identifiers_equal("first", "First")).toBeFalse();
    $expect(data->treeTestHelper.identifiers_equal("second", "second")).toBeTrue();

    data->treeTestHelper.set_case_sensitive_identifiers(false);

    $expect(data->treeTestHelper._case_sensitive_identifiers).toBeFalse();
    $expect(data->treeTestHelper.identifiers_equal("first", "First")).toBeTrue();
    $expect(data->treeTestHelper.identifiers_equal("second", "second")).toBeTrue();
  });

  $it("Object type determination", [this]() {
    // Testing for database objects.
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Schema)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Table)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::View)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Procedure)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Function)).toBeTrue();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::TableCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ViewCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ProcedureCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::FunctionCollection)).toBeFalse();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ColumnCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::IndexCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::TriggerCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ForeignKeyCollection)).toBeFalse();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Trigger)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::TableColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ViewColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ForeignKey)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Index)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ForeignKeyColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::IndexColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Any)).toBeFalse();

    // Testing for schema objects.
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Table)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::View)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Procedure)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Function)).toBeTrue();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Schema)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::TableCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ViewCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ProcedureCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::FunctionCollection)).toBeFalse();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ColumnCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::IndexCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::TriggerCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ForeignKeyCollection)).toBeFalse();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Trigger)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::TableColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ViewColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ForeignKey)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Index)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ForeignKeyColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::IndexColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Any)).toBeFalse();

    // Testing for table/view objects.
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Table)).toBeTrue();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::View)).toBeTrue();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Schema)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Procedure)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Function)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::TableCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ViewCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ProcedureCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::FunctionCollection)).toBeFalse();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ColumnCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::IndexCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::TriggerCollection)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ForeignKeyCollection)).toBeFalse();

    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Trigger)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::TableColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ViewColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ForeignKey)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Index)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ForeignKeyColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::IndexColumn)).toBeFalse();
    $expect(data->treeTestHelper.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Any)).toBeFalse();
  });

  $it("Setting a model view", [this]() {
    data->treeTestHelper.set_model_view(NULL);
    $expect(data->treeTestHelper._model_view).toBeNull();

    data->treeTestHelper.set_model_view(data->pModelView);
    $expect(data->treeTestHelper._model_view).Not.toBeNull();
    $expect(data->treeTestHelper._model_view).toEqual(data->pModelView);
  });

  $it("Setting a data delegate", [this]() {
    std::shared_ptr<LiveSchemaTree::Delegate> null_delegate;
    std::shared_ptr<LiveSchemaTree::FetchDelegate> null_fetch_delegate;

    {
      data->treeTestHelper.set_delegate(null_delegate);
      data->treeTestHelper.set_fetch_delegate(null_fetch_delegate);

      std::shared_ptr<LiveSchemaTree::Delegate> found_delegate = data->treeTestHelper._delegate.lock();
      std::shared_ptr<LiveSchemaTree::FetchDelegate> found_fetch_delegate = data->treeTestHelper._fetch_delegate.lock();

      $expect(found_delegate.get()).toBeNull();
      $expect(found_fetch_delegate.get()).toBeNull();
    }

    {
      data->treeTestHelper.set_delegate(data->delegate);
      data->treeTestHelper.set_fetch_delegate(data->delegate);

      std::shared_ptr<LiveSchemaTree::Delegate> found_delegate = data->treeTestHelper._delegate.lock();
      std::shared_ptr<LiveSchemaTree::FetchDelegate> found_fetch_delegate = data->treeTestHelper._fetch_delegate.lock();

      $expect(found_delegate.get()).Not.toBeNull();
      $expect(found_fetch_delegate.get()).Not.toBeNull();

      $expect(found_delegate.get()).toEqual(data->delegate.get());
      $expect(found_fetch_delegate.get()).toEqual(data->delegate.get());
    }
  });

  $it("Internalizing a token", []() {
    $expect(LiveSchemaTree::internalize_token("")).toEqual(0);
    $expect(LiveSchemaTree::internalize_token("whatever")).toEqual(0);
    $expect(LiveSchemaTree::internalize_token("CASCADE")).toEqual(1);
    $expect(LiveSchemaTree::internalize_token("SET NULL")).toEqual(2);
    $expect(LiveSchemaTree::internalize_token("SET DEFAULT")).toEqual(3);
    $expect(LiveSchemaTree::internalize_token("RESTRICT")).toEqual(4);
    $expect(LiveSchemaTree::internalize_token("NO ACTION")).toEqual(5);
    $expect(LiveSchemaTree::internalize_token("BTREE")).toEqual(6);
    $expect(LiveSchemaTree::internalize_token("FULLTEXT")).toEqual(7);
    $expect(LiveSchemaTree::internalize_token("HASH")).toEqual(8);
    $expect(LiveSchemaTree::internalize_token("RTREE")).toEqual(9);
    $expect(LiveSchemaTree::internalize_token("SPATIAL")).toEqual(10);
    $expect(LiveSchemaTree::internalize_token("INSERT")).toEqual(11);
    $expect(LiveSchemaTree::internalize_token("UPDATE")).toEqual(12);
    $expect(LiveSchemaTree::internalize_token("DELETE")).toEqual(13);
    $expect(LiveSchemaTree::internalize_token("BEFORE")).toEqual(14);
    $expect(LiveSchemaTree::internalize_token("AFTER")).toEqual(15);
  });

  $it("Externalizing a token", []() {
    $expect(LiveSchemaTree::externalize_token(0)).toEqual("");
    $expect(LiveSchemaTree::externalize_token(20)).toEqual("");
    $expect( LiveSchemaTree::externalize_token(1)).toEqual("CASCADE");
    $expect(LiveSchemaTree::externalize_token(2)).toEqual("SET NULL");
    $expect(LiveSchemaTree::externalize_token(3)).toEqual("SET DEFAULT");
    $expect(LiveSchemaTree::externalize_token(4)).toEqual("RESTRICT");
    $expect(LiveSchemaTree::externalize_token(5)).toEqual("NO ACTION");
    $expect(LiveSchemaTree::externalize_token(6)).toEqual("BTREE");
    $expect(LiveSchemaTree::externalize_token(7)).toEqual("FULLTEXT");
    $expect(LiveSchemaTree::externalize_token(8)).toEqual("HASH");
    $expect(LiveSchemaTree::externalize_token(9)).toEqual("RTREE");
    $expect(LiveSchemaTree::externalize_token(10)).toEqual("SPATIAL");
    $expect(LiveSchemaTree::externalize_token(11)).toEqual("INSERT");
    $expect(LiveSchemaTree::externalize_token(12)).toEqual("UPDATE");
    $expect(LiveSchemaTree::externalize_token(13)).toEqual("DELETE");
    $expect(LiveSchemaTree::externalize_token(14)).toEqual("BEFORE");
    $expect(LiveSchemaTree::externalize_token(15)).toEqual("AFTER");
  });

  $it("Updating live objects", [this]() {
    data->treeTestHelper.enable_events(true);

    mforms::TreeNodeRef object_node;
    // Testing Schema Object.
    {
      // Ensures the schema doesn't exist.
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
      $expect(object_node.ptr()).toBeNull();

      // Ensures a schema node is created.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Schema, "", "", "schema1");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
      $expect(object_node.ptr()).Not.toBeNull();

      LiveSchemaTree::SchemaData* pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(object_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Schema);

      // Ensures a schema node is deleted.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Schema, "", "schema1", "");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
      $expect(object_node.ptr()).toBeNull();
    }

    // Adds a schema object.
    data->treeTestHelper.update_live_object_state(LiveSchemaTree::Schema, "", "", "schema1");

    // Testing View Object.
    {
      // Ensures the view doesn't exist.
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
      $expect(object_node.ptr()).toBeNull();

      // Ensures a view node is created.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::View, "schema1", "", "view1");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
      $expect(object_node.ptr()).Not.toBeNull();

      LiveSchemaTree::ViewData* pdata = dynamic_cast<LiveSchemaTree::ViewData*>(object_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::View);

      // Ensures a view node is renamed.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::View, "schema1", "view1", "view2");
      $expect(object_node->get_string(0)).toEqual("view2");

      // Ensures a loaded data is NOT reloaded when the node is not expanded.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "view2");
      $expect(pdata->get_loading_mask()).toEqual(0);
      $expect(pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();

      // Now expands the node operation but expanding the node.
      data->delegate->_expect_fetch_object_details_call = true;
      data->delegate->_mock_schema_name = "schema1";
      data->delegate->_mock_object_name = "view2";
      data->delegate->_mock_object_type = LiveSchemaTree::View;
      data->delegate->_mock_flags = LiveSchemaTree::COLUMN_DATA;

      // Emulates the node expansion.
      data->treeTestHelper.expand_toggled(object_node, true);
      object_node->expand();

      // Ensures the needed calls were done.
      data->delegate->check_and_reset("TF019CHK002");

      // Now as the node was expanded, the data should be reloaded.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "view2");
      $expect(pdata->get_loading_mask()).toEqual(LiveSchemaTree::COLUMN_DATA);
      $expect(pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();

      // Marks the data as already loaded.
      pdata->set_loading_mask(0);
      pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA);

      data->delegate->_expect_fetch_object_details_call = true;
      data->delegate->_mock_schema_name = "schema1";
      data->delegate->_mock_object_name = "view2";
      data->delegate->_mock_object_type = LiveSchemaTree::View;
      data->delegate->_mock_flags = LiveSchemaTree::COLUMN_DATA;

      // Now as the node was expanded, the data should be reloaded.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "view2");
      $expect(pdata->get_loading_mask()).toEqual((short)LiveSchemaTree::COLUMN_DATA);
      $expect(pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();

      // Ensures the needed calls were done.
      data->delegate->check_and_reset("TF019CHK002");

      // Ensures a view node is deleted.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::View, "view2");
      $expect(object_node.ptr()).toBeNull();
    }

    // Testing Table Object.
    {
      // Ensures the table doesn't exist.
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
      $expect(object_node.ptr()).toBeNull();

      // Ensures a table node is created.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Table, "schema1", "", "table1");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
      $expect(object_node.ptr()).Not.toBeNull();

      LiveSchemaTree::TableData* pdata = dynamic_cast<LiveSchemaTree::TableData*>(object_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect( pdata->get_type()).toEqual(LiveSchemaTree::Table);

      // Ensures a view node is renamed.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Table, "schema1", "table1", "table2");
      $expect(object_node->get_string(0)).toEqual("table2");

      // Ensures no data is reloaded on collapsed node.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Table, "schema1", "table2", "table2");
      $expect(pdata->get_loading_mask()).toEqual(0);
      $expect(pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();
      $expect(pdata->is_data_loaded(LiveSchemaTree::FK_DATA)).toBeFalse();

      // Expands the table to repeat the test on an expanded table.
      data->delegate->_expect_fetch_object_details_call = true;
      data->delegate->_mock_schema_name = "schema1";
      data->delegate->_mock_object_name = "table2";
      data->delegate->_mock_object_type = LiveSchemaTree::Table;
      data->delegate->_mock_flags = LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA;

      // Emulates the node expansion.
      data->treeTestHelper.expand_toggled(object_node, true);
      object_node->expand();

      // Ensures the needed calls were done.
      data->delegate->check_and_reset("TF019CHK003");

      $expect(pdata->get_loading_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      $expect(pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();
      $expect(pdata->is_data_loaded(LiveSchemaTree::INDEX_DATA)).toBeFalse();

      // Marks the data as already loaded.
      pdata->set_loading_mask(0);
      pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);

      // Now exnsures the data is actually reloaded if the table was expanded.
      data->delegate->_expect_fetch_object_details_call = true;
      data->delegate->_mock_schema_name = "schema1";
      data->delegate->_mock_object_name = "table2";
      data->delegate->_mock_object_type = LiveSchemaTree::Table;
      data->delegate->_mock_flags = LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA;
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Table, "schema1", "table2", "table2");

      $expect(pdata->get_loading_mask()).toEqual(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
      $expect(pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA)).toBeFalse();
      $expect(pdata->is_data_loaded(LiveSchemaTree::INDEX_DATA)).toBeFalse();

      // Ensures the needed calls were done.
      data->delegate->check_and_reset("TF019CHK003");

      // Ensures a table node is deleted.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Table, "schema1", "table1", "");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
      $expect(object_node.ptr()).toBeNull();
    }

    // Testing Procedure Object.
    {
      // Ensures the procedure doesn't exist.
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
      $expect( object_node.ptr()).toBeNull();

      // Ensures a procedure node is created.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Procedure, "schema1", "", "procedure1");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
      $expect(object_node.ptr()).Not.toBeNull();

      LiveSchemaTree::ProcedureData* pdata = dynamic_cast<LiveSchemaTree::ProcedureData*>(object_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Procedure);

      // Ensures a procedure node is renamed.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Procedure, "schema1", "procedure1", "procedure2");
      $expect(object_node->get_string(0)).toEqual("procedure2");

      // Ensures a procedure node is deleted.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Procedure, "schema1", "procedure2", "");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure2");
      $expect(object_node.ptr()).toBeNull();
    }

    // Testing Function Object.
    {
      // Ensures the function doesn't exist
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
      $expect(object_node.ptr()).toBeNull();

      // Ensures a function node is created.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Function, "schema1", "", "function1");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
      $expect(object_node.ptr()).Not.toBeNull();

      LiveSchemaTree::FunctionData* pdata = dynamic_cast<LiveSchemaTree::FunctionData*>(object_node->get_data());
      $expect(pdata).Not.toBeNull();
      $expect(pdata->get_type()).toEqual(LiveSchemaTree::Function);

      // Ensures a function node is renamed.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Function, "schema1", "function1", "function2");
      $expect(object_node->get_string(0)).toEqual("function2");

      // Ensures a function node is deleted.
      data->treeTestHelper.update_live_object_state(LiveSchemaTree::Function, "schema1", "function2", "");
      object_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Function, "function2");
      $expect(object_node.ptr()).toBeNull();
    }
  });

  $it("Field description", [this]() {
    // Fills the tree using the real structure.
    mforms::TreeNodeRef node;
    mforms::TreeNodeRef child_node;
    mforms::TreeNodeRef leaf_node;

    data->fillBasicSchema("Field description");

    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, "");

    // The schema node and it's direct children return the schema description.
    $expect(data->treeTestHelper.get_field_description(node)).toEqual("<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
    child_node = node->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
    child_node = node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
    child_node = node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
    child_node = node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");

    // The table node and it's direct children return the table description.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
    $expect(data->treeTestHelper.get_field_description(node)).toEqual("<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
      "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : table_column1"
      "</table><br><br>"
      "<div><b>Related Tables:</b></div>"
      "MOCK LOADED Foreign Key : fk1");
    child_node = node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
      "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : table_column1"
      "</table><br><br>"
      "<div><b>Related Tables:</b></div>"
      "MOCK LOADED Foreign Key : fk1");
    leaf_node = child_node->get_child(0);
    $expect(data->treeTestHelper.get_field_description(leaf_node)).toEqual("<b>Column:</b> <font color='#148814'><b>table_column1</b></font><br><br>"
      "<b>Definition:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : table_column1"
      "</table><br><br>");

    child_node = node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
      "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : table_column1"
      "</table><br><br>"
      "<div><b>Related Tables:</b></div>"
      "MOCK LOADED Foreign Key : fk1");
    leaf_node = child_node->get_child(0);
    $expect(data->treeTestHelper.get_field_description(leaf_node)).toEqual("<b>Index:</b> <font color='#148814'><b>index1</b></font><br><br><b>Definition:</b><br>MOCK LOADED Index : index1");

    child_node = node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
      "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : table_column1"
      "</table><br><br>"
      "<div><b>Related Tables:</b></div>"
      "MOCK LOADED Foreign Key : fk1");
    leaf_node = child_node->get_child(0);
    $expect(data->treeTestHelper.get_field_description(leaf_node)).toEqual("<b>Trigger:</b> <font color='#148814'><b>trigger1</b></font><br><br><b>Definition:</b><br>MOCK LOADED "
      "Trigger : trigger1");

    child_node = node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
      "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : table_column1"
      "</table><br><br>"
      "<div><b>Related Tables:</b></div>"
      "MOCK LOADED Foreign Key : fk1");
    leaf_node = child_node->get_child(0);
    $expect(data->treeTestHelper.get_field_description(leaf_node)).toEqual("<b>Foreign Key:</b> <font color='#148814'><b>fk1</b></font><br><br>"
      "<b>Definition:</b><br>"
      "MOCK LOADED Foreign Key : fk1");

    // The view node and it's direct children return the table description.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
    $expect(data->treeTestHelper.get_field_description(node)).toEqual("<b>View:</b> <font color='#148814'><b>view1</b></font><br><br>"
      "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : view_column1"
      "</table><br><br>");
    child_node = node->get_child(0);
    $expect(data->treeTestHelper.get_field_description(child_node)).toEqual("<b>Column:</b> <font color='#148814'><b>view_column1</b></font><br><br>"
      "<b>Definition:</b><table style=\"border: none; border-collapse: collapse;\">"
      "MOCK LOADED Column : view_column1"
      "</table><br><br>");

    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
    $expect(data->treeTestHelper.get_field_description(node)).toEqual("<b>Procedure:</b> <font color='#148814'><b>procedure1</b></font><br><br>");

    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
    $expect(data->treeTestHelper.get_field_description(node)).toEqual("<b>Function:</b> <font color='#148814'><b>function1</b></font><br><br>");

    data->pModelView->root_node()->remove_children();
  });

  $it("Node creation for object", [this]() {
    mforms::TreeNodeRef schema_node;
    mforms::TreeNodeRef object_node;
    LiveSchemaTree::LSTData* pdata = nullptr;

    schema_node = data->treeTestHelper.get_node_for_object("schema_object", LiveSchemaTree::Schema, "");
    object_node = data->treeTestHelper.get_node_for_object("schema_object", LiveSchemaTree::Table, "table_object");

    $expect(schema_node.ptr()).toBeNull();
    $expect(object_node.ptr()).toBeNull();

    // Tests the schema and object nodes are created if they don't exist.
    object_node = data->treeTestHelper.create_node_for_object("schema_object", LiveSchemaTree::Table, "table_object");
    schema_node = data->treeTestHelper.get_node_for_object("schema_object", LiveSchemaTree::Schema, "");

    $expect(schema_node.ptr()).Not.toBeNull();
    $expect(object_node.ptr()).Not.toBeNull();
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
    $expect(pdata).Not.toBeNull();
    $expect( pdata->get_type()).toEqual(LiveSchemaTree::Table);

    // Tests the view object is created under an existing schema if it already exists.
    object_node = data->treeTestHelper.get_node_for_object("schema_object", LiveSchemaTree::View, "view_object");
    $expect(object_node.ptr()).toBeNull();

    object_node = data->treeTestHelper.create_node_for_object("schema_object", LiveSchemaTree::View, "view_object");
    $expect(object_node.ptr()).Not.toBeNull();
    $expect(schema_node.ptr()).toEqual(object_node->get_parent()->get_parent().ptr());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
    $expect(pdata).Not.toBeNull();
    $expect(pdata->get_type()).toEqual(LiveSchemaTree::View);

    // Tests the procedure object is created under an existing schema if it already exists.
    object_node = data->treeTestHelper.get_node_for_object("schema_object", LiveSchemaTree::Procedure, "procedure_object");
    $expect(object_node.ptr()).toBeNull();

    object_node = data->treeTestHelper.create_node_for_object("schema_object", LiveSchemaTree::Procedure, "procedure_object");
    $expect(object_node.ptr()).Not.toBeNull();
    $expect(schema_node.ptr()).toEqual(object_node->get_parent()->get_parent().ptr());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
    $expect(pdata).Not.toBeNull();
    $expect(pdata->get_type()).toEqual(LiveSchemaTree::Procedure);

    // Tests the function object is created under an existing schema if it already exists.
    object_node = data->treeTestHelper.get_node_for_object("schema_object", LiveSchemaTree::Function, "function_object");
    $expect( object_node.ptr()).toBeNull();

    object_node = data->treeTestHelper.create_node_for_object("schema_object", LiveSchemaTree::Function, "function_object");
    $expect(object_node.ptr()).Not.toBeNull();
    $expect(schema_node.ptr()).toEqual(object_node->get_parent()->get_parent().ptr());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
    $expect(pdata).Not.toBeNull();
    $expect(pdata->get_type()).toEqual(LiveSchemaTree::Function);

    // Ensures no other object types alter the tree structure.
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::Schema, "whatever");
    $expect(object_node.ptr()).toBeNull();

    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::TableCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::ViewCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::ProcedureCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::FunctionCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();

    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::ColumnCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::IndexCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::TriggerCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::ForeignKeyCollection, "whatever");
    $expect(object_node.ptr()).toBeNull();

    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::Trigger, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::TableColumn, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::ViewColumn, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::Index, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::ForeignKey, "whatever");
    $expect(object_node.ptr()).toBeNull();

    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::ForeignKeyColumn, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::IndexColumn, "whatever");
    $expect(object_node.ptr()).toBeNull();
    object_node = data->treeTestHelper.create_node_for_object("fake_schema_object", LiveSchemaTree::Any, "whatever");
    $expect(object_node.ptr()).toBeNull();

    schema_node = data->treeTestHelper.get_node_for_object("fake_schema_object", LiveSchemaTree::Schema, "");
    $expect(schema_node.ptr()).toBeNull();
  });

  $it("Enabling/disabling schema content", [this]() {
    bool backup = data->treeTestHelper.is_schema_contents_enabled();

    data->treeTestHelper.is_schema_contents_enabled(true);
    $expect(data->treeTestHelper.is_schema_contents_enabled());

    data->treeTestHelper.is_schema_contents_enabled(false);
    $expect(data->treeTestHelper.is_schema_contents_enabled()).toBeFalse();

    data->treeTestHelper.is_schema_contents_enabled(backup);
  });

  $it("Recursive schema name search", [this]() {
    data->fillBasicSchema("Recursive schema name search");

    data->checkGetSchemaNameRecursive(&data->treeTestHelper, data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, ""));

    data->pModelView->root_node()->remove_children();
  });

  $it("Recursive node paths", [this]() {
    data->fillBasicSchema("Recursive node paths");

    data->checkNodePathsRecursive(&data->treeTestHelper, data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, ""));

    data->pModelView->root_node()->remove_children();
  });

  $it("Enabling/disableing tree events", [this]() {
    bool backup = data->treeTestHelper.getEnabledEvents();

    data->treeTestHelper.enable_events(true);
    $expect(data->treeTestHelper.getEnabledEvents()).toBeTrue();

    data->treeTestHelper.enable_events(false);
    $expect(data->treeTestHelper.getEnabledEvents()).toBeFalse();

    data->treeTestHelper.enable_events(backup);
  });

  $it("Expanding/collapsing tree nodes", [this]() {
    // Fills the tree using the real structure.
    base::StringListPtr schemas(new std::list<std::string>());
    mforms::TreeNodeRef schema_node;
    mforms::TreeNodeRef schema_node_filtered;
    mforms::TreeNodeRef child_node;
    mforms::TreeNodeRef child_node_filtered;
    mforms::TreeNodeRef object_node;
    mforms::TreeNodeRef object_node_filtered;

    schemas->push_back("schema1");
    schemas->push_back("schema2");
    schemas->push_back("schema3");

    data->treeTestHelper.enable_events(true);

    // Fills a schema.
    data->treeTestHelper.update_schemata(schemas);
    schema_node = data->treeTestHelper.get_node_for_object("schema2", LiveSchemaTree::Schema, "");

    // Fills the schema content.
    data->delegate->expect_fetch_schema_contents_call();
    data->delegate->_mock_view_list->push_back("view1");
    data->delegate->_mock_table_list->push_back("table1");
    data->delegate->_mock_table_list->push_back("table2");
    data->delegate->_mock_table_list->push_back("table3");
    data->delegate->_mock_procedure_list->push_back("procedure1");
    data->delegate->_mock_function_list->push_back("function1");
    data->delegate->_mock_call_back_slot = true;
    data->delegate->_mock_schema_name = "schema2";
    data->delegate->_check_id = "TF026CHK001";

    data->treeTestHelper.expand_toggled(schema_node, true);
    data->delegate->check_and_reset("TF026CHK001");

    data->treeTestHelper.expand_toggled(schema_node, false);
    data->delegate->check_and_reset("TF026CHK002");

    data->treeTestHelper.expand_toggled(schema_node, true);
    data->delegate->check_and_reset("TF026CHK003");

    // Ensures nothing happens when the expansion is toglled for the table collection node.
    child_node = schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Expands a table node.
    data->delegate->_mock_schema_name = "schema2";
    data->delegate->_mock_object_name = "table3";
    data->delegate->_mock_object_type = LiveSchemaTree::Table;
    data->delegate->_expect_fetch_object_details_call = true;
    data->delegate->_mock_column_list->clear();
    data->delegate->_mock_index_list->clear();
    data->delegate->_mock_column_list->push_back("table_column1");
    data->delegate->_mock_index_list->push_back("index1");
    data->delegate->_mock_trigger_list->push_back("trigger1");
    data->delegate->_mock_fk_list->push_back("fk1");
    data->delegate->_mock_call_back_slot_columns = true;
    data->delegate->_mock_call_back_slot_indexes = true;
    data->delegate->_mock_call_back_slot_triggers = true;
    data->delegate->_mock_call_back_slot_foreign_keys = true;
    data->delegate->_check_id = "TF026CHK004";

    // Takes the third table node.
    object_node = child_node->get_child(2);
    data->treeTestHelper.expand_toggled(object_node, true);
    data->treeTestHelper.expand_toggled(object_node, false);
    data->treeTestHelper.expand_toggled(object_node, true);

    data->delegate->check_and_reset("TF026CHK004");

    // Ensures nothing happens when the expansion is toglled for the column collection node.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Ensures nothing happens when the expansion is toglled for the index collection node.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Ensures nothing happens when the expansion is toglled for the trigger collection node.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Ensures nothing happens when the expansion is toglled for the foreign key collection node.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Ensures nothing happens when the expansion is toglled for the column collection node.
    child_node = schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Fills view column.
    data->delegate->_mock_schema_name = "schema2";
    data->delegate->_mock_object_name = "view1";
    data->delegate->_mock_object_type = LiveSchemaTree::View;
    data->delegate->_expect_fetch_object_details_call = true;
    data->delegate->_mock_column_list->push_back("view_column1");
    data->delegate->_mock_call_back_slot_columns = true;
    data->delegate->_check_id = "TF026CHK005";

    object_node = child_node->get_child(0);
    data->treeTestHelper.expand_toggled(object_node, true);
    data->treeTestHelper.expand_toggled(object_node, false);
    data->treeTestHelper.expand_toggled(object_node, true);

    data->delegate->check_and_reset("TF026CHK005");

    // Ensures nothing happens when the expansion is toglled for the procedures collection node.
    child_node = schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Ensures nothing happens when the expansion is toglled for the functions collection node.
    child_node = schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
    data->treeTestHelper.expand_toggled(child_node, true);
    data->treeTestHelper.expand_toggled(child_node, false);
    data->treeTestHelper.expand_toggled(child_node, true);

    // Now create a filtered tree based on the loaded data to check.
    // Expansion state is propagated to the base tree.
    data->treeTestHelperFiltered.set_base(&data->treeTestHelper);
    data->treeTestHelperFiltered.set_filter("schema2.table3");
    data->treeTestHelperFiltered.filter_data();

    schema_node_filtered = data->treeTestHelperFiltered.get_node_for_object("schema2", LiveSchemaTree::Schema, "");

    // Ensures the schema expansion state on base tree is propagated from the state at the filtered tree.
    data->treeTestHelperFiltered.expand_toggled(schema_node_filtered, true);
    $expect(schema_node->is_expanded()).toBeTrue();
    data->treeTestHelperFiltered.expand_toggled(schema_node_filtered, false);
    $expect(schema_node->is_expanded()).toBeFalse();
    data->treeTestHelperFiltered.expand_toggled(schema_node_filtered, true);
    $expect( schema_node->is_expanded()).toBeTrue();

    // Ensures the table expansion state on base tree is propagated from the state at the filtered tree.
    data->treeTestHelperFiltered.expand_toggled(schema_node_filtered->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0), true);
    $expect(schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(2)->is_expanded()).toBeTrue();
    data->treeTestHelperFiltered.expand_toggled(schema_node_filtered->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0), false);
    $expect(schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(2)->is_expanded()).toBeFalse();
    data->treeTestHelperFiltered.expand_toggled(schema_node_filtered->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0), true);
    $expect(schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(2)->is_expanded()).toBeTrue();

    data->pModelViewFiltered->root_node()->remove_children();
    data->pModelView->root_node()->remove_children();
  });

  $it("Activating a tree node", [this]() {
    mforms::TreeNodeRef schema_node;
    mforms::TreeNodeRef child_node;
    mforms::TreeNodeRef object_node;

    data->fillBasicSchema("Activating a tree node");

    schema_node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, "");

    LiveSchemaTree::ChangeRecord change;
    change.schema = "";
    change.type = LiveSchemaTree::Schema;
    change.name = "schema1";
    data->delegate->_mock_expected_changes.push_back(change);
    data->delegate->_mock_expected_action = "activate";
    data->delegate->_expect_tree_activate_objects = true;

    // Test activating a schema.
    data->treeTestHelper.node_activated(schema_node, 0);
    data->delegate->check_and_reset("TF027CHK001");

    // Test activating the tables collection.
    child_node = schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);

    // Test activating a table node.
    object_node = child_node->get_child(0);
    data->delegate->_mock_expected_text = "table1";
    data->treeTestHelper.node_activated(object_node, 0);
    data->delegate->check_and_reset("TF027CHK003");

    // Test activating the columns collection.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);

    // Test activating a column node.
    data->delegate->_mock_expected_text = "table_column1";
    data->treeTestHelper.node_activated(child_node->get_child(0), 0);
    data->delegate->check_and_reset("TF027CHK005");

    // Test activating the indexes collection.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);

    // Test activating an index node.
    data->delegate->_mock_expected_text = "index1";
    data->treeTestHelper.node_activated(child_node->get_child(0), 0);
    data->delegate->check_and_reset("TF027CHK007");

    // Test activating the triggers collection.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);

    // Test activating a trigger node.
    data->delegate->_mock_expected_text = "trigger1";
    data->treeTestHelper.node_activated(child_node->get_child(0), 0);
    data->delegate->check_and_reset("TF027CHK009");

    // Test activating the triggers collection.
    child_node = object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);

    // Test activating a foreign key node.
    data->delegate->_mock_expected_text = "fk1";
    data->treeTestHelper.node_activated(child_node->get_child(0), 0);
    data->delegate->check_and_reset("TF027CHK011");

    // Test activating the view collection.
    child_node = schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);

    // Test activating a view node.
    object_node = child_node->get_child(0);
    data->delegate->_mock_expected_text = "view1";
    data->treeTestHelper.node_activated(object_node, 0);
    data->delegate->check_and_reset("TF027CHK013");

    // Test activating a view column node.
    data->delegate->_mock_expected_text = "view_column1";
    data->treeTestHelper.node_activated(object_node->get_child(0), 0);
    data->delegate->check_and_reset("TF027CHK014");

    // Test activating the routines collection.
    child_node = schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);
    data->delegate->check_and_reset("TF027CHK015");

    // Test activating a procedure node.
    object_node = child_node->get_child(0);
    data->delegate->_mock_expected_text = "procedure1";
    data->treeTestHelper.node_activated(object_node, 0);
    data->delegate->check_and_reset("TF027CHK016");

    // Test activating the routines collection.
    child_node = schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
    data->treeTestHelper.node_activated(child_node, 0);
    data->delegate->check_and_reset("TF027CHK017");

    // Test activating a function node.
    object_node = child_node->get_child(0);
    data->delegate->_mock_expected_text = "function1";
    data->treeTestHelper.node_activated(object_node, 0);
    data->delegate->check_and_reset("TF027CHK018");

    data->pModelView->root_node()->remove_children();
  });

  $it("Getting popup items for nodes", [this]() {

    $pending("test code was never enabled and needs fixes");

    bec::MenuItemList items;
    std::list<mforms::TreeNodeRef> nodes;
    mforms::TreeNodeRef schema_node;
    mforms::TreeNodeRef object_node;

    data->fillBasicSchema("Getting popup items for nodes");

    data->setNodes(&data->treeTestHelper, nodes, SCHEMA);

    //================= Schema and Schema's Collection Nodes =================//
    // Reviewing items for a schema
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 8 Real items plus 3 separators
    $expect(items.size()).toEqual(11U);
    data->ensureMenuItemsExist("TF028CHK002", items,
      SET_DEF_SCH | FIL_TO_SCH | COPY_TC | SEND_TE | CREATE | ALTER | DROP | REFRESH, SUB_NAME|SUB_CREATE, "Schema", "");

    // Reviewing items for multiple schemas
    data->setNodes(&data->treeTestHelper, nodes, SCHEMA);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 6 Real items plus 2 separators
    $expect(items.size()).toEqual(8U);
    data->ensureMenuItemsExist("TF028CHK003", items, COPY_TC|SEND_TE|CREATE|ALTER|DROP|REFRESH, SUB_NAME|SUB_CREATE,
                               "Schema", "2 Schemas");

    // Testing the schema table collection options
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, TABLES);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 2 Real items plus 1 separators
    $expect(items.size()).toEqual(3U);
    data->ensureMenuItemsExist("TF028CHK004", items, CREATE|REFRESH, 0, "Table", "");

    // Testing the schema view collection options
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, VIEWS);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 2 Real items plus 1 separators
    $expect(items.size()).toEqual(3U);
    data->ensureMenuItemsExist("TF028CHK005", items, CREATE|REFRESH, 0, "View", "");

    // Testing the schema procedures collection options
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, PROCEDURES);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 3 Real items plus 1 separators
    $expect(items.size()).toEqual(3U);
    data->ensureMenuItemsExist("TF028CHK006", items, CREATE|REFRESH, 0, "Procedure", "");

    // Testing the schema procedures collection options
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, FUNCTIONS);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 3 Real items plus 1 separators
    $expect(items.size()).toEqual(3U);
    data->ensureMenuItemsExist("TF028CHK006", items, CREATE|REFRESH, 0, "Function", "");


    //================= Table, Table's Collection Nodes  and Nodes on each collection =================//
    // Testing for a single schema node
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, TABLE);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 7 Real items plus 2 separators
    $expect(items.size()).toEqual(10U);
    data->ensureMenuItemsExist("TF028CHK007", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|ALTER|DROP|REFRESH,
                               SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_INSERT|SUB_UPDATE|SUB_DELETE|SUB_CREATE, "Table", "");

    // Testing for multiple Table nodes...
    data->setNodes(&data->treeTestHelper, nodes, TABLE);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 7 Real items plus 2 separators
    $expect(items.size()).toEqual(9U);
    data->ensureMenuItemsExist("TF028CHK008", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|ALTER|DROP|REFRESH,
                               SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_INSERT|SUB_UPDATE|SUB_DELETE|SUB_CREATE, "Table", "2 Tables");

    // Columns collection...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, COLUMNS);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 5 Real items plus 1 separators
    $expect(items.size()).toEqual(6U);
    data->ensureMenuItemsExist("TF028CHK009", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|REFRESH,
                               SUB_NAME_S|SUB_NAME_L|SUB_SEL_COL|SUB_INSERT|SUB_UPDATE, "Table", "2 Tables");

    // Column Node
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, TABLE_COLUMN);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 5 Real items plus 1 separators
    $expect(items.size()).toEqual(6U);
    data->ensureMenuItemsExist("TF028CHK010", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|REFRESH,
                               SUB_NAME_S|SUB_NAME_L|SUB_SEL_COL|SUB_INSERT|SUB_UPDATE, "Table", "2 Tables");

    // Multiple Column Nodes
    data->setNodes(&data->treeTestHelper, nodes, TABLE_COLUMN);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Just like a single column.
    $expect(items.size()).toEqual(6U);
    data->ensureMenuItemsExist("TF028CHK010", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|REFRESH,
                               SUB_NAME_S|SUB_NAME_L|SUB_SEL_COL|SUB_INSERT|SUB_UPDATE, "Table", "2 Tables");

    // Index collection...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, INDEXES);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK012", items, REFRESH, 0, "", "");

    // Index Node...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, INDEX);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK013", items, REFRESH, 0, "", "");

    // Multiple Index Nodes...
    data->setNodes(&data->treeTestHelper, nodes, INDEX);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK014", items, REFRESH, 0, "", "");

    // Trigger collection...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, TRIGGERS);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK015", items, REFRESH, 0, "", "");

    // Trigger Node...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, TRIGGER);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK016", items, REFRESH, 0, "", "");

    // Multiple Trigger Nodes...
    data->setNodes(&data->treeTestHelper, nodes, TRIGGER);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK017", items, REFRESH, 0, "", "");

    // Foreign Key collection...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, FKS);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK018", items, REFRESH, 0, "", "");

    // Foreign Key Node...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, FK);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK019", items, REFRESH, 0, "", "");

    // Multiple Foreign Key Nodes...
    data->setNodes(&data->treeTestHelper, nodes, FK);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // Refresh All
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK020", items, REFRESH, 0, "", "");


    //================= View Nodes =================//
    // Single View Node...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, VIEW);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 7 Real items plus 2 separators
    $expect(items.size()).toEqual(9U);
    data->ensureMenuItemsExist("TF028CHK021", items, SEL_ROWS|COPY_TC|SEND_TE|CREATE|ALTER|DROP|REFRESH,
                               SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_CREATE, "View", "");

    // Multiple View Nodes...
    data->setNodes(&data->treeTestHelper, nodes, VIEW);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    // 5 Real items plus 2 separators
    $expect(items.size()).toEqual(7U);
    data->ensureMenuItemsExist("TF028CHK022", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH,
                               SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_CREATE, "View", "2 Views");

    //================= Procedure Nodes =================//
    // Single Procedure Node...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, PROCEDURE);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    $expect(items.size()).toEqual(8U);
    data->ensureMenuItemsExist("TF028CHK023", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
                               "Procedure", "");

    // Multiple Procedure Nodes...
    data->setNodes(&data->treeTestHelper, nodes, PROCEDURE);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    $expect(items.size()).toEqual(7U);
    data->ensureMenuItemsExist("TF028CHK024", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
                               "Procedure", "2 Procedures");

    //================= Function Nodes =================//
    // Single Function Node...
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, FUNCTION);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    $expect(items.size()).toEqual(8U);
    data->ensureMenuItemsExist("TF028CHK025", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
                               "Function", "");

    // Multiple Function Nodes...
    data->setNodes(&data->treeTestHelper, nodes, FUNCTION);
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);

    $expect(items.size()).toEqual(7U);
    data->ensureMenuItemsExist("TF028CHK026", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
                               "Function", "2 Functions");

    //================= No Nodes =================//
    nodes.clear();
    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);
    $expect(items.size()).toEqual(1U);
    data->ensureMenuItemsExist("TF028CHK027", items, REFRESH, 0, "", "");


    //================= Multiple Nodes of Different Type =================//
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, SCHEMA|TABLES|TABLE);
    data->setNodes(&data->treeTestHelper, nodes, TABLE|TABLE_COLUMN|VIEW|PROCEDURE|FUNCTION);

    items = data->treeTestHelper.get_popup_items_for_nodes(nodes);
    $expect(items.size()).toEqual(4U);
    data->ensureMenuItemsExist("TF028CHK028", items, COPY_TC|SEND_TE|REFRESH, SUB_NAME_S|SUB_NAME_L, "", "6 Objects");

    data->pModelView->root_node()->remove_children();

  });

  $it("Activating a popup item for a node", [this]() {

    $pending("test code was never enabled and needs fixes");

    std::list<mforms::TreeNodeRef> nodes;
    mforms::TreeNodeRef schema_node;
    mforms::TreeNodeRef object_node;
    LiveSchemaTree::ChangeRecord change;

    data->fillBasicSchema("Activating a popup item for a node");

    // Tests the Refresh All function.
    data->delegate->_expect_tree_refresh = true;
    data->delegate->_check_id = "TF029CHK002";
    data->treeTestHelper.activate_popup_item_for_nodes("refresh", nodes);
    data->delegate->check_and_reset("TF029CHK002");

    //================= Performs the action for multiple nodes of different type =================//
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, SCHEMA | TABLE);
    data->setNodes(&data->treeTestHelper, nodes, TABLE | TABLE_COLUMN | VIEW | PROCEDURE | FUNCTION );

    // Tests the Alter function
    data->setChangeRecords(data->delegate->_mock_expected_changes, SCHEMA | TABLE);
    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE | VIEW | PROCEDURE | FUNCTION);

    data->delegate->_check_id = "TF029CHK003";
    data->delegate->_expect_tree_alter_objects = true;
    data->treeTestHelper.activate_popup_item_for_nodes("alter", nodes);
    data->delegate->check_and_reset("TF029CHK003");


    // Tests the Drop function
    data->setChangeRecords(data->delegate->_mock_expected_changes, SCHEMA|TABLE);
    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE|VIEW|PROCEDURE|FUNCTION);

    data->delegate->_check_id = "TF029CHK004";
    data->delegate->_expect_tree_drop_objects = true;
    data->treeTestHelper.activate_popup_item_for_nodes("drop", nodes);
    data->delegate->check_and_reset("TF029CHK004");

    // Tests the edit data option with tables
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, TABLE);
    data->setNodes(&data->treeTestHelper, nodes, TABLE);

    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE);
    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE);

    data->delegate->_check_id = "TF029CHK005";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "edit_data";
    data->treeTestHelper.activate_popup_item_for_nodes("edit_data", nodes);
    data->delegate->check_and_reset("TF029CHK005");

    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE);
    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE);
    data->delegate->_check_id = "TF029CHK005.1";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "select_data";
    data->treeTestHelper.activate_popup_item_for_nodes("select_data", nodes);
    data->delegate->check_and_reset("TF029CHK005.1");

    // Tests the edit data option with views
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, TABLE|VIEW|PROCEDURE|FUNCTION);

    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE|VIEW|PROCEDURE|FUNCTION);

    data->delegate->_check_id = "TF029CHK006";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "edit_data";
    data->treeTestHelper.activate_popup_item_for_nodes("edit_data", nodes);
    data->delegate->check_and_reset("TF029CHK006");

    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE|VIEW|PROCEDURE|FUNCTION);

    data->delegate->_check_id = "TF029CHK006.1";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "select_data";
    data->treeTestHelper.activate_popup_item_for_nodes("select_data", nodes);
    data->delegate->check_and_reset("TF029CHK006.1");

    // Tests the edit data option with table columns
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, COLUMNS|TABLE_COLUMN|VIEW_COLUMN);

    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE);
    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE|VIEW);
    data->delegate->_mock_expected_changes[0].detail = "table_column1";
    data->delegate->_mock_expected_changes[1].detail = "table_column1";
    data->delegate->_mock_expected_changes[2].detail = "view_column1";

    data->delegate->_check_id = "TF029CHK007";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "edit_data_columns";
    data->treeTestHelper.activate_popup_item_for_nodes("edit_data_columns", nodes);
    data->delegate->check_and_reset("TF029CHK007");

    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE);
    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE|VIEW);
    data->delegate->_mock_expected_changes[0].detail = "table_column1";
    data->delegate->_mock_expected_changes[1].detail = "table_column1";
    data->delegate->_mock_expected_changes[2].detail = "view_column1";

    data->delegate->_check_id = "TF029CHK007.1";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "select_data_columns";
    data->treeTestHelper.activate_popup_item_for_nodes("select_data_columns", nodes);
    data->delegate->check_and_reset("TF029CHK007.1");

    // Tests the edit data option with the nodes that should be ignored
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, SCHEMA|TABLES|VIEWS|PROCEDURES|FUNCTIONS|INDEXES|INDEX|TRIGGERS|TRIGGER|FKS|FK);

    data->delegate->_check_id = "TF029CHK008";
    data->treeTestHelper.activate_popup_item_for_nodes("edit_data", nodes);
    data->delegate->check_and_reset("TF029CHK008");

    data->delegate->_check_id = "TF029CHK008.1";
    data->treeTestHelper.activate_popup_item_for_nodes("select_data", nodes);
    data->delegate->check_and_reset("TF029CHK008.1");

    // Tests the create function without objects
    nodes.clear();
    data->setChangeRecords(data->delegate->_mock_expected_changes, SCHEMA);
    data->delegate->_mock_expected_changes[0].schema = "";
    data->delegate->_mock_expected_changes[0].name = "";

    data->delegate->_check_id = "TF029CHK009";
    data->delegate->_expect_tree_create_object = true;
    data->treeTestHelper.activate_popup_item_for_nodes("create", nodes);
    data->delegate->check_and_reset("TF029CHK009");

    // Tests the create functions for schema object
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, SCHEMA|TABLES|TABLE|VIEWS|VIEW|PROCEDURE|FUNCTION);

    data->setChangeRecords(data->delegate->_mock_expected_changes, SCHEMA|TABLE);
    data->setChangeRecords(data->delegate->_mock_expected_changes, TABLE|VIEW);
    data->setChangeRecords(data->delegate->_mock_expected_changes, VIEW|PROCEDURE|FUNCTION);

    data->delegate->_mock_expected_changes[0].schema = "";
    while (data->delegate->_mock_expected_changes.size()) {
      data->delegate->_mock_expected_changes[0].name = "";
      data->delegate->_check_id = "TF029CHK010";
      data->delegate->_expect_tree_create_object = true;
      data->treeTestHelper.activate_popup_item_for_nodes("create", nodes);
      nodes.erase(nodes.begin());
    }

    data->delegate->check_and_reset("TF029CHK010");

    // Testing create in routines collection produces a procedure node.
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, PROCEDURES);
    data->setChangeRecords(data->delegate->_mock_expected_changes, PROCEDURE);

    data->delegate->_check_id = "TF029CHK011";
    data->delegate->_expect_tree_create_object = true;
    data->delegate->_mock_expected_changes[0].name = "";
    data->treeTestHelper.activate_popup_item_for_nodes("create", nodes);
    data->delegate->check_and_reset("TF029CHK011");

    // Testing create in routines collection produces a procedure node.
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, FUNCTIONS);
    data->setChangeRecords(data->delegate->_mock_expected_changes, FUNCTION);

    data->delegate->_check_id = "TF029CHK012";
    data->delegate->_expect_tree_create_object = true;
    data->delegate->_mock_expected_changes[0].name = "";
    data->treeTestHelper.activate_popup_item_for_nodes("create", nodes);
    data->delegate->check_and_reset("TF029CHK012");

    // Tests the set active schema function.
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, SCHEMA);
    data->setChangeRecords(data->delegate->_mock_expected_changes, SCHEMA);
    data->delegate->_mock_expected_changes[0].schema = "";
    data->delegate->_mock_expected_changes[0].name = "schema1";

    data->delegate->_check_id = "TF029CHK013";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "activate";
    data->treeTestHelper.activate_popup_item_for_nodes("set_active_schema", nodes);
    data->delegate->check_and_reset("TF029CHK013");

    // Tests the set filter schema function.
    data->setChangeRecords(data->delegate->_mock_expected_changes, SCHEMA);
    data->delegate->_mock_expected_changes[0].schema = "";
    data->delegate->_mock_expected_changes[0].name = "schema1";

    data->delegate->_check_id = "TF029CHK014";
    data->delegate->_expect_tree_activate_objects = true;
    data->delegate->_mock_expected_action = "filter";
    data->treeTestHelper.activate_popup_item_for_nodes("filter_schema", nodes);
    data->delegate->check_and_reset("TF029CHK014");

    //////////////////////////////////////////////////////////////////////
    //  deprecated
    //////////////////////////////////////////////////////////////////////

    // Tests a custom functions for the database objects.
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes, SCHEMA|TABLE|VIEW|PROCEDURE|FUNCTION);
    data->setChangeRecords(data->delegate->_mock_expected_changes, SCHEMA|TABLE|VIEW|PROCEDURE|FUNCTION);
    data->delegate->_mock_expected_changes[0].detail = "schema";
    data->delegate->_mock_expected_changes[1].detail = "table";
    data->delegate->_mock_expected_changes[2].detail = "view";
    data->delegate->_mock_expected_changes[3].detail = "routine";
    data->delegate->_mock_expected_changes[4].detail = "routine";
    data->delegate->_mock_expected_changes[0].schema= "schema1";
    data->delegate->_mock_expected_changes[0].name= "";

    $expect(data->delegate->_mock_expected_changes.size()).toEqual(nodes.size());
    while (data->delegate->_mock_expected_changes.size()) {
      data->delegate->_expect_plugin_item_call = true;

      data->delegate->_check_id = "TF029CHK015";
      data->delegate->_mock_expected_action = "whatever";
      data->treeTestHelper.activate_popup_item_for_nodes("whatever", nodes);
      nodes.erase(nodes.begin());
    }

    data->delegate->check_and_reset("TF029CHK015");

    // Ensures custom doesn't work for non database nodes.
    nodes.clear();
    data->setNodes(&data->treeTestHelper, nodes,
      TABLES | VIEWS | PROCEDURES | COLUMNS | TABLE_COLUMN | INDEXES | INDEX | TRIGGERS | TRIGGER | FKS | FK | VIEW_COLUMN);

    while (nodes.size()) {
      data->delegate->_check_id = "TF029CHK016";
      data->treeTestHelper.activate_popup_item_for_nodes("whatever", nodes);
      nodes.erase(nodes.begin());
    }

    data->delegate->check_and_reset("TF029CHK016");
    data->pModelView->root_node()->remove_children();
  });

  $it("Filter wildcards", [this]() {
    // Using the default wildcard type.
    $expect(data->treeTestHelper.get_filter_wildcard("")).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("*")).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("a")).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("a*")).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a")).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a*")).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema")).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema*")).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema")).toEqual("*schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema*")).toEqual("*schema*");

    $expect(data->treeTestHelper.get_filter_wildcard("", LiveSchemaTree::LocalLike)).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("*", LiveSchemaTree::LocalLike)).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("a", LiveSchemaTree::LocalLike)).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("a*", LiveSchemaTree::LocalLike)).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a", LiveSchemaTree::LocalLike)).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a*", LiveSchemaTree::LocalLike)).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema", LiveSchemaTree::LocalLike)).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema*", LiveSchemaTree::LocalLike)).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema", LiveSchemaTree::LocalLike)).toEqual("*schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema*", LiveSchemaTree::LocalLike)).toEqual("*schema*");

    $expect(data->treeTestHelper.get_filter_wildcard("", LiveSchemaTree::LocalRegexp)).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("*", LiveSchemaTree::LocalRegexp)).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("a", LiveSchemaTree::LocalRegexp)).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("a*", LiveSchemaTree::LocalRegexp)).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a", LiveSchemaTree::LocalRegexp)).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a*", LiveSchemaTree::LocalRegexp)).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema", LiveSchemaTree::LocalRegexp)).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema*", LiveSchemaTree::LocalRegexp)).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema", LiveSchemaTree::LocalRegexp)).toEqual("*schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema*", LiveSchemaTree::LocalRegexp)).toEqual("*schema*");

    $expect(data->treeTestHelper.get_filter_wildcard("", LiveSchemaTree::RemoteRegexp)).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("*", LiveSchemaTree::RemoteRegexp)).toEqual("*");
    $expect(data->treeTestHelper.get_filter_wildcard("a", LiveSchemaTree::RemoteRegexp)).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("a*", LiveSchemaTree::RemoteRegexp)).toEqual("a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a", LiveSchemaTree::RemoteRegexp)).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("*a*", LiveSchemaTree::RemoteRegexp)).toEqual("*a*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema", LiveSchemaTree::RemoteRegexp)).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("schema*", LiveSchemaTree::RemoteRegexp)).toEqual("schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema", LiveSchemaTree::RemoteRegexp)).toEqual("*schema*");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema*", LiveSchemaTree::RemoteRegexp)).toEqual("*schema*");

    $expect(data->treeTestHelper.get_filter_wildcard("", LiveSchemaTree::RemoteLike)).toEqual("%");
    $expect(data->treeTestHelper.get_filter_wildcard("*", LiveSchemaTree::RemoteLike)).toEqual("%");
    $expect(data->treeTestHelper.get_filter_wildcard("a", LiveSchemaTree::RemoteLike)).toEqual("a%");
    $expect(data->treeTestHelper.get_filter_wildcard("a*", LiveSchemaTree::RemoteLike)).toEqual("a%");
    $expect(data->treeTestHelper.get_filter_wildcard("*a", LiveSchemaTree::RemoteLike)).toEqual("%a%");
    $expect(data->treeTestHelper.get_filter_wildcard("*a*", LiveSchemaTree::RemoteLike)).toEqual("%a%");
    $expect(data->treeTestHelper.get_filter_wildcard("schema", LiveSchemaTree::RemoteLike)).toEqual("schema%");
    $expect(data->treeTestHelper.get_filter_wildcard("schema*", LiveSchemaTree::RemoteLike)).toEqual("schema%");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema", LiveSchemaTree::RemoteLike)).toEqual("%schema%");
    $expect(data->treeTestHelper.get_filter_wildcard("*schema*", LiveSchemaTree::RemoteLike)).toEqual("%schema%");

    $expect(data->treeTestHelper.get_filter_wildcard("?", LiveSchemaTree::RemoteLike)).toEqual("_%");
    $expect(data->treeTestHelper.get_filter_wildcard("a?", LiveSchemaTree::RemoteLike)).toEqual("a_%");
    $expect(data->treeTestHelper.get_filter_wildcard("?a", LiveSchemaTree::RemoteLike)).toEqual("_a%");
    $expect(data->treeTestHelper.get_filter_wildcard("?a?", LiveSchemaTree::RemoteLike)).toEqual("_a_%");
    $expect(data->treeTestHelper.get_filter_wildcard("sc?ema", LiveSchemaTree::RemoteLike)).toEqual("sc_ema%");
    $expect(data->treeTestHelper.get_filter_wildcard("sc?e?a*", LiveSchemaTree::RemoteLike)).toEqual("sc_e_a%");
    $expect(data->treeTestHelper.get_filter_wildcard("sc_ema", LiveSchemaTree::RemoteLike)).toEqual("sc\\_ema%");
    $expect(data->treeTestHelper.get_filter_wildcard("sch%ma*", LiveSchemaTree::RemoteLike)).toEqual("sch\\%ma%");
  });

  $it("Getting a node for an object", [this]() {
    mforms::TreeNodeRef node;
    mforms::TreeNodeRef schema_node;

    data->fillBasicSchema("Getting a node for an object");

    schema_node = data->pModelView->root_node()->get_child(0);

    // Searching for invalid schema.
    node = data->treeTestHelper.get_node_for_object("dummy_schema", LiveSchemaTree::Schema, "");
    $expect(node.ptr()).toBeNull();

    // Searching for a valid schema.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
    $expect(node.ptr()).Not.toBeNull();
    $expect(node.ptr()).toEqual(schema_node.ptr());

    // Searching for a invalid table.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Table, "tableX");
    $expect( node.ptr()).toBeNull();

    // Searching for a valid table.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
    $expect(node.ptr()).Not.toBeNull();
    $expect(node.ptr()).toEqual(schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0).ptr());

    // Searching for a invalid view.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::View, "viewX");
    $expect(node.ptr()).toBeNull();

    // Searching for a valid view.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
    $expect(node.ptr()).Not.toBeNull();
    $expect(node.ptr()).toEqual(schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(0).ptr());

    // Searching for a invalid function.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedureX");
    $expect(node.ptr()).toBeNull();

    // Searching for a valid procedure.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
    $expect(node.ptr()).Not.toBeNull();
    $expect(node.ptr()).toEqual(schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(0).ptr());

    // Searching for a invalid function.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Function, "functionX");
    $expect(node.ptr()).toBeNull();

    // Searching for a valid function.
    node = data->treeTestHelper.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
    $expect(node.ptr()).Not.toBeNull();
    $expect(node.ptr()).toEqual(schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(0).ptr());

    data->pModelView->root_node()->remove_children();
  });

  $it("Switching filtered and unfiltered tree", [this]() {
    $expect(data->treeTestHelper.getBase()).toBeNull();

    data->treeTestHelperFiltered.set_base(&data->treeTestHelper);
    $expect(data->treeTestHelperFiltered.getBase()).toEqual(&data->treeTestHelper);

    data->treeTestHelperFiltered.set_base(nullptr);
    $expect(data->treeTestHelperFiltered.getBase()).toBeNull();
  });

  $it("Children copies when switching filters", [this]() {
    // Test filter_children and filter_children_collection without filters established to make
    // sure effectively all the data is copied from one tree to the other.
    mforms::TreeNodeRef root_node = data->pModelView->root_node();
    mforms::TreeNodeRef root_node_f = data->pModelViewFiltered->root_node();
    mforms::TreeNodeRef schema_node;
    mforms::TreeNodeRef schema_node_f;
    mforms::TreeNodeRef object_node;
    mforms::TreeNodeRef object_node_f;
    mforms::TreeNodeRef sub_node;
    mforms::TreeNodeRef sub_node_f;

    data->fillComplexSchema("TF033CHK001");

    // Ensure no matter the type, all the children are copied if no filter is specified.
    $expect(root_node_f->count()).toEqual(0);
    data->treeTestHelper.filter_children(LiveSchemaTree::Schema, root_node, root_node_f);
    $expect(root_node_f->count()).toEqual(root_node->count());

    for (int schema_index = 0; schema_index < root_node->count(); schema_index++) {
      schema_node = root_node->get_child(schema_index);
      schema_node_f = root_node_f->get_child(schema_index);

      $expect(schema_node_f->get_data()).toEqual(schema_node->get_data());
      $expect(schema_node_f->count()).toEqual(schema_node->count());
      $expect(schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count()).toEqual(schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count());
      $expect(schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count()).toEqual(schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count());
      $expect(schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count()).toEqual(schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count());
      $expect(schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count()).toEqual(schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count());

      for (int table_index = 0; table_index < schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count(); table_index++) {
        object_node = schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(table_index);
        object_node_f = schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(table_index);

        $expect(object_node->get_data()).toEqual(object_node_f->get_data());
        $expect(object_node_f->count()).toEqual(object_node->count());
        $expect(object_node_f->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->count()).toEqual(object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->count());
        $expect(object_node_f->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->count()).toEqual(object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->count());
        $expect(object_node_f->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->count()).toEqual(object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->count());
        $expect(object_node_f->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->count()).toEqual(object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->count());

        for (int column_index = 0;
             column_index < object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->count(); column_index++) {
          sub_node = object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->get_child(column_index);
          sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->get_child(column_index);

          $expect(sub_node->get_data()).toEqual(sub_node_f->get_data());
        }

        for (int index_index = 0; index_index < object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->count();
             index_index++) {
          sub_node = object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->get_child(index_index);
          sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->get_child(index_index);

          $expect(sub_node->get_data()).toEqual(sub_node_f->get_data());
        }

        for (int trigger_index = 0;
             trigger_index < object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->count();
             trigger_index++) {
          sub_node = object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->get_child(trigger_index);
          sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->get_child(trigger_index);

          $expect(sub_node->get_data()).toEqual(sub_node_f->get_data());
        }

        for (int fk_index = 0; fk_index < object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->count();
             fk_index++) {
          sub_node = object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->get_child(fk_index);
          sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->get_child(fk_index);

          $expect(sub_node->get_data()).toEqual(sub_node_f->get_data());
        }
      }

      for (int view_index = 0; view_index < schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count();
           view_index++) {
        object_node = schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(view_index);
        object_node_f = schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(view_index);

        $expect(object_node_f->get_data() == object_node->get_data());
        $expect(object_node_f->count()).toEqual(object_node->count());

        for (int column_index = 0; column_index < object_node->count(); column_index++) {
          sub_node = object_node->get_child(column_index);
          sub_node_f = object_node_f->get_child(column_index);

          $expect(sub_node->get_data()).toEqual(sub_node_f->get_data());
        }
      }

      for (int procedure_index = 0;
           procedure_index < schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count();
           procedure_index++) {
        object_node = schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(procedure_index);
        object_node_f = schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(procedure_index);

        $expect(object_node_f->get_data()).toEqual(object_node->get_data());
      }

      for (int function_index = 0;
           function_index < schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count(); function_index++) {
        object_node = schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(function_index);
        object_node_f = schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(function_index);

        $expect(object_node_f->get_data()).toEqual(object_node->get_data());
      }
    }

    root_node->remove_children();
    root_node_f->remove_children();
  });

  $it("Filtering", [this]() {
    std::vector<std::string> schemas;
    std::vector<std::string> tables;
    std::vector<std::string> views;
    std::vector<std::string> procedures;
    std::vector<std::string> functions;
    mforms::TreeNodeRef root_node_f = data->pModelViewFiltered->root_node();

    data->fillComplexSchema("TF034CHK001");

    // Sets the filter and does the filtering...
    data->treeTestHelperFiltered.set_base(&data->treeTestHelper);

    // Filtering only specifying a full schema name...
    schemas.clear();

    schemas.push_back("dev_schema");
    tables.clear();
    tables.push_back("client");
    tables.push_back("customer");
    tables.push_back("product");
    tables.push_back("store");
    views.clear();
    views.push_back("first_view");
    views.push_back("second_view");
    views.push_back("secure_view");
    views.push_back("third");
    procedures.clear();
    procedures.push_back("get_debths");
    procedures.push_back("get_lazy");
    procedures.push_back("get_payments");
    functions.clear();
    functions.push_back("calc_debth_list");
    functions.push_back("calc_income");
    functions.push_back("dummy");

    data->treeTestHelperFiltered.set_filter("dev_schema");
    data->treeTestHelperFiltered.filter_data();
    data->verifyFilterResult("TF034CHK002", data->pModelViewFiltered->root_node(), schemas, tables, views, procedures, functions);

    // Filtering specifying a schema wildcard...
    schemas.clear();
    schemas.push_back("basic_schema");
    schemas.push_back("dev_schema");
    schemas.push_back("test_schema");
    data->treeTestHelperFiltered.set_filter("*schema");
    data->treeTestHelperFiltered.filter_data();
    data->verifyFilterResult("TF034CHK003", data->pModelViewFiltered->root_node(), schemas, tables, views, procedures, functions);

    // Filtering specifying a different schema wildcard...
    schemas.clear();
    schemas.push_back("basic_schema");
    schemas.push_back("basic_training");
    data->treeTestHelperFiltered.set_filter("basic*");
    data->treeTestHelperFiltered.filter_data();
    data->verifyFilterResult("TF034CHK004", data->pModelViewFiltered->root_node(), schemas, tables, views, procedures, functions);

    // Filtering using both schema and object filter
    schemas.clear();
    schemas.push_back("basic_schema");
    schemas.push_back("basic_training");
    tables.clear();
    views.clear();
    views.push_back("second_view");
    views.push_back("secure_view");
    procedures.clear();
    functions.clear();
    data->treeTestHelperFiltered.set_filter("basic*.sec*");
    data->treeTestHelperFiltered.filter_data();
    data->verifyFilterResult("TF034CHK005", data->pModelViewFiltered->root_node(), schemas, tables, views, procedures, functions);

    // Filtering using both schema and object filter
    schemas.clear();
    schemas.push_back("basic_schema");
    schemas.push_back("basic_training");
    tables.clear();
    tables.push_back("customer");
    tables.push_back("store");
    views.clear();
    views.push_back("first_view");
    views.push_back("second_view");
    views.push_back("secure_view");
    procedures.clear();
    procedures.push_back("get_debths");
    procedures.push_back("get_payments");
    functions.clear();
    functions.push_back("calc_debth_list");

    data->treeTestHelperFiltered.set_filter("?asic_*.*s*");
    data->treeTestHelperFiltered.filter_data();
    data->verifyFilterResult("TF034CHK006", data->pModelViewFiltered->root_node(), schemas, tables, views, procedures, functions);

    data->pModelView->root_node()->remove_children();
    root_node_f->remove_children();
  });

  $it("Filter patterns", [this]() {
    data->treeTestHelperFiltered.clean_filter();

    $expect(data->treeTestHelperFiltered.getFilter()).toEqual("");
    $expect(data->treeTestHelperFiltered._schema_pattern).toBeNull();
    $expect(data->treeTestHelperFiltered._object_pattern).toBeNull();

    data->treeTestHelperFiltered.set_filter("dummy_filter");
    $expect(data->treeTestHelperFiltered.getFilter()).toEqual("dummy_filter");
    $expect(data->treeTestHelperFiltered._schema_pattern).Not.toBeNull();
    $expect(data->treeTestHelperFiltered._object_pattern).toBeNull();

    data->treeTestHelperFiltered.set_filter("some*.tab?");
    $expect(data->treeTestHelperFiltered.getFilter()).toEqual("some*.tab?");
    $expect(data->treeTestHelperFiltered._schema_pattern).Not.toBeNull();
    $expect(data->treeTestHelperFiltered._object_pattern).Not.toBeNull();

    data->treeTestHelperFiltered.set_filter("sch?ema*");
    $expect(data->treeTestHelperFiltered.getFilter()).toEqual("sch?ema*");
    $expect(data->treeTestHelperFiltered._schema_pattern).Not.toBeNull();
    $expect(data->treeTestHelperFiltered._object_pattern).toBeNull();

    data->treeTestHelperFiltered.clean_filter();
    $expect(data->treeTestHelperFiltered.getFilter()).toEqual("");
    $expect(data->treeTestHelperFiltered._schema_pattern).toBeNull();
    $expect(data->treeTestHelperFiltered._object_pattern).toBeNull();
  });

  $it("Load data for filters", [this]() {
    data->delegateFiltered->_expect_fetch_data_for_filter = true;
    data->delegateFiltered->_check_id = "TF036CHK001";
    data->delegateFiltered->_mock__schema_pattern = "%sample%";
    data->delegateFiltered->_mock__object_pattern = "_bject%";
    data->treeTestHelperFiltered.load_data_for_filter("*sample", "?bject");
    data->delegateFiltered->check_and_reset("TF036CHK001");
  });

}

}
