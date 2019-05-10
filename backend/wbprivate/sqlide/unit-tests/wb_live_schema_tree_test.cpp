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

#include "base/symbol-info.h"

#include "stub/stub_mforms.h"
#include "sqlide/wb_live_schema_tree.h"
#include "grt.h"
#include "test.h"

using namespace base;
using namespace grt;
using namespace wb;

namespace wb {
  class LiveSchemaTreeTester {
  private:
    LiveSchemaTree* _tree;

  public:
    void set_target(LiveSchemaTree* target) {
      _tree = target;
    }
    std::string get_active_schema() {
      return _tree->_active_schema;
    }
    void load_schema_content(mforms::TreeNodeRef& schema_node) {
      _tree->load_schema_content(schema_node);
    }
    bool get_case_sensitive_identifiers() {
      return _tree->_case_sensitive_identifiers;
    }
    bool identifiers_equal(const std::string& a, const std::string& b) {
      return _tree->identifiers_equal(a, b);
    }

    bool is_object_type(LiveSchemaTree::ObjectTypeValidation validation, LiveSchemaTree::ObjectType type) {
      return _tree->is_object_type(validation, type);
    }

    mforms::TreeView* get_model_view() {
      return _tree->_model_view;
    }
    std::weak_ptr<LiveSchemaTree::Delegate> get_delegate() {
      return _tree->_delegate;
    }
    std::weak_ptr<LiveSchemaTree::FetchDelegate> get_fetch_delegate() {
      return _tree->_fetch_delegate;
    }

    void enable_events(bool enabled) {
      return _tree->enable_events(enabled);
    }
    bool enabled_events() {
      return _tree->_enabled_events;
    }
    std::string get_filter_wildcard(const std::string& filter,
                                    LiveSchemaTree::FilterType type = LiveSchemaTree::LocalLike) {
      return _tree->get_filter_wildcard(filter, type);
    }
    LiveSchemaTree* get_base() {
      return _tree->_base;
    }
    void setup_node(mforms::TreeNodeRef& node, LiveSchemaTree::ObjectType type, mforms::TreeNodeData* pdata = NULL) {
      _tree->setup_node(node, type, pdata);
    }
    bool filter_children(LiveSchemaTree::ObjectType type, mforms::TreeNodeRef& source, mforms::TreeNodeRef& target,
                         GPatternSpec* pattern = NULL) {
      return _tree->filter_children(type, source, target, pattern);
    }

    void expand_toggled(mforms::TreeNodeRef node, bool value) {
      _tree->expand_toggled(node, value);
    }

    void clean_filter() {
      _tree->clean_filter();
    }
    void set_filter(std::string filter) {
      _tree->set_filter(filter);
    }
    std::string string_filter() {
      return _tree->_filter;
    }
    GPatternSpec* schema_filter() {
      return _tree->_schema_pattern;
    }
    GPatternSpec* object_filter() {
      return _tree->_object_pattern;
    }
  };
}

BEGIN_TEST_DATA_CLASS(wb_live_schema_tree_test)

public:
class LiveTreeTestDelegate : public wb::LiveSchemaTree::Delegate, public wb::LiveSchemaTree::FetchDelegate {
public:
  wb::LiveSchemaTree* ptree;
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
  std::string _mock_schema_filter;
  std::string _mock_object_filter;
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
  } // To silence clang.

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
    ensure(_check_id + " : Unexpected call to fetch_schema_list", _expect_fetch_schema_list_call);
    _expect_fetch_schema_list_call = false;
    std::vector<std::string> slist;
    slist.assign(_mock_schema_list->begin(), _mock_schema_list->end());
    return slist;
  }

  virtual bool fetch_data_for_filter(const std::string& schema_filter, const std::string& object_filter,
                                     const wb::LiveSchemaTree::NewSchemaContentArrivedSlot& arrived_slot) {
    ensure(_check_id + " : Unexpected call to fetch_data_for_filter", _expect_fetch_data_for_filter);
    _expect_fetch_data_for_filter = false;

    ensure_equals(_check_id + " : Unexpected schema filter on fetch_schema_list", _mock_schema_filter, schema_filter);
    ensure_equals(_check_id + " : Unexpected object filter on fetch_schema_list", _mock_object_filter, object_filter);

    return true;
  }

  virtual bool fetch_schema_contents(const std::string& schema_name,
                                     const wb::LiveSchemaTree::NewSchemaContentArrivedSlot& arrived_slot) {
    ensure(_check_id + " : Unexpected call to fetch_schema_contents", _expect_fetch_schema_contents_call);
    _expect_fetch_schema_contents_call = false;

    ensure_equals(_check_id + " : Unexpected schema name on call to fetch_schema_contents", schema_name,
                  _mock_schema_name);

    if (_mock_call_back_slot)
      arrived_slot(_mock_schema_name, _mock_table_list, _mock_view_list, _mock_procedure_list, _mock_function_list,
                   _mock_just_append);

    return true;
  }

  virtual bool fetch_object_details(const std::string& schema_name, const std::string& obj_name,
                                    wb::LiveSchemaTree::ObjectType obj_type, short flags,
                                    const wb::LiveSchemaTree::NodeChildrenUpdaterSlot& updater_slot) {
    mforms::TreeNodeRef parent;
    LiveSchemaTree::ViewData* pviewdata;

    ensure(_check_id + " : Unexpected call to fetch_object_details", _expect_fetch_object_details_call);
    _expect_fetch_object_details_call = false;

    ensure_equals(_check_id + " : Unexpected schema name on call to fetch_object_details", schema_name,
                  _mock_schema_name);
    ensure_equals(_check_id + " : Unexpected object name on call to fetch_object_details", obj_name, _mock_object_name);
    ensure_equals(_check_id + " : Unexpected object type on call to fetch_object_details", obj_type, _mock_object_type);

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
                                     wb::LiveSchemaTree::ObjectType obj_type) {
    return true;
  }

  virtual void tree_refresh() {
    ensure(_check_id + " : Unexpected call to tree_refresh.", _expect_tree_refresh);
    _expect_tree_refresh = false;
  }

  virtual bool sidebar_action(const std::string& action) {
    return true;
  }

  void check_expected_changes(const std::string& change, const std::vector<wb::LiveSchemaTree::ChangeRecord>& changes) {
    ensure_equals(_check_id + " : Unexpected number of objects " + change, changes.size(),
                  _mock_expected_changes.size());

    for (size_t index = 0; index < changes.size(); index++) {
      ensure_equals(_check_id + " : Unexpected object type has been " + change, changes[index].type,
                    _mock_expected_changes[index].type);
      ensure_equals(_check_id + " : Unexpected schema has been " + change, changes[index].schema,
                    _mock_expected_changes[index].schema);
      ensure_equals(_check_id + " : Unexpected object has been " + change, changes[index].name,
                    _mock_expected_changes[index].name);
      ensure_equals(_check_id + " : Unexpected sub_object has been " + change, changes[index].detail,
                    _mock_expected_changes[index].detail);
    }

    _mock_expected_changes.clear();
  }

  virtual void tree_activate_objects(const std::string& action,
                                     const std::vector<wb::LiveSchemaTree::ChangeRecord>& changes) {
    ensure(_check_id + " : Unexpected call to tree_activate_objects.", _expect_tree_activate_objects);
    ensure_equals(_check_id + " : Unexpected action received on tree_activate_objects.", action, _mock_expected_action);
    _expect_tree_activate_objects = false;
    ensure_equals(_check_id + " : Unexpected action has been activated", action, _mock_expected_action);
    check_expected_changes("activated", changes);
  }

  virtual void tree_alter_objects(const std::vector<wb::LiveSchemaTree::ChangeRecord>& changes) {
    ensure(_check_id + " : Unexpected call to tree_alter_objects.", _expect_tree_alter_objects);
    _expect_tree_alter_objects = false;
    check_expected_changes("altered", changes);
  }

  virtual void tree_create_object(wb::LiveSchemaTree::ObjectType type, const std::string& schema_name,
                                  const std::string& object_name) {
    ensure(_check_id + " : Unexpected call to tree_create_object.", _expect_tree_create_object);
    ensure_equals(_check_id + " : Unexpected schema name.", schema_name, _mock_expected_changes[0].schema);
    ensure_equals(_check_id + " : Unexpected object type.", type, _mock_expected_changes[0].type);
    ensure_equals(_check_id + " : Unexpected object name.", object_name, _mock_expected_changes[0].name);

    _mock_expected_changes.erase(_mock_expected_changes.begin());

    _expect_tree_create_object = false;
  }

  virtual void tree_drop_objects(const std::vector<wb::LiveSchemaTree::ChangeRecord>& changes) {
    ensure(_check_id + " : Unexpected call to tree_drop_objects.", _expect_tree_drop_objects);
    _expect_tree_drop_objects = false;
    check_expected_changes("dropped", changes);
  }

  void check_and_reset(const std::string& check_id) {
    ensure(check_id + " : Missed call to fetch_schema_list", !_expect_fetch_schema_list_call);
    ensure(check_id + " : Missed call to fetch_schema_contents", !_expect_fetch_schema_contents_call);
    ensure(check_id + " : Missed call to fetch_object_details", !_expect_fetch_object_details_call);
    ensure(check_id + " : Missing expected changes.", _mock_expected_changes.size() == 0);

    ensure(check_id + " : Missed call to tree_refresh", !_expect_tree_refresh);
    ensure(check_id + " : Missed call to tree_activate_objects", !_expect_tree_activate_objects);
    ensure(check_id + " : Missed call to tree_alter_objects", !_expect_tree_alter_objects);
    ensure(check_id + " : Missed call to tree_create_object", !_expect_tree_create_object);
    ensure(check_id + " : Missed call to tree_drop_objects", !_expect_tree_drop_objects);
    ensure(check_id + " : Missed call to plugin_item_call", !_expect_plugin_item_call);
    ensure(check_id + " : Missed call to fetch_data_for_filter", !_expect_fetch_data_for_filter);

    _expect_fetch_schema_list_call = false;
    _expect_fetch_schema_contents_call = false;
    _expect_fetch_object_details_call = false;
    _expect_plugin_item_call = false;
  }
};

public:
LiveSchemaTree _lst;
LiveSchemaTree _lst_filtered;
mforms::TreeView* pmodel_view;
mforms::TreeView* pmodel_view_filtered;
LiveSchemaTreeTester _tester;
LiveSchemaTreeTester _tester_filtered;
std::shared_ptr<LiveTreeTestDelegate> deleg;
std::shared_ptr<LiveTreeTestDelegate> deleg_filtered;

class DummyLST : public LiveSchemaTree::LSTData {
  virtual LiveSchemaTree::ObjectType get_type() {
    return LiveSchemaTree::Any;
  }
  virtual std::string get_object_name() {
    return "DummyLST";
  }
};

TEST_DATA_CONSTRUCTOR(wb_live_schema_tree_test)
  : _lst(MySQLVersion::MySQL80), _lst_filtered(MySQLVersion::MySQL80),
    deleg(new LiveTreeTestDelegate()), deleg_filtered(new LiveTreeTestDelegate()) {
  grt::GRT::get()->set("/wb", grt::DictRef(true));
  grt::GRT::get()->set("/wb/options", grt::DictRef(true));
  grt::GRT::get()->set("/wb/options/options", grt::DictRef(true));
  grt::GRT::get()->set("/wb/options/options/SqlEditor:AutoFetchColumnInfo", grt::IntegerRef(1));

  mforms::stub::init(NULL);
  pmodel_view =
    new mforms::TreeView(mforms::TreeNoColumns | mforms::TreeNoBorder | mforms::TreeSidebar | mforms::TreeNoHeader);
  pmodel_view_filtered =
    new mforms::TreeView(mforms::TreeNoColumns | mforms::TreeNoBorder | mforms::TreeSidebar | mforms::TreeNoHeader);

  _lst.set_model_view(pmodel_view);
  _lst_filtered.set_model_view(pmodel_view_filtered);

  // Sets the delegate
  _lst.set_delegate(deleg);
  _lst.set_fetch_delegate(deleg);

  _lst_filtered.set_delegate(deleg_filtered);
  _lst_filtered.set_fetch_delegate(deleg_filtered);

  _tester.set_target(&_lst);
  _tester_filtered.set_target(&_lst_filtered);

  deleg->ptree = &_lst;
  deleg_filtered->ptree = &_lst_filtered;
}

void fill_basic_schema(const std::string& check_id) {
  // Fills the tree using the real structure..
  base::StringListPtr schemas(new std::list<std::string>());
  mforms::TreeNodeRef node;

  schemas->push_back("schema1");

  // Fills a schema...
  _lst.update_schemata(schemas);
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, "");

  // Fills the schema content.
  deleg->expect_fetch_schema_contents_call();
  deleg->_mock_view_list->push_back("view1");
  deleg->_mock_table_list->push_back("table1");
  deleg->_mock_procedure_list->push_back("procedure1");
  deleg->_mock_function_list->push_back("function1");
  deleg->_mock_call_back_slot = true;
  deleg->_mock_schema_name = "schema1";
  deleg->_check_id = check_id;

  _tester.load_schema_content(node);

  deleg->check_and_reset(check_id);

  // Fills view column...
  deleg->_mock_schema_name = "schema1";
  deleg->_mock_object_name = "view1";
  deleg->_mock_object_type = LiveSchemaTree::View;
  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_column_list->push_back("view_column1");
  deleg->_mock_call_back_slot_columns = true;
  deleg->_check_id = check_id;

  _lst.load_table_details(LiveSchemaTree::View, "schema1", "view1", LiveSchemaTree::COLUMN_DATA);

  deleg->check_and_reset(check_id);

  // Fills table data...
  deleg->_mock_schema_name = "schema1";
  deleg->_mock_object_name = "table1";
  deleg->_mock_object_type = LiveSchemaTree::Table;
  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_column_list->clear();
  deleg->_mock_index_list->clear();
  deleg->_mock_column_list->push_back("table_column1");
  deleg->_mock_index_list->push_back("index1");
  deleg->_mock_trigger_list->push_back("trigger1");
  deleg->_mock_fk_list->push_back("fk1");
  deleg->_mock_call_back_slot_columns = true;
  deleg->_mock_call_back_slot_indexes = true;
  deleg->_mock_call_back_slot_triggers = true;
  deleg->_mock_call_back_slot_foreign_keys = true;
  deleg->_check_id = check_id;

  _lst.load_table_details(
    LiveSchemaTree::Table, "schema1", "table1",
    LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA | LiveSchemaTree::FK_DATA);

  deleg->check_and_reset(check_id);
}

void fill_schema_object_lists() {
  deleg->_mock_view_list->clear();
  deleg->_mock_view_list->push_back("first_view");
  deleg->_mock_view_list->push_back("second_view");
  deleg->_mock_view_list->push_back("secure_view");
  deleg->_mock_view_list->push_back("third");

  deleg->_mock_table_list->clear();
  deleg->_mock_table_list->push_back("customer");
  deleg->_mock_table_list->push_back("client");
  deleg->_mock_table_list->push_back("store");
  deleg->_mock_table_list->push_back("product");

  deleg->_mock_procedure_list->clear();
  deleg->_mock_procedure_list->push_back("get_debths");
  deleg->_mock_procedure_list->push_back("get_payments");
  deleg->_mock_procedure_list->push_back("get_lazy");

  deleg->_mock_function_list->clear();
  deleg->_mock_function_list->push_back("calc_income");
  deleg->_mock_function_list->push_back("calc_debth_list");
  deleg->_mock_function_list->push_back("dummy");
}

void fill_complex_schema(const std::string& check_id) {
  // Fills the tree using the real structure..
  base::StringListPtr schemas(new std::list<std::string>());
  mforms::TreeNodeRef node;
  deleg->_check_id = check_id;

  schemas->push_back("test_schema");
  schemas->push_back("basic_schema");
  schemas->push_back("basic_training");
  schemas->push_back("dev_schema");

  // Fills a schema...
  _lst.update_schemata(schemas);

  // Fills the schema content.
  deleg->expect_fetch_schema_contents_call();
  fill_schema_object_lists();
  deleg->_mock_call_back_slot = true;
  deleg->_mock_schema_name = "test_schema";
  node = _lst.get_node_for_object("test_schema", LiveSchemaTree::Schema, "");
  _tester.load_schema_content(node);

  deleg->expect_fetch_schema_contents_call();
  fill_schema_object_lists();
  deleg->_mock_call_back_slot = true;
  deleg->_mock_schema_name = "basic_schema";
  node = _lst.get_node_for_object("basic_schema", LiveSchemaTree::Schema, "");
  _tester.load_schema_content(node);

  deleg->expect_fetch_schema_contents_call();
  fill_schema_object_lists();
  deleg->_mock_call_back_slot = true;
  deleg->_mock_schema_name = "basic_training";
  node = _lst.get_node_for_object("basic_training", LiveSchemaTree::Schema, "");
  _tester.load_schema_content(node);

  deleg->expect_fetch_schema_contents_call();
  fill_schema_object_lists();
  deleg->_mock_call_back_slot = true;
  deleg->_mock_schema_name = "dev_schema";
  node = _lst.get_node_for_object("dev_schema", LiveSchemaTree::Schema, "");
  _tester.load_schema_content(node);

  deleg->check_and_reset(check_id);

  // Fills view column...
  deleg->_mock_column_list->push_back("view_col1");
  deleg->_mock_column_list->push_back("view_col2");
  deleg->_mock_column_list->push_back("view_col3");
  deleg->_mock_column_list->push_back("view_col4");

  deleg->_mock_schema_name = "test_schema";
  deleg->_mock_object_name = "first_view";
  deleg->_mock_object_type = LiveSchemaTree::View;

  std::list<std::string> view_list;
  view_list.push_back("first_view");
  view_list.push_back("second_view");
  view_list.push_back("secure_view");
  view_list.push_back("third");

  std::list<std::string>::iterator v_index, v_end = view_list.end();
  for (v_index = view_list.begin(); v_index != v_end; v_index++) {
    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_call_back_slot_columns = true;
    deleg->_mock_object_name = *v_index;
    _lst.load_table_details(LiveSchemaTree::View, "test_schema", *v_index, LiveSchemaTree::COLUMN_DATA);
    deleg->check_and_reset(check_id);
  }

  // Fills table data...
  deleg->_mock_schema_name = "test_schema";
  deleg->_mock_object_type = LiveSchemaTree::Table;
  deleg->_mock_column_list->clear();
  deleg->_mock_index_list->clear();
  deleg->_mock_column_list->push_back("id");
  deleg->_mock_column_list->push_back("name");
  deleg->_mock_column_list->push_back("relation");
  deleg->_mock_index_list->push_back("primary_key");
  deleg->_mock_index_list->push_back("name_unique");
  deleg->_mock_trigger_list->push_back("a_trigger");
  deleg->_mock_fk_list->push_back("some_fk");
  deleg->_mock_call_back_slot_columns = true;
  deleg->_mock_call_back_slot_indexes = true;
  deleg->_mock_call_back_slot_triggers = true;
  deleg->_mock_call_back_slot_foreign_keys = true;

  std::list<std::string> table_list;
  table_list.push_back("customer");
  table_list.push_back("client");
  table_list.push_back("store");
  table_list.push_back("product");

  std::list<std::string>::iterator t_index, t_end = table_list.end();
  for (t_index = table_list.begin(); t_index != t_end; t_index++) {
    deleg->_mock_object_name = *t_index;
    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_call_back_slot_columns = true;
    deleg->_mock_call_back_slot_indexes = true;
    deleg->_mock_call_back_slot_triggers = true;
    deleg->_mock_call_back_slot_foreign_keys = true;
    _lst.load_table_details(LiveSchemaTree::Table, "test_schema", *t_index,
                            LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA |
                              LiveSchemaTree::FK_DATA);
    deleg->check_and_reset(check_id);
  }
}
END_TEST_DATA_CLASS;

TEST_MODULE(wb_live_schema_tree_test, "live schema tree");

