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

#include "mysql_table_editor_fk_page.h"

#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>
#include <gtkmm/box.h>
#include <gtkmm/cellrenderercombo.h>
#include <gtk/gtkentry.h>

//------------------------------------------------------------------------------
DbMySQLTableEditorFKPage::DbMySQLTableEditorFKPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be,
                                                   Glib::RefPtr<Gtk::Builder> xml)
  : _owner(owner), _be(be), _xml(xml), _edit_conn(0), _ce(0), _fk_page_content(0), _fk_page_not_supported_label(0) {
  _xml->get_widget("fks", _fk_tv);
  _xml->get_widget("fk_columns", _fk_columns_tv);
  _fk_tv->set_enable_tree_lines(true);
  _fk_tv->set_headers_visible(true);
  _fk_tv->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

  switch_be(_be);

  _fk_tv->signal_cursor_changed().connect(sigc::mem_fun(*this, &DbMySQLTableEditorFKPage::fk_cursor_changed));

  _xml->get_widget("fk_update", _fk_update_combo);
  setup_combo_for_string_list(_fk_update_combo);
  fill_combo_from_string_list(_fk_update_combo, _be->get_fk_action_options());
  _fk_update_combo->signal_changed().connect(
    sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorFKPage::combo_box_changed), ::bec::FKConstraintListBE::OnUpdate));

  _xml->get_widget("fk_delete", _fk_delete_combo);
  setup_combo_for_string_list(_fk_delete_combo);
  fill_combo_from_string_list(_fk_delete_combo, _be->get_fk_action_options());
  _fk_delete_combo->signal_changed().connect(
    sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorFKPage::combo_box_changed), ::bec::FKConstraintListBE::OnDelete));

  Gtk::TextView* fk_comment(0);
  _xml->get_widget("fk_comment", fk_comment);
  _owner->add_text_change_timer(fk_comment, sigc::mem_fun(this, &DbMySQLTableEditorFKPage::set_comment));

  _xml->get_widget("fk_model_only", _fk_model_only);
  _fk_model_only->signal_toggled().connect(sigc::mem_fun(this, &DbMySQLTableEditorFKPage::model_only_toggled));

  _xml->get_widget("fk_page_content_box", _fk_page_content);
  _xml->get_widget("fk_page_not_supported_label", _fk_page_not_supported_label);

  _fk_page_not_supported_label->set_text(
    "Note: foreign keys can only be defined for certain storage engines (like InnoDB)."
    " The server accepts foreign key definitions for other storage engines but silently ignores"
    " them. Switch your table engine (on the Table tab) to one that supports foreign"
    " keys to allow adjustments here.");

  _fk_page_not_supported_label->set_line_wrap(true);
  _fk_page_not_supported_label->hide();

  check_fk_support();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::switch_be(MySQLTableEditorBE* be) {
  _fk_columns_model.clear();
  _fk_columns_tv->unset_model();
  _fk_columns_tv->remove_all_columns();

  _be = be;

  _fk_tv->remove_all_columns();

  _fk_model = ListModelWrapper::create(_be->get_fks(), _fk_tv, "DbMySQLTableEditorFKPage");

  _fk_tables_model = model_from_string_list(_be->get_all_table_names());

  _fk_model->model().append_string_column(bec::FKConstraintListBE::Name, "Foreign Key Name", EDITABLE, NO_ICON);
  _fk_model->model().append_combo_column(bec::FKConstraintListBE::RefTable, "Referenced Table", _fk_tables_model,
                                         EDITABLE, true);
  _fk_tv->set_model(_fk_model);

  Gtk::CellRenderer* rend = _fk_tv->get_column_cell_renderer(0);
  g_signal_connect(rend->gobj(), "editing-started", GCallback(&DbMySQLTableEditorFKPage::cell_editing_started), this);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::refresh() {
  check_fk_support();

  _fk_columns_tv->unset_model();
  _fk_columns_tv->remove_all_columns();

  bec::ListModel* m = _fk_model->get_be_model();
  _fk_model->set_be_model(0);
  _fk_tv->unset_model();

  _fk_tv->set_model(_fk_model);

  _fk_model->set_be_model(m);
  _fk_tv->unset_model();
  _fk_model->refresh();
  recreate_model_from_string_list(_fk_tables_model, _be->get_all_table_names());
  _fk_tv->set_model(_fk_model);

  const bool has_columns = _be->get_columns()->count() > 1;
  _fk_tv->set_sensitive(has_columns);
  _fk_columns_tv->set_sensitive(has_columns);

  fk_cursor_changed();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::fk_cursor_changed() {
  _fk_columns_tv->unset_model();
  _fk_columns_tv->remove_all_columns();

  Gtk::TreeModel::Path path;
  Gtk::TreeView::Column* column(0);

  _fk_tv->get_cursor(path, column);

  bec::NodeId node = _fk_model->get_node_for_path(path);

  if (node.is_valid()) {
    _fk_node = node;
    if (_be->get_fks()->real_count() > _fk_node.back())
      _be->get_fks()->select_fk(_fk_node);
  }
  update_fk_details();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::update_fk_details() {
  ::bec::FKConstraintListBE* fk_be = _be->get_fks();
  fk_be->refresh();

  ::bec::FKConstraintColumnsListBE* fk_columns_be = fk_be->get_columns();
  fk_columns_be->refresh();

  std::string text;
  if (_fk_node.is_valid())
    fk_be->get_field(_fk_node, ::bec::FKConstraintListBE::OnUpdate, text);
  set_selected_combo_item(_fk_update_combo, text);

  if (_fk_node.is_valid())
    fk_be->get_field(_fk_node, ::bec::FKConstraintListBE::OnDelete, text);
  set_selected_combo_item(_fk_delete_combo, text);

  if (_fk_node.is_valid())
    fk_be->get_field(_fk_node, ::bec::FKConstraintListBE::Comment, text);
  Gtk::TextView* fk_comment(0);
  _xml->get_widget("fk_comment", fk_comment);
  fk_comment->get_buffer()->set_text(text);

  Gtk::Label* label;
  _xml->get_widget("fk_index_name", label);
  if (_fk_node.is_valid())
    fk_be->get_field(_fk_node, ::bec::FKConstraintListBE::Index, text);
  else
    text = "";
  label->set_text(text);

  ssize_t model_only = 0;
  if (_fk_node.is_valid())
    fk_be->get_field(_fk_node, ::bec::FKConstraintListBE::ModelOnly, model_only);
  _fk_model_only->set_active(model_only != 0);

  // Update columns
  _fk_columns_tv->unset_model();
  if (fk_columns_be->count() >= 1 && _fk_node.is_valid() && _fk_node.back() < _be->get_fks()->real_count()) {
    _fk_columns_tv->remove_all_columns();

    _fk_columns_model = ListModelWrapper::create(fk_columns_be, _fk_columns_tv, "FKColumnsModel");

    _fk_columns_model->model().append_check_column(bec::FKConstraintColumnsListBE::Enabled, "", EDITABLE);
    _fk_columns_model->model().append_string_column(bec::FKConstraintColumnsListBE::Column, "Column", RO, NO_ICON);
    const std::vector<std::string> list;
    _fk_columns_model->model().append_combo_column(::bec::FKConstraintColumnsListBE::RefColumn, "Referenced Column",
                                                   model_from_string_list(list), EDITABLE, true);

    _fk_columns_tv->set_model(_fk_columns_model);

    // Connect signal so we can fill referenced columns combobox cell with correct values
    _fkcol_cell_edit_conn.disconnect();
    Gtk::CellRendererCombo* rend = static_cast<Gtk::CellRendererCombo*>(_fk_columns_tv->get_column_cell_renderer(2));
    if (rend) {
      _fkcol_cell_edit_conn = rend->signal_editing_started().connect(
        sigc::mem_fun(this, &DbMySQLTableEditorFKPage::fkcol_cell_editing_started));
    } else
      g_message("REND is 0!");

    _fk_update_combo->set_sensitive(true);
    _fk_delete_combo->set_sensitive(true);
    fk_comment->set_sensitive(true);
    _fk_model_only->set_sensitive(true);
  } else {
    _fk_update_combo->set_sensitive(false);
    _fk_delete_combo->set_sensitive(false);
    fk_comment->set_sensitive(false);
    _fk_model_only->set_sensitive(false);
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::fkcol_cell_editing_started(Gtk::CellEditable* cell, const Glib::ustring& path) {
  bec::NodeId node(path);
  if (node.is_valid())
    _fk_node = node;

  Gtk::CellRendererCombo* rend = static_cast<Gtk::CellRendererCombo*>(_fk_columns_tv->get_column_cell_renderer(2));

  const std::vector<std::string> list = _be->get_fks()->get_columns()->get_ref_columns_list(_fk_node, false);
  Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_static(rend->property_model().get_value());
  recreate_model_from_string_list(model, list);
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::cell_editing_started(GtkCellRenderer* cr, GtkCellEditable* ce, gchar* path,
                                                    gpointer udata) {
  DbMySQLTableEditorFKPage* self = reinterpret_cast<DbMySQLTableEditorFKPage*>(udata);

  bec::NodeId node(path);
  if (node.is_valid())
    self->_fk_node = node;

  if (GTK_IS_ENTRY(ce)) // Fill in name of the foreign key column
  {
    ::bec::FKConstraintListBE* fk_be = self->_be->get_fks();
    Gtk::Entry* entry = Glib::wrap(GTK_ENTRY(ce));

    std::string name;
    if (node.back() == fk_be->count() - 1)
      fk_be->set_field(node, bec::FKConstraintListBE::Name, 1);

    fk_be->get_field(node, bec::FKConstraintListBE::Name, name);
    entry->set_text(name);
  }

  // clean up edit_done signal/slotl
  if (self->_ce && self->_edit_conn) {
    g_signal_handler_disconnect(self->_ce, self->_edit_conn);
    self->_ce = 0;
    self->_edit_conn = 0;
  }

  if (GTK_IS_CELL_EDITABLE(ce)) {
    self->_ce = ce;
    self->_edit_conn =
      g_signal_connect(ce, "editing-done", GCallback(&DbMySQLTableEditorFKPage::cell_editing_done), udata);
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::cell_editing_done(GtkCellEditable* ce, gpointer udata) {
  DbMySQLTableEditorFKPage* self = reinterpret_cast<DbMySQLTableEditorFKPage*>(udata);

  if (self->_ce && self->_edit_conn) {
    g_signal_handler_disconnect(self->_ce, self->_edit_conn);
    self->_ce = 0;
    self->_edit_conn = 0;
  }

  if (self->_fk_node.back() == self->_be->get_fks()->real_count() - 1) {
    self->refresh();
    self->_fk_tv->set_cursor(node2path(self->_fk_node));
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::combo_box_changed(const int model_column) {
  Gtk::ComboBox* combo(0);

  if (::bec::FKConstraintListBE::OnUpdate == model_column)
    combo = _fk_update_combo;
  else if (::bec::FKConstraintListBE::OnDelete == model_column)
    combo = _fk_delete_combo;

  if (combo)
    if (!_be->get_fks()->set_field(_fk_node, model_column, get_selected_combo_item(combo))) {
      if (::bec::FKConstraintListBE::OnUpdate == model_column) {
        if (_fk_node.is_valid()) {
          std::string _prev;
          _be->get_fks()->get_field(_fk_node, ::bec::FKConstraintListBE::OnUpdate, _prev);
          set_selected_combo_item(combo, _prev);
        }
      } else if (::bec::FKConstraintListBE::OnDelete == model_column) {
        if (_fk_node.is_valid()) {
          std::string _prev;
          _be->get_fks()->get_field(_fk_node, ::bec::FKConstraintListBE::OnDelete, _prev);
          set_selected_combo_item(combo, _prev);
        }
      }
    }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::set_comment(const std::string& comment) {
  _be->get_fks()->set_field(_fk_node, ::bec::FKConstraintListBE::Comment, comment);
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::model_only_toggled() {
  _be->get_fks()->set_field(_fk_node, ::bec::FKConstraintListBE::ModelOnly, _fk_model_only->get_active());
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorFKPage::check_fk_support() {
  if (_fk_page_content && _fk_page_not_supported_label) {
    if (_be && _be->engine_supports_foreign_keys()) {
      _fk_page_not_supported_label->hide();
      _fk_page_content->show();
    } else {
      _fk_page_not_supported_label->show();
      _fk_page_content->hide();
    }
  }
}
