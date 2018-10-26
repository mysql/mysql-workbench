/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _NAME_MAPPING_EDITOR_H_
#define _NAME_MAPPING_EDITOR_H_

#include "mforms/treeview.h"
#include "mforms/label.h"
#include "mforms/textbox.h"
#include "mforms/box.h"
#include "mforms/form.h"
#include "mforms/panel.h"
#include "mforms/button.h"
#include "mforms/selector.h"
#include "mforms/checkbox.h"
#include "mforms/table.h"
#include "mforms/uistyle.h"
#include "grts/structs.db.h"
#include "grtpp_util.h"
#include "db_mysql_sql_script_sync.h"
#include <set>

class TableNameMappingEditor : public mforms::Form {
public:
  class NodeData : public mforms::TreeNodeData {
  public:
    db_TableRef left, right;
    NodeData(db_TableRef aleft, db_TableRef aright) : left(aleft), right(aright) {
    }
  };

  TableNameMappingEditor(mforms::Form *owner, SynchronizeDifferencesPageBEInterface *diff, db_SchemaRef left_schema,
                         db_SchemaRef right_schema)
    : mforms::Form(owner),
      _diff(diff),
      _left_schema(left_schema),
      _right_schema(right_schema),
      _vbox(false),
      _tree(mforms::TreeShowColumnLines | mforms::TreeAltRowColors),
      _bbox(true) {
    set_title(_("Table Name Mapping"));
    set_name("Table Name Mapping Editor");

    _vbox.add(&_heading, false, true);
    _heading.set_text(
      _("If a table is being incorrectly mapped between source and destination schemas, you can change the mapping "
        "below."));

    _vbox.set_padding(MF_WINDOW_PADDING);
    _vbox.set_spacing(MF_TABLE_ROW_SPACING);
    _vbox.add(&_tree, true, true);

    _tree.add_column(mforms::IconStringColumnType, _("Source Table"), 200, false);
    _tree.add_column(mforms::IconStringColumnType, _("Original Target Table"), 200, false);
    _tree.add_column(mforms::StringColumnType, _("Target Table"), 200, false);
    _tree.add_column(mforms::IconStringColumnType, _("Expected Action"), 100, false);
    _tree.end_columns();
    _tree.signal_changed()->connect(std::bind(&TableNameMappingEditor::list_selection_changed, this));

    mforms::Panel *p = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    _panel = p;
    p->set_title(_("Change Mapping"));

    mforms::Table *table = mforms::manage(new mforms::Table());
    table->set_row_count(3);
    table->set_column_count(2);
    table->set_row_spacing(MF_TABLE_ROW_SPACING);
    table->set_column_spacing(MF_TABLE_COLUMN_SPACING);
    table->set_padding(MF_PANEL_PADDING);
    table->add(mforms::manage(new mforms::Label(_("Table:"), true)), 0, 1, 0, 1, 0);
    table->add(&_model_name, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
    table->add(mforms::manage(new mforms::Label(_("Default Target Table:"), true)), 0, 1, 1, 2, 0);
    table->add(&_original_name, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);

    mforms::Label *l = mforms::manage(new mforms::Label(_("Desired Target Table:"), true));
    table->add(l, 0, 1, 2, 3, 0);
    table->add(&_remap_selector, 1, 2, 2, 3, mforms::HFillFlag | mforms::HExpandFlag);

    scoped_connect(_remap_selector.signal_changed(), std::bind(&TableNameMappingEditor::remap_selected, this));

    _vbox.add(p, false, true);
    p->add(table);

    _bbox.set_spacing(MF_BUTTON_SPACING);

    _ok_button.set_text(_("OK"));
    _cancel_button.set_text(_("Cancel"));

    mforms::Utilities::add_end_ok_cancel_buttons(&_bbox, &_ok_button, &_cancel_button);
    _vbox.add(&_bbox, false, true);

    set_content(&_vbox);

    set_size(800, 600);
    center();

    update_remap_selector();
    update_name_tree();
  }

  void apply_changes(std::list<db_TableRef> &changed_Tables) {
    for (int c = _tree.count(), i = 0; i < c; i++) {
      mforms::TreeNodeRef child = _tree.node_at_row(i);
      NodeData *data = dynamic_cast<NodeData *>(child->get_data());
      if (data) {
        std::string new_name = child->get_string(2);
        if (data->left.is_valid() && data->left->oldName() != new_name) {
          data->left->oldName(new_name);
          changed_Tables.push_back(data->left);
        }
      }
    }
  }

  bool run(std::list<db_TableRef> &changed_Tables) {
    if (run_modal(&_ok_button, &_cancel_button)) {
      apply_changes(changed_Tables);
      return true;
    }
    return false;
  }

  void list_selection_changed() {
    update_remap_selector();
  }

  void update_action(mforms::TreeNodeRef node) {
    NodeData *data = dynamic_cast<NodeData *>(node->get_data());
    if (!data->left.is_valid()) {
      // new Table
      if (node->get_string(2) == node->get_string(1)) {
        node->set_string(3, "CREATE");
        node->set_icon_path(3, bec::IconManager::get_instance()->get_icon_path("change_alert_create.png"));
      } else {
        node->set_string(3, "");
        node->set_icon_path(3, "");
      }
    } else {
      if (node->get_string(2).empty()) {
        node->set_string(3, "DROP");
        node->set_icon_path(3, bec::IconManager::get_instance()->get_icon_path("change_alert_drop.png"));
      } else if (node->get_string(2) != node->get_string(0)) {
        node->set_string(3, "RENAME");
        node->set_icon_path(3, bec::IconManager::get_instance()->get_icon_path("change_alert_thin.png"));
      } else if (_diff->get_sql_for_object(data->left).empty() && _diff->get_sql_for_object(data->right).empty()) {
        node->set_string(3, "");
        node->set_icon_path(3, "");
      } else {
        node->set_string(3, "CHANGE");
        node->set_icon_path(3, bec::IconManager::get_instance()->get_icon_path("change_alert_thin.png"));
      }
    }
  }

  void remap_selected() {
    mforms::TreeNodeRef node = _tree.get_selected_node();
    if (node) {
      int i = _remap_selector.get_selected_index();
      if (i >= 0) {
        std::string item = _remap_selector.get_item_title(i);
        node->set_string(2, item);

        for (int j = 0; j < _tree.count(); j++) {
          mforms::TreeNodeRef n(_tree.node_at_row(j));
          if (n != node && n->get_string(2) == item) {
            n->set_string(2, "");
            n->set_icon_path(3, "");
            update_action(n);
            break;
          }
        }
        update_action(node);
      }
    }
  }

  void update_remap_selector() {
    _remap_selector.clear();
    _model_name.set_text("");
    _original_name.set_text("");

    bool enabled = false;
    mforms::TreeNodeRef selected(_tree.get_selected_node());
    if (selected) {
      NodeData *data = dynamic_cast<NodeData *>(selected->get_data());
      std::string target;
      if (data) {
        std::list<std::string> names;

        _model_name.set_text(selected->get_string(0));
        _original_name.set_text(selected->get_string(1));

        if (selected->get_string(0).empty()) {
          // new objects can only be skipped or created with the same name
          names.push_back("");
          names.push_back(selected->get_string(1));
        } else {
          names.push_back("");
          GRTLIST_FOREACH(db_Table, _right_schema->tables(), table) {
            names.push_back((*table)->name());
          }
        }
        enabled = names.size() > 1;
        _remap_selector.add_items(names);

        if (!selected->get_string(2).empty())
          target = selected->get_string(2);

        if (!target.empty()) {
          int i = _remap_selector.index_of_item_with_title(target);
          if (i >= 0)
            _remap_selector.set_selected(i);
          else
            _remap_selector.set_selected(0);
        }
      }
    }
    _panel->set_enabled(enabled);
  }

  void update_name_tree() {
    _tree.clear();
    if (_left_schema.is_valid()) {
      std::map<std::string, db_TableRef> right_tables;
      GRTLIST_FOREACH(db_Table, _right_schema->tables(), table) {
        right_tables[(*table)->name()] = *table;
      }

      GRTLIST_FOREACH(db_Table, _left_schema->tables(), table) {
        mforms::TreeNodeRef table_node(_tree.add_node());
        table_node->set_icon_path(0, "db.Table.16x16.png");
        table_node->set_string(0, *(*table)->name());
        std::map<std::string, db_TableRef>::iterator it;
        if ((it = right_tables.find((*table)->oldName())) == right_tables.end()) {
          table_node->set_data(new NodeData(*table, db_TableRef()));
          table_node->set_string(1, "(" + *(*table)->oldName() + ")");
          table_node->set_string(2, "");
        } else {
          table_node->set_data(new NodeData(*table, it->second));
          table_node->set_icon_path(1, "db.Table.16x16.png");
          table_node->set_string(1, *(*table)->oldName());
          table_node->set_string(2, *(*table)->oldName());
          right_tables.erase(it);
        }
        update_action(table_node);
      }

      for (std::map<std::string, db_TableRef>::const_iterator it = right_tables.begin(); it != right_tables.end();
           ++it) {
        mforms::TreeNodeRef table_node(_tree.add_node());
        table_node->set_string(0, "");
        table_node->set_icon_path(1, "db.Table.16x16.png");
        table_node->set_string(1, it->first);
        table_node->set_string(2, it->first);
        table_node->set_data(new NodeData(db_TableRef(), it->second));
        update_action(table_node);
      }
    }
  }

private:
  SynchronizeDifferencesPageBEInterface *_diff;
  db_SchemaRef _left_schema;
  db_SchemaRef _right_schema;
  mforms::Box _vbox;
  mforms::Label _heading;
  mforms::TreeView _tree;

  mforms::Panel *_panel;

  mforms::Box _bbox;
  mforms::Button _ok_button;
  mforms::Button _cancel_button;

  mforms::Label _model_name;
  mforms::Label _original_name;
  mforms::Selector _remap_selector;
};

class ColumnNameMappingEditor : public mforms::Form {
public:
  class NodeData : public mforms::TreeNodeData {
  public:
    db_ColumnRef left, right;
    NodeData(db_ColumnRef aleft, db_ColumnRef aright) : left(aleft), right(aright) {
    }
  };