TEST_FUNCTION(1) {
  mforms::TreeNodeRef test_node_ref = pmodel_view->root_node();
  test_node_ref->set_string(0, "Dummy");

  // Adds a schema node

  // Testing copy and get details at LSTData (data root)
  {
    DummyLST source, target;
    source.details = "This is a sample";

    ensure_equals("TF001CHK001: Unexpected initial details", target.details, "");

    target.copy(&source);
    ensure_equals("TF001CHK001: Unexpected copied details", target.details, "This is a sample");

    ensure_equals("TF001CHK001: Unexpected raw details", target.get_details(false, test_node_ref), "This is a sample");
    ensure_equals("TF001CHK001: Unexpected full details", target.get_details(true, test_node_ref),
                  "<b>DummyLST:</b> <font color='#148814'><b>Dummy</b></font><br><br>");
  }

  // Testing a ColumnData node
  {
    LiveSchemaTree::ColumnData source, target;
    source.details = "This is a sample";
    source.default_value = "A default value";
    source.is_fk = true;
    source.is_id = true;
    source.is_pk = true;

    ensure_equals("TF001CHK002: Unexpected object name", target.get_object_name(), "Column");
    ensure_equals("TF001CHK002: Unexpected object type", target.get_type(), LiveSchemaTree::TableColumn);

    ensure_equals("TF001CHK002: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK002: Unexpected initial default value", target.default_value, "");
    ensure_equals("TF001CHK002: Unexpected initial fk", target.is_fk, false);
    ensure_equals("TF001CHK002: Unexpected initial id", target.is_id, false);
    ensure_equals("TF001CHK002: Unexpected initial pk", target.is_pk, false);

    target.copy(&source);
    ensure_equals("TF001CHK002: Unexpected copied details", target.details, "This is a sample");
    ensure_equals("TF001CHK002: Unexpected copied default value", target.default_value, "A default value");
    ensure_equals("TF001CHK002: Unexpected copied fk", target.is_fk, true);
    ensure_equals("TF001CHK002: Unexpected copied id", target.is_id, true);
    ensure_equals("TF001CHK002: Unexpected copied pk", target.is_pk, true);

    ensure_equals("TF001CHK002: Unexpected raw details", target.get_details(false, test_node_ref), "This is a sample");
    ensure_equals("TF001CHK002: Unexpected full details", target.get_details(true, test_node_ref),
                  "<b>Column:</b> <font color='#148814'><b>Dummy</b></font><br><br>"
                  "<b>Definition:</b><table style=\"border: none; border-collapse: collapse;\">"
                  "This is a sample"
                  "</table><br><br>");
  }

  // Testing a ForeignKey node
  {
    LiveSchemaTree::FKData source, target;
    source.details = "This is a sample to test copy";
    source.delete_rule = 1;
    source.update_rule = 5;
    source.referenced_table = "destino";
    source.from_cols = "one, two";
    source.to_cols = "uno, dos";

    ensure_equals("TF001CHK003: Unexpected object name", target.get_object_name(), "Foreign Key");
    ensure_equals("TF001CHK003: Unexpected object type", target.get_type(), LiveSchemaTree::ForeignKey);

    ensure_equals("TF001CHK003: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK003: Unexpected initial delete rule", target.delete_rule, 0);
    ensure_equals("TF001CHK003: Unexpected initial update rule", target.update_rule, 0);
    ensure_equals("TF001CHK003: Unexpected initial referenced table", target.referenced_table, "");
    ensure_equals("TF001CHK003: Unexpected initial source cols", target.referenced_table, "");
    ensure_equals("TF001CHK003: Unexpected initial target cols", target.referenced_table, "");

    target.copy(&source);
    ensure_equals("TF001CHK003: Unexpected copied details", target.details, "This is a sample to test copy");
    ensure_equals("TF001CHK003: Unexpected copied delete rule", target.delete_rule, 1);
    ensure_equals("TF001CHK003: Unexpected copied update rule", target.update_rule, 5);
    ensure_equals("TF001CHK003: Unexpected copied referenced table", target.referenced_table, "destino");
    ensure_equals("TF001CHK003: Unexpected copied source cols", target.from_cols, "one, two");
    ensure_equals("TF001CHK003: Unexpected copied target cols", target.to_cols, "uno, dos");

    // Cleans details to test dynamic generation
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

    ensure_equals("TF001CHK003: Unexpected raw details", target.get_details(false, test_node_ref), expected);
    expected = "<b>Foreign Key:</b> <font color='#148814'><b>Dummy</b></font><br><br><b>Definition:</b><br>" + expected;
    ensure_equals("TF001CHK003: Unexpected full details", target.get_details(true, test_node_ref), expected);
  }

  // Testing an Index node
  {
    LiveSchemaTree::IndexData source, target;
    source.details = "This is a sample to test copy";
    source.columns.push_back("one");
    source.columns.push_back("two");
    source.type = 6;
    source.unique = true;

    ensure_equals("TF001CHK004: Unexpected object name", target.get_object_name(), "Index");
    ensure_equals("TF001CHK004: Unexpected object type", target.get_type(), LiveSchemaTree::Index);

    ensure_equals("TF001CHK004: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK004: Unexpected initial number of columns", target.columns.size(), 0U);
    ensure_equals("TF001CHK004: Unexpected initial type", target.type, 0);
    ensure_equals("TF001CHK004: Unexpected initial unique", target.unique, false);

    target.copy(&source);
    ensure_equals("TF001CHK004: Unexpected copied details", target.details, "This is a sample to test copy");
    ensure_equals("TF001CHK004: Unexpected copied number of columns", target.columns.size(), 2U);
    ensure_equals("TF001CHK004: Unexpected copied column 1", target.columns[0], "one");
    ensure_equals("TF001CHK004: Unexpected copied column 2", target.columns[1], "two");
    ensure_equals("TF001CHK004: Unexpected copied type", target.type, 6);
    ensure_equals("TF001CHK004: Unexpected copied unique", target.unique, true);

    // Cleans details to test dynamic generation
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

    ensure_equals("TF001CHK004: Unexpected raw details", target.get_details(false, test_node_ref), expected);
    expected = "<b>Index:</b> <font color='#148814'><b>Dummy</b></font><br><br><b>Definition:</b><br>" + expected;
    ensure_equals("TF001CHK004: Unexpected full details", target.get_details(true, test_node_ref), expected);
  }

  // Testing copy and get_details for a Trigger node
  {
    LiveSchemaTree::TriggerData source, target;
    source.details = "This is a sample to test copy";
    source.event_manipulation = 11;
    source.timing = 15;

    ensure_equals("TF001CHK005: Unexpected object name", target.get_object_name(), "Trigger");
    ensure_equals("TF001CHK005: Unexpected object type", target.get_type(), LiveSchemaTree::Trigger);

    ensure_equals("TF001CHK005: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK005: Unexpected initial event manipulation", target.event_manipulation, 0);
    ensure_equals("TF001CHK005: Unexpected initial timing", target.timing, 0);

    target.copy(&source);
    ensure_equals("TF001CHK005: Unexpected copied details", target.details, "This is a sample to test copy");
    ensure_equals("TF001CHK005: Unexpected copied event manipulation", target.event_manipulation, 11);
    ensure_equals("TF001CHK005: Unexpected copied timing", target.timing, 15);

    // Cleans details to test dynamic generation
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
    ensure_equals("TF001CHK005: Unexpected raw details", target.get_details(false, test_node_ref), expected);
    expected = "<b>Trigger:</b> <font color='#148814'><b>Dummy</b></font><br><br><b>Definition:</b><br>" + expected;
    ensure_equals("TF001CHK005: Unexpected full details", target.get_details(true, test_node_ref), expected);
  }

  // Testing an Object node
  {
    LiveSchemaTree::ObjectData source, target;
    source.details = "This is a sample";
    source.fetched = true;
    source.fetching = true;

    ensure_equals("TF001CHK006: Unexpected object name", target.get_object_name(), "Object");
    ensure_equals("TF001CHK006: Unexpected object type", target.get_type(), LiveSchemaTree::Any);

    ensure_equals("TF001CHK006: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK006: Unexpected initial fetched", target.fetched, false);
    ensure_equals("TF001CHK006: Unexpected initial fetching", target.fetching, false);

    target.copy(&source);
    ensure_equals("TF001CHK006: Unexpected copied details", target.details, "This is a sample");
    ensure_equals("TF001CHK006: Unexpected copied fetched", target.fetched, true);
    ensure_equals("TF001CHK006: Unexpected copied fetching", target.fetching, true);

    ensure_equals("TF001CHK006: Unexpected raw details", target.get_details(false, test_node_ref), "This is a sample");
    ensure_equals("TF001CHK006: Unexpected full details", target.get_details(true, test_node_ref),
                  "<b>Object:</b> <font color='#148814'><b>Dummy</b></font><br><br>");
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

    ensure_equals("TF001CHK007: Unexpected object name", target.get_object_name(), "Function");
    ensure_equals("TF001CHK007: Unexpected object type", target.get_type(), LiveSchemaTree::Function);

    ensure_equals("TF001CHK007: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK007: Unexpected initial fetched", target.fetched, false);
    ensure_equals("TF001CHK007: Unexpected initial fetching", target.fetching, false);

    target.copy(&source);
    ensure_equals("TF001CHK007: Unexpected copied details", target.details, "This is a sample");
    ensure_equals("TF001CHK007: Unexpected copied fetched", target.fetched, true);
    ensure_equals("TF001CHK007: Unexpected copied fetching", target.fetching, true);

    ensure_equals("TF001CHK007: Unexpected raw details", target.get_details(false, test_node_ref),
                  "<b>Function:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
    ensure_equals("TF001CHK007: Unexpected full details", target.get_details(true, test_node_ref),
                  "<b>Function:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
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

    ensure_equals("TF001CHK008: Unexpected object name", target.get_object_name(), "Procedure");
    ensure_equals("TF001CHK008: Unexpected object type", target.get_type(), LiveSchemaTree::Procedure);

    ensure_equals("TF001CHK008: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK008: Unexpected initial fetched", target.fetched, false);
    ensure_equals("TF001CHK008: Unexpected initial fetching", target.fetching, false);

    target.copy(&source);
    ensure_equals("TF001CHK008: Unexpected copied details", target.details, "This is a sample");
    ensure_equals("TF001CHK008: Unexpected copied fetched", target.fetched, true);
    ensure_equals("TF001CHK008: Unexpected copied fetching", target.fetching, true);

    ensure_equals("TF001CHK008: Unexpected raw details", target.get_details(false, test_node_ref),
                  "<b>Procedure:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
    ensure_equals("TF001CHK008: Unexpected full details", target.get_details(true, test_node_ref),
                  "<b>Procedure:</b> <font color='#148814'><b>Dummy</b></font><br><br>This is a sample");
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

    ensure_equals("TF001CHK009: Unexpected object name", target.get_object_name(), "View");
    ensure_equals("TF001CHK009: Unexpected object type", target.get_type(), LiveSchemaTree::View);

    ensure_equals("TF001CHK009: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK009: Unexpected initial columns load error", target.columns_load_error, false);
    ensure_equals("TF001CHK009: Unexpected initial fetched", target.fetched, false);
    ensure_equals("TF001CHK009: Unexpected initial fetching", target.fetching, false);
    ensure_equals("TF001CHK009: Unexpected initial loaded mask", target._loaded_mask, 0);
    ensure_equals("TF001CHK009: Unexpected initial loading mask", target._loading_mask, 0);

    target.copy(&source);
    ensure_equals("TF001CHK009: Unexpected copied details", target.details, "This is a sample");
    ensure_equals("TF001CHK009: Unexpected copied columns load error", target.columns_load_error, true);
    ensure_equals("TF001CHK009: Unexpected copied fetched", target.fetched, true);
    ensure_equals("TF001CHK009: Unexpected copied fetching", target.fetching, true);
    ensure_equals("TF001CHK009: Unexpected copied loaded mask", target._loaded_mask, 1);
    ensure_equals("TF001CHK009: Unexpected copied loading mask", target._loading_mask, 1);

    // Fills the tree using the real structure..
    base::StringListPtr schemas(new std::list<std::string>());
    mforms::TreeNodeRef schema;
    mforms::TreeNodeRef view;
    LiveSchemaTree::ViewData* pdata;

    schemas->push_back("one");

    _lst.update_schemata(schemas);
    schema = _lst.get_child_node(test_node_ref, "one");

    deleg->expect_fetch_schema_contents_call();
    deleg->_mock_view_list->push_back("view1");
    deleg->_mock_call_back_slot = true;
    deleg->_mock_schema_name = "one";
    deleg->_check_id = "TF001CHK009";
    _tester.load_schema_content(schema);

    deleg->check_and_reset("TF001CHK009");

    deleg->_mock_schema_name = "one";
    deleg->_mock_object_name = "view1";
    deleg->_mock_object_type = LiveSchemaTree::View;
    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_column_list->push_back("first_column");
    deleg->_mock_column_list->push_back("second_column");
    deleg->_mock_call_back_slot_columns = true;

    _lst.load_table_details(LiveSchemaTree::View, "one", "view1", LiveSchemaTree::COLUMN_DATA);

    deleg->check_and_reset("TF001CHK009");

    view = _lst.get_node_for_object("one", LiveSchemaTree::View, "view1");
    pdata = dynamic_cast<LiveSchemaTree::ViewData*>(view->get_data());

    ensure("TF001CHK009 : Invalid data returned for view", pdata != NULL);

    ensure_equals("TF001CHK009: Unexpected full details", pdata->get_details(true, view),
                  "<b>View:</b> <font color='#148814'><b>view1</b></font><br><br>"
                  "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                  "MOCK LOADED Column : first_column"
                  "MOCK LOADED Column : second_column"
                  "</table><br><br>");

    test_node_ref->remove_children();

    /* Testing the flag setting logic */
    LiveSchemaTree::ViewData view_node;

    ensure_equals("TF001CHK010: Unexpected loaded mask 1", view_node.get_loaded_mask(), 0);
    ensure_equals("TF001CHK010: Unexpected loading mask 1", view_node.get_loading_mask(), 0);

    view_node.set_loading_mask(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA |
                               LiveSchemaTree::TRIGGER_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 2", view_node.get_loaded_mask(), 0);
    ensure_equals("TF001CHK010: Unexpected loading mask 2", view_node.get_loading_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA |
                    LiveSchemaTree::TRIGGER_DATA);

    view_node.set_loaded_data(LiveSchemaTree::COLUMN_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 3", view_node.get_loaded_mask(),
                  (short)LiveSchemaTree::COLUMN_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 3", view_node.get_loading_mask(),
                  LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA | LiveSchemaTree::TRIGGER_DATA);

    view_node.set_loaded_data(LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 4", view_node.get_loaded_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 4", view_node.get_loading_mask(),
                  LiveSchemaTree::FK_DATA | LiveSchemaTree::TRIGGER_DATA);

    view_node.set_loaded_data(LiveSchemaTree::FK_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 5", view_node.get_loaded_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 5", view_node.get_loading_mask(),
                  (short)LiveSchemaTree::TRIGGER_DATA);

    view_node.set_loaded_data(LiveSchemaTree::TRIGGER_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 6", view_node.get_loaded_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA |
                    LiveSchemaTree::TRIGGER_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 6", view_node.get_loading_mask(), 0);

    view_node.set_unloaded_data(LiveSchemaTree::TRIGGER_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 7", view_node.get_loaded_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::FK_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 7", view_node.get_loading_mask(), 0);

    view_node.set_unloaded_data(LiveSchemaTree::FK_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 8", view_node.get_loaded_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 8", view_node.get_loading_mask(), 0);

    view_node.set_unloaded_data(LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 9", view_node.get_loaded_mask(),
                  (short)LiveSchemaTree::COLUMN_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 9", view_node.get_loading_mask(), 0);

    view_node.set_unloaded_data(LiveSchemaTree::COLUMN_DATA);
    ensure_equals("TF001CHK010: Unexpected loaded mask 10", view_node.get_loaded_mask(), 0);
    ensure_equals("TF001CHK010: Unexpected loading mask 10", view_node.get_loading_mask(), 0);

    /* Tests the inconsistency scenarios */
    /* Loading items get cleaned if they get loaded, tho if other items are to be set as loaded they are done */
    view_node.set_loading_mask(LiveSchemaTree::COLUMN_DATA);
    view_node.set_loaded_data(LiveSchemaTree::INDEX_DATA);
    ensure("TF001CHK010: Unexpected column data loaded 1", !view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA));
    ensure("TF001CHK010: Unexpected index data unloaded 1", view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA));
    ensure_equals("TF001CHK010: Unexpected loaded mask 11", view_node.get_loaded_mask(),
                  (short)LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 11", view_node.get_loading_mask(),
                  (short)LiveSchemaTree::COLUMN_DATA);

    view_node.set_loaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    ensure("TF001CHK010: Unexpected column data unloaded 1",
           view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA));
    ensure("TF001CHK010: Unexpected index data unloaded 2", view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA));
    ensure_equals("TF001CHK010: Unexpected loaded mask 12", view_node.get_loaded_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 12", view_node.get_loading_mask(), 0);

    /* In order to set data unloaded, must be at loaded state first */
    view_node.set_loading_mask(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    view_node.set_loaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    view_node.set_unloaded_data(LiveSchemaTree::FK_DATA);
    ensure("TF001CHK010: Unexpected column data unloaded 2", view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA));
    ensure("TF001CHK010: Unexpected index data unloaded 2", view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA));
    ensure_equals("TF001CHK010: Unexpected loaded mask 13", view_node.get_loaded_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 13", view_node.get_loading_mask(), 0);

    view_node.set_unloaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA);
    ensure("TF001CHK010: Unexpected column data loaded 3", !view_node.is_data_loaded(LiveSchemaTree::COLUMN_DATA));
    ensure("TF001CHK010: Unexpected index data unloaded 2", view_node.is_data_loaded(LiveSchemaTree::INDEX_DATA));
    ensure_equals("TF001CHK010: Unexpected loaded mask 14", view_node.get_loaded_mask(),
                  (short)LiveSchemaTree::INDEX_DATA);
    ensure_equals("TF001CHK010: Unexpected loading mask 14", view_node.get_loading_mask(), 0);
  }

  // Testing a Table node
  {
    LiveSchemaTree::TableData source, target;
    source.details = "This is a sample";
    source.columns_load_error = true;
    source.fetched = true;
    source.fetching = true;
    source._loaded_mask = 1;
    source._loading_mask = 1;

    ensure_equals("TF001CHK011: Unexpected object name", target.get_object_name(), "Table");
    ensure_equals("TF001CHK011: Unexpected object type", target.get_type(), LiveSchemaTree::Table);

    ensure_equals("TF001CHK011: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK011: Unexpected initial columns load error", target.columns_load_error, false);
    ensure_equals("TF001CHK011: Unexpected initial fetched", target.fetched, false);
    ensure_equals("TF001CHK011: Unexpected initial fetching", target.fetching, false);
    ensure_equals("TF001CHK011: Unexpected initial loaded mask", target._loaded_mask, 0);
    ensure_equals("TF001CHK011: Unexpected initial loading mask", target._loading_mask, 0);

    target.copy(&source);
    ensure_equals("TF001CHK011: Unexpected copied details", target.details, "This is a sample");
    ensure_equals("TF001CHK011: Unexpected copied columns load error", target.columns_load_error, true);
    ensure_equals("TF001CHK011: Unexpected copied fetched", target.fetched, true);
    ensure_equals("TF001CHK011: Unexpected copied fetching", target.fetching, true);
    ensure_equals("TF001CHK011: Unexpected copied loaded mask", target._loaded_mask, 1);
    ensure_equals("TF001CHK011: Unexpected copied loading mask", target._loading_mask, 1);

    // Fills the tree using the real structure..
    base::StringListPtr schemas(new std::list<std::string>());
    mforms::TreeNodeRef schema;
    mforms::TreeNodeRef table;
    LiveSchemaTree::TableData* pdata;

    schemas->push_back("one");

    _lst.update_schemata(schemas);
    schema = _lst.get_child_node(test_node_ref, "one");

    deleg->expect_fetch_schema_contents_call();
    deleg->_mock_table_list->push_back("table1");
    deleg->_mock_call_back_slot = true;
    deleg->_mock_schema_name = "one";
    deleg->_check_id = "TF001CHK011";
    _tester.load_schema_content(schema);

    deleg->check_and_reset("TF001CHK011");

    deleg->_mock_schema_name = "one";
    deleg->_mock_object_name = "table1";
    deleg->_mock_object_type = LiveSchemaTree::Table;
    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_fk_list->push_back("fk_1");
    deleg->_mock_fk_list->push_back("fk_2");
    deleg->_mock_call_back_slot_indexes = true;
    deleg->_mock_index_list->push_back("first_column");
    deleg->_mock_call_back_slot_triggers = true;
    deleg->_mock_trigger_list->push_back("trigger1");
    deleg->_mock_call_back_slot_columns = true;
    deleg->_mock_call_back_slot_foreign_keys = true;
    deleg->_mock_column_list->push_back("first_column");
    deleg->_mock_column_list->push_back("second_column");

    _lst.load_table_details(LiveSchemaTree::Table, "one", "table1",
                            LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA | LiveSchemaTree::TRIGGER_DATA |
                              LiveSchemaTree::INDEX_DATA);

    deleg->check_and_reset("TF001CHK011");

    table = _lst.get_node_for_object("one", LiveSchemaTree::Table, "table1");
    pdata = dynamic_cast<LiveSchemaTree::TableData*>(table->get_data());

    ensure("TF001CHK011 : Invalid data returned for table", pdata != NULL);

    ensure_equals("TF001CHK011: Unexpected full details", pdata->get_details(true, table),
                  "<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
                  "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                  "MOCK LOADED Column : first_column"
                  "MOCK LOADED Column : second_column"
                  "</table><br><br>"
                  "<div><b>Related Tables:</b></div>"
                  "MOCK LOADED Foreign Key : fk_1"
                  "MOCK LOADED Foreign Key : fk_2");
  }

  // Testing copy and get_details for a Schema node
  {
    LiveSchemaTree::SchemaData source, target;
    source.details = "This is a sample";
    source.fetched = true;
    source.fetching = true;

    ensure_equals("TF001CHK012: Unexpected object name", target.get_object_name(), "Schema");
    ensure_equals("TF001CHK012: Unexpected object type", target.get_type(), LiveSchemaTree::Schema);

    ensure_equals("TF001CHK012: Unexpected initial details", target.details, "");
    ensure_equals("TF001CHK012: Unexpected initial fetched", target.fetched, false);
    ensure_equals("TF001CHK012: Unexpected initial fetching", target.fetching, false);

    target.copy(&source);
    ensure_equals("TF001CHK012: Unexpected copied details", target.details, "This is a sample");
    ensure_equals("TF001CHK012: Unexpected copied fetched", target.fetched, true);
    ensure_equals("TF001CHK012: Unexpected copied fetching", target.fetching, true);

    ensure_equals("TF001CHK012: Unexpected raw details", target.get_details(false, test_node_ref), "This is a sample");
    ensure_equals("TF001CHK011: Unexpected full details", target.get_details(true, test_node_ref),
                  "<b>Schema:</b> <font color='#148814'><b>Dummy</b></font><br><br>");
  }
}

/*
*  Tests the setup_node and update_node_icon functions
*/
TEST_FUNCTION(5) {
  std::string path;
  mforms::TreeNodeRef node = pmodel_view->root_node();

  /* Testing SchemaNode */
  {
    LiveSchemaTree::SchemaData* pdata = NULL;
    LiveSchemaTree::SchemaData* pdata_temp = new LiveSchemaTree::SchemaData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::Schema, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(temp_node->get_data());
    ensure("TF005CHK001: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::Schema);
    pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(temp_node->get_data());
    ensure("TF005CHK001: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK001: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK001: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::Schema);

    pdata->fetching = true;
    _lst.update_node_icon(temp_node);
    path = bec::IconManager::get_instance()->get_icon_file(
      bec::IconManager::get_instance()->get_icon_id("db.Schema.loading.side.$.png", bec::Icon16));
    ensure_equals("TF005CHK001: Unexpected node icon", temp_node->get_string(1), path);

    pdata->fetched = true;
    _lst.update_node_icon(temp_node);
    path = bec::IconManager::get_instance()->get_icon_file(
      bec::IconManager::get_instance()->get_icon_id("db.Schema.side.$.png", bec::Icon16));
    ensure_equals("TF005CHK001: Unexpected node icon", temp_node->get_string(1), path);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing TableNode */
  {
    LiveSchemaTree::TableData* pdata = NULL;
    LiveSchemaTree::TableData* pdata_temp = new LiveSchemaTree::TableData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::Table, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::TableData*>(temp_node->get_data());
    ensure("TF005CHK002: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::Table);
    pdata = dynamic_cast<LiveSchemaTree::TableData*>(temp_node->get_data());
    ensure("TF005CHK002: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK002: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK002: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::Table);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing ViewNode */
  {
    LiveSchemaTree::ViewData* pdata = NULL;
    LiveSchemaTree::ViewData* pdata_temp = new LiveSchemaTree::ViewData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::View, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::ViewData*>(temp_node->get_data());
    ensure("TF005CHK003: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::View);
    pdata = dynamic_cast<LiveSchemaTree::ViewData*>(temp_node->get_data());
    ensure("TF005CHK003: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK003: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK003: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::View);

    // Checking for icon setup...
    pdata->columns_load_error = true;
    _lst.update_node_icon(temp_node);
    path = bec::IconManager::get_instance()->get_icon_file(
      bec::IconManager::get_instance()->get_icon_id("db.View.broken.side.$.png", bec::Icon16));
    ensure_equals("TF005CHK003: Unexpected node icon", temp_node->get_string(1), path);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing ProcedureNode */
  {
    LiveSchemaTree::ProcedureData* pdata = NULL;
    LiveSchemaTree::ProcedureData* pdata_temp = new LiveSchemaTree::ProcedureData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::Procedure, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::ProcedureData*>(temp_node->get_data());
    ensure("TF005CHK004: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::Procedure);
    pdata = dynamic_cast<LiveSchemaTree::ProcedureData*>(temp_node->get_data());
    ensure("TF005CHK004: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK004: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK004: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::Procedure);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing FunctionNode */
  {
    LiveSchemaTree::FunctionData* pdata = NULL;
    LiveSchemaTree::FunctionData* pdata_temp = new LiveSchemaTree::FunctionData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::Function, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::FunctionData*>(temp_node->get_data());
    ensure("TF005CHK005: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::Function);
    pdata = dynamic_cast<LiveSchemaTree::FunctionData*>(temp_node->get_data());
    ensure("TF005CHK005: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK005: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK005: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::Function);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing ViewColumnNode */
  {
    LiveSchemaTree::ColumnData* pdata = NULL;
    LiveSchemaTree::ColumnData* pdata_temp = new LiveSchemaTree::ColumnData(LiveSchemaTree::ViewColumn);
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data it is requested to not create data if not passed
    _lst.setup_node(temp_node, LiveSchemaTree::ViewColumn, NULL, true);
    ensure("TF005CHK006: Unexpected data returned 0", !temp_node->get_data());

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::ViewColumn, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
    ensure("TF005CHK006: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::ViewColumn);
    pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
    ensure("TF005CHK006: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK006: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK006: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::ViewColumn);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing TableColumnNode */
  {
    LiveSchemaTree::ColumnData* pdata = NULL;
    LiveSchemaTree::ColumnData* pdata_temp = new LiveSchemaTree::ColumnData(LiveSchemaTree::TableColumn);
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::TableColumn, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
    ensure("TF005CHK007: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::TableColumn);
    pdata = dynamic_cast<LiveSchemaTree::ColumnData*>(temp_node->get_data());
    ensure("TF005CHK007: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK007: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK007: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::TableColumn);

    // Checking for icon setup...
    pdata->is_fk = true;
    _lst.update_node_icon(temp_node);
    path = bec::IconManager::get_instance()->get_icon_file(
      bec::IconManager::get_instance()->get_icon_id("db.Column.fk.side.$.png", bec::Icon16));
    ensure_equals("TF005CHK007: Unexpected node icon", temp_node->get_string(1), path);

    pdata->is_pk = true;
    _lst.update_node_icon(temp_node);
    path = bec::IconManager::get_instance()->get_icon_file(
      bec::IconManager::get_instance()->get_icon_id("db.Column.pk.side.$.png", bec::Icon16));
    ensure_equals("TF005CHK007: Unexpected node icon", temp_node->get_string(1), path);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing IndexNode */
  {
    LiveSchemaTree::IndexData* pdata = NULL;
    LiveSchemaTree::IndexData* pdata_temp = new LiveSchemaTree::IndexData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::Index, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::IndexData*>(temp_node->get_data());
    ensure("TF005CHK008: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::Index);
    pdata = dynamic_cast<LiveSchemaTree::IndexData*>(temp_node->get_data());
    ensure("TF005CHK008: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK008: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK008: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::Index);

    // Checking for icon setup...
    // path =
    // bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.Index.side." +
    // suffix, bec::Icon16));
    // ensure_equals("TF005CHK008: Unexpected node icon", temp_node->get_string(1), path);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing TriggerNode */
  {
    LiveSchemaTree::TriggerData* pdata = NULL;
    LiveSchemaTree::TriggerData* pdata_temp = new LiveSchemaTree::TriggerData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data it is requested to not create data if not passed
    _lst.setup_node(temp_node, LiveSchemaTree::Trigger, NULL, true);
    ensure("TF005CHK009: Unexpected data returned 0", !temp_node->get_data());

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::Trigger, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::TriggerData*>(temp_node->get_data());
    ensure("TF005CHK009: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::Trigger);
    pdata = dynamic_cast<LiveSchemaTree::TriggerData*>(temp_node->get_data());
    ensure("TF005CHK009: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK009: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK009: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::Trigger);

    // Checking for icon setup...
    // path =
    // bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.Trigger.side."
    // + suffix, bec::Icon16));
    // ensure_equals("TF005CHK009: Unexpected node icon", temp_node->get_string(1), path);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }

  /* Testing ForeignKeyNode */
  {
    LiveSchemaTree::FKData* pdata = NULL;
    LiveSchemaTree::FKData* pdata_temp = new LiveSchemaTree::FKData();
    mforms::TreeNodeRef temp_node = node->add_child();

    // Testing when a data it is requested to not create data if not passed
    _lst.setup_node(temp_node, LiveSchemaTree::ForeignKey, NULL, true);
    ensure("TF005CHK010: Unexpected data returned 0", !temp_node->get_data());

    // Testing when a data pointer is passed, it is used
    _lst.setup_node(temp_node, LiveSchemaTree::ForeignKey, pdata_temp);
    pdata = dynamic_cast<LiveSchemaTree::FKData*>(temp_node->get_data());
    ensure("TF005CHK010: Unexpected data returned 1", pdata == pdata_temp);

    // Testing without data so a new instance is created
    _lst.setup_node(temp_node, LiveSchemaTree::ForeignKey);
    pdata = dynamic_cast<LiveSchemaTree::FKData*>(temp_node->get_data());
    ensure("TF005CHK010: Unexpected data object created", pdata != NULL);
    ensure("TF005CHK010: Unexpected data returned 2", pdata != pdata_temp);
    ensure_equals("TF005CHK010: Unexpected data type returned", pdata->get_type(), LiveSchemaTree::ForeignKey);

    // Checking for icon setup...
    // path =
    // bec::IconManager::get_instance()->get_icon_file(bec::IconManager::get_instance()->get_icon_id("db.ForeignKey.side."
    // + suffix, bec::Icon16));
    // ensure_equals("TF005CHK010: Unexpected node icon", temp_node->get_string(1), path);

    node->remove_children();
    delete pdata;
    delete pdata_temp;
  }
}

/*
*  Tests the get_child_node function, not it will work in the next ways:
*  - Binary Search
*  - Sequential Search
*
*  The Binary Search is used for schemas, tables, views and routines for
*  which the assumption is that the list of elements is sorted.
*
*  The Sequential search is for the rest of the objects which should keep
*  the order in which they were created. If specified, the type parameter will
*  enforce that the searched node is of the specified type. This may not be used
*  as all the nodes containing unsorted nodes have objects of the same type
*
*/
TEST_FUNCTION(6) {
  LiveSchemaTree::LSTData* pdata = NULL;
  mforms::TreeNodeRef node = pmodel_view->root_node();
  mforms::TreeNodeRef child01 = node->add_child();
  mforms::TreeNodeRef child02 = node->add_child();
  mforms::TreeNodeRef child03 = node->add_child();
  mforms::TreeNodeRef child04 = node->add_child();
  mforms::TreeNodeRef child05 = node->add_child();
  mforms::TreeNodeRef found_node;

  // All the nodes are added to the tree root as we are not
  // Testing the structure but the search operation..
  _lst.setup_node(child01, LiveSchemaTree::Schema);
  _lst.setup_node(child02, LiveSchemaTree::Table);
  _lst.setup_node(child03, LiveSchemaTree::View);
  _lst.setup_node(child04, LiveSchemaTree::Table);
  _lst.setup_node(child05, LiveSchemaTree::View);

  child01->set_string(0, "uno");
  child02->set_string(0, "uno");
  child03->set_string(0, "uno");
  child04->set_string(0, "dos");
  child05->set_string(0, "dos");

  {
    found_node = _lst.get_child_node(node, "uno", LiveSchemaTree::Schema, false);
    ensure_equals("TF006CHK001: Unexpected node returned", found_node, child01);
    ensure_equals("TF006CHK001: Unexpected node name returned", found_node->get_string(0), child01->get_string(0));
    ensure("TF006CHK001: Unexpected node data returned", found_node->get_data() == child01->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK001: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Schema);
  }

  {
    found_node = _lst.get_child_node(node, "uno", LiveSchemaTree::Table, false);
    ensure_equals("TF006CHK002: Unexpected node returned", found_node, child02);
    ensure_equals("TF006CHK002: Unexpected node name returned", found_node->get_string(0), child02->get_string(0));
    ensure("TF006CHK002: Unexpected node data returned", found_node->get_data() == child02->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK002: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Table);
  }

  {
    found_node = _lst.get_child_node(node, "uno", LiveSchemaTree::View, false);
    ensure_equals("TF006CHK003: Unexpected node returned", found_node, child03);
    ensure_equals("TF006CHK003: Unexpected node name returned", found_node->get_string(0), child03->get_string(0));
    ensure("TF006CHK003: Unexpected node data returned", found_node->get_data() == child03->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK003: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::View);
  }

  {
    found_node = _lst.get_child_node(node, "dos", LiveSchemaTree::Table, false);
    ensure_equals("TF006CHK004: Unexpected node returned", found_node, child04);
    ensure_equals("TF006CHK004: Unexpected node name returned", found_node->get_string(0), child04->get_string(0));
    ensure("TF006CHK004: Unexpected node data returned", found_node->get_data() == child04->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK004: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Table);
  }

  {
    found_node = _lst.get_child_node(node, "dos", LiveSchemaTree::View, false);
    ensure_equals("TF006CHK005: Unexpected node returned", found_node, child05);
    ensure_equals("TF006CHK005: Unexpected node name returned", found_node->get_string(0), child05->get_string(0));
    ensure("TF006CHK005: Unexpected node data returned", found_node->get_data() == child05->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK005: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::View);
  }

  {
    found_node = _lst.get_child_node(node, "tres", LiveSchemaTree::View, false);
    ensure("TF006CHK006: Unexpected node found", !found_node.is_valid());
  }

  // Now we create a series of tables
  std::vector<mforms::TreeNodeRef> tables;
  for (size_t index = 0; index < 50; index++) {
    tables.push_back(child01->add_child());
    _lst.setup_node(tables[index], LiveSchemaTree::Table);
    tables[index]->set_string(0, base::strfmt("Table%02d", (int)(index + 1)));
  }

  {
    found_node = _lst.get_child_node(child01, "Table01", LiveSchemaTree::Table, true);
    ensure_equals("TF006CHK010: Unexpected node returned", found_node, tables[0]);
    ensure_equals("TF006CHK010: Unexpected node name returned", found_node->get_string(0), tables[0]->get_string(0));
    ensure("TF006CHK010: Unexpected node data returned", found_node->get_data() == tables[0]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK010: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Table);
  }

  {
    found_node = _lst.get_child_node(child01, "Table26", LiveSchemaTree::Table, true);
    ensure_equals("TF006CHK011: Unexpected node returned", found_node, tables[25]);
    ensure_equals("TF006CHK011: Unexpected node name returned", found_node->get_string(0), tables[25]->get_string(0));
    ensure("TF006CHK011: Unexpected node data returned", found_node->get_data() == tables[25]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK011: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Table);
  }

  {
    found_node = _lst.get_child_node(child01, "Table50", LiveSchemaTree::Table, true);
    ensure_equals("TF006CHK012: Unexpected node returned", found_node, tables[49]);
    ensure_equals("TF006CHK012: Unexpected node name returned", found_node->get_string(0), tables[49]->get_string(0));
    ensure("TF006CHK012: Unexpected node data returned", found_node->get_data() == tables[49]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK012: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Table);
  }

  {
    found_node = _lst.get_child_node(child01, "Table51", LiveSchemaTree::View, true);
    ensure("TF006CHK013: Unexpected node found", !found_node.is_valid());
  }

  // Now we create a series of procedures and functions
  std::vector<mforms::TreeNodeRef> procedures;
  for (size_t index = 0; index < 25; index++) {
    procedures.push_back(child02->add_child());
    _lst.setup_node(procedures[index], LiveSchemaTree::Procedure);
    procedures[index]->set_string(0, base::strfmt("Procedure%02d", (int)(index + 1)));
  }

  {
    found_node = _lst.get_child_node(child02, "Procedure01", LiveSchemaTree::Procedure, true);
    ensure_equals("TF006CHK015: Unexpected node returned", found_node, procedures[0]);
    ensure_equals("TF006CHK015: Unexpected node name returned", found_node->get_string(0),
                  procedures[0]->get_string(0));
    ensure("TF006CHK015: Unexpected node data returned", found_node->get_data() == procedures[0]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK015: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Procedure);
  }

  {
    found_node = _lst.get_child_node(child02, "Procedure13", LiveSchemaTree::Procedure, true);
    ensure_equals("TF006CHK016: Unexpected node returned", found_node, procedures[12]);
    ensure_equals("TF006CHK016: Unexpected node name returned", found_node->get_string(0),
                  procedures[12]->get_string(0));
    ensure("TF006CHK016: Unexpected node data returned", found_node->get_data() == procedures[12]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK016: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Procedure);
  }

  {
    found_node = _lst.get_child_node(child02, "Procedure25", LiveSchemaTree::Procedure, true);
    ensure_equals("TF006CHK017: Unexpected node returned", found_node, procedures[24]);
    ensure_equals("TF006CHK017: Unexpected node name returned", found_node->get_string(0),
                  procedures[24]->get_string(0));
    ensure("TF006CHK017: Unexpected node data returned", found_node->get_data() == procedures[24]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK017: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Procedure);
  }

  {
    found_node = _lst.get_child_node(child02, "Procedure26", LiveSchemaTree::Procedure, true);
    ensure("TF006CHK018: Unexpected node found", !found_node.is_valid());
  }

  std::vector<mforms::TreeNodeRef> functions;
  for (size_t index = 0; index < 25; index++) {
    functions.push_back(child03->add_child());
    _lst.setup_node(functions[index], LiveSchemaTree::Function);
    functions[index]->set_string(0, base::strfmt("Function%02d", (int)(index + 1)));
  }

  {
    found_node = _lst.get_child_node(child03, "Function01", LiveSchemaTree::Function, true);
    ensure_equals("TF006CHK020: Unexpected node returned", found_node, functions[0]);
    ensure_equals("TF006CHK020: Unexpected node name returned", found_node->get_string(0), functions[0]->get_string(0));
    ensure("TF006CHK020: Unexpected node data returned", found_node->get_data() == functions[0]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK020: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Function);
  }

  {
    found_node = _lst.get_child_node(child03, "Function13", LiveSchemaTree::Function, true);
    ensure_equals("TF006CHK021: Unexpected node returned", found_node, functions[12]);
    ensure_equals("TF006CHK021: Unexpected node name returned", found_node->get_string(0),
                  functions[12]->get_string(0));
    ensure("TF006CHK021: Unexpected node data returned", found_node->get_data() == functions[12]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK021: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Function);
  }

  {
    found_node = _lst.get_child_node(child03, "Function25", LiveSchemaTree::Function, true);
    ensure_equals("TF006CHK022: Unexpected node returned", found_node, functions[24]);
    ensure_equals("TF006CHK022: Unexpected node name returned", found_node->get_string(0),
                  functions[24]->get_string(0));
    ensure("TF006CHK022: Unexpected node data returned", found_node->get_data() == functions[24]->get_data());
    pdata = dynamic_cast<LiveSchemaTree::LSTData*>(found_node->get_data());
    ensure_equals("TF006CHK022: Unexpected node type returned", pdata->get_type(), LiveSchemaTree::Function);
  }

  {
    found_node = _lst.get_child_node(child03, "Function26", LiveSchemaTree::Function, true);
    ensure("TF006CHK023: Unexpected node found", !found_node.is_valid());
  }
}

/*
*  Tests the update_node_children function
*/
TEST_FUNCTION(7) {
  mforms::TreeNodeRef node = pmodel_view->root_node();
  mforms::TreeNodeRef node_filtered = pmodel_view->root_node();
  base::StringListPtr children01(new std::list<std::string>());
  base::StringListPtr children02(new std::list<std::string>());
  children01->push_back("actor");
  children01->push_back("address");
  children01->push_back("client");
  children02->push_back("client");
  children02->push_back("film");
  children02->push_back("movie");

  // Clears the node to have a clean start of the test
  node->remove_children();
  ensure_equals("TF001CHK001: Unexpected number of schema nodes", node->count(), 0);

  // The first update will add the client nodes into the root
  ensure("TF007CHK002: Unexpected failure updating children",
         _lst.update_node_children(node, children01, LiveSchemaTree::Schema));
  ensure_equals("TF007CHK002: Unexpected number of nodes", node->count(), 3);
  ensure("TF007CHK002: Unable to find node actor", _lst.get_child_node(node, "actor"));
  ensure("TF007CHK002: Unable to find node address", _lst.get_child_node(node, "address"));
  ensure("TF007CHK002: Unable to find node client", _lst.get_child_node(node, "client"));

  // Testing an operation that will result in no changes
  ensure("TF007CHK003: Unexpected failure updating children",
         !_lst.update_node_children(node, children01, LiveSchemaTree::Schema));
  ensure_equals("TF007CHK003: Unexpected number of nodes", node->count(), 3);

  // Testing an update removing nodes for unexisting names and appending new nodes
  ensure("TF007CHK004: Unexpected failure updating children",
         _lst.update_node_children(node, children02, LiveSchemaTree::Schema, true));
  ensure_equals("TF007CHK004: Unexpected number of nodes", node->count(), 3);
  ensure("TF007CHK004: Found unexpected node actor", !_lst.get_child_node(node, "actor"));
  ensure("TF007CHK004: Found unexpected node address", !_lst.get_child_node(node, "address"));
  ensure("TF007CHK004: Unable to find node client", _lst.get_child_node(node, "client"));
  ensure("TF007CHK004: Unable to find node film", _lst.get_child_node(node, "film"));
  ensure("TF007CHK004: Unable to find node movie", _lst.get_child_node(node, "movie"));

  children01->push_back("actor");
  children01->push_back("address");
  children01->push_back("client");

  // Testing an update removing nodes for unexisting names and appending new nodes
  ensure("TF007CHK005: Unexpected failure updating children",
         _lst.update_node_children(node, children01, LiveSchemaTree::Schema, true, true));
  ensure_equals("TF007CHK005: Unexpected number of nodes", node->count(), 5);
  ensure("TF007CHK005: Unable to find node actor", _lst.get_child_node(node, "actor"));
  ensure("TF007CHK005: Unable to find node address", _lst.get_child_node(node, "address"));
  ensure("TF007CHK005: Unable to find node client", _lst.get_child_node(node, "client"));
  ensure("TF007CHK005: Unable to find node film", _lst.get_child_node(node, "film"));
  ensure("TF007CHK005: Unable to find node movie", _lst.get_child_node(node, "movie"));

  // Repeat the tests using a filtered tree..
  _lst_filtered.set_base(&_lst);
  _lst_filtered.set_filter("*e*.*");
  _lst_filtered.filter_data();

  node_filtered = pmodel_view_filtered->root_node();

  ensure_equals("TF007CHK011: Unexpected number of schema nodes", node_filtered->count(), 3);

  // The first update will add the client nodes into the root
  children01->push_back("filtered");
  children01->push_back("finally");
  children01->push_back("done");
  ensure("TF007CHK002: Unexpected failure updating children",
         _lst_filtered.update_node_children(node_filtered, children01, LiveSchemaTree::Schema, true, true));
  ensure_equals("TF007CHK012: Unexpected number of nodes on base", node->count(), 8);
  ensure_equals("TF007CHK012: Unexpected number of nodes on filtered", node_filtered->count(), 5);
  ensure("TF007CHK012: Unable to find node address", _lst_filtered.get_child_node(node_filtered, "address"));
  ensure("TF007CHK012: Unable to find node client", _lst_filtered.get_child_node(node_filtered, "client"));
  ensure("TF007CHK012: Unable to find node done", _lst_filtered.get_child_node(node_filtered, "done"));
  ensure("TF007CHK012: Unable to find node filtered", _lst_filtered.get_child_node(node_filtered, "filtered"));
  ensure("TF007CHK012: Unable to find node movie", _lst_filtered.get_child_node(node_filtered, "movie"));

  // Testing an operation that will result in no changes
  ensure("TF007CHK013: Unexpected failure updating children",
         !_lst_filtered.update_node_children(node_filtered, children01, LiveSchemaTree::Schema, true, true));
  ensure_equals("TF007CHK013: Unexpected number of nodes on base", node->count(), 8);
  ensure_equals("TF007CHK013: Unexpected number of nodes on filtered", node_filtered->count(), 5);

  children02->push_back("client");
  children02->push_back("customer");

  // Testing an update removing nodes for unexisting names and appending new nodes
  ensure("TF007CHK014: Unexpected failure updating children",
         _lst_filtered.update_node_children(node_filtered, children02, LiveSchemaTree::Schema, true, false));
  ensure_equals("TF007CHK014: Unexpected number of nodes on base ", node->count(), 4);
  ensure_equals("TF007CHK014: Unexpected number of nodes on filtered", node_filtered->count(), 3);
  ensure("TF007CHK014: Unable to find node client", _lst_filtered.get_child_node(node_filtered, "client"));
  ensure("TF007CHK014: Unable to find node customer", _lst_filtered.get_child_node(node_filtered, "customer"));
  ensure("TF007CHK014: Unable to find node movie", _lst_filtered.get_child_node(node_filtered, "movie"));

  children01->push_back("actor");
  children01->push_back("address");
  children01->push_back("client");
  children01->push_back("film");
  children01->push_back("filtered");
  children01->push_back("finally");
  children01->push_back("movie");
  children01->push_back("done");

  // Testing an update removing nodes for unexisting names and appending new nodes
  ensure("TF007CHK015: Unexpected failure updating children",
         _lst_filtered.update_node_children(node_filtered, children01, LiveSchemaTree::Schema, true, false));
  ensure_equals("TF007CHK015: Unexpected number of nodes on base", node->count(), 8);
  ensure_equals("TF007CHK015: Unexpected number of nodes on filtered", node_filtered->count(), 5);
  ensure("TF007CHK015: Unable to find node address", _lst_filtered.get_child_node(node_filtered, "address"));
  ensure("TF007CHK015: Unable to find node client", _lst_filtered.get_child_node(node_filtered, "client"));
  ensure("TF007CHK015: Unable to find node client", _lst_filtered.get_child_node(node_filtered, "filtered"));
  ensure("TF007CHK015: Unable to find node client", _lst_filtered.get_child_node(node_filtered, "movie"));
  ensure("TF007CHK015: Unable to find node client", _lst_filtered.get_child_node(node_filtered, "done"));

  node->remove_children();
  node_filtered->remove_children();
}

/*
*  Tests the set_active_schema function
*/
TEST_FUNCTION(8) {
  mforms::TreeNodeRef node = pmodel_view->root_node();
  mforms::TreeNodeRef schema;
  base::StringListPtr schemas(new std::list<std::string>());

  schemas->push_back("one");
  schemas->push_back("two");
  schemas->push_back("three");

  _lst.update_node_children(node, schemas, LiveSchemaTree::Schema, true, true);

  _lst_filtered.set_base(&_lst);
  _lst_filtered.set_filter("*e*");
  _lst_filtered.filter_data();

  _lst_filtered.set_active_schema("one");
  ensure_equals("TF008CHK001: Unexpected active shchema", _tester_filtered.get_active_schema(), "one");
  ensure_equals("TF008CHK001: Unexpected active shchema on base", _tester.get_active_schema(), "one");

  _lst_filtered.set_active_schema("three");
  ensure_equals("TF008CHK002: Unexpected active shchema", _tester_filtered.get_active_schema(), "three");
  ensure_equals("TF008CHK002: Unexpected active shchema on base", _tester.get_active_schema(), "three");

  node->remove_children();
  pmodel_view_filtered->root_node()->remove_children();
}

/*
*  Tests the function update_schemata
*/
TEST_FUNCTION(9) {
  mforms::TreeNodeRef node = pmodel_view->root_node();
  mforms::TreeNodeRef schema;
  base::StringListPtr schemas(new std::list<std::string>());
  LiveSchemaTree::SchemaData* pdata = NULL;

  ensure_equals("TF009CHK001: Unexpected number of nodes in root", node->count(), 0);

  schemas->push_back("one");
  schemas->push_back("two");
  schemas->push_back("three");

  _lst.update_schemata(schemas);

  ensure_equals("TF009CHK001: Unexpected number of nodes in root", node->count(), 3);

  schema = _lst.get_child_node(node, "one");
  ensure("TF009CHK002: Unable to find schema one", schema);
  pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());
  ensure("TF009CHK002: Unable to get the schema data", pdata != NULL);
  ensure_equals("TF009CHK002: Invalid schema data", pdata->get_type(), LiveSchemaTree::Schema);
  pdata = NULL;

  schema = _lst.get_child_node(node, "two");
  ensure("TF009CHK003: Unable to find schema two", schema);
  pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());
  ensure("TF009CHK003: Unable to get the schema data", pdata != NULL);
  ensure_equals("TF009CHK003: Invalid schema data", pdata->get_type(), LiveSchemaTree::Schema);
  pdata = NULL;

  schema = _lst.get_child_node(node, "three");
  ensure("TF009CHK004: Unable to find schema three", schema);
  pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());
  ensure("TF009CHK004: Unable to get the schema data", pdata != NULL);
  ensure_equals("TF009CHK004: Invalid schema data", pdata->get_type(), LiveSchemaTree::Schema);

  // Simulating schema expansion to ensure a loaded schema triggers a data reload
  schema->expand();
  pdata->fetched = true;
  deleg->expect_fetch_schema_contents_call();
  deleg->_mock_call_back_slot = false;
  deleg->_mock_schema_name = "three";

  _lst.update_schemata(schemas);

  deleg->check_and_reset("TF009CHK004");

  node->remove_children();
}

/*
*  Tests the function load_schema_content
*/
TEST_FUNCTION(10) {
  mforms::TreeNodeRef node_base = pmodel_view->root_node();
  mforms::TreeNodeRef node = pmodel_view_filtered->root_node();
  mforms::TreeNodeRef schema;
  mforms::TreeNodeRef schema_base;
  mforms::TreeNodeRef child;
  base::StringListPtr schemas(new std::list<std::string>());
  LiveSchemaTree::SchemaData* pdata = NULL;

  ensure_equals("TF010CHK001: Unexpected number of nodes in root", node->count(), 0);

  schemas->push_back("one");
  schemas->push_back("two");
  schemas->push_back("three");

  _lst.update_schemata(schemas);

  _lst_filtered.set_base(&_lst);
  _lst_filtered.set_filter("one");
  _lst_filtered.filter_data();

  schema_base = _lst.get_child_node(node_base, "one");
  schema = _lst_filtered.get_child_node(node, "one");
  pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());

  // Validates the previous state...
  ensure("TF010CHK002: Unexpected pre fetched state", !pdata->fetched);
  ensure("TF010CHK002: Unexpected pre fetching state", !pdata->fetching);
  ensure_equals("TF010CHK002: Unexpected pre caption for tables",
                schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0), LiveSchemaTree::TABLES_CAPTION);
  ensure_equals("TF010CHK002: Unexpected pre caption for views",
                schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0), LiveSchemaTree::VIEWS_CAPTION);
  ensure_equals("TF010CHK002: Unexpected pre caption for procedures",
                schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION);
  ensure_equals("TF010CHK002: Unexpected pre caption for functions",
                schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION);

  ensure_equals("TF010CHK002: Unexpected pre caption for tables on base",
                schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::TABLES_CAPTION);
  ensure_equals("TF010CHK002: Unexpected pre caption for views on base",
                schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0), LiveSchemaTree::VIEWS_CAPTION);
  ensure_equals("TF010CHK002: Unexpected pre caption for procedures on base",
                schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION);
  ensure_equals("TF010CHK002: Unexpected pre caption for functions on base",
                schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION);

  // Simulating schema expansion to ensure a loaded schema triggers a data reload
  deleg_filtered->expect_fetch_schema_contents_call();
  deleg_filtered->_mock_call_back_slot = false;
  deleg_filtered->_mock_schema_name = "one";

  _tester_filtered.load_schema_content(schema);

  deleg_filtered->check_and_reset("TF010CHK002");

  // Validates the previous state...
  ensure("TF010CHK002: Unexpected post fetching state", pdata->fetching);
  ensure_equals("TF010CHK002: Unexpected post caption for tables",
                schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::TABLES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
  ensure_equals("TF010CHK002: Unexpected post caption for views",
                schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::VIEWS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
  ensure_equals("TF010CHK002: Unexpected post caption for procedures",
                schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
  ensure_equals("TF010CHK002: Unexpected post caption for functions",
                schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);

  ensure_equals("TF010CHK002: Unexpected post caption for tables",
                schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::TABLES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
  ensure_equals("TF010CHK002: Unexpected post caption for views",
                schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::VIEWS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
  ensure_equals("TF010CHK002: Unexpected post caption for procedures",
                schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);
  ensure_equals("TF010CHK002: Unexpected post caption for functions",
                schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION + " " + LiveSchemaTree::FETCHING_CAPTION);

  pmodel_view->root_node()->remove_children();
  pmodel_view_filtered->root_node()->remove_children();
}

/*
*  Tests the function schema_content_arrived
*/
TEST_FUNCTION(11) {
  mforms::TreeNodeRef node_base = pmodel_view->root_node();
  mforms::TreeNodeRef node = pmodel_view_filtered->root_node();
  mforms::TreeNodeRef schema;
  mforms::TreeNodeRef schema_base;
  mforms::TreeNodeRef child;
  base::StringListPtr schemas(new std::list<std::string>());
  LiveSchemaTree::SchemaData* pdata = NULL;

  ensure_equals("TF011CHK001: Unexpected number of nodes in root", node->count(), 0);

  schemas->push_back("one");
  schemas->push_back("two");
  schemas->push_back("three");

  _lst.update_schemata(schemas);

  _lst_filtered.set_base(&_lst);
  _lst_filtered.set_filter("one");
  _lst_filtered.filter_data();

  schema_base = _lst.get_child_node(node_base, "one");
  schema = _lst_filtered.get_child_node(node, "one");
  pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());

  // Validates the previous state...
  ensure("TF011CHK002: Unexpected pre fetching state", !pdata->fetching);
  ensure_equals("TF011CHK002: Unexpected pre caption for tables",
                schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0), LiveSchemaTree::TABLES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected pre caption for views",
                schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0), LiveSchemaTree::VIEWS_CAPTION);
  ensure_equals("TF011CHK002: Unexpected pre caption for procedures",
                schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected pre caption for functions",
                schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION);

  ensure_equals("TF011CHK002: Unexpected pre caption for tables on base",
                schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::TABLES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected pre caption for views on base",
                schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0), LiveSchemaTree::VIEWS_CAPTION);
  ensure_equals("TF011CHK002: Unexpected pre caption for procedures on base",
                schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected pre caption for functions on base",
                schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION);

  // Simulating schema expansion to ensure a loaded schema triggers a data reload
  deleg_filtered->expect_fetch_schema_contents_call();
  deleg_filtered->_mock_call_back_slot = true;
  deleg_filtered->_mock_schema_name = "one";
  deleg_filtered->_mock_table_list->push_back("table1");
  deleg_filtered->_mock_table_list->push_back("table2");
  deleg_filtered->_mock_table_list->push_back("table3");
  deleg_filtered->_mock_view_list->push_back("view1");
  deleg_filtered->_mock_view_list->push_back("view2");
  deleg_filtered->_mock_procedure_list->push_back("procedure1");
  deleg_filtered->_mock_function_list->push_back("function1");

  _tester_filtered.load_schema_content(schema);

  deleg_filtered->check_and_reset("TF011CHK002");

  schema = _lst_filtered.get_child_node(node, "one");

  // Validates the previous state...
  ensure("TF011CHK002: Unexpected post fetched state", pdata->fetched);
  ensure("TF011CHK002: Unexpected post fetching state", !pdata->fetching);

  child = schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for tables", child->get_string(0),
                LiveSchemaTree::TABLES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of tables", child->count(), 3);

  child = schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for views", child->get_string(0), LiveSchemaTree::VIEWS_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of views", child->count(), 2);

  child = schema->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for procedures", child->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of procedures", child->count(), 1);

  child = schema->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for functions", child->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of functions", child->count(), 1);

  child = schema_base->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for tables", child->get_string(0),
                LiveSchemaTree::TABLES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of tables", child->count(), 3);

  child = schema_base->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for views", child->get_string(0), LiveSchemaTree::VIEWS_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of views", child->count(), 2);

  child = schema_base->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for procedures", child->get_string(0),
                LiveSchemaTree::PROCEDURES_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of procedures", child->count(), 1);

  child = schema_base->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
  ensure_equals("TF011CHK002: Unexpected post caption for functions", child->get_string(0),
                LiveSchemaTree::FUNCTIONS_CAPTION);
  ensure_equals("TF011CHK002: Unexpected number of functions", child->count(), 1);

  node->remove_children();
  node_base->remove_children();

  //_lst_filtered.set_base(&_lst);

  // ensure_equals("TF011CHK001: Unexpected number of nodes in root", node->count(), 0);

  // schemas.push_back("one");
  // schemas.push_back("two");
  // schemas.push_back("three");

  //_lst.update_schemata(schemas);

  // schema = _lst.get_child_node(node, "one");
  // pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(schema->get_data());

  //// Validates the previous state...
  // ensure("TF011CHK002: Unexpected pre fetching state", !pdata->fetching);
  // ensure_equals("TF011CHK002: Unexpected pre caption for tables",
  // schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_string(0), LiveSchemaTree::TABLES_CAPTION);
  // ensure_equals("TF011CHK002: Unexpected pre caption for views",
  // schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_string(0), LiveSchemaTree::VIEWS_CAPTION);
  // ensure_equals("TF011CHK002: Unexpected pre caption for routines",
  // schema->get_child(LiveSchemaTree::ROUTINES_NODE_INDEX)->get_string(0), LiveSchemaTree::ROUTINES_CAPTION);

  //// Simulating schema expansion to ensure a loaded schema triggers a data reload
  // deleg->_expect_fetch_schema_contents_call = true;
  // deleg->_mock_call_back_slot = true;
  // deleg->_mock_schema_name = "one";
  // deleg->_mock_table_list->push_back("table1");
  // deleg->_mock_table_list->push_back("table2");
  // deleg->_mock_table_list->push_back("table3");
  // deleg->_mock_view_list->push_back("view1");
  // deleg->_mock_view_list->push_back("view2");
  // deleg->_mock_procedure_list->push_back("procedure1");
  // deleg->_mock_function_list->push_back("function1");

  //_tester.load_schema_content(schema);
  //
  // deleg->check_and_reset("TF011CHK002");

  //// Validates the previous state...
  // ensure("TF011CHK002: Unexpected post fetched state", pdata->fetched);
  // ensure("TF011CHK002: Unexpected post fetching state", !pdata->fetching);

  // child = schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
  // ensure_equals("TF011CHK002: Unexpected post caption for tables", child->get_string(0),
  // LiveSchemaTree::TABLES_CAPTION);
  // ensure_equals("TF011CHK002: Unexpected number of tables", child->count(), 3);

  // child = schema->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
  // ensure_equals("TF011CHK002: Unexpected post caption for views", child->get_string(0),
  // LiveSchemaTree::VIEWS_CAPTION);
  // ensure_equals("TF011CHK002: Unexpected number of views", child->count(), 2);

  // child = schema->get_child(LiveSchemaTree::ROUTINES_NODE_INDEX);
  // ensure_equals("TF011CHK002: Unexpected post caption for routines", child->get_string(0),
  // LiveSchemaTree::ROUTINES_CAPTION);
  // ensure_equals("TF011CHK002: Unexpected number of routines", child->count(), 2);

  // node->remove_children();
}

/*
*  Tests the load_table_details logic
*/
TEST_FUNCTION(12) {
  mforms::TreeNodeRef node = pmodel_view->root_node();
  mforms::TreeNodeRef schema;
  mforms::TreeNodeRef table;
  base::StringListPtr schemas(new std::list<std::string>());
  LiveSchemaTree::TableData* pdata = NULL;

  ensure_equals("TF012CHK001: Unexpected number of nodes in root", node->count(), 0);

  schemas->push_back("one");

  _lst.update_schemata(schemas);

  schema = _lst.get_child_node(node, "one");

  // Simulating schema expansion to ensure a loaded schema triggers a data reload
  deleg->expect_fetch_schema_contents_call();
  deleg->_mock_call_back_slot = true;
  deleg->_mock_schema_name = "one";
  deleg->_mock_table_list->push_back("table1");
  deleg->_mock_table_list->push_back("table2");
  deleg->_mock_view_list->push_back("view1");
  deleg->_mock_procedure_list->push_back("procedure1");
  deleg->_mock_function_list->push_back("function1");
  deleg->_check_id = "TF012CHK001";

  _tester.load_schema_content(schema);
  deleg->check_and_reset("TF012CHK001");

  // Initial test, nothing has been loaded
  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_flags = LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA;
  deleg->_mock_schema_name = "one";
  deleg->_mock_object_name = "table1";
  deleg->_mock_object_type = LiveSchemaTree::Table;
  deleg->_check_id = "TF012CHK002";

  _lst.load_table_details(LiveSchemaTree::Table, "one", "table1",
                          LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);

  deleg->check_and_reset("TF012CHK002");

  // Initial test, same loading requested but as it's already in the process of
  // loading ( because of the previous step ) no fetch call is done
  deleg->_expect_fetch_object_details_call = false;
  deleg->_check_id = "TF012CHK003";
  _lst.load_table_details(LiveSchemaTree::Table, "one", "table1",
                          LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
  deleg->check_and_reset("TF012CHK003");

  // Third test, reloading existing data and additional info, causes only additional info
  // to be requested
  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_flags = LiveSchemaTree::TRIGGER_DATA;
  deleg->_check_id = "TF012CHK004";
  _lst.load_table_details(LiveSchemaTree::Table, "one", "table1",
                          LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA);
  deleg->check_and_reset("TF012CHK004");

  // Repeat the tests but now marking some information as already loaded
  table = _lst.get_child_node(schema->get_child(LiveSchemaTree::TABLES_NODE_INDEX), "table2");
  pdata = dynamic_cast<LiveSchemaTree::TableData*>(table->get_data());

  pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA);

  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_flags = LiveSchemaTree::INDEX_DATA;
  deleg->_mock_schema_name = "one";
  deleg->_mock_object_name = "table2";
  deleg->_mock_object_type = LiveSchemaTree::Table;
  deleg->_check_id = "TF012CHK005";

  _lst.load_table_details(LiveSchemaTree::Table, "one", "table2",
                          LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);

  deleg->check_and_reset("TF012CHK005");

  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_flags = LiveSchemaTree::TRIGGER_DATA | LiveSchemaTree::FK_DATA;
  deleg->_mock_schema_name = "one";
  deleg->_mock_object_name = "table2";
  deleg->_mock_object_type = LiveSchemaTree::Table;
  deleg->_check_id = "TF012CHK006";

  _lst.load_table_details(
    LiveSchemaTree::Table, "one", "table2",
    LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA | LiveSchemaTree::TRIGGER_DATA | LiveSchemaTree::FK_DATA);

  deleg->check_and_reset("TF012CHK006");
}

/*
*  Tests the identifiers_equal function
*/
TEST_FUNCTION(13) {
  _lst.set_case_sensitive_identifiers(true);

  ensure_equals("TF013CHK001: Unexpected no case sensitive identifiers", _tester.get_case_sensitive_identifiers(),
                true);
  ensure("TF013CHK001: Unexpected identifiers equal", !_tester.identifiers_equal("first", "First"));
  ensure("TF013CHK001: Unexpected identifiers different", _tester.identifiers_equal("second", "second"));

  _lst.set_case_sensitive_identifiers(false);

  ensure_equals("TF013CHK001: Unexpected case sensitive identifiers", _tester.get_case_sensitive_identifiers(), false);
  ensure("TF013CHK001: Unexpected identifiers different", _tester.identifiers_equal("first", "First"));
  ensure("TF013CHK001: Unexpected identifiers different", _tester.identifiers_equal("second", "second"));
}

/*
*  Tests the is_object_type function
*/
TEST_FUNCTION(14) {
  // Testing for database objects...
  ensure("TF014CHK001: Unexpected non database object Schema",
         _tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Schema));
  ensure("TF014CHK001: Unexpected non database object Table",
         _tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Table));
  ensure("TF014CHK001: Unexpected non database object View",
         _tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::View));
  ensure("TF014CHK001: Unexpected non database object Procedure",
         _tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Procedure));
  ensure("TF014CHK001: Unexpected non database object Function",
         _tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Function));

  ensure("TF014CHK001: Unexpected database object TableCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::TableCollection));
  ensure("TF014CHK001: Unexpected database object ViewCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ViewCollection));
  ensure("TF014CHK001: Unexpected database object ProcedureCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ProcedureCollection));
  ensure("TF014CHK001: Unexpected database object FunctionCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::FunctionCollection));

  ensure("TF014CHK001: Unexpected database object ColumnCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ColumnCollection));
  ensure("TF014CHK001: Unexpected database object IndexCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::IndexCollection));
  ensure("TF014CHK001: Unexpected database object TriggerCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::TriggerCollection));
  ensure("TF014CHK001: Unexpected database object ForeignKeyCollection",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ForeignKeyCollection));

  ensure("TF014CHK001: Unexpected database object Trigger",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Trigger));
  ensure("TF014CHK001: Unexpected database object TableColumn",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::TableColumn));
  ensure("TF014CHK001: Unexpected database object ViewColumn",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ViewColumn));
  ensure("TF014CHK001: Unexpected database object ForeignKey",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ForeignKey));
  ensure("TF014CHK001: Unexpected database object Index",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Index));
  ensure("TF014CHK001: Unexpected database object ForeignKeyColumn",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::ForeignKeyColumn));
  ensure("TF014CHK001: Unexpected database object IndexColumn",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::IndexColumn));
  ensure("TF014CHK001: Unexpected database object Any",
         !_tester.is_object_type(LiveSchemaTree::DatabaseObject, LiveSchemaTree::Any));

  // Testing for schema objects...
  ensure("TF014CHK002: Unexpected non schema object Table",
         _tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Table));
  ensure("TF014CHK002: Unexpected non schema object View",
         _tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::View));
  ensure("TF014CHK002: Unexpected non schema object Procedure",
         _tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Procedure));
  ensure("TF014CHK002: Unexpected non schema object Function",
         _tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Function));

  ensure("TF014CHK002: Unexpected schema object Schema",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Schema));
  ensure("TF014CHK002: Unexpected schema object TableCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::TableCollection));
  ensure("TF014CHK002: Unexpected schema object ViewCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ViewCollection));
  ensure("TF014CHK002: Unexpected schema object ProcedureCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ProcedureCollection));
  ensure("TF014CHK002: Unexpected schema object FunctionCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::FunctionCollection));

  ensure("TF014CHK002: Unexpected schema object ColumnCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ColumnCollection));
  ensure("TF014CHK002: Unexpected schema object IndexCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::IndexCollection));
  ensure("TF014CHK002: Unexpected schema object TriggerCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::TriggerCollection));
  ensure("TF014CHK002: Unexpected schema object ForeignKeyCollection",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ForeignKeyCollection));

  ensure("TF014CHK002: Unexpected schema object Trigger",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Trigger));
  ensure("TF014CHK002: Unexpected schema object TableColumn",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::TableColumn));
  ensure("TF014CHK002: Unexpected schema object ViewColumn",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ViewColumn));
  ensure("TF014CHK002: Unexpected schema object ForeignKey",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ForeignKey));
  ensure("TF014CHK002: Unexpected schema object Index",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Index));
  ensure("TF014CHK002: Unexpected schema object ForeignKeyColumn",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::ForeignKeyColumn));
  ensure("TF014CHK002: Unexpected schema object IndexColumn",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::IndexColumn));
  ensure("TF014CHK002: Unexpected schema object Any",
         !_tester.is_object_type(LiveSchemaTree::SchemaObject, LiveSchemaTree::Any));

  // Testing for table/view objects...
  ensure("TF014CHK002: Unexpected non table or view Table",
         _tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Table));
  ensure("TF014CHK002: Unexpected non table or view View",
         _tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::View));

  ensure("TF014CHK003: Unexpected table or view Schema",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Schema));
  ensure("TF014CHK003: Unexpected table or view Procedure",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Procedure));
  ensure("TF014CHK003: Unexpected table or view Function",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Function));
  ensure("TF014CHK003: Unexpected table or view TableCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::TableCollection));
  ensure("TF014CHK003: Unexpected table or view ViewCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ViewCollection));
  ensure("TF014CHK003: Unexpected table or view ProcedureCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ProcedureCollection));
  ensure("TF014CHK003: Unexpected table or view FunctionCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::FunctionCollection));

  ensure("TF014CHK003: Unexpected table or view ColumnCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ColumnCollection));
  ensure("TF014CHK003: Unexpected table or view IndexCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::IndexCollection));
  ensure("TF014CHK003: Unexpected table or view TriggerCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::TriggerCollection));
  ensure("TF014CHK003: Unexpected table or view ForeignKeyCollection",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ForeignKeyCollection));

  ensure("TF014CHK003: Unexpected table or view Trigger",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Trigger));
  ensure("TF014CHK003: Unexpected table or view TableColumn",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::TableColumn));
  ensure("TF014CHK003: Unexpected table or view ViewColumn",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ViewColumn));
  ensure("TF014CHK003: Unexpected table or view ForeignKey",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ForeignKey));
  ensure("TF014CHK003: Unexpected table or view Index",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Index));
  ensure("TF014CHK003: Unexpected table or view ForeignKeyColumn",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::ForeignKeyColumn));
  ensure("TF014CHK003: Unexpected table or view IndexColumn",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::IndexColumn));
  ensure("TF014CHK003: Unexpected table or view Any",
         !_tester.is_object_type(LiveSchemaTree::TableOrView, LiveSchemaTree::Any));
}

/*
*  Tests the set_model_view function
*/
TEST_FUNCTION(15) {
  _lst.set_model_view(NULL);
  ensure("TF014CHK001: Unexpected valid model view", _tester.get_model_view() == NULL);

  _lst.set_model_view(pmodel_view);
  ensure("TF014CHK001: Unexpected invalid model view", _tester.get_model_view() != NULL);
  ensure("TF014CHK001: Unexpected model view", _tester.get_model_view() == pmodel_view);
}

/*
*  Tests the delegate setter functions
*/
TEST_FUNCTION(16) {
  std::shared_ptr<LiveSchemaTree::Delegate> null_delegate;
  std::shared_ptr<LiveSchemaTree::FetchDelegate> null_fetch_delegate;

  {
    _lst.set_delegate(null_delegate);
    _lst.set_fetch_delegate(null_fetch_delegate);

    std::shared_ptr<LiveSchemaTree::Delegate> found_delegate = _tester.get_delegate().lock();
    std::shared_ptr<LiveSchemaTree::FetchDelegate> found_fetch_delegate = _tester.get_fetch_delegate().lock();

    ensure("TF016CHK001: Unexpected valid delegate", found_delegate.get() == NULL);
    ensure("TF016CHK001: Unexpected valid fetch delegate", found_fetch_delegate.get() == NULL);
  }

  {
    _lst.set_delegate(deleg);
    _lst.set_fetch_delegate(deleg);

    std::shared_ptr<LiveSchemaTree::Delegate> found_delegate = _tester.get_delegate().lock();
    std::shared_ptr<LiveSchemaTree::FetchDelegate> found_fetch_delegate = _tester.get_fetch_delegate().lock();

    ensure("TF016CHK002: Unexpected invalid delegate", found_delegate.get() != NULL);
    ensure("TF016CHK002: Unexpected invalid fetch delegate", found_fetch_delegate.get() != NULL);

    ensure("TF016CHK002: Unexpected delegate", found_delegate.get() == deleg.get());
    ensure("TF016CHK002: Unexpected fetch delegate", found_fetch_delegate.get() == deleg.get());
  }
}

// Test wb::LiveSchemaTree::internalize_token
TEST_FUNCTION(17) {
  ensure_equals("TF017CHK001: Invalid identifier returned for ''", wb::LiveSchemaTree::internalize_token(""), 0);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'whatever'",
                wb::LiveSchemaTree::internalize_token("whatever"), 0);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'CASCADE'",
                wb::LiveSchemaTree::internalize_token("CASCADE"), 1);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'SET NULL'",
                wb::LiveSchemaTree::internalize_token("SET NULL"), 2);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'SET DEFAULT'",
                wb::LiveSchemaTree::internalize_token("SET DEFAULT"), 3);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'RESTRICT'",
                wb::LiveSchemaTree::internalize_token("RESTRICT"), 4);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'NO ACTION'",
                wb::LiveSchemaTree::internalize_token("NO ACTION"), 5);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'BTREE'", wb::LiveSchemaTree::internalize_token("BTREE"),
                6);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'FULLTEXT'",
                wb::LiveSchemaTree::internalize_token("FULLTEXT"), 7);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'HASH'", wb::LiveSchemaTree::internalize_token("HASH"),
                8);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'RTREE'", wb::LiveSchemaTree::internalize_token("RTREE"),
                9);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'RTREE'",
                wb::LiveSchemaTree::internalize_token("SPATIAL"), 10);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'INSERT'",
                wb::LiveSchemaTree::internalize_token("INSERT"), 11);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'UPDATE'",
                wb::LiveSchemaTree::internalize_token("UPDATE"), 12);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'DELETE'",
                wb::LiveSchemaTree::internalize_token("DELETE"), 13);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'BEFORE'",
                wb::LiveSchemaTree::internalize_token("BEFORE"), 14);
  ensure_equals("TF017CHK001: Invalid identifier returned for 'AFTER'", wb::LiveSchemaTree::internalize_token("AFTER"),
                15);
}

