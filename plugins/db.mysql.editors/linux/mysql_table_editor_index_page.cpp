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

#include "mysql_table_editor_index_page.h"
#include <glibmm/main.h>
#include <gtkmm/notebook.h>
#include <gtkmm/treeview.h>
#include <gtkmm/textview.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/cellrenderercombo.h>

//------------------------------------------------------------------------------
DbMySQLTableEditorIndexPage::DbMySQLTableEditorIndexPage(DbMySQLTableEditor *owner, MySQLTableEditorBE *be,
                                                         Glib::RefPtr<Gtk::Builder> xml)
  : _owner(owner), _be(be), _xml(xml), _editing_done_id(0), _editable_cell(0) {
  _xml->get_widget("indexes", _indexes_tv);
  _indexes_tv->set_enable_tree_lines(true);
  _indexes_tv->set_headers_visible(true);

  switch_be(be);

  _indexes_tv->signal_cursor_changed().connect(
    sigc::mem_fun(*this, &DbMySQLTableEditorIndexPage::index_cursor_changed));

  std::vector<std::string> asc_desc_list;
  asc_desc_list.push_back("ASC");
  asc_desc_list.push_back("DESC");
  _sort_order_model = model_from_string_list(asc_desc_list);

  _xml->get_widget("index_storage_combo", _index_storage_combo);
  setup_combo_for_string_list(_index_storage_combo);
  fill_combo_from_string_list(_index_storage_combo, _be->get_index_storage_types());
  _index_storage_combo_conn = _index_storage_combo->signal_changed().connect(
    sigc::mem_fun(this, &DbMySQLTableEditorIndexPage::update_index_storage_type_in_be));

  _owner->bind_entry_and_be_setter("index_key_block_size", this,
                                   &DbMySQLTableEditorIndexPage::set_index_key_block_size);
  _owner->bind_entry_and_be_setter("index_parser", this, &DbMySQLTableEditorIndexPage::set_index_parser);

  _xml->get_widget("index_visibility", _indexVisibility);

  auto toggleFunc = [this]() {
    if (_index_node.is_valid()) {
      _be->get_indexes()->set_field(_index_node, ::MySQLTableIndexListBE::Visible, _indexVisibility->get_active());
    }
  };

  _visibilitySignal = _indexVisibility->signal_toggled().connect(toggleFunc);

  Gtk::TextView *text(0);
  _xml->get_widget("index_comment", text);
  _owner->add_text_change_timer(text, sigc::mem_fun(this, &DbMySQLTableEditorIndexPage::set_index_comment));
  this->update_gui_for_server();

  _order_model = model_from_string_list(std::vector<std::string>());
}
//------------------------------------------------------------------------------
DbMySQLTableEditorIndexPage::~DbMySQLTableEditorIndexPage() {
  if (_editing_done_id != 0 && _editable_cell != 0) {
    g_signal_handler_disconnect(_editable_cell, _editing_done_id);
    _editing_done_id = 0;
    _editable_cell = 0;
  }

  if(!_visibilitySignal.empty())
    _visibilitySignal.disconnect();

  if (!_editing_sig.empty())
    _editing_sig.disconnect();

  if (!_refresh_sig.empty())
    _refresh_sig.disconnect();
}