  ColumnNameMappingEditor(mforms::Form *owner, SynchronizeDifferencesPageBEInterface *diff, db_TableRef left_table,
                          db_TableRef right_table)
    : mforms::Form(owner),
      _diff(diff),
      _left_table(left_table),
      _right_table(right_table),
      _vbox(false),
      _tree(mforms::TreeShowColumnLines | mforms::TreeAltRowColors),
      _bbox(true) {
    set_title(_("Column Name Mapping"));
    set_name("Column Name Mapping Editor");

    _vbox.add(&_heading, false, true);
    _heading.set_text(
      _("If a column is being incorrectly mapped between source and destination schemas, you can change the mapping "
        "below."));

    _vbox.set_padding(MF_WINDOW_PADDING);
    _vbox.set_spacing(MF_TABLE_ROW_SPACING);
    _vbox.add(&_tree, true, true);

    _tree.add_column(mforms::IconStringColumnType, _("Source Column"), 200, false);
    _tree.add_column(mforms::IconStringColumnType, _("Original Target Column"), 200, false);
    _tree.add_column(mforms::StringColumnType, _("Target Column"), 200, false);
    _tree.add_column(mforms::IconStringColumnType, _("Expected Action"), 100, false);
    _tree.end_columns();
    _tree.signal_changed()->connect(std::bind(&ColumnNameMappingEditor::list_selection_changed, this));

    mforms::Panel *p = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    _panel = p;
    p->set_title(_("Change Mapping"));

    mforms::Table *table = mforms::manage(new mforms::Table());
    table->set_row_count(3);
    table->set_column_count(2);
    table->set_row_spacing(MF_TABLE_ROW_SPACING);
    table->set_column_spacing(MF_TABLE_COLUMN_SPACING);
    table->set_padding(MF_PANEL_PADDING);
    table->add(mforms::manage(new mforms::Label(_("Column:"), true)), 0, 1, 0, 1, 0);
    table->add(&_model_name, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
    table->add(mforms::manage(new mforms::Label(_("Default Target Column:"), true)), 0, 1, 1, 2, 0);
    table->add(&_original_name, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);

    mforms::Label *l = mforms::manage(new mforms::Label(_("Desired Target Column:"), true));
    table->add(l, 0, 1, 2, 3, 0);
    table->add(&_remap_selector, 1, 2, 2, 3, mforms::HFillFlag | mforms::HExpandFlag);

    scoped_connect(_remap_selector.signal_changed(), std::bind(&ColumnNameMappingEditor::remap_selected, this));

    _vbox.add(p, false, true);
    p->add(table);

    _bbox.set_spacing(MF_BUTTON_SPACING);

    _ok_button.set_text(_("OK"));
    _cancel_button.set_text(_("Cancel"));

    mforms::Utilities::add_end_ok_cancel_buttons(&_bbox, &_ok_button, &_cancel_button);
    _vbox.add(&_bbox, false, true);

    set_content(&_vbox);

    set_size(800, 600);
    center();

    update_remap_selector();
    update_name_tree();
  }

  void apply_changes(std::list<db_ColumnRef> &changed_columns) {
    for (int c = _tree.count(), i = 0; i < c; i++) {
      mforms::TreeNodeRef child = _tree.node_at_row(i);
      NodeData *data = dynamic_cast<NodeData *>(child->get_data());
      if (data) {
        std::string new_name = child->get_string(2);
        if (data->left.is_valid() && data->left->oldName() != new_name) {
          data->left->oldName(new_name);
          changed_columns.push_back(data->left);
        }
      }
    }
  }

  bool run(std::list<db_ColumnRef> &changed_columns) {
    if (run_modal(&_ok_button, &_cancel_button)) {
      apply_changes(changed_columns);
      return true;
    }
    return false;
  }

  void list_selection_changed() {
    update_remap_selector();
  }

  void update_action(mforms::TreeNodeRef node) {
    NodeData *data = dynamic_cast<NodeData *>(node->get_data());
    if (!data->left.is_valid()) {
      // new column
      if (node->get_string(2) == node->get_string(1))
        node->set_string(3, "CREATE");
      else
        node->set_string(3, "");
    } else {
      if (node->get_string(2).empty())
        node->set_string(3, "DROP");
      else if (node->get_string(2) != node->get_string(0))
        node->set_string(3, "RENAME");
      else if (_diff->get_sql_for_object(data->left).empty() && _diff->get_sql_for_object(data->right).empty())
        node->set_string(3, "");
      else
        node->set_string(3, "CHANGE");
    }
  }

  void remap_selected() {
    mforms::TreeNodeRef node = _tree.get_selected_node();
    if (node) {
      int i = _remap_selector.get_selected_index();
      if (i >= 0) {
        std::string item = _remap_selector.get_item_title(i);
        node->set_string(2, item);

        for (int j = 0; j < _tree.count(); j++) {
          mforms::TreeNodeRef n(_tree.node_at_row(j));
          if (n != node && n->get_string(2) == item) {
            n->set_string(2, "");
            update_action(n);
            break;
          }
        }
        update_action(node);
      }
    }
  }

  void update_remap_selector() {
    _remap_selector.clear();
    _model_name.set_text("");
    _original_name.set_text("");

    bool enabled = false;
    mforms::TreeNodeRef selected(_tree.get_selected_node());
    if (selected) {
      NodeData *data = dynamic_cast<NodeData *>(selected->get_data());
      std::string target;
      if (data) {
        std::list<std::string> names;

        _model_name.set_text(selected->get_string(0));
        _original_name.set_text(selected->get_string(1));

        if (selected->get_string(0).empty()) {
          // new objects can only be skipped or created with the same name
          names.push_back("");
          names.push_back(selected->get_string(1));
        } else {
          names.push_back("");
          GRTLIST_FOREACH(db_Column, _right_table->columns(), column) {
            names.push_back((*column)->name());
          }
        }
        enabled = names.size() > 1;
        _remap_selector.add_items(names);

        if (!selected->get_string(2).empty())
          target = selected->get_string(2);

        if (!target.empty()) {
          int i = _remap_selector.index_of_item_with_title(target);
          if (i >= 0)
            _remap_selector.set_selected(i);
          else
            _remap_selector.set_selected(0);
        }
      }
    }
    _panel->set_enabled(enabled);
  }

  void update_name_tree() {
    _tree.clear();
    if (_left_table.is_valid()) {
      std::map<std::string, db_ColumnRef> right_columns;
      GRTLIST_FOREACH(db_Column, _right_table->columns(), column) {
        right_columns[(*column)->name()] = *column;
      }

      GRTLIST_FOREACH(db_Column, _left_table->columns(), column) {
        mforms::TreeNodeRef column_node(_tree.add_node());
        column_node->set_icon_path(0, "db.Column.16x16.png");
        column_node->set_string(0, *(*column)->name());
        std::map<std::string, db_ColumnRef>::iterator it;
        if ((it = right_columns.find((*column)->oldName())) == right_columns.end()) {
          column_node->set_data(new NodeData(*column, db_ColumnRef()));
          column_node->set_string(1, "(" + *(*column)->oldName() + ")");
          column_node->set_string(2, "");
        } else {
          column_node->set_data(new NodeData(*column, it->second));
          column_node->set_icon_path(1, "db.Column.16x16.png");
          column_node->set_string(1, *(*column)->oldName());
          column_node->set_string(2, *(*column)->oldName());
          right_columns.erase(it);
        }
        update_action(column_node);
      }

      for (std::map<std::string, db_ColumnRef>::const_iterator it = right_columns.begin(); it != right_columns.end();
           ++it) {
        mforms::TreeNodeRef column_node(_tree.add_node());
        column_node->set_string(0, "");
        column_node->set_icon_path(1, "db.Column.16x16.png");
        column_node->set_string(1, it->first);
        column_node->set_string(2, it->first);
        column_node->set_data(new NodeData(db_ColumnRef(), it->second));
        update_action(column_node);
      }
    }
  }

private:
  SynchronizeDifferencesPageBEInterface *_diff;
  db_TableRef _left_table;
  db_TableRef _right_table;
  mforms::Box _vbox;
  mforms::Label _heading;
  mforms::TreeView _tree;

  mforms::Panel *_panel;

  mforms::Box _bbox;
  mforms::Button _ok_button;
  mforms::Button _cancel_button;

  mforms::Label _model_name;
  mforms::Label _original_name;
  mforms::Selector _remap_selector;
};

#endif