// Test wb::LiveSchemaTree::externalize_token
TEST_FUNCTION(18) {
  ensure("TF018CHK001: Invalid token returned for ''", wb::LiveSchemaTree::externalize_token(0) == "");
  ensure("TF018CHK001: Invalid token returned for 'whatever'", wb::LiveSchemaTree::externalize_token(20) == "");
  ensure("TF018CHK001: Invalid token returned for 'CASCADE'", wb::LiveSchemaTree::externalize_token(1) == "CASCADE");
  ensure("TF018CHK001: Invalid token returned for 'SET NULL'", wb::LiveSchemaTree::externalize_token(2) == "SET NULL");
  ensure("TF018CHK001: Invalid token returned for 'SET DEFAULT'",
         wb::LiveSchemaTree::externalize_token(3) == "SET DEFAULT");
  ensure("TF018CHK001: Invalid token returned for 'RESTRICT'", wb::LiveSchemaTree::externalize_token(4) == "RESTRICT");
  ensure("TF018CHK001: Invalid token returned for 'NO ACTION'",
         wb::LiveSchemaTree::externalize_token(5) == "NO ACTION");
  ensure("TF018CHK001: Invalid token returned for 'BTREE'", wb::LiveSchemaTree::externalize_token(6) == "BTREE");
  ensure("TF018CHK001: Invalid token returned for 'FULLTEXT'", wb::LiveSchemaTree::externalize_token(7) == "FULLTEXT");
  ensure("TF018CHK001: Invalid token returned for 'HASH'", wb::LiveSchemaTree::externalize_token(8) == "HASH");
  ensure("TF018CHK001: Invalid token returned for 'RTREE'", wb::LiveSchemaTree::externalize_token(9) == "RTREE");
  ensure("TF018CHK001: Invalid token returned for 'RTREE'", wb::LiveSchemaTree::externalize_token(10) == "SPATIAL");
  ensure("TF018CHK001: Invalid token returned for 'INSERT'", wb::LiveSchemaTree::externalize_token(11) == "INSERT");
  ensure("TF018CHK001: Invalid token returned for 'UPDATE'", wb::LiveSchemaTree::externalize_token(12) == "UPDATE");
  ensure("TF018CHK001: Invalid token returned for 'DELETE'", wb::LiveSchemaTree::externalize_token(13) == "DELETE");
  ensure("TF018CHK001: Invalid token returned for 'BEFORE'", wb::LiveSchemaTree::externalize_token(14) == "BEFORE");
  ensure("TF018CHK001: Invalid token returned for 'AFTER'", wb::LiveSchemaTree::externalize_token(15) == "AFTER");
}

