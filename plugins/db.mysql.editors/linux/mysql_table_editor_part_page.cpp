/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_table_editor_fe.h"
#include "grtdb/db_object_helpers.h"
#include "treemodel_wrapper.h"

#include "mysql_table_editor_part_page.h"
#include <gtkmm/comboboxtext.h>
#include <gtkmm/togglebutton.h>

//------------------------------------------------------------------------------
DbMySQLTableEditorPartPage::DbMySQLTableEditorPartPage(DbMySQLTableEditor *owner, MySQLTableEditorBE *be,
                                                       Glib::RefPtr<Gtk::Builder> xml)
  : _owner(owner), _be(be), _xml(xml), _refreshing(false) {
  init_widgets();

  Gtk::ToggleButton *btn;
  _xml->get_widget("enable_part_checkbutton", btn);
  btn->signal_toggled().connect(sigc::mem_fun(this, &DbMySQLTableEditorPartPage::enabled_checkbutton_toggled));

  _xml->get_widget("part_tv", _part_tv);
  switch_be(_be);

  refresh();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::switch_be(MySQLTableEditorBE *be) {
  _be = be;

  _part_tv->remove_all_columns();

  _part_model = ListModelWrapper::create(_be->get_partitions(), _part_tv, "DbMySQLTableEditorPartPage");
  _part_model->model().append_string_column(MySQLTablePartitionTreeBE::Name, "Partition", EDITABLE, WITH_ICON);
  _part_model->model().append_string_column(MySQLTablePartitionTreeBE::Value, "Value", EDITABLE, NO_ICON);
  _part_model->model().append_string_column(MySQLTablePartitionTreeBE::DataDirectory, "Data Directory", EDITABLE,
                                            NO_ICON);
  _part_model->model().append_string_column(MySQLTablePartitionTreeBE::IndexDirectory, "Index Directory", EDITABLE,
                                            NO_ICON);
  _part_model->model().append_string_column(MySQLTablePartitionTreeBE::MinRows, "Min Rows", EDITABLE, NO_ICON);
  _part_model->model().append_string_column(MySQLTablePartitionTreeBE::MaxRows, "Max Rows", EDITABLE, NO_ICON);
  _part_model->model().append_string_column(MySQLTablePartitionTreeBE::Comment, "Comment", EDITABLE, NO_ICON);

  _part_tv->set_model(_part_model);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::refresh() {
  _refreshing = true;

  const std::string part_type = _be->get_partition_type();
  Gtk::ToggleButton *btn;
  _xml->get_widget("enable_part_checkbutton", btn);

  bool enabled = !(part_type.empty() || part_type == "");
  btn->set_active(enabled);

  _part_by_combo->set_sensitive(enabled);
  _part_params_entry->set_sensitive(enabled);
  _part_count_entry->set_sensitive(enabled);
  _part_manual_checkbtn->set_sensitive(enabled);

  _subpart_by_combo->set_sensitive(enabled);
  _subpart_params_entry->set_sensitive(enabled);
  _subpart_count_entry->set_sensitive(_be->subpartition_count_allowed());
  _subpart_manual_checkbtn->set_sensitive(enabled);

  if (enabled) {
    set_selected_combo_item(_part_by_combo, _be->get_partition_type());
    _part_params_entry->set_text(_be->get_partition_expression());
    _part_manual_checkbtn->set_active(_be->get_explicit_partitions());

    char buf[32];
    snprintf(buf, sizeof(buf) / sizeof(*buf), "%i", _be->get_partition_count());
    _part_count_entry->set_text(buf);

    set_selected_combo_item(_subpart_by_combo, _be->get_subpartition_type());
    _subpart_params_entry->set_text(_be->get_subpartition_expression());
    _subpart_manual_checkbtn->set_active(_be->get_explicit_subpartitions());

    snprintf(buf, sizeof(buf) / sizeof(*buf), "%i", _be->get_subpartition_count());
    _subpart_count_entry->set_text(buf);
  }

  _part_tv->unset_model();
  _part_model->refresh();
  _part_tv->set_model(_part_model);

  _refreshing = false;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::init_widgets() {
  // Init subpart combo
  _xml->get_widget("subpart_by_combo", _subpart_by_combo);
  std::vector<std::string> list;
  list.push_back("Disable");
  list.push_back("HASH");
  list.push_back("LINEAR HASH");
  list.push_back("KEY");
  list.push_back("LINEAR KEY");
  setup_combo_for_string_list(_subpart_by_combo);
  fill_combo_from_string_list(_subpart_by_combo, list);
#if GTK_VERSION_GT(2, 10)
  _subpart_by_combo->set_tooltip_text("Function that is used to determine the partition.");
#endif

  list.clear();
  _xml->get_widget("part_by_combo", _part_by_combo);
  list.push_back("HASH");
  list.push_back("LINEAR HASH");
  list.push_back("KEY");
  list.push_back("LINEAR KEY");
  list.push_back("RANGE");
  list.push_back("LIST");
  setup_combo_for_string_list(_part_by_combo);
  fill_combo_from_string_list(_part_by_combo, list);
#if GTK_VERSION_GT(2, 10)
  _part_by_combo->set_tooltip_text("Function that is used to determine the partition.");
#endif
  _xml->get_widget("part_params_entry", _part_params_entry);
#if GTK_VERSION_GT(2, 10)
  _part_params_entry->set_tooltip_text(
    "The expression or column list used by the function to determine the partition.");
#endif
  _owner->add_entry_change_timer(_part_params_entry,
                                 sigc::mem_fun(this, &DbMySQLTableEditorPartPage::set_part_params_to_be));

  _xml->get_widget("subpart_params_entry", _subpart_params_entry);
#if GTK_VERSION_GT(2, 10)
  _subpart_params_entry->set_tooltip_text(
    "The expression or column list used by the function to determine the partition.");
#endif
  _owner->add_entry_change_timer(_subpart_params_entry,
                                 sigc::mem_fun(this, &DbMySQLTableEditorPartPage::set_subpart_params_to_be));

  _xml->get_widget("part_manual_checkbtn", _part_manual_checkbtn);
#if GTK_VERSION_GT(2, 10)
  _part_manual_checkbtn->set_tooltip_text("Check to manually specify partitioning ranges/values.");
#endif
  _xml->get_widget("subpart_manual_checkbtn", _subpart_manual_checkbtn);
#if GTK_VERSION_GT(2, 10)
  _subpart_manual_checkbtn->set_tooltip_text("Check to manually specify partitioning ranges/values.");
#endif

  _xml->get_widget("part_count_entry", _part_count_entry);
  _part_count_entry->property_width_request() = 96;
  _part_count_entry->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorPartPage::part_count_changed));

  _xml->get_widget("subpart_count_entry", _subpart_count_entry);
  _subpart_count_entry->signal_changed().connect(
    sigc::mem_fun(this, &DbMySQLTableEditorPartPage::subpart_count_changed));
  _subpart_count_entry->property_width_request() = 96;

  _subpart_by_combo->signal_changed().connect(
    sigc::mem_fun(this, &DbMySQLTableEditorPartPage::subpart_function_changed));
  _part_by_combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorPartPage::part_function_changed));

  _part_manual_checkbtn->signal_toggled().connect(
    sigc::mem_fun(this, &DbMySQLTableEditorPartPage::part_manual_toggled));
  _subpart_manual_checkbtn->signal_toggled().connect(
    sigc::mem_fun(this, &DbMySQLTableEditorPartPage::subpart_manual_toggled));
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::enabled_checkbutton_toggled() {
  if (!_refreshing) {
    Gtk::ToggleButton *btn;
    _xml->get_widget("enable_part_checkbutton", btn);
    bool enabled = btn->get_active();

    // Enable/disable widgets according to state of the "enable partition" check button.
    // simply passing the main enabling button state to widgets
    _part_by_combo->set_sensitive(enabled);
    _part_params_entry->set_sensitive(enabled);
    _part_count_entry->set_sensitive(enabled);
    _part_manual_checkbtn->set_sensitive(enabled);

    if (enabled) {
      if (_be->get_partition_type() == "") {
        _be->set_partition_type("HASH");
        part_function_changed();
      }
    } else
      _be->set_partition_type("");

    const std::string part_function = get_selected_combo_item(_part_by_combo);
    if (part_function == "" || !(part_function == "RANGE" || part_function == "LIST"))
      enabled = false;

    _subpart_by_combo->set_sensitive(_be->subpartition_count_allowed());
    _subpart_params_entry->set_sensitive(_be->subpartition_count_allowed());
    _subpart_count_entry->set_sensitive(_be->subpartition_count_allowed());
    _subpart_manual_checkbtn->set_sensitive(_be->subpartition_count_allowed());

    _owner->add_entry_change_timer(_part_params_entry,
                                   sigc::mem_fun(this, &DbMySQLTableEditorPartPage::set_part_params_to_be));
    _owner->add_entry_change_timer(_subpart_params_entry,
                                   sigc::mem_fun(this, &DbMySQLTableEditorPartPage::set_subpart_params_to_be));
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::part_function_changed() {
  if (!_refreshing) {
    const std::string part_function = get_selected_combo_item(_part_by_combo);

    if (part_function == "") {
      set_selected_combo_item(_part_by_combo, _be->get_partition_type());
      return;
    }

    if (part_function != _be->get_partition_type()) {
      if (!_be->set_partition_type(part_function)) {
        set_selected_combo_item(_part_by_combo, _be->get_partition_type());
        return;
      }
    }

    if (_be->subpartition_count_allowed()) {
      _subpart_by_combo->set_sensitive(true);
      _subpart_params_entry->set_sensitive(true);
      _subpart_count_entry->set_sensitive(true);
      _subpart_manual_checkbtn->set_sensitive(true);
    } else {
      _subpart_by_combo->set_sensitive(false);
      _subpart_params_entry->set_sensitive(false);
      _subpart_count_entry->set_sensitive(false);
      _subpart_manual_checkbtn->set_sensitive(false);
    }
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::subpart_function_changed() {
  if (!_refreshing) {
    const std::string subpart_function = get_selected_combo_item(_subpart_by_combo);

    if (subpart_function != _be->get_subpartition_type()) {
      if (subpart_function == "" || !_be->set_subpartition_type(subpart_function)) {
        set_selected_combo_item(_subpart_by_combo, _be->get_subpartition_type());
      }
    }
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::set_part_params_to_be(const std::string &value) {
  _be->set_partition_expression(value);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::set_subpart_params_to_be(const std::string &value) {
  _be->set_subpartition_expression(value);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::part_count_changed() {
  const std::string count = _part_count_entry->get_text();
  if (!count.empty()) {
    _be->set_partition_count(base::atoi<int>(count, 0));
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::subpart_count_changed() {
  const std::string &count = _subpart_count_entry->get_text();
  if (!count.empty()) {
    _be->set_subpartition_count(base::atoi<int>(count, 0));
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::part_manual_toggled() {
  _be->set_explicit_partitions(_part_manual_checkbtn->get_active());
  char buf[32];
  snprintf(buf, sizeof(buf) / sizeof(*buf), "%i", _be->get_partition_count());
  _part_count_entry->set_text(buf);
  refresh();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorPartPage::subpart_manual_toggled() {
  _be->set_explicit_subpartitions(_subpart_manual_checkbtn->get_active());
  char buf[32];
  snprintf(buf, sizeof(buf) / sizeof(*buf), "%i", _be->get_subpartition_count());
  _subpart_count_entry->set_text(buf);
  refresh();
}