#include <iostream>
//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::update_gui_for_server() {
  Gtk::TextView *text(0);
  _xml->get_widget("index_comment", text);
  if (_be->is_editing_live_object()) {
    if (!bec::is_supported_mysql_version_at_least(_be->get_catalog()->version(), 5, 5)) {
      text->set_sensitive(false);
    }
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::switch_be(MySQLTableEditorBE *be) {
  if (!_editing_sig.empty())
    _editing_sig.disconnect();

  _index_node = bec::NodeId();

  _indexes_columns_model.clear();
  Gtk::TreeView *tv = 0;
  _xml->get_widget("index_columns", tv);
  tv->remove_all_columns();
  tv->unset_model();

  _be = be;

  _be->get_indexes()->select_index(_index_node);
  // refresh is done from TableEd

  _indexes_tv->remove_all_columns();

  _indexes_model = ListModelWrapper::create(_be->get_indexes(), _indexes_tv, "DbMySQLTableEditorIndexPage");

  _indexes_model->model().append_string_column(0, "Index Name", EDITABLE, NO_ICON);
  _indexes_model->model().append_combo_column(1, "Type", model_from_string_list(_be->get_index_types()), EDITABLE,
                                              true);

  _indexes_tv->set_model(_indexes_model);

  Gtk::CellRenderer *rend = _indexes_tv->get_column_cell_renderer(0);

  _editing_sig =
    rend->signal_editing_started().connect(sigc::mem_fun(this, &DbMySQLTableEditorIndexPage::cell_editing_started));
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::refresh() {
  if (!_refresh_sig.empty())
    _refresh_sig.disconnect();
  // We need to call it from idle, because of some bug in model implementation.
  _refresh_sig = Glib::signal_idle().connect(sigc::mem_fun(this, &DbMySQLTableEditorIndexPage::real_refresh));
}

bool DbMySQLTableEditorIndexPage::real_refresh() {
  if (!_editing_sig.empty())
    _editing_sig.disconnect();

  Gtk::TreeView *tv = 0;
  _xml->get_widget("index_columns", tv);

  tv->unset_model();
  tv->remove_all_columns();

  _index_node = bec::NodeId();
  _be->get_indexes()->select_index(_index_node);

  fill_combo_from_string_list(_index_storage_combo, _be->get_index_storage_types());

  _indexes_tv->remove_all_columns();

  _indexes_model = ListModelWrapper::create(_be->get_indexes(), _indexes_tv, "DbMySQLTableEditorIndexPage");

  _indexes_model->model().append_string_column(0, "Index Name", EDITABLE, NO_ICON);
  _indexes_model->model().append_combo_column(1, "Type", model_from_string_list(_be->get_index_types()), EDITABLE,
                                              true);

  _indexes_tv->set_model(_indexes_model);

  Gtk::CellRenderer *rend = _indexes_tv->get_column_cell_renderer(0);

  _editing_sig =
    rend->signal_editing_started().connect(sigc::mem_fun(this, &DbMySQLTableEditorIndexPage::cell_editing_started));

  const bool has_columns = _be->get_columns()->count() > 1;
  tv->set_sensitive(has_columns);
  _indexes_tv->set_sensitive(has_columns);

  index_cursor_changed();
  return false;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::index_cursor_changed() {
  Gtk::TreeModel::Path path;
  Gtk::TreeView::Column *column(0);

  _indexes_tv->get_cursor(path, column);

  bec::NodeId node = _indexes_model->get_node_for_path(path);

  _index_node = node;
  update_index_details();
}

//------------------------------------------------------------------------------
Glib::RefPtr<Gtk::ListStore> DbMySQLTableEditorIndexPage::recreate_order_model() {
  MySQLTableIndexListBE *indices_be = _be->get_indexes();
  std::vector<std::string> order_list;

  if (indices_be->count() > 1) {
    bec::IndexColumnsListBE *columns_be = indices_be->get_columns();

    if (columns_be && indices_be->count() > 1) {
      const int max_idx_order = columns_be->get_max_order_index();
      char buf[32];

      for (int i = 1; i <= max_idx_order; i++) {
        snprintf(buf, sizeof(buf) / sizeof(*buf), "%i", i);
        order_list.push_back(buf);
      }
    }
  } else
    order_list.push_back("0");

  recreate_model_from_string_list(_order_model, order_list);
  return _order_model;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::update_index_details() {
  MySQLTableIndexListBE *indices_be = _be->get_indexes();
  if (indices_be) {
    std::string index_name;
    indices_be->refresh();

    Gtk::TreeView *tv = 0;
    _xml->get_widget("index_columns", tv);
    tv->remove_all_columns();
    tv->unset_model();

    const bool got_indices = indices_be->count() > 1;
    if (_index_node.is_valid() && got_indices && _index_node.back() < indices_be->real_count()) {
      indices_be->get_field(_index_node, 0, index_name);
      if (!index_name.empty()) {
        indices_be->select_index(_index_node);
        indices_be->refresh();
      }

      _indexes_columns_model = ListModelWrapper::create(indices_be->get_columns(), tv, "IndexesColumnsModel");

      // negative column means value getting/setting is delegated back to us
      _indexes_columns_model->model().append_check_column(-8, "", EDITABLE);
      _indexes_columns_model->model().append_string_column(::bec::IndexColumnsListBE::Name, "Column", RO, NO_ICON);
      _indexes_columns_model->model().append_combo_column(::bec::IndexColumnsListBE::OrderIndex, "#",
                                                          recreate_order_model(), EDITABLE);
      _indexes_columns_model->model().append_combo_column(-2, "Order", _sort_order_model, EDITABLE, true);
      _indexes_columns_model->model().append_int_column(::bec::IndexColumnsListBE::Length, "Length", EDITABLE);

      _indexes_columns_model->set_fake_column_value_getter(
        sigc::mem_fun(this, &DbMySQLTableEditorIndexPage::get_value));
      _indexes_columns_model->set_fake_column_value_setter(
        sigc::mem_fun(this, &DbMySQLTableEditorIndexPage::set_value));

      tv->set_model(_indexes_columns_model);

      std::string text;
      indices_be->get_field(_index_node, ::MySQLTableIndexListBE::StorageType, text);
      _index_storage_combo_conn.block();
      set_selected_combo_item(_index_storage_combo, text);
      _index_storage_combo_conn.unblock();
    } else {
      set_selected_combo_item(_index_storage_combo, "");
    }

    Gtk::Entry *entry(0);
    std::string text;

    // Update row block size
    _xml->get_widget("index_key_block_size", entry);
    indices_be->get_field(_index_node, ::MySQLTableIndexListBE::RowBlockSize, text);
    entry->set_text(text);

    // Update index parser
    _xml->get_widget("index_parser", entry);
    indices_be->get_field(_index_node, ::MySQLTableIndexListBE::Parser, text);
    entry->set_text(text);

    Gtk::TextView *textview(0);
    // update index comment
    _xml->get_widget("index_comment", textview);
    indices_be->get_field(_index_node, ::MySQLTableIndexListBE::Comment, text);
    textview->get_buffer()->set_text(text);

    _xml->get_widget("index_key_block_size", entry);
    entry->set_sensitive(got_indices && _index_node.is_valid() && _index_node.back() < indices_be->real_count());
    _xml->get_widget("index_parser", entry);
    entry->set_sensitive(got_indices && _index_node.is_valid() && _index_node.back() < indices_be->real_count());
    _xml->get_widget("index_comment", textview);
    textview->set_sensitive(got_indices && _index_node.is_valid() && _index_node.back() < indices_be->real_count());


    // We've got always +1 no of indices in the indices_be because the last one is a placeholder.
    if (bec::is_supported_mysql_version_at_least(_be->get_catalog()->version(), 8, 0, 0)) {
      if (_index_node.is_valid()) {
        std::string type;
        indices_be->get_field(_index_node, ::MySQLTableIndexListBE::Type, type);
        ssize_t visible = 1;
        if (type != "PRIMARY")
          indices_be->get_field(_index_node, ::MySQLTableIndexListBE::Visible, visible);
        _visibilitySignal.block();
        _indexVisibility->set_active(visible == 1);
        _visibilitySignal.unblock();

        if (type == "PRIMARY" || (type == "UNIQUE" && indices_be->count() == 2)) {
          _indexVisibility->set_sensitive(0);
        } else {
          _indexVisibility->set_sensitive(1);
        }
      }
    } else {
      _indexVisibility->set_sensitive(0);
    }

    this->update_gui_for_server();

    _index_storage_combo->set_sensitive(got_indices && _index_node.is_valid() &&
                                        _index_node.back() < indices_be->real_count());
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::update_index_storage_type_in_be() {
  MySQLTableIndexListBE *indices_be = _be->get_indexes();

  if (indices_be && _index_node.is_valid()) {
    const std::string new_storage_type = get_selected_combo_item(_index_storage_combo);
    indices_be->set_field(_index_node, ::MySQLTableIndexListBE::StorageType, new_storage_type);
  }
}
void DbMySQLTableEditorIndexPage::cell_editing_done_proxy(GtkCellEditable *ce, gpointer data) {
  DbMySQLTableEditorIndexPage *this_ptr = static_cast<DbMySQLTableEditorIndexPage *>(data);
  if (this_ptr)
    this_ptr->cell_editing_done(ce);
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::cell_editing_started(Gtk::CellEditable *cell, const Glib::ustring &path) {
  bec::NodeId node(path);
  if (node.is_valid())
    _index_node = node;

  if (_editing_done_id != 0 && _editable_cell != 0) {
    g_signal_handler_disconnect(_editable_cell, _editing_done_id);
    _editing_done_id = 0;
    _editable_cell = 0;
  }

  if (GTK_IS_CELL_EDITABLE(cell->gobj())) {
    _be->get_indexes()->get_field(node, MySQLTableIndexListBE::Name, _user_index_name);
    _editable_cell = cell->gobj();
    _editing_done_id = g_signal_connect(_editable_cell, "editing-done",
                                        GCallback(&DbMySQLTableEditorIndexPage::cell_editing_done_proxy), this);
  }
}
//--------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::cell_editing_done(GtkCellEditable *ce) {
  if (_editing_done_id != 0 && _editable_cell != 0) {
    g_signal_handler_disconnect(_editable_cell, _editing_done_id);
    _editing_done_id = 0;
    _editable_cell = 0;
  }

  // If it's Gtk::Entry, we try to find out if maybe user leave edit field empty,
  // if so we revert it to the last known value or to the default one.
  if (GTK_IS_ENTRY(ce)) {
    GtkEntry *entry_widget = GTK_ENTRY(ce);
    if (entry_widget) {
      Gtk::Entry *entry = Glib::wrap(entry_widget);

      if (entry && entry->get_text_length() == 0) {
        Gtk::TreeModel::Path path;
        Gtk::TreeView::Column *column(0);
        _indexes_tv->get_cursor(path, column);
        bec::NodeId node(path.to_string());
        if (node.is_valid()) {
          std::string name = _user_index_name;
          if (name.empty())
            name = strfmt("index%i", path[0] + 1);

          _be->get_indexes()->set_field(node, MySQLTableIndexListBE::Name, name);
          entry->set_text(name);
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::get_value(const Gtk::TreeModel::iterator &iter, int column, GType type,
                                            Glib::ValueBase &value) {
  bec::IndexColumnsListBE *columns_be = _be->get_indexes()->get_columns();
  bec::NodeId node(_indexes_columns_model->node_for_iter(iter));

  if (!node.is_valid())
    return;

  switch (column) {
    case -2: // Sort order
    {
      ssize_t val = 0;
      columns_be->get_field(node, ::bec::IndexColumnsListBE::Descending, val);
      set_glib_string(value, val == 0 ? "ASC" : "DESC");
      break;
    }
    case -8: // Set/unset
    {
      const bool bv = columns_be->get_column_enabled(node);
      set_glib_bool(value, bv);
      break;
    }
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::set_value(const Gtk::TreeModel::iterator &iter, int column, GType type,
                                            const Glib::ValueBase &value) {
  bec::IndexColumnsListBE *columns_be = _be->get_indexes()->get_columns();
  bec::NodeId node(_indexes_columns_model->node_for_iter(iter));

  if (node.is_valid()) {
    switch (column) {
      case -2: {
        Glib::Value<std::string> v;
        v.init(value.gobj());

        columns_be->set_field(node, ::bec::IndexColumnsListBE::Descending, v.get() == "ASC" ? 0 : 1);
        break;
      }
      case -8: {
        Glib::Value<bool> v;
        v.init(value.gobj());
        columns_be->set_column_enabled(node, v.get());
        recreate_order_model();
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::set_index_key_block_size(const std::string &value) {
  MySQLTableIndexListBE *indices_be = _be->get_indexes();
  indices_be->set_field(_index_node, ::MySQLTableIndexListBE::RowBlockSize, value);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::set_index_parser(const std::string &value) {
  MySQLTableIndexListBE *indices_be = _be->get_indexes();
  indices_be->set_field(_index_node, ::MySQLTableIndexListBE::Parser, value);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorIndexPage::set_index_comment(const std::string &value) {
  MySQLTableIndexListBE *indices_be = _be->get_indexes();
  indices_be->set_field(_index_node, ::MySQLTableIndexListBE::Comment, value);
}