// Test update_live_object_state
TEST_FUNCTION(19) {
  _tester.enable_events(true);

  mforms::TreeNodeRef object_node;
  // Testing Schema Object
  {
    // Ensures the schema doesn't exist
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
    ensure("TF019CHK001: Unexpected schema found", object_node.ptr() == NULL);

    // Ensures a schema node is created
    _lst.update_live_object_state(LiveSchemaTree::Schema, "", "", "schema1");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
    ensure("TF019CHK001: Expected schema not found", object_node.ptr() != NULL);

    LiveSchemaTree::SchemaData* pdata = dynamic_cast<LiveSchemaTree::SchemaData*>(object_node->get_data());
    ensure("TF019CHK001: Expected schema data not found", pdata != NULL);
    ensure_equals("TF019CHK001: Invalid schema data found", pdata->get_type(), LiveSchemaTree::Schema);

    // Ensures a schema node is deleted
    _lst.update_live_object_state(LiveSchemaTree::Schema, "", "schema1", "");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
    ensure("TF019CHK001: Unexpected schema found", object_node.ptr() == NULL);
  }

  // Adds a schema object...
  _lst.update_live_object_state(LiveSchemaTree::Schema, "", "", "schema1");

  // Testing View Object
  {
    // Ensures the view doesn't exist
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
    ensure("TF019CHK002: Unexpected view found", object_node.ptr() == NULL);

    // Ensures a view node is created
    _lst.update_live_object_state(LiveSchemaTree::View, "schema1", "", "view1");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
    ensure("TF019CHK002: Expected view not found", object_node.ptr() != NULL);

    LiveSchemaTree::ViewData* pdata = dynamic_cast<LiveSchemaTree::ViewData*>(object_node->get_data());
    ensure("TF019CHK002: Expected view data not found", pdata != NULL);
    ensure_equals("TF019CHK002: Invalid view data found", pdata->get_type(), LiveSchemaTree::View);

    // Ensures a view node is renamed
    _lst.update_live_object_state(LiveSchemaTree::View, "schema1", "view1", "view2");
    ensure_equals("TF019CHK002: Expected rename did not occur", object_node->get_string(0), "view2");

    // Ensures a loaded data is NOT reloaded when the node is not expanded
    _lst.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "view2");
    ensure_equals("TF019CHK002: Unexpected loading flags on collapsed view", pdata->get_loading_mask(), 0);
    ensure("TF019CHK002: Unexpected loaded columns on collapsed view",
           !pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA));

    // Now expands the node operation but expanding the node
    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_schema_name = "schema1";
    deleg->_mock_object_name = "view2";
    deleg->_mock_object_type = LiveSchemaTree::View;
    deleg->_mock_flags = LiveSchemaTree::COLUMN_DATA;

    // Emulates the node expansion
    _tester.expand_toggled(object_node, true);
    object_node->expand();

    // Ensures the needed calls were done
    deleg->check_and_reset("TF019CHK002");

    // Now as the node was expanded, the data should be reloaded
    _lst.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "view2");
    ensure_equals("TF019CHK002: Unexpected loading flags on expanded view", pdata->get_loading_mask(),
                  (short)LiveSchemaTree::COLUMN_DATA);
    ensure("TF019CHK002: Unexpected loaded columns on expanded view",
           !pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA));

    // Marks the data as already loaded
    pdata->set_loading_mask(0);
    pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA);

    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_schema_name = "schema1";
    deleg->_mock_object_name = "view2";
    deleg->_mock_object_type = LiveSchemaTree::View;
    deleg->_mock_flags = LiveSchemaTree::COLUMN_DATA;

    // Now as the node was expanded, the data should be reloaded
    _lst.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "view2");
    ensure_equals("TF019CHK002: Unexpected loading flags on expanded view", pdata->get_loading_mask(),
                  (short)LiveSchemaTree::COLUMN_DATA);
    ensure("TF019CHK002: Unexpected loaded columns on expanded view",
           !pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA));

    // Ensures the needed calls were done
    deleg->check_and_reset("TF019CHK002");

    // Ensures a view node is deleted
    _lst.update_live_object_state(LiveSchemaTree::View, "schema1", "view2", "");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::View, "view2");
    ensure("TF019CHK002: Unexpected view found", object_node.ptr() == NULL);
  }

  // Testing Table Object
  {
    // Ensures the table doesn't exist
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
    ensure("TF019CHK003: Unexpected view found", object_node.ptr() == NULL);

    // Ensures a table node is created
    _lst.update_live_object_state(LiveSchemaTree::Table, "schema1", "", "table1");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
    ensure("TF019CHK003: Expected view not found", object_node.ptr() != NULL);

    LiveSchemaTree::TableData* pdata = dynamic_cast<LiveSchemaTree::TableData*>(object_node->get_data());
    ensure("TF019CHK003: Expected view data not found", pdata != NULL);
    ensure_equals("TF019CHK003: Invalid view data found", pdata->get_type(), LiveSchemaTree::Table);

    // Ensures a view node is renamed
    _lst.update_live_object_state(LiveSchemaTree::Table, "schema1", "table1", "table2");
    ensure_equals("TF019CHK003: Expected rename did not occur", object_node->get_string(0), "table2");

    // Ensures no data is reloaded on collapsed node
    _lst.update_live_object_state(LiveSchemaTree::Table, "schema1", "table2", "table2");
    ensure_equals("TF019CHK003: Unexpected loading flags on collapsed table", pdata->get_loading_mask(), 0);
    ensure("TF019CHK003: Unexpected loaded columns on collapsed table",
           !pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA));
    ensure("TF019CHK003: Unexpected loaded foreign keys on collapsed table",
           !pdata->is_data_loaded(LiveSchemaTree::FK_DATA));

    // Expands the table to repeat the test on an expanded table
    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_schema_name = "schema1";
    deleg->_mock_object_name = "table2";
    deleg->_mock_object_type = LiveSchemaTree::Table;
    deleg->_mock_flags = LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA;

    // Emulates the node expansion
    _tester.expand_toggled(object_node, true);
    object_node->expand();

    // Ensures the needed calls were done
    deleg->check_and_reset("TF019CHK003");

    ensure_equals("TF019CHK003: Unexpected loading flags on expanded table", pdata->get_loading_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    ensure("TF019CHK003: Unexpected loaded columns on expanded table",
           !pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA));
    ensure("TF019CHK003: Unexpected loaded foreign keys on expanded table",
           !pdata->is_data_loaded(LiveSchemaTree::INDEX_DATA));

    // Marks the data as already loaded
    pdata->set_loading_mask(0);
    pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);

    // Now exnsures the data is actually reloaded if the table was expanded
    deleg->_expect_fetch_object_details_call = true;
    deleg->_mock_schema_name = "schema1";
    deleg->_mock_object_name = "table2";
    deleg->_mock_object_type = LiveSchemaTree::Table;
    deleg->_mock_flags = LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::FK_DATA;
    _lst.update_live_object_state(LiveSchemaTree::Table, "schema1", "table2", "table2");

    ensure_equals("TF019CHK003: Unexpected loading flags on expanded table", pdata->get_loading_mask(),
                  LiveSchemaTree::COLUMN_DATA | LiveSchemaTree::INDEX_DATA);
    ensure("TF019CHK003: Unexpected loaded columns on expanded table",
           !pdata->is_data_loaded(LiveSchemaTree::COLUMN_DATA));
    ensure("TF019CHK003: Unexpected loaded foreign keys on expanded table",
           !pdata->is_data_loaded(LiveSchemaTree::INDEX_DATA));

    // Ensures the needed calls were done
    deleg->check_and_reset("TF019CHK003");

    // Ensures a table node is deleted
    _lst.update_live_object_state(LiveSchemaTree::Table, "schema1", "table1", "");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
    ensure("TF019CHK003: Unexpected view found", object_node.ptr() == NULL);
  }

  // Testing Procedure Object
  {
    // Ensures the procedure doesn't exist
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
    ensure("TF019CHK004: Unexpected procedure found", object_node.ptr() == NULL);

    // Ensures a procedure node is created
    _lst.update_live_object_state(LiveSchemaTree::Procedure, "schema1", "", "procedure1");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
    ensure("TF019CHK004: Expected procedure not found", object_node.ptr() != NULL);

    LiveSchemaTree::ProcedureData* pdata = dynamic_cast<LiveSchemaTree::ProcedureData*>(object_node->get_data());
    ensure("TF019CHK004: Expected procedure data not found", pdata != NULL);
    ensure_equals("TF019CHK004: Invalid procedure data found", pdata->get_type(), LiveSchemaTree::Procedure);

    // Ensures a procedure node is renamed
    _lst.update_live_object_state(LiveSchemaTree::Procedure, "schema1", "procedure1", "procedure2");
    ensure_equals("TF019CHK004: Expected rename did not occur", object_node->get_string(0), "procedure2");

    // Ensures a procedure node is deleted
    _lst.update_live_object_state(LiveSchemaTree::Procedure, "schema1", "procedure2", "");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure2");
    ensure("TF019CHK004: Unexpected procedure found", object_node.ptr() == NULL);
  }

  // Testing Function Object
  {
    // Ensures the function doesn't exist
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
    ensure("TF019CHK005: Unexpected function found", object_node.ptr() == NULL);

    // Ensures a function node is created
    _lst.update_live_object_state(LiveSchemaTree::Function, "schema1", "", "function1");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
    ensure("TF019CHK005: Expected function not found", object_node.ptr() != NULL);

    LiveSchemaTree::FunctionData* pdata = dynamic_cast<LiveSchemaTree::FunctionData*>(object_node->get_data());
    ensure("TF019CHK005: Expected function data not found", pdata != NULL);
    ensure_equals("TF019CHK005: Invalid function data found", pdata->get_type(), LiveSchemaTree::Function);

    // Ensures a function node is renamed
    _lst.update_live_object_state(LiveSchemaTree::Function, "schema1", "function1", "function2");
    ensure_equals("TF019CHK004: Expected rename did not occur", object_node->get_string(0), "function2");

    // Ensures a function node is deleted
    _lst.update_live_object_state(LiveSchemaTree::Function, "schema1", "function2", "");
    object_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Function, "function2");
    ensure("TF019CHK005: Unexpected function found", object_node.ptr() == NULL);
  }
}

// Test get_field_description
TEST_FUNCTION(20) {
  // Fills the tree using the real structure..
  // std::list<std::string> schemas;
  mforms::TreeNodeRef node;
  mforms::TreeNodeRef child_node;
  mforms::TreeNodeRef leaf_node;

  fill_basic_schema("TF020CHK001");

  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, "");

  // The schema node and it's direct children return the schema description
  ensure_equals("TF020CHK004: Unexpected node description", _lst.get_field_description(node),
                "<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
  child_node = node->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
  ensure_equals("TF020CHK004: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
  child_node = node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
  ensure_equals("TF020CHK004: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
  child_node = node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
  ensure_equals("TF020CHK004: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");
  child_node = node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
  ensure_equals("TF020CHK004: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Schema:</b> <font color='#148814'><b>schema1</b></font><br><br>");

  // The table node and it's direct children return the table description
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
  ensure_equals("TF020CHK005: Unexpected node description", _lst.get_field_description(node),
                "<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
                "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : table_column1"
                "</table><br><br>"
                "<div><b>Related Tables:</b></div>"
                "MOCK LOADED Foreign Key : fk1");
  child_node = node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
  ensure_equals("TF020CHK005: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
                "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : table_column1"
                "</table><br><br>"
                "<div><b>Related Tables:</b></div>"
                "MOCK LOADED Foreign Key : fk1");
  leaf_node = child_node->get_child(0);
  ensure_equals("TF020CHK005: Unexpected node description", _lst.get_field_description(leaf_node),
                "<b>Column:</b> <font color='#148814'><b>table_column1</b></font><br><br>"
                "<b>Definition:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : table_column1"
                "</table><br><br>");

  child_node = node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);
  ensure_equals("TF020CHK006: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
                "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : table_column1"
                "</table><br><br>"
                "<div><b>Related Tables:</b></div>"
                "MOCK LOADED Foreign Key : fk1");
  leaf_node = child_node->get_child(0);
  ensure_equals(
    "TF020CHK006: Unexpected node description", _lst.get_field_description(leaf_node),
    "<b>Index:</b> <font color='#148814'><b>index1</b></font><br><br><b>Definition:</b><br>MOCK LOADED Index : index1");

  child_node = node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);
  ensure_equals("TF020CHK007: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
                "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : table_column1"
                "</table><br><br>"
                "<div><b>Related Tables:</b></div>"
                "MOCK LOADED Foreign Key : fk1");
  leaf_node = child_node->get_child(0);
  ensure_equals("TF020CHK007: Unexpected node description", _lst.get_field_description(leaf_node),
                "<b>Trigger:</b> <font color='#148814'><b>trigger1</b></font><br><br><b>Definition:</b><br>MOCK LOADED "
                "Trigger : trigger1");

  child_node = node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);
  ensure_equals("TF020CHK008: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Table:</b> <font color='#148814'><b>table1</b></font><br><br>"
                "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : table_column1"
                "</table><br><br>"
                "<div><b>Related Tables:</b></div>"
                "MOCK LOADED Foreign Key : fk1");
  leaf_node = child_node->get_child(0);
  ensure_equals("TF020CHK008: Unexpected node description", _lst.get_field_description(leaf_node),
                "<b>Foreign Key:</b> <font color='#148814'><b>fk1</b></font><br><br>"
                "<b>Definition:</b><br>"
                "MOCK LOADED Foreign Key : fk1");

  // The view node and it's direct children return the table description
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
  ensure_equals("TF020CHK009: Unexpected node description", _lst.get_field_description(node),
                "<b>View:</b> <font color='#148814'><b>view1</b></font><br><br>"
                "<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : view_column1"
                "</table><br><br>");
  child_node = node->get_child(0);
  ensure_equals("TF020CHK009: Unexpected node description", _lst.get_field_description(child_node),
                "<b>Column:</b> <font color='#148814'><b>view_column1</b></font><br><br>"
                "<b>Definition:</b><table style=\"border: none; border-collapse: collapse;\">"
                "MOCK LOADED Column : view_column1"
                "</table><br><br>");

  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
  ensure_equals("TF020CHK010: Unexpected node description", _lst.get_field_description(node),
                "<b>Procedure:</b> <font color='#148814'><b>procedure1</b></font><br><br>");

  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
  ensure_equals("TF020CHK010: Unexpected node description", _lst.get_field_description(node),
                "<b>Function:</b> <font color='#148814'><b>function1</b></font><br><br>");

  pmodel_view->root_node()->remove_children();
}

// Test create_node_for_object
TEST_FUNCTION(21) {
  mforms::TreeNodeRef schema_node;
  mforms::TreeNodeRef object_node;
  LiveSchemaTree::LSTData* pdata = NULL;

  schema_node = _lst.get_node_for_object("schema_object", LiveSchemaTree::Schema, "");
  object_node = _lst.get_node_for_object("schema_object", LiveSchemaTree::Table, "table_object");

  ensure("TF021CHK001: Unexpected schema found: schema_object", schema_node.ptr() == NULL);
  ensure("TF021CHK001: Unexpected table found: table_object", object_node.ptr() == NULL);

  /* Tests the schema and object nodes are created if they don't exist */
  object_node = _lst.create_node_for_object("schema_object", LiveSchemaTree::Table, "table_object");
  schema_node = _lst.get_node_for_object("schema_object", LiveSchemaTree::Schema, "");

  ensure("TF021CHK001: Expected schema not found: schema_object", schema_node.ptr() != NULL);
  ensure("TF021CHK001: Expected table not found: table_object", object_node.ptr() != NULL);
  pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
  ensure("TF021CHK002: Expected data not found", pdata != NULL);
  ensure("TF021CHK002: Expected data type not found", pdata->get_type() == LiveSchemaTree::Table);

  /* Tests the view object is created under an existing schema if it already exists */
  object_node = _lst.get_node_for_object("schema_object", LiveSchemaTree::View, "view_object");
  ensure("TF021CHK002: Unexpected view found: view_object", object_node.ptr() == NULL);

  object_node = _lst.create_node_for_object("schema_object", LiveSchemaTree::View, "view_object");
  ensure("TF021CHK002: Expected view not found: view_object", object_node.ptr() != NULL);
  ensure("TF021CHK002: View created under unexpected schema",
         schema_node.ptr() == object_node->get_parent()->get_parent().ptr());
  pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
  ensure("TF021CHK002: Expected data not found", pdata != NULL);
  ensure("TF021CHK002: Expected data type not found", pdata->get_type() == LiveSchemaTree::View);

  /* Tests the procedure object is created under an existing schema if it already exists */
  object_node = _lst.get_node_for_object("schema_object", LiveSchemaTree::Procedure, "procedure_object");
  ensure("TF021CHK003: Unexpected procedure found: procedure_object", object_node.ptr() == NULL);

  object_node = _lst.create_node_for_object("schema_object", LiveSchemaTree::Procedure, "procedure_object");
  ensure("TF021CHK003: Expected procedure not found: procedure_object", object_node.ptr() != NULL);
  ensure("TF021CHK003: View created under unexpected schema",
         schema_node.ptr() == object_node->get_parent()->get_parent().ptr());
  pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
  ensure("TF021CHK003: Expected data not found", pdata != NULL);
  ensure("TF021CHK003: Expected data type not found", pdata->get_type() == LiveSchemaTree::Procedure);

  /* Tests the function object is created under an existing schema if it already exists */
  object_node = _lst.get_node_for_object("schema_object", LiveSchemaTree::Function, "function_object");
  ensure("TF021CHK004: Unexpected function found: function_object", object_node.ptr() == NULL);

  object_node = _lst.create_node_for_object("schema_object", LiveSchemaTree::Function, "function_object");
  ensure("TF021CHK004: Expected function not found: function_object", object_node.ptr() != NULL);
  ensure("TF021CHK004: View created under unexpected schema",
         schema_node.ptr() == object_node->get_parent()->get_parent().ptr());
  pdata = dynamic_cast<LiveSchemaTree::LSTData*>(object_node->get_data());
  ensure("TF021CHK004: Expected data not found", pdata != NULL);
  ensure("TF021CHK004: Expected data type not found", pdata->get_type() == LiveSchemaTree::Function);

  /* Ensures no other object types alter the tree structure */
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::Schema, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: Schema", object_node.ptr() == NULL);

  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::TableCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: TableCollection", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::ViewCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: ViewCollection", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::ProcedureCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: ProcedureCollection", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::FunctionCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: FunctionCollection", object_node.ptr() == NULL);

  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::ColumnCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: ColumnCollection", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::IndexCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: IndexCollection", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::TriggerCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: TriggerCollection", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::ForeignKeyCollection, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: ForeignKeyCollection", object_node.ptr() == NULL);

  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::Trigger, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: Trigger", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::TableColumn, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: TableColumn", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::ViewColumn, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: ViewColumn", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::Index, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: Index", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::ForeignKey, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: ForeignKey", object_node.ptr() == NULL);

  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::ForeignKeyColumn, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: ForeignKeyColumn", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::IndexColumn, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: IndexColumn", object_node.ptr() == NULL);
  object_node = _lst.create_node_for_object("fake_schema_object", LiveSchemaTree::Any, "whatever");
  ensure("TF021CHK005: Unexpected object has been created: Any", object_node.ptr() == NULL);

  schema_node = _lst.get_node_for_object("fake_schema_object", LiveSchemaTree::Schema, "");
  ensure("TF021CHK006: Unexpected schema has been created", schema_node.ptr() == NULL);
}

TEST_FUNCTION(22) {
  bool backup = _lst.is_schema_contents_enabled();

  _lst.is_schema_contents_enabled(true);
  ensure("TF022CHK001: Unexpected schema content disabled", _lst.is_schema_contents_enabled());

  _lst.is_schema_contents_enabled(false);
  ensure("TF022CHK001: Unexpected schema content enabled", !_lst.is_schema_contents_enabled());

  _lst.is_schema_contents_enabled(backup);
}

void check_get_schema_name_recursive(LiveSchemaTree* lst, mforms::TreeNodeRef root) {
  ensure_equals("TF023CHK002 : Unexpected schema found", lst->get_schema_name(root), "schema1");

  for (int index = 0; index < root->count(); index++) {
    check_get_schema_name_recursive(lst, root->get_child(index));
  }
}

TEST_FUNCTION(23) {
  fill_basic_schema("TF023CHK001");

  check_get_schema_name_recursive(&_lst, _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, ""));

  pmodel_view->root_node()->remove_children();
}

void check_node_paths_recursive(LiveSchemaTree* lst, mforms::TreeNodeRef root) {
  std::vector<std::string> path = lst->get_node_path(root);
  mforms::TreeNodeRef other_node = lst->get_node_from_path(path);

  ensure(base::strfmt("TF024CHK001: Unable to find node from path : %s", ((std::string*)path.data())->c_str()),
         root.ptr() == other_node.ptr());

  for (int index = 0; index < root->count(); index++) {
    check_node_paths_recursive(lst, root->get_child(index));
  }
}

TEST_FUNCTION(24) {
  fill_basic_schema("TF024CHK001");

  check_node_paths_recursive(&_lst, _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, ""));

  pmodel_view->root_node()->remove_children();
}

TEST_FUNCTION(25) {
  bool backup = _tester.enabled_events();

  _lst.enable_events(true);
  ensure("TF025CHK001: Unexpected events disabled", _tester.enabled_events());

  _lst.enable_events(false);
  ensure("TF025CHK002: Unexpected events enabled", !_tester.enabled_events());

  _lst.enable_events(backup);
}

TEST_FUNCTION(26) {
  // Fills the tree using the real structure..
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

  _lst.enable_events(true);

  // Fills a schema...
  _lst.update_schemata(schemas);
  schema_node = _lst.get_node_for_object("schema2", LiveSchemaTree::Schema, "");

  // Fills the schema content.
  deleg->expect_fetch_schema_contents_call();
  deleg->_mock_view_list->push_back("view1");
  deleg->_mock_table_list->push_back("table1");
  deleg->_mock_table_list->push_back("table2");
  deleg->_mock_table_list->push_back("table3");
  deleg->_mock_procedure_list->push_back("procedure1");
  deleg->_mock_function_list->push_back("function1");
  deleg->_mock_call_back_slot = true;
  deleg->_mock_schema_name = "schema2";
  deleg->_check_id = "TF026CHK001";

  _lst.expand_toggled(schema_node, true);
  deleg->check_and_reset("TF026CHK001");

  _lst.expand_toggled(schema_node, false);
  deleg->check_and_reset("TF026CHK002");

  _lst.expand_toggled(schema_node, true);
  deleg->check_and_reset("TF026CHK003");

  // Ensures nothing happens when the expansion is toglled for the table collection node
  child_node = schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Expands a table node...
  deleg->_mock_schema_name = "schema2";
  deleg->_mock_object_name = "table3";
  deleg->_mock_object_type = LiveSchemaTree::Table;
  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_column_list->clear();
  deleg->_mock_index_list->clear();
  deleg->_mock_column_list->push_back("table_column1");
  deleg->_mock_index_list->push_back("index1");
  deleg->_mock_trigger_list->push_back("trigger1");
  deleg->_mock_fk_list->push_back("fk1");
  deleg->_mock_call_back_slot_columns = true;
  deleg->_mock_call_back_slot_indexes = true;
  deleg->_mock_call_back_slot_triggers = true;
  deleg->_mock_call_back_slot_foreign_keys = true;
  deleg->_check_id = "TF026CHK004";

  // Takes the third table node...
  object_node = child_node->get_child(2);
  _lst.expand_toggled(object_node, true);
  _lst.expand_toggled(object_node, false);
  _lst.expand_toggled(object_node, true);

  deleg->check_and_reset("TF026CHK004");

  // Ensures nothing happens when the expansion is toglled for the column collection node
  child_node = object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Ensures nothing happens when the expansion is toglled for the index collection node
  child_node = object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Ensures nothing happens when the expansion is toglled for the trigger collection node
  child_node = object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Ensures nothing happens when the expansion is toglled for the foreign key collection node
  child_node = object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Ensures nothing happens when the expansion is toglled for the column collection node
  child_node = schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Fills view column...
  deleg->_mock_schema_name = "schema2";
  deleg->_mock_object_name = "view1";
  deleg->_mock_object_type = LiveSchemaTree::View;
  deleg->_expect_fetch_object_details_call = true;
  deleg->_mock_column_list->push_back("view_column1");
  deleg->_mock_call_back_slot_columns = true;
  deleg->_check_id = "TF026CHK005";

  object_node = child_node->get_child(0);
  _lst.expand_toggled(object_node, true);
  _lst.expand_toggled(object_node, false);
  _lst.expand_toggled(object_node, true);

  deleg->check_and_reset("TF026CHK005");

  // Ensures nothing happens when the expansion is toglled for the procedures collection node
  child_node = schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Ensures nothing happens when the expansion is toglled for the functions collection node
  child_node = schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
  _lst.expand_toggled(child_node, true);
  _lst.expand_toggled(child_node, false);
  _lst.expand_toggled(child_node, true);

  // Now creates a filtered tree based on the loaded data to check
  // Expansion state is ropagated to the base tree
  _lst_filtered.set_base(&_lst);
  _lst_filtered.set_filter("schema2.table3");
  _lst_filtered.filter_data();

  schema_node_filtered = _lst_filtered.get_node_for_object("schema2", LiveSchemaTree::Schema, "");

  // Ensures the schema expansion state on base tree is propagated from the state
  // at the filtered tree
  _lst_filtered.expand_toggled(schema_node_filtered, true);
  ensure("TF026CHK006: Unexpected collapsed schema node on the base tree", schema_node->is_expanded());
  _lst_filtered.expand_toggled(schema_node_filtered, false);
  ensure("TF026CHK006: Unexpected expanded schema node on the base tree", !schema_node->is_expanded());
  _lst_filtered.expand_toggled(schema_node_filtered, true);
  ensure("TF026CHK006: Unexpected collapsed schema node on the base tree", schema_node->is_expanded());

  // Ensures the table expansion state on base tree is propagated from the state
  // at the filtered tree
  _lst_filtered.expand_toggled(schema_node_filtered->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0), true);
  ensure("TF026CHK006: Unexpected collapsed table node on the base tree",
         schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(2)->is_expanded());
  _lst_filtered.expand_toggled(schema_node_filtered->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0), false);
  ensure("TF026CHK006: Unexpected expanded table node on the base tree",
         !schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(2)->is_expanded());
  _lst_filtered.expand_toggled(schema_node_filtered->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0), true);
  ensure("TF026CHK006: Unexpected collapsed table node on the base tree",
         schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(2)->is_expanded());

  pmodel_view_filtered->root_node()->remove_children();
  pmodel_view->root_node()->remove_children();
}

/* Test wb::LiveSchemaTree::activate_node */
TEST_FUNCTION(27) {
  mforms::TreeNodeRef schema_node;
  mforms::TreeNodeRef child_node;
  mforms::TreeNodeRef object_node;

  fill_basic_schema("TF027CHK001");

  schema_node = _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, "");

  LiveSchemaTree::ChangeRecord change;
  change.schema = "";
  change.type = LiveSchemaTree::Schema;
  change.name = "schema1";
  deleg->_mock_expected_changes.push_back(change);
  deleg->_mock_expected_action = "activate";
  deleg->_expect_tree_activate_objects = true;

  // Test activating a schema
  _lst.node_activated(schema_node, 0);
  deleg->check_and_reset("TF027CHK001");

  // Test activating the tables collection
  child_node = schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX);
  _lst.node_activated(child_node, 0);

  // Test activating a table node
  object_node = child_node->get_child(0);
  deleg->_mock_expected_text = "table1";
  _lst.node_activated(object_node, 0);
  deleg->check_and_reset("TF027CHK003");

  // Test activating the columns collection
  child_node = object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
  _lst.node_activated(child_node, 0);

  // Test activating a column node
  deleg->_mock_expected_text = "table_column1";
  _lst.node_activated(child_node->get_child(0), 0);
  deleg->check_and_reset("TF027CHK005");

  // Test activating the indexes collection
  child_node = object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);
  _lst.node_activated(child_node, 0);

  // Test activating an index node
  deleg->_mock_expected_text = "index1";
  _lst.node_activated(child_node->get_child(0), 0);
  deleg->check_and_reset("TF027CHK007");

  // Test activating the triggers collection
  child_node = object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);
  _lst.node_activated(child_node, 0);

  // Test activating a trigger node
  deleg->_mock_expected_text = "trigger1";
  _lst.node_activated(child_node->get_child(0), 0);
  deleg->check_and_reset("TF027CHK009");

  // Test activating the triggers collection
  child_node = object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);
  _lst.node_activated(child_node, 0);

  // Test activating a foreign key node
  deleg->_mock_expected_text = "fk1";
  _lst.node_activated(child_node->get_child(0), 0);
  deleg->check_and_reset("TF027CHK011");

  // Test activating the view collection
  child_node = schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX);
  _lst.node_activated(child_node, 0);

  // Test activating a view node
  object_node = child_node->get_child(0);
  deleg->_mock_expected_text = "view1";
  _lst.node_activated(object_node, 0);
  deleg->check_and_reset("TF027CHK013");

  // Test activating a view column node
  deleg->_mock_expected_text = "view_column1";
  _lst.node_activated(object_node->get_child(0), 0);
  deleg->check_and_reset("TF027CHK014");

  // Test activating the routines collection
  child_node = schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX);
  _lst.node_activated(child_node, 0);
  deleg->check_and_reset("TF027CHK015");

  // Test activating a procedure node
  object_node = child_node->get_child(0);
  deleg->_mock_expected_text = "procedure1";
  _lst.node_activated(object_node, 0);
  deleg->check_and_reset("TF027CHK016");

  // Test activating the routines collection
  child_node = schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX);
  _lst.node_activated(child_node, 0);
  deleg->check_and_reset("TF027CHK017");

  // Test activating a function node
  object_node = child_node->get_child(0);
  deleg->_mock_expected_text = "function1";
  _lst.node_activated(object_node, 0);
  deleg->check_and_reset("TF027CHK018");

  pmodel_view->root_node()->remove_children();
}

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

void set_nodes(LiveSchemaTree* lst, std::list<mforms::TreeNodeRef>& nodes, int flags) {
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

bool ensure_item_exists(const bec::MenuItemList& items, const std::string& item_caption) {
  bool found = false;

  for (size_t index = 0; !found && index < items.size(); index++) {
    found = (items[index].caption == item_caption);
  }

  return found;
}

bool ensure_sub_item_exists(const std::string& item_caption, const bec::MenuItemList& items,
                            const std::string& subitem_caption) {
  bool found = false;

  for (size_t index = 0; !found && index < items.size(); index++) {
    if (items[index].caption == item_caption)
      found = ensure_item_exists(items[index].subitems, subitem_caption);
  }

  return found;
}
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

void ensure_menu_items_exist(const std::string check, const bec::MenuItemList& items, int main_items, int sub_items,
                             const std::string& single, const std::string& multi) {
  std::string custom_caption = single;

  if (SET_DEF_SCH & main_items)
    ensure(check + ": Expected \"Set as Default Schema\" menu item not found",
           ensure_item_exists(items, "Set as Default Schema"));

  if (FIL_TO_SCH & main_items)
    ensure(check + ": Expected \"Filter to This Schema\" menu item not found",
           ensure_item_exists(items, "Filter to This Schema"));

  if (COPY_TC & main_items)
    ensure(check + ": Expected \"Copy to Clipboard\" menu item not found",
           ensure_item_exists(items, "Copy to Clipboard"));

  if (SEND_TE & main_items)
    ensure(check + ": Expected \"Send to SQL Editor\" menu item not found",
           ensure_item_exists(items, "Send to SQL Editor"));

  if (CREATE & main_items)
    ensure(check + ": Expected \"Create " + custom_caption + "...\" menu item not found",
           ensure_item_exists(items, "Create " + custom_caption + "..."));

  if (multi.length() > 0)
    custom_caption = multi;

  if (ALTER & main_items)
    ensure(check + ": Expected \"Alter " + custom_caption + "...\" menu item not found",
           ensure_item_exists(items, "Alter " + custom_caption + "..."));

  if (DROP & main_items)
    ensure(check + ": Expected \"Drop " + custom_caption + "...\" menu item not found",
           ensure_item_exists(items, "Drop " + custom_caption + "..."));

  if (REFRESH & main_items)
    ensure(check + ": Expected \"Refresh All\" menu item not found", ensure_item_exists(items, "Refresh All"));

  if (SEL_ROWS & main_items)
    ensure(check + ": Expected \"Select Rows\" menu item not found", ensure_item_exists(items, "Select Rows"));

  if (EDIT & main_items)
    ensure(check + ": Expected \"Edit Table Data\" menu item not found", ensure_item_exists(items, "Edit Table Data"));

  if (SUB_NAME & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Name\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Name"));
    ensure(check + ": Expected \"Send to SQL Editor\\Name\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Name"));
  }

  if (SUB_NAME_S & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Name (short)\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Name (short)"));
    ensure(check + ": Expected \"Send to SQL Editor\\Name (short)\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Name (short)"));
  }

  if (SUB_NAME_L & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Name (long)\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Name (long)"));
    ensure(check + ": Expected \"Send to SQL Editor\\Name (long)\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Name (long)"));
  }

  if (SUB_SEL_ALL & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Select All Statement\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Select All Statement"));
    ensure(check + ": Expected \"Send to SQL Editor\\Select All Statement\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Select All Statement"));
  }

  if (SUB_SEL_COL & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Select Columns Statement\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Select Columns Statement"));
    ensure(check + ": Expected \"Send to SQL Editor\\Select Columns Statement\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Select Columns Statement"));
  }

  if (SUB_CREATE & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Create Statement\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Create Statement"));
    ensure(check + ": Expected \"Send to SQL Editor\\Create Statement\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Create Statement"));
  }

  if (SUB_INSERT & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Insert Statement\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Insert Statement"));
    ensure(check + ": Expected \"Send to SQL Editor\\Insert Statement\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Insert Statement"));
  }

  if (SUB_UPDATE & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Update Statement\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Update Statement"));
    ensure(check + ": Expected \"Send to SQL Editor\\Update Statement\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Update Statement"));
  }

  if (SUB_DELETE & sub_items) {
    ensure(check + ": Expected \"Copy to Clipboard\\Delete Statement\" menu item not found",
           ensure_sub_item_exists("Copy to Clipboard", items, "Delete Statement"));
    ensure(check + ": Expected \"Send to SQL Editor\\Delete Statement\" menu item not found",
           ensure_sub_item_exists("Send to SQL Editor", items, "Delete Statement"));
  }
}

// Testing get_popup_items_for_nodes
TEST_FUNCTION(28) {
  // TODO: Enable Back once the functionality is completely defined
  /*
  bec::MenuItemList items;
  std::list<mforms::TreeNodeRef> nodes;
  mforms::TreeNodeRef schema_node;
  mforms::TreeNodeRef object_node;

  fill_basic_schema("TF028CHK001");

  set_nodes(&_lst, nodes, SCHEMA);

  //================= Schema and Schema's Collection Nodes =================//
  // Reviewing items for a schema
  items = _lst.get_popup_items_for_nodes(nodes);

  // 8 Real items plus 3 separators
  ensure_equals("TF028CHK002: Unexpected number of menu items", items.size(), 11);
  ensure_menu_items_exist("TF028CHK002", items, SET_DEF_SCH|FIL_TO_SCH|COPY_TC|SEND_TE|CREATE|ALTER|DROP|REFRESH,
  SUB_NAME|SUB_CREATE, "Schema", "");

  // Reviewing items for multiple schemas
  set_nodes(&_lst, nodes, SCHEMA);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 6 Real items plus 2 separators
  ensure_equals("TF028CHK003: Unexpected number of menu items", items.size(), 8);
  ensure_menu_items_exist("TF028CHK003", items, COPY_TC|SEND_TE|CREATE|ALTER|DROP|REFRESH, SUB_NAME|SUB_CREATE,
  "Schema", "2 Schemas");

  // Testing the schema table collection options
  nodes.clear();
  set_nodes(&_lst, nodes, TABLES);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 2 Real items plus 1 separators
  ensure_equals("TF028CHK004: Unexpected number of menu items", items.size(), 3);
  ensure_menu_items_exist("TF028CHK004", items, CREATE|REFRESH, 0, "Table", "");

  // Testing the schema view collection options
  nodes.clear();
  set_nodes(&_lst, nodes, VIEWS);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 2 Real items plus 1 separators
  ensure_equals("TF028CHK005: Unexpected number of menu items", items.size(), 3);
  ensure_menu_items_exist("TF028CHK005", items, CREATE|REFRESH, 0, "View", "");

  // Testing the schema procedures collection options
  nodes.clear();
  set_nodes(&_lst, nodes, PROCEDURES);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 3 Real items plus 1 separators
  ensure_equals("TF028CHK006: Unexpected number of menu items", items.size(), 3);
  ensure_menu_items_exist("TF028CHK006", items, CREATE|REFRESH, 0, "Procedure", "");

  // Testing the schema procedures collection options
  nodes.clear();
  set_nodes(&_lst, nodes, FUNCTIONS);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 3 Real items plus 1 separators
  ensure_equals("TF028CHK006: Unexpected number of menu items", items.size(), 3);
  ensure_menu_items_exist("TF028CHK006", items, CREATE|REFRESH, 0, "Function", "");


  //================= Table, Table's Collection Nodes  and Nodes on each collection =================//
  // Testing for a single schema node
  nodes.clear();
  set_nodes(&_lst, nodes, TABLE);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 7 Real items plus 2 separators
  ensure_equals("TF028CHK007: Unexpected number of menu items", items.size(), 10);
  ensure_menu_items_exist("TF028CHK007", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|ALTER|DROP|REFRESH,
  SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_INSERT|SUB_UPDATE|SUB_DELETE|SUB_CREATE, "Table", "");

  // Testing for multiple Table nodes...
  set_nodes(&_lst, nodes, TABLE);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 7 Real items plus 2 separators
  ensure_equals("TF028CHK008: Unexpected number of menu items", items.size(), 9);
  ensure_menu_items_exist("TF028CHK008", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|ALTER|DROP|REFRESH,
  SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_INSERT|SUB_UPDATE|SUB_DELETE|SUB_CREATE, "Table", "2 Tables");

  // Columns collection...
  nodes.clear();
  set_nodes(&_lst, nodes, COLUMNS);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 5 Real items plus 1 separators
  ensure_equals("TF028CHK009: Unexpected number of menu items", items.size(), 6);
  ensure_menu_items_exist("TF028CHK009", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|REFRESH,
  SUB_NAME_S|SUB_NAME_L|SUB_SEL_COL|SUB_INSERT|SUB_UPDATE, "Table", "2 Tables");

  // Column Node
  nodes.clear();
  set_nodes(&_lst, nodes, TABLE_COLUMN);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 5 Real items plus 1 separators
  ensure_equals("TF028CHK010: Unexpected number of menu items", items.size(), 6);
  ensure_menu_items_exist("TF028CHK010", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|REFRESH,
  SUB_NAME_S|SUB_NAME_L|SUB_SEL_COL|SUB_INSERT|SUB_UPDATE, "Table", "2 Tables");

  // Multiple Column Nodes
  set_nodes(&_lst, nodes, TABLE_COLUMN);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Just like a single column.
  ensure_equals("TF028CHK011: Unexpected number of menu items", items.size(), 6);
  ensure_menu_items_exist("TF028CHK010", items, SEL_ROWS|EDIT|COPY_TC|SEND_TE|REFRESH,
  SUB_NAME_S|SUB_NAME_L|SUB_SEL_COL|SUB_INSERT|SUB_UPDATE, "Table", "2 Tables");

  // Index collection...
  nodes.clear();
  set_nodes(&_lst, nodes, INDEXES);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK012: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK012", items, REFRESH, 0, "", "");

  // Index Node...
  nodes.clear();
  set_nodes(&_lst, nodes, INDEX);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK013: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK013", items, REFRESH, 0, "", "");

  // Multiple Index Nodes...
  set_nodes(&_lst, nodes, INDEX);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK014: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK014", items, REFRESH, 0, "", "");

  // Trigger collection...
  nodes.clear();
  set_nodes(&_lst, nodes, TRIGGERS);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK015: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK015", items, REFRESH, 0, "", "");

  // Trigger Node...
  nodes.clear();
  set_nodes(&_lst, nodes, TRIGGER);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK016: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK016", items, REFRESH, 0, "", "");

  // Multiple Trigger Nodes...
  set_nodes(&_lst, nodes, TRIGGER);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK017: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK017", items, REFRESH, 0, "", "");

  // Foreign Key collection...
  nodes.clear();
  set_nodes(&_lst, nodes, FKS);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK018: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK018", items, REFRESH, 0, "", "");

  // Foreign Key Node...
  nodes.clear();
  set_nodes(&_lst, nodes, FK);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK019: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK019", items, REFRESH, 0, "", "");

  // Multiple Foreign Key Nodes...
  set_nodes(&_lst, nodes, FK);
  items = _lst.get_popup_items_for_nodes(nodes);

  // Refresh All
  ensure_equals("TF028CHK020: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK020", items, REFRESH, 0, "", "");


  //================= View Nodes =================//
  // Single View Node...
  nodes.clear();
  set_nodes(&_lst, nodes, VIEW);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 7 Real items plus 2 separators
  ensure_equals("TF028CHK021: Unexpected number of menu items", items.size(), 9);
  ensure_menu_items_exist("TF028CHK021", items, SEL_ROWS|COPY_TC|SEND_TE|CREATE|ALTER|DROP|REFRESH,
  SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_CREATE, "View", "");

  // Multiple View Nodes...
  set_nodes(&_lst, nodes, VIEW);
  items = _lst.get_popup_items_for_nodes(nodes);

  // 5 Real items plus 2 separators
  ensure_equals("TF028CHK022: Unexpected number of menu items", items.size(), 7);
  ensure_menu_items_exist("TF028CHK022", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH,
  SUB_NAME_S|SUB_NAME_L|SUB_SEL_ALL|SUB_CREATE, "View", "2 Views");

  //================= Procedure Nodes =================//
  // Single Procedure Node...
  nodes.clear();
  set_nodes(&_lst, nodes, PROCEDURE);
  items = _lst.get_popup_items_for_nodes(nodes);

  ensure_equals("TF028CHK023: Unexpected number of menu items", items.size(), 8);
  ensure_menu_items_exist("TF028CHK023", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
  "Procedure", "");

  // Multiple Procedure Nodes...
  set_nodes(&_lst, nodes, PROCEDURE);
  items = _lst.get_popup_items_for_nodes(nodes);

  ensure_equals("TF028CHK024: Unexpected number of menu items", items.size(), 7);
  ensure_menu_items_exist("TF028CHK024", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
  "Procedure", "2 Procedures");

  //================= Function Nodes =================//
  // Single Function Node...
  nodes.clear();
  set_nodes(&_lst, nodes, FUNCTION);
  items = _lst.get_popup_items_for_nodes(nodes);

  ensure_equals("TF028CHK025: Unexpected number of menu items", items.size(), 8);
  ensure_menu_items_exist("TF028CHK025", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
  "Function", "");

  // Multiple Function Nodes...
  set_nodes(&_lst, nodes, FUNCTION);
  items = _lst.get_popup_items_for_nodes(nodes);

  ensure_equals("TF028CHK026: Unexpected number of menu items", items.size(), 7);
  ensure_menu_items_exist("TF028CHK026", items, COPY_TC|SEND_TE|ALTER|DROP|REFRESH, SUB_NAME_S|SUB_NAME_L|SUB_CREATE,
  "Function", "2 Functions");

  //================= No Nodes =================//
  nodes.clear();
  items = _lst.get_popup_items_for_nodes(nodes);
  ensure_equals("TF028CHK027: Unexpected number of menu items", items.size(), 1);
  ensure_menu_items_exist("TF028CHK027", items, REFRESH, 0, "", "");


  //================= Multiple Nodes of Different Type =================//
  nodes.clear();
  set_nodes(&_lst, nodes, SCHEMA|TABLES|TABLE);
  set_nodes(&_lst, nodes, TABLE|TABLE_COLUMN|VIEW|PROCEDURE|FUNCTION);

  items = _lst.get_popup_items_for_nodes(nodes);
  ensure_equals("TF028CHK028: Unexpected number of menu items", items.size(), 4);
  ensure_menu_items_exist("TF028CHK028", items, COPY_TC|SEND_TE|REFRESH, SUB_NAME_S|SUB_NAME_L, "", "6 Objects");

  pmodel_view->root_node()->remove_children();
  */
}

void set_change_records(std::vector<LiveSchemaTree::ChangeRecord>& change_records, int flags) {
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

// Tests activate_popup_item_for_nodes
TEST_FUNCTION(29) {
  // TODO: Fix once the menu items stuff is already complete
  /*
  std::list<mforms::TreeNodeRef> nodes;
  mforms::TreeNodeRef schema_node;
  mforms::TreeNodeRef object_node;
  LiveSchemaTree::ChangeRecord change;

  fill_basic_schema("TF029CHK001");

  // Tests the Refresh All function
  deleg->_expect_tree_refresh = true;
  deleg->_check_id = "TF029CHK002";
  _lst.activate_popup_item_for_nodes("refresh", nodes);
  deleg->check_and_reset("TF029CHK002");


  //================= Performs the action for multiple nodes of different type =================//
  nodes.clear();
  set_nodes(&_lst, nodes, SCHEMA | TABLE);
  set_nodes(&_lst, nodes, TABLE | TABLE_COLUMN | VIEW | PROCEDURE | FUNCTION );

  // Tests the Alter function
  set_change_records(deleg->_mock_expected_changes, SCHEMA|TABLE);
  set_change_records(deleg->_mock_expected_changes, TABLE|VIEW|PROCEDURE|FUNCTION);

  deleg->_check_id = "TF029CHK003";
  deleg->_expect_tree_alter_objects = true;
  _lst.activate_popup_item_for_nodes("alter", nodes);
  deleg->check_and_reset("TF029CHK003");


  // Tests the Drop function
  set_change_records(deleg->_mock_expected_changes, SCHEMA|TABLE);
  set_change_records(deleg->_mock_expected_changes, TABLE|VIEW|PROCEDURE|FUNCTION);

  deleg->_check_id = "TF029CHK004";
  deleg->_expect_tree_drop_objects = true;
  _lst.activate_popup_item_for_nodes("drop", nodes);
  deleg->check_and_reset("TF029CHK004");

  // Tests the edit data option with tables
  nodes.clear();
  set_nodes(&_lst, nodes, TABLE);
  set_nodes(&_lst, nodes, TABLE);

  set_change_records(deleg->_mock_expected_changes, TABLE);
  set_change_records(deleg->_mock_expected_changes, TABLE);

  deleg->_check_id = "TF029CHK005";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "edit_data";
  _lst.activate_popup_item_for_nodes("edit_data", nodes);
  deleg->check_and_reset("TF029CHK005");

  set_change_records(deleg->_mock_expected_changes, TABLE);
  set_change_records(deleg->_mock_expected_changes, TABLE);
  deleg->_check_id = "TF029CHK005.1";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "select_data";
  _lst.activate_popup_item_for_nodes("select_data", nodes);
  deleg->check_and_reset("TF029CHK005.1");

  // Tests the edit data option with views
  nodes.clear();
  set_nodes(&_lst, nodes, TABLE|VIEW|PROCEDURE|FUNCTION);

  set_change_records(deleg->_mock_expected_changes, TABLE|VIEW|PROCEDURE|FUNCTION);

  deleg->_check_id = "TF029CHK006";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "edit_data";
  _lst.activate_popup_item_for_nodes("edit_data", nodes);
  deleg->check_and_reset("TF029CHK006");

  set_change_records(deleg->_mock_expected_changes, TABLE|VIEW|PROCEDURE|FUNCTION);

  deleg->_check_id = "TF029CHK006.1";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "select_data";
  _lst.activate_popup_item_for_nodes("select_data", nodes);
  deleg->check_and_reset("TF029CHK006.1");

  // Tests the edit data option with table columns
  nodes.clear();
  set_nodes(&_lst, nodes, COLUMNS|TABLE_COLUMN|VIEW_COLUMN);

  set_change_records(deleg->_mock_expected_changes, TABLE);
  set_change_records(deleg->_mock_expected_changes, TABLE|VIEW);
  deleg->_mock_expected_changes[0].detail = "table_column1";
  deleg->_mock_expected_changes[1].detail = "table_column1";
  deleg->_mock_expected_changes[2].detail = "view_column1";

  deleg->_check_id = "TF029CHK007";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "edit_data_columns";
  _lst.activate_popup_item_for_nodes("edit_data_columns", nodes);
  deleg->check_and_reset("TF029CHK007");

  set_change_records(deleg->_mock_expected_changes, TABLE);
  set_change_records(deleg->_mock_expected_changes, TABLE|VIEW);
  deleg->_mock_expected_changes[0].detail = "table_column1";
  deleg->_mock_expected_changes[1].detail = "table_column1";
  deleg->_mock_expected_changes[2].detail = "view_column1";

  deleg->_check_id = "TF029CHK007.1";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "select_data_columns";
  _lst.activate_popup_item_for_nodes("select_data_columns", nodes);
  deleg->check_and_reset("TF029CHK007.1");

  // Tests the edit data option with the nodes that should be ignored
  nodes.clear();
  set_nodes(&_lst, nodes, SCHEMA|TABLES|VIEWS|PROCEDURES|FUNCTIONS|INDEXES|INDEX|TRIGGERS|TRIGGER|FKS|FK);

  deleg->_check_id = "TF029CHK008";
  _lst.activate_popup_item_for_nodes("edit_data", nodes);
  deleg->check_and_reset("TF029CHK008");

  deleg->_check_id = "TF029CHK008.1";
  _lst.activate_popup_item_for_nodes("select_data", nodes);
  deleg->check_and_reset("TF029CHK008.1");

  // Tests the create function without objects
  nodes.clear();
  set_change_records(deleg->_mock_expected_changes, SCHEMA);
  deleg->_mock_expected_changes[0].schema = "";
  deleg->_mock_expected_changes[0].name = "";

  deleg->_check_id = "TF029CHK009";
  deleg->_expect_tree_create_object = true;
  _lst.activate_popup_item_for_nodes("create", nodes);
  deleg->check_and_reset("TF029CHK009");


  // Tests the create functions for schema object
  nodes.clear();
  set_nodes(&_lst, nodes, SCHEMA|TABLES|TABLE|VIEWS|VIEW|PROCEDURE|FUNCTION);

  set_change_records(deleg->_mock_expected_changes, SCHEMA|TABLE);
  set_change_records(deleg->_mock_expected_changes, TABLE|VIEW);
  set_change_records(deleg->_mock_expected_changes, VIEW|PROCEDURE|FUNCTION);

  deleg->_mock_expected_changes[0].schema = "";
  while(deleg->_mock_expected_changes.size())
  {
  deleg->_mock_expected_changes[0].name = "";

  deleg->_check_id = "TF029CHK010";
  deleg->_expect_tree_create_object = true;
  _lst.activate_popup_item_for_nodes("create", nodes);
  nodes.erase(nodes.begin());
  }

  deleg->check_and_reset("TF029CHK010");

  // Testing create in routines collection produces a procedure node
  nodes.clear();
  set_nodes(&_lst, nodes, PROCEDURES);
  set_change_records(deleg->_mock_expected_changes, PROCEDURE);

  deleg->_check_id = "TF029CHK011";
  deleg->_expect_tree_create_object = true;
  deleg->_mock_expected_changes[0].name = "";
  _lst.activate_popup_item_for_nodes("create", nodes);
  deleg->check_and_reset("TF029CHK011");

  // Testing create in routines collection produces a procedure node
  nodes.clear();
  set_nodes(&_lst, nodes, FUNCTIONS);
  set_change_records(deleg->_mock_expected_changes, FUNCTION);

  deleg->_check_id = "TF029CHK012";
  deleg->_expect_tree_create_object = true;
  deleg->_mock_expected_changes[0].name = "";
  _lst.activate_popup_item_for_nodes("create", nodes);
  deleg->check_and_reset("TF029CHK012");

  // Tests the set active schema function
  nodes.clear();
  set_nodes(&_lst, nodes, SCHEMA);
  set_change_records(deleg->_mock_expected_changes, SCHEMA);
  deleg->_mock_expected_changes[0].schema = "";
  deleg->_mock_expected_changes[0].name = "schema1";

  deleg->_check_id = "TF029CHK013";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "activate";
  _lst.activate_popup_item_for_nodes("set_active_schema", nodes);
  deleg->check_and_reset("TF029CHK013");

  // Tests the set filter schema function
  set_change_records(deleg->_mock_expected_changes, SCHEMA);
  deleg->_mock_expected_changes[0].schema = "";
  deleg->_mock_expected_changes[0].name = "schema1";

  deleg->_check_id = "TF029CHK014";
  deleg->_expect_tree_activate_objects = true;
  deleg->_mock_expected_action = "filter";
  _lst.activate_popup_item_for_nodes("filter_schema", nodes);
  deleg->check_and_reset("TF029CHK014");

  //////////////////////////////////////////////////////////////////////
  //  deprecated
  //////////////////////////////////////////////////////////////////////

  // Tests a custom functions for the database objects
  nodes.clear();
  set_nodes(&_lst, nodes, SCHEMA|TABLE|VIEW|PROCEDURE|FUNCTION);
  set_change_records(deleg->_mock_expected_changes, SCHEMA|TABLE|VIEW|PROCEDURE|FUNCTION);
  deleg->_mock_expected_changes[0].detail = "schema";
  deleg->_mock_expected_changes[1].detail = "table";
  deleg->_mock_expected_changes[2].detail = "view";
  deleg->_mock_expected_changes[3].detail = "routine";
  deleg->_mock_expected_changes[4].detail = "routine";
  deleg->_mock_expected_changes[0].schema= "schema1";
  deleg->_mock_expected_changes[0].name= "";

  ensure_equals("number of changes vs selected nodes", deleg->_mock_expected_changes.size(), nodes.size());
  while(deleg->_mock_expected_changes.size())
  {
  deleg->_expect_plugin_item_call = true;

  deleg->_check_id = "TF029CHK015";
  deleg->_mock_expected_action = "whatever";
  _lst.activate_popup_item_for_nodes("whatever", nodes);
  nodes.erase(nodes.begin());
  }

  deleg->check_and_reset("TF029CHK015");

  // Ensures custom doesn't work for non database nodes
  nodes.clear();
  set_nodes(&_lst, nodes, TABLES|VIEWS|ROUTINES|COLUMNS|TABLE_COLUMN|INDEXES|INDEX|TRIGGERS|TRIGGER|FKS|FK|VIEW_COLUMN);

  while(nodes.size())
  {
  deleg->_check_id = "TF029CHK016";
  _lst.activate_popup_item_for_nodes("whatever", nodes);

  nodes.erase(nodes.begin());
  }
  //////////
  deleg->check_and_reset("TF029CHK016");


  pmodel_view->root_node()->remove_children();
  */
}

// Test wb::LiveSchemaTree::get_filter_wildcard
TEST_FUNCTION(30) {
  /* Using the default wildcard type */
  ensure_equals("TF030CHK01 : Failure getting wildcard string", _tester.get_filter_wildcard(""), "*");
  ensure_equals("TF030CHK02 : Failure getting wildcard string", _tester.get_filter_wildcard("*"), "*");
  ensure_equals("TF030CHK03 : Failure getting wildcard string", _tester.get_filter_wildcard("a"), "a*");
  ensure_equals("TF030CHK04 : Failure getting wildcard string", _tester.get_filter_wildcard("a*"), "a*");
  ensure_equals("TF030CHK05 : Failure getting wildcard string", _tester.get_filter_wildcard("*a"), "*a*");
  ensure_equals("TF030CHK06 : Failure getting wildcard string", _tester.get_filter_wildcard("*a*"), "*a*");
  ensure_equals("TF030CHK07 : Failure getting wildcard string", _tester.get_filter_wildcard("schema"), "schema*");
  ensure_equals("TF030CHK08 : Failure getting wildcard string", _tester.get_filter_wildcard("schema*"), "schema*");
  ensure_equals("TF030CHK09 : Failure getting wildcard string", _tester.get_filter_wildcard("*schema"), "*schema*");
  ensure_equals("TF030CHK10 : Failure getting wildcard string", _tester.get_filter_wildcard("*schema*"), "*schema*");

  ensure_equals("TF030CHK11 : Failure getting wildcard string",
                _tester.get_filter_wildcard("", LiveSchemaTree::LocalLike), "*");
  ensure_equals("TF030CHK12 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*", LiveSchemaTree::LocalLike), "*");
  ensure_equals("TF030CHK13 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a", LiveSchemaTree::LocalLike), "a*");
  ensure_equals("TF030CHK14 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a*", LiveSchemaTree::LocalLike), "a*");
  ensure_equals("TF030CHK15 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a", LiveSchemaTree::LocalLike), "*a*");
  ensure_equals("TF030CHK16 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a*", LiveSchemaTree::LocalLike), "*a*");
  ensure_equals("TF030CHK17 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema", LiveSchemaTree::LocalLike), "schema*");
  ensure_equals("TF030CHK18 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema*", LiveSchemaTree::LocalLike), "schema*");
  ensure_equals("TF030CHK19 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema", LiveSchemaTree::LocalLike), "*schema*");
  ensure_equals("TF030CHK20 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema*", LiveSchemaTree::LocalLike), "*schema*");

  ensure_equals("TF030CHK21 : Failure getting wildcard string",
                _tester.get_filter_wildcard("", LiveSchemaTree::LocalRegexp), "*");
  ensure_equals("TF030CHK22 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*", LiveSchemaTree::LocalRegexp), "*");
  ensure_equals("TF030CHK23 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a", LiveSchemaTree::LocalRegexp), "a*");
  ensure_equals("TF030CHK24 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a*", LiveSchemaTree::LocalRegexp), "a*");
  ensure_equals("TF030CHK25 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a", LiveSchemaTree::LocalRegexp), "*a*");
  ensure_equals("TF030CHK26 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a*", LiveSchemaTree::LocalRegexp), "*a*");
  ensure_equals("TF030CHK27 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema", LiveSchemaTree::LocalRegexp), "schema*");
  ensure_equals("TF030CHK28 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema*", LiveSchemaTree::LocalRegexp), "schema*");
  ensure_equals("TF030CHK29 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema", LiveSchemaTree::LocalRegexp), "*schema*");
  ensure_equals("TF030CHK30 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema*", LiveSchemaTree::LocalRegexp), "*schema*");

  ensure_equals("TF030CHK31 : Failure getting wildcard string",
                _tester.get_filter_wildcard("", LiveSchemaTree::RemoteRegexp), "*");
  ensure_equals("TF030CHK32 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*", LiveSchemaTree::RemoteRegexp), "*");
  ensure_equals("TF030CHK33 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a", LiveSchemaTree::RemoteRegexp), "a*");
  ensure_equals("TF030CHK34 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a*", LiveSchemaTree::RemoteRegexp), "a*");
  ensure_equals("TF030CHK35 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a", LiveSchemaTree::RemoteRegexp), "*a*");
  ensure_equals("TF030CHK36 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a*", LiveSchemaTree::RemoteRegexp), "*a*");
  ensure_equals("TF030CHK37 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema", LiveSchemaTree::RemoteRegexp), "schema*");
  ensure_equals("TF030CHK38 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema*", LiveSchemaTree::RemoteRegexp), "schema*");
  ensure_equals("TF030CHK39 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema", LiveSchemaTree::RemoteRegexp), "*schema*");
  ensure_equals("TF030CHK40 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema*", LiveSchemaTree::RemoteRegexp), "*schema*");

  ensure_equals("TF030CHK41 : Failure getting wildcard string",
                _tester.get_filter_wildcard("", LiveSchemaTree::RemoteLike), "%");
  ensure_equals("TF030CHK42 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*", LiveSchemaTree::RemoteLike), "%");
  ensure_equals("TF030CHK43 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a", LiveSchemaTree::RemoteLike), "a%");
  ensure_equals("TF030CHK44 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a*", LiveSchemaTree::RemoteLike), "a%");
  ensure_equals("TF030CHK45 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a", LiveSchemaTree::RemoteLike), "%a%");
  ensure_equals("TF030CHK46 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*a*", LiveSchemaTree::RemoteLike), "%a%");
  ensure_equals("TF030CHK47 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema", LiveSchemaTree::RemoteLike), "schema%");
  ensure_equals("TF030CHK48 : Failure getting wildcard string",
                _tester.get_filter_wildcard("schema*", LiveSchemaTree::RemoteLike), "schema%");
  ensure_equals("TF030CHK49 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema", LiveSchemaTree::RemoteLike), "%schema%");
  ensure_equals("TF030CHK50 : Failure getting wildcard string",
                _tester.get_filter_wildcard("*schema*", LiveSchemaTree::RemoteLike), "%schema%");

  ensure_equals("TF030CHK53 : Failure getting wildcard string",
                _tester.get_filter_wildcard("?", LiveSchemaTree::RemoteLike), "_%");
  ensure_equals("TF030CHK54 : Failure getting wildcard string",
                _tester.get_filter_wildcard("a?", LiveSchemaTree::RemoteLike), "a_%");
  ensure_equals("TF030CHK55 : Failure getting wildcard string",
                _tester.get_filter_wildcard("?a", LiveSchemaTree::RemoteLike), "_a%");
  ensure_equals("TF030CHK56 : Failure getting wildcard string",
                _tester.get_filter_wildcard("?a?", LiveSchemaTree::RemoteLike), "_a_%");
  ensure_equals("TF030CHK57 : Failure getting wildcard string",
                _tester.get_filter_wildcard("sc?ema", LiveSchemaTree::RemoteLike), "sc_ema%");
  ensure_equals("TF030CHK58 : Failure getting wildcard string",
                _tester.get_filter_wildcard("sc?e?a*", LiveSchemaTree::RemoteLike), "sc_e_a%");
  ensure_equals("TF030CHK59 : Failure getting wildcard string",
                _tester.get_filter_wildcard("sc_ema", LiveSchemaTree::RemoteLike), "sc\\_ema%");
  ensure_equals("TF030CHK60 : Failure getting wildcard string",
                _tester.get_filter_wildcard("sch%ma*", LiveSchemaTree::RemoteLike), "sch\\%ma%");
}

// Test wb::LiveSchemaTree::get_node_for_object
TEST_FUNCTION(31) {
  mforms::TreeNodeRef node;
  mforms::TreeNodeRef schema_node;

  fill_basic_schema("TF031CHK001");

  schema_node = pmodel_view->root_node()->get_child(0);

  // Searching for invalid schema...
  node = _lst.get_node_for_object("dummy_schema", LiveSchemaTree::Schema, "");
  ensure("TF031CHK001: Unexpected node found searching for invalid schema", node.ptr() == NULL);

  // Searching for a valid schema...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Schema, "");
  ensure("TF031CHK001: Unexpected failure searching for schema node", node.ptr() != NULL);
  ensure("TF031CHK001: Unexpected schema found", node.ptr() == schema_node.ptr());

  // Searching for a invalid table...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Table, "tableX");
  ensure("TF031CHK001: Unexpected node found searching for invalid table", node.ptr() == NULL);

  // Searching for a valid table...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Table, "table1");
  ensure("TF031CHK001: Unexpected failure searching for table node", node.ptr() != NULL);
  ensure("TF031CHK001: Unexpected table found",
         node.ptr() == schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(0).ptr());

  // Searching for a invalid view...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::View, "viewX");
  ensure("TF031CHK001: Unexpected node found searching for invalid table", node.ptr() == NULL);

  // Searching for a valid view...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::View, "view1");
  ensure("TF031CHK001: Unexpected failure searching for view node", node.ptr() != NULL);
  ensure("TF031CHK001: Unexpected view found",
         node.ptr() == schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(0).ptr());

  // Searching for a invalid function...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedureX");
  ensure("TF031CHK001: Unexpected node found searching for invalid procedure", node.ptr() == NULL);

  // Searching for a valid procedure...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Procedure, "procedure1");
  ensure("TF031CHK001: Unexpected failure searching for procedure node", node.ptr() != NULL);
  ensure("TF031CHK001: Unexpected procedure found",
         node.ptr() == schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(0).ptr());

  // Searching for a invalid function...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Function, "functionX");
  ensure("TF031CHK001: Unexpected node found searching for invalid function", node.ptr() == NULL);

  // Searching for a valid function...
  node = _lst.get_node_for_object("schema1", LiveSchemaTree::Function, "function1");
  ensure("TF031CHK001: Unexpected failure searching for function node", node.ptr() != NULL);
  ensure("TF031CHK001: Unexpected procedure found",
         node.ptr() == schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(0).ptr());

  pmodel_view->root_node()->remove_children();
}

TEST_FUNCTION(32) {
  ensure("TF032CHK002: Unexpected base LST returned", _tester.get_base() == NULL);

  _lst_filtered.set_base(&_lst);
  ensure("TF032CHK001: Unexpected base LST returned", _tester_filtered.get_base() == &_lst);

  _lst_filtered.set_base(NULL);
  ensure("TF032CHK002: Unexpected base LST returned", _tester_filtered.get_base() == NULL);
}

GPatternSpec* schema_pattern = NULL;
GPatternSpec* object_pattern = NULL;
void set_patterns(GPatternSpec** schema_pattern, GPatternSpec** object_pattern, const std::string& filter) {
  std::vector<std::string> filters = base::split(filter, ".", 2);

  if (*schema_pattern) {
    g_pattern_spec_free(*schema_pattern);
    *schema_pattern = NULL;
  }

  if (*object_pattern) {
    g_pattern_spec_free(*object_pattern);
    *object_pattern = NULL;
  }

  // Creates the schema/table patterns
  *schema_pattern = g_pattern_spec_new(base::toupper(filters[0]).c_str());
  if (filters.size() > 1)
    *object_pattern = g_pattern_spec_new(base::toupper(filters[1]).c_str());
}

// Test filter_children and filter_children_collection without filters stablished to make
// Sure effectively all the data is copied from one tree to the other
TEST_FUNCTION(33) {
  mforms::TreeNodeRef root_node = pmodel_view->root_node();
  mforms::TreeNodeRef root_node_f = pmodel_view_filtered->root_node();
  mforms::TreeNodeRef schema_node;
  mforms::TreeNodeRef schema_node_f;
  mforms::TreeNodeRef object_node;
  mforms::TreeNodeRef object_node_f;
  mforms::TreeNodeRef sub_node;
  mforms::TreeNodeRef sub_node_f;

  fill_complex_schema("TF033CHK001");

  // Ensure no matter the type, all the children are copied if no filter is specified
  // This test indeed includes the testing of
  ensure_equals("TF033CHK002: Unexpected number of schema nodes before filtering", root_node_f->count(), 0);
  _tester.filter_children(LiveSchemaTree::Schema, root_node, root_node_f);
  ensure_equals("TF033CHK002: Unexpected number of schema nodes after filtering", root_node_f->count(),
                root_node->count());

  for (int schema_index = 0; schema_index < root_node->count(); schema_index++) {
    schema_node = root_node->get_child(schema_index);
    schema_node_f = root_node_f->get_child(schema_index);

    ensure("TF033CHK002: Unexpected schema data in filtered schema",
           schema_node_f->get_data() == schema_node->get_data());

    ensure_equals("TF033CHK002: Unexpected number of schema collection nodes after filtering", schema_node_f->count(),
                  schema_node->count());
    ensure_equals("TF033CHK002: Unexpected number of table nodes after filtering",
                  schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count(),
                  schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count());
    ensure_equals("TF033CHK002: Unexpected number of view nodes after filtering",
                  schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count(),
                  schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count());
    ensure_equals("TF033CHK002: Unexpected number of procedure nodes after filtering",
                  schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count(),
                  schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count());
    ensure_equals("TF033CHK002: Unexpected number of function nodes after filtering",
                  schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count(),
                  schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count());

    for (int table_index = 0; table_index < schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count();
         table_index++) {
      object_node = schema_node->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(table_index);
      object_node_f = schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(table_index);

      ensure("TF033CHK002: Unexpected table data in filtered table",
             object_node->get_data() == object_node_f->get_data());
      ensure_equals("TF033CHK002: Unexpected number of table collection nodes after filtering", object_node_f->count(),
                    object_node->count());
      ensure_equals("TF033CHK002: Unexpected number of column nodes after filtering",
                    object_node_f->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->count(),
                    object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->count());
      ensure_equals("TF033CHK002: Unexpected number of index nodes after filtering",
                    object_node_f->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->count(),
                    object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->count());
      ensure_equals("TF033CHK002: Unexpected number of trigger nodes after filtering",
                    object_node_f->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->count(),
                    object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->count());
      ensure_equals("TF033CHK002: Unexpected number of foreign key nodes after filtering",
                    object_node_f->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->count(),
                    object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->count());

      for (int column_index = 0;
           column_index < object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->count(); column_index++) {
        sub_node = object_node->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->get_child(column_index);
        sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX)->get_child(column_index);

        ensure("TF033CHK002: Unexpected column data in filtered table column",
               sub_node->get_data() == sub_node_f->get_data());
      }

      for (int index_index = 0; index_index < object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->count();
           index_index++) {
        sub_node = object_node->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->get_child(index_index);
        sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_INDEXES_NODE_INDEX)->get_child(index_index);

        ensure("TF033CHK002: Unexpected index data in filtered index", sub_node->get_data() == sub_node_f->get_data());
      }

      for (int trigger_index = 0;
           trigger_index < object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->count();
           trigger_index++) {
        sub_node = object_node->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->get_child(trigger_index);
        sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX)->get_child(trigger_index);

        ensure("TF033CHK002: Unexpected trigger data in filtered trigger",
               sub_node->get_data() == sub_node_f->get_data());
      }

      for (int fk_index = 0; fk_index < object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->count();
           fk_index++) {
        sub_node = object_node->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->get_child(fk_index);
        sub_node_f = object_node_f->get_child(LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX)->get_child(fk_index);

        ensure("TF033CHK002: Unexpected foreign key data in filtered foreign key",
               sub_node->get_data() == sub_node_f->get_data());
      }
    }

    for (int view_index = 0; view_index < schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count();
         view_index++) {
      object_node = schema_node->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(view_index);
      object_node_f = schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(view_index);

      ensure("TF033CHK002: Unexpected table data in filtered view",
             object_node_f->get_data() == object_node->get_data());
      ensure_equals("TF033CHK002: Unexpected number of view column nodes after filtering", object_node_f->count(),
                    object_node->count());

      for (int column_index = 0; column_index < object_node->count(); column_index++) {
        sub_node = object_node->get_child(column_index);
        sub_node_f = object_node_f->get_child(column_index);

        ensure("TF033CHK002: Unexpected column data in filtered view column",
               sub_node->get_data() == sub_node_f->get_data());
      }
    }

    for (int procedure_index = 0;
         procedure_index < schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count();
         procedure_index++) {
      object_node = schema_node->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(procedure_index);
      object_node_f = schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(procedure_index);

      ensure("TF033CHK002: Unexpected procedure data in filtered routine",
             object_node_f->get_data() == object_node->get_data());
    }

    for (int function_index = 0;
         function_index < schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count(); function_index++) {
      object_node = schema_node->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(function_index);
      object_node_f = schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(function_index);

      ensure("TF033CHK002: Unexpected function data in filtered routine",
             object_node_f->get_data() == object_node->get_data());
    }
  }

  root_node->remove_children();
  root_node_f->remove_children();
}

void verify_filter_result(const std::string& check, mforms::TreeNodeRef root, const std::vector<std::string>& schemas,
                          const std::vector<std::string>& tables, const std::vector<std::string>& views,
                          const std::vector<std::string>& procedures, const std::vector<std::string>& functions) {
  mforms::TreeNodeRef schema_node_f;
  mforms::TreeNodeRef object_node_f;

  ensure_equals(check + ": Unexpected number of schema nodes after filtering", (size_t)root->count(), schemas.size());

  for (int schema_index = 0; schema_index < root->count(); schema_index++) {
    schema_node_f = root->get_child(schema_index);

    ensure_equals(check + ": Unexpected schema name after filtering", schema_node_f->get_string(0),
                  schemas[schema_index]);

    ensure_equals(check + ": Unexpected number of table nodes after filtering",
                  (size_t)schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count(), tables.size());
    ensure_equals(check + ": Unexpected number of view nodes after filtering",
                  (size_t)schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count(), views.size());
    ensure_equals(check + ": Unexpected number of procedure nodes after filtering",
                  (size_t)schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count(), procedures.size());
    ensure_equals(check + ": Unexpected number of function nodes after filtering",
                  (size_t)schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count(), functions.size());

    for (int table_index = 0; table_index < schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->count();
         table_index++) {
      object_node_f = schema_node_f->get_child(LiveSchemaTree::TABLES_NODE_INDEX)->get_child(table_index);
      ensure_equals(check + ": Unexpected table node after filtering", object_node_f->get_string(0),
                    tables[table_index]);
    }

    for (int view_index = 0; view_index < schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->count();
         view_index++) {
      object_node_f = schema_node_f->get_child(LiveSchemaTree::VIEWS_NODE_INDEX)->get_child(view_index);
      ensure_equals(check + ": Unexpected view node after filtering", object_node_f->get_string(0), views[view_index]);
    }

    for (int procedure_index = 0;
         procedure_index < schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->count();
         procedure_index++) {
      object_node_f = schema_node_f->get_child(LiveSchemaTree::PROCEDURES_NODE_INDEX)->get_child(procedure_index);
      ensure_equals(check + ": Unexpected procedure node after filtering", object_node_f->get_string(0),
                    procedures[procedure_index]);
    }

    for (int function_index = 0;
         function_index < schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->count(); function_index++) {
      object_node_f = schema_node_f->get_child(LiveSchemaTree::FUNCTIONS_NODE_INDEX)->get_child(function_index);
      ensure_equals(check + ": Unexpected function node after filtering", object_node_f->get_string(0),
                    functions[function_index]);
    }
  }
}

TEST_FUNCTION(34) {
  std::vector<std::string> schemas;
  std::vector<std::string> tables;
  std::vector<std::string> views;
  std::vector<std::string> procedures;
  std::vector<std::string> functions;
  mforms::TreeNodeRef root_node_f = pmodel_view_filtered->root_node();

  fill_complex_schema("TF034CHK001");

  // Sets the filter and does the filtering...
  _lst_filtered.set_base(&_lst);

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

  _lst_filtered.set_filter("dev_schema");
  _lst_filtered.filter_data();
  verify_filter_result("TF034CHK002", pmodel_view_filtered->root_node(), schemas, tables, views, procedures, functions);

  // Filtering specifying a schema wildcard...
  schemas.clear();
  schemas.push_back("basic_schema");
  schemas.push_back("dev_schema");
  schemas.push_back("test_schema");
  _lst_filtered.set_filter("*schema");
  _lst_filtered.filter_data();
  verify_filter_result("TF034CHK003", pmodel_view_filtered->root_node(), schemas, tables, views, procedures, functions);

  // Filtering specifying a different schema wildcard...
  schemas.clear();
  schemas.push_back("basic_schema");
  schemas.push_back("basic_training");
  _lst_filtered.set_filter("basic*");
  _lst_filtered.filter_data();
  verify_filter_result("TF034CHK004", pmodel_view_filtered->root_node(), schemas, tables, views, procedures, functions);

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
  _lst_filtered.set_filter("basic*.sec*");
  _lst_filtered.filter_data();
  verify_filter_result("TF034CHK005", pmodel_view_filtered->root_node(), schemas, tables, views, procedures, functions);

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

  _lst_filtered.set_filter("?asic_*.*s*");
  _lst_filtered.filter_data();
  verify_filter_result("TF034CHK006", pmodel_view_filtered->root_node(), schemas, tables, views, procedures, functions);

  pmodel_view->root_node()->remove_children();
  root_node_f->remove_children();
}

TEST_FUNCTION(35) {
  _tester_filtered.clean_filter();

  ensure_equals("TF035CHK001: Unexpected text filter", _tester_filtered.string_filter(), "");
  ensure("TF035CHK001: Unexpected schema filter", _tester_filtered.schema_filter() == NULL);
  ensure("TF035CHK001: Unexpected object filter", _tester_filtered.object_filter() == NULL);

  _lst_filtered.set_filter("dummy_filter");
  ensure_equals("TF035CHK002: Unexpected text filter", _tester_filtered.string_filter(), "dummy_filter");
  ensure("TF035CHK002: Unexpected schema filter", _tester_filtered.schema_filter() != NULL);
  ensure("TF035CHK002: Unexpected object filter", _tester_filtered.object_filter() == NULL);

  _lst_filtered.set_filter("some*.tab?");
  ensure_equals("TF035CHK003: Unexpected text filter", _tester_filtered.string_filter(), "some*.tab?");
  ensure("TF035CHK003: Unexpected schema filter", _tester_filtered.schema_filter() != NULL);
  ensure("TF035CHK003: Unexpected object filter", _tester_filtered.object_filter() != NULL);

  _lst_filtered.set_filter("sch?ema*");
  ensure_equals("TF035CHK004: Unexpected text filter", _tester_filtered.string_filter(), "sch?ema*");
  ensure("TF035CHK004: Unexpected schema filter", _tester_filtered.schema_filter() != NULL);
  ensure("TF035CHK004: Unexpected object filter", _tester_filtered.object_filter() == NULL);

  _tester_filtered.clean_filter();
  ensure_equals("TF035CHK005: Unexpected text filter", _tester_filtered.string_filter(), "");
  ensure("TF035CHK005: Unexpected schema filter", _tester_filtered.schema_filter() == NULL);
  ensure("TF035CHK005: Unexpected object filter", _tester_filtered.object_filter() == NULL);
}

// Tests load_data_for_filter
TEST_FUNCTION(36) {
  deleg_filtered->_expect_fetch_data_for_filter = true;
  deleg_filtered->_check_id = "TF036CHK001";
  deleg_filtered->_mock_schema_filter = "%sample%";
  deleg_filtered->_mock_object_filter = "_bject%";
  _lst_filtered.load_data_for_filter("*sample", "?bject");
  deleg_filtered->check_and_reset("TF036CHK001");
}

END_TESTS
