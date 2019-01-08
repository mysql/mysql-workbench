/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

//#include "gtk/gtk.h"
#include "mysql_table_editor_fe.h"
#include "grtdb/db_object_helpers.h"
#include "treemodel_wrapper.h"
#include "gtk_helpers.h"
#include "auto_completable.h"

#include "mysql_table_editor_column_page.h"

#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>
#include <glibmm/main.h>
#include <set>
#include <memory>

std::shared_ptr<AutoCompletable> DbMySQLTableEditorColumnPage::_types_completion;
std::shared_ptr<AutoCompletable> DbMySQLTableEditorColumnPage::_names_completion;

extern const char *DEFAULT_CHARSET_CAPTION;
extern const char *DEFAULT_COLLATION_CAPTION;


//------------------------------------------------------------------------------
DbMySQLTableEditorColumnPage::DbMySQLTableEditorColumnPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be,
                                                           Glib::RefPtr<Gtk::Builder> xml)
  : _owner(owner),
    _be(be),
    _xml(xml),
    _tv(0),
    _tv_holder(0),
    _edit_conn(0),
    _ce(0),
    _auto_edit_pending(false),
    _editing(false) {
  _xml->get_widget("table_columns_holder", _tv_holder);

  _old_column_count = 0;

  refill_columns_tv();
  refill_completions();

  Gtk::TextView* column_comment;
  _xml->get_widget("column_comment", column_comment);
  _owner->add_text_change_timer(column_comment, sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::set_comment));

  _xml->get_widget("column_charset_combo", _charset_combo);
  _charset_combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::set_charset));
  setup_combo_for_string_list(_charset_combo);
  fill_combo_from_string_list(_charset_combo, _be->get_charset_list());
  set_selected_combo_item(_charset_combo, DEFAULT_CHARSET_CAPTION);
  
  _xml->get_widget("column_collation_combo", _collation_combo);
  _collation_combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::set_collation));
  setup_combo_for_string_list(_collation_combo);
  fill_combo_from_string_list(_collation_combo, _be->get_charset_collation_list(DEFAULT_CHARSET_CAPTION));
  set_selected_combo_item(_collation_combo, DEFAULT_COLLATION_CAPTION);
  
  Gtk::Box* box;
  _xml->get_widget("gc_storage_type_box", box);
  if (box)
    box->set_sensitive(false);

  _xml->get_widget("gc_stored_radio", _radioStored);
  _xml->get_widget("gc_virtual_radio", _radioVirtual);
  if (_radioStored == 0 || _radioVirtual == 0)
    throw std::logic_error("Glade file is missing gc_stored_radio or gc_virtual_radio");

  _radioStored->signal_toggled().connect(sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::set_gc_storage_type));
  _radioVirtual->signal_toggled().connect(sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::set_gc_storage_type));
}

//------------------------------------------------------------------------------
DbMySQLTableEditorColumnPage::~DbMySQLTableEditorColumnPage() {
  sigQueryTooltip.disconnect();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::switch_be(MySQLTableEditorBE* be) {
  _be = be;

  refill_columns_tv();
  refill_completions();

  // refresh is done from TableEditor
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::refill_completions() {
  types_completion()->clear();
  std::vector<std::string> types(_be->get_columns()->get_datatype_names());

  for (std::vector<std::string>::const_iterator iter = types.begin(); iter != types.end(); ++iter) {
    if (*iter != "-")
      types_completion()->add_completion_text(*iter);
  }

  names_completion()->clear();

  // Add to completion list all column names in a schema
  bec::ColumnNamesSet column_names = _be->get_columns()->get_column_names_completion_list();

  bec::ColumnNamesSet::const_iterator it = column_names.begin();
  bec::ColumnNamesSet::const_iterator last = column_names.end();
  for (; last != it; ++it)
    names_completion()->add_completion_text(*it);
}

//--------------------------------------------------------------------------------

const static std::map<std::string, std::string> titleMap = { { "PK", "Primary Key" },    { "NN", "Not Null" },
                                                  { "UQ", "Unique" },         { "BIN", "Binary" },
                                                  { "UN", "Unsigned" },       { "ZF", "Zero Fill" },
                                                  { "AI", "Auto Increment" }, { "G", "Generated" } };

void DbMySQLTableEditorColumnPage::refill_columns_tv() {
  std::auto_ptr<Gtk::TreeView> new_tv(new Gtk::TreeView());

  // Replace old treeview with  newly created treeview
  _tv_holder->remove();

  sigQueryTooltip.disconnect();
  if (_tv)
    _tv->remove_all_columns();
  delete _tv;

  _tv = new_tv.get();
  _tv->set_enable_tree_lines(true);
  _tv->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
  _tv->set_has_tooltip(true);

  sigQueryTooltip = _tv->signal_query_tooltip().connect([&] (int x, int y, bool keyboard, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
    Gtk::TreeIter iter;
    _tv->get_tooltip_context_iter(x, y, keyboard, iter);
    Gtk::TreePath path(iter);
    std::vector<Gtk::TreeViewColumn*> cols = _tv->get_columns();
    for (const auto &it: cols) {
      if (x >= it->get_x_offset() && x <= (it->get_x_offset() + it->get_width())) {
        auto name = titleMap.find(it->get_title());
        if (name != titleMap.end()) {
          tooltip->set_text(name->second);
          _tv->set_tooltip_row(tooltip, path);
          return true;
        }
        return false;

      }
    }

    return true;
  });


  Glib::RefPtr<ListModelWrapper> model =
    ListModelWrapper::create(_be->get_columns(), _tv, "DbMySQLTableEditorColumnPage");

  model->model().append_string_column(MySQLTableColumnsListBE::Name, "Column Name", EDITABLE, WITH_ICON);
  model->model().append_combo_column(MySQLTableColumnsListBE::Type, "Datatype",
                                     model->model().create_model(get_types_for_table(_be->get_table())), EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsPK, "PK", EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsNotNull, "NN", EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsUnique, "UQ", EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsBinary, "BIN", EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsUnsigned, "UN", EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsZerofill, "ZF", EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsAutoIncrement, "AI", EDITABLE);
  model->model().append_check_column(MySQLTableColumnsListBE::IsGenerated, "G", EDITABLE);
  model->model().append_string_column(MySQLTableColumnsListBE::Default, "Default / Expression", EDITABLE);
  _model = model;
  new_tv.release();

  _tv_holder->add(*Gtk::manage(_tv));
  _tv->show();

  _tv->set_model(_model);

  std::vector<Gtk::TreeViewColumn*> cols = _tv->get_columns();
  for (int j = cols.size() - 1; j >= 0; --j) {
    std::vector<Gtk::CellRenderer*> rends = cols[j]->get_cells();

    for (int i = rends.size() - 1; i >= 0; --i) {
      GtkCellRenderer* rend = rends[i]->gobj();
      rends[i]->set_data("idx", (gpointer)(long)j);
      g_signal_connect(rend, "editing-started", GCallback(&DbMySQLTableEditorColumnPage::type_cell_editing_started),
                       this);
    }

    switch (j) {
      case 2:
        break;
    }
  }

  _tv->signal_event().connect(sigc::mem_fun(*this, &DbMySQLTableEditorColumnPage::process_event));
  _tv->signal_cursor_changed().connect(sigc::mem_fun(*this, &DbMySQLTableEditorColumnPage::cursor_changed));
  _tv->signal_size_allocate().connect(sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::check_resize));
  _tv->signal_visibility_notify_event().connect(sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::do_on_visible));

  _tv->set_reorderable(true);
}

//--------------------------------------------------------------------------------
bec::NodeId DbMySQLTableEditorColumnPage::get_selected() {
  Gtk::TreePath path;
  Gtk::TreeViewColumn* column;
  _tv->get_cursor(path, column);

  if (path.empty())
    return bec::NodeId();
  return _model->get_node_for_path(path);
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::refresh() {
  if (!_editing) {
    Gtk::TreePath first_row, last_row;

    _tv->get_visible_range(first_row, last_row);
    _tv_holder->freeze_notify();
    _tv->freeze_notify();

    bec::ListModel* m = _model->get_be_model();
    _model->set_be_model(0);
    _tv->unset_model();

    _tv->set_model(_model);

    _model->set_be_model(m);
    _tv->unset_model();
    _model->refresh();
    _tv->set_model(_model);

    cursor_changed();

    if (!first_row.empty())
      _tv->scroll_to_row(first_row);
    _tv->thaw_notify();
    _tv_holder->thaw_notify();
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::partial_refresh(const int what) {
  switch (what) {
    case ::bec::TableEditorBE::RefreshColumnCollation:
      update_collation();
      break;
    case ::bec::TableEditorBE::RefreshColumnMoveUp: {
      std::vector<Gtk::TreePath> rows = _tv->get_selection()->get_selected_rows();
      if (!rows.empty()) {
        _tv->get_selection()->unselect_all();
        Gtk::TreePath path = rows.front();
        if (path.prev() && _tv->get_model()->get_iter(path))
          _tv->get_selection()->select(path);
      }
      break;
    }
    case ::bec::TableEditorBE::RefreshColumnMoveDown: {
      std::vector<Gtk::TreePath> rows = _tv->get_selection()->get_selected_rows();
      if (!rows.empty()) {
        _tv->get_selection()->unselect_all();
        Gtk::TreePath path = rows.front();
        path.next();
        if (_tv->get_model()->get_iter(path))
          _tv->get_selection()->select(path);
      }
      break;
    }
    default:;
  }
}

//------------------------------------------------------------------------------
std::shared_ptr<AutoCompletable> DbMySQLTableEditorColumnPage::types_completion() {
  if (_types_completion == NULL) {
    _types_completion = std::shared_ptr<AutoCompletable>(new AutoCompletable);
  }
  return _types_completion;
}

//------------------------------------------------------------------------------
std::shared_ptr<AutoCompletable> DbMySQLTableEditorColumnPage::names_completion() {
  if (_names_completion == NULL) {
    _names_completion = std::shared_ptr<AutoCompletable>(new AutoCompletable);
  }
  return _names_completion;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::type_cell_editing_started(GtkCellRenderer* cr, GtkCellEditable* ce, gchar* path,
                                                             gpointer udata) {
  // Connect edit_done, so we can trigger refresh
  DbMySQLTableEditorColumnPage* columns_page = reinterpret_cast<DbMySQLTableEditorColumnPage*>(udata);
  columns_page->_editing = true;

  //  GTK_WIDGET()
  const int idx = (int)((long long)g_object_get_data(G_OBJECT(cr), "idx"));

  bec::NodeId node(path);

  columns_page->_old_column_count = columns_page->_be->get_columns()->count();

  if (GTK_IS_COMBO_BOX(ce) && idx == 1) // Attach types auto completion to the cell
  {
    GtkBin* combo = GTK_BIN(ce);
    GtkWidget* widget = gtk_bin_get_child(combo);
    if (GTK_IS_ENTRY(widget)) {
      Gtk::Entry* entry = Glib::wrap((GtkEntry*)widget);

      if (entry)
        types_completion()->add_to_entry(entry);
    }
  } else if (GTK_IS_ENTRY(ce) && idx == 0) // Fill in name of the column
  {
    Gtk::Entry* entry = Glib::wrap(GTK_ENTRY(ce));

    std::string name;
    if (node.back() == columns_page->_be->get_columns()->count() - 1)
      columns_page->_be->get_columns()->set_field(node, MySQLTableColumnsListBE::Name, 1);

    columns_page->_be->get_columns()->get_field(node, MySQLTableColumnsListBE::Name, name);
    entry->set_text(name);

    names_completion()->add_to_entry(entry);
  }

  // clean up edit_done signal/slot
  if (columns_page->_ce && columns_page->_edit_conn) {
    g_signal_handler_disconnect(columns_page->_ce, columns_page->_edit_conn);
    columns_page->_ce = 0;
    columns_page->_edit_conn = 0;
  }

  if (GTK_IS_CELL_EDITABLE(ce)) {
    columns_page->_ce = ce;
    columns_page->_edit_conn =
      g_signal_connect(ce, "editing-done", GCallback(&DbMySQLTableEditorColumnPage::cell_editing_done), udata);
  }
}

//--------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::cell_editing_done(GtkCellEditable* ce, gpointer udata) {
  DbMySQLTableEditorColumnPage* columns_page = reinterpret_cast<DbMySQLTableEditorColumnPage*>(udata);
  columns_page->_editing = false;

  if (columns_page->_ce && columns_page->_edit_conn) {
    g_signal_handler_disconnect(columns_page->_ce, columns_page->_edit_conn);
    columns_page->_ce = 0;
    columns_page->_edit_conn = 0;
  }

  Gtk::TreeModel::Path path;
  Gtk::TreeView::Column* column(0);
  columns_page->_tv->get_cursor(path, column);

  int new_count = columns_page->_be->get_columns()->count();

  if (columns_page->_old_column_count < new_count) {
    double hadj = columns_page->_tv_holder->get_hadjustment()->get_value();
    double vadj = columns_page->_tv_holder->get_vadjustment()->get_value();

    // a refresh of the number of rows is needed or the placeholder wont appear
    // after a new row is added
    columns_page->refresh();

    columns_page->_tv->set_cursor(path);

    columns_page->_tv_holder->get_hadjustment()->set_value(hadj);
    columns_page->_tv_holder->get_hadjustment()->value_changed();
    columns_page->_tv_holder->get_vadjustment()->set_value(vadj);
    columns_page->_tv_holder->get_vadjustment()->value_changed();
  } else
    columns_page->_tv->set_cursor(path);

  // If it's Gtk::Entry, we try to find out if maybe user cancelled edit operation,
  // if so we revert the placeholder row to the initial state.
  if (GTK_IS_ENTRY(ce)) {
    GtkEntry* entry = GTK_ENTRY(ce);
    if (entry) {
      gboolean user_canceled = false;

      g_object_get(entry, "editing-canceled", &user_canceled, NULL);
      if (user_canceled) {
        std::string name;
        bec::NodeId node(path.to_string());
        columns_page->_be->get_columns()->reset_placeholder();
        columns_page->_be->get_columns()->get_field(node, MySQLTableColumnsListBE::Name, name);
        gtk_entry_set_completion(entry, NULL);
        gtk_entry_set_text(entry, name.c_str());
      }
    }
  }
}

//------------------------------------------------------------------------------
grt::StringListRef DbMySQLTableEditorColumnPage::get_types_for_table(const db_TableRef table) {
  grt::StringListRef list(grt::Initialized);
  std::vector<std::string> types(_be->get_columns()->get_datatype_names());

  for (std::vector<std::string>::const_iterator iter = types.begin(); iter != types.end(); ++iter) {
    if (*iter == "-")
      list.insert("----------");
    else
      list.insert(*iter);
  }

  return list;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::check_resize(Gtk::Allocation& r) {
  //   0       1        2  3  4   5   6   7   8   9    10
  // name    type      PK  NN UQ BIN  UN  ZF  AI  G    Default
  const int step = r.get_width() / 10;
  _tv->get_column(0)->set_min_width(4 * step);
  _tv->get_column(1)->set_min_width(2 * step);
  _tv->get_column(10)->set_min_width(2 * step);
}

//------------------------------------------------------------------------------
bool DbMySQLTableEditorColumnPage::process_event(GdkEvent* event) {
  if (event->type == GDK_KEY_RELEASE) {
    type_column_event(event);
  }
  return false;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::type_column_event(GdkEvent* event) {
  if (event->type == GDK_KEY_RELEASE) {
    const int key = event->key.keyval;
    if (key == GDK_KEY_Tab /* || key == GDK_Return */) {
      // Advance to the next column
      Gtk::TreeModel::Path path;
      Gtk::TreeView::Column* column(0);
      _tv->get_cursor(path, column);

      if (column) {
        Glib::ListHandle<Gtk::TreeView::Column*> columns = _tv->get_columns();

        Glib::ListHandle<Gtk::TreeView::Column*>::const_iterator it = columns.begin();
        Glib::ListHandle<Gtk::TreeView::Column*>::const_iterator last = columns.end();

        int i = 0;
        for (; last != it; ++it) {
          if ((*it)->get_title() == column->get_title())
            break;
          ++i;
        }

        // unnecessary and causes scrolling
        // refresh();

        ++it;
        ++i;
        if (it != last && i <= 1)
          _tv->set_cursor(path, **it, true);
        else // next row
        {
          path.next();
          _tv->set_cursor(path, **columns.begin(), true);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::cursor_changed() {
  if (!_editing) {
    update_column_details(get_selected());
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::update_column_details(const ::bec::NodeId& node) {
  Gtk::TextView* column_comment;
  _xml->get_widget("column_comment", column_comment);

  if (node.is_valid()) {
    std::string comment;
    _be->get_columns()->get_field(node, MySQLTableColumnsListBE::Comment, comment);

    column_comment->set_sensitive(true);
    column_comment->get_buffer()->set_text(comment);
  } else {
    column_comment->get_buffer()->set_text("");
    column_comment->set_sensitive(false);
  }

  update_collation();
  update_gc_storage_type();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::set_comment(const std::string& comment) {
  const bec::NodeId node = get_selected();
  if (node.is_valid()) {
    ::bec::TableColumnsListBE* cols = _be->get_columns();
    cols->set_field(node, (int)MySQLTableColumnsListBE::Comment, comment);
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::set_charset() {
  const bec::NodeId node = get_selected();
  if (node.is_valid()) {
    bec::TableColumnsListBE* columns = _be->get_columns();
    std::string charset = get_selected_combo_item(_charset_combo);

    fill_combo_from_string_list(_collation_combo, _be->get_charset_collation_list(charset));
    set_selected_combo_item(_collation_combo, DEFAULT_COLLATION_CAPTION);

    if (charset == DEFAULT_CHARSET_CAPTION)
      charset = "";
    
    columns->set_field(node, MySQLTableColumnsListBE::Charset, charset);
  }
}

//------------------------------------------------------------------------------

void DbMySQLTableEditorColumnPage::set_collation() {
  const bec::NodeId node = get_selected();
  if (node.is_valid()) {
    bec::TableColumnsListBE* columns = _be->get_columns();
    std::string collation = get_selected_combo_item(_collation_combo);
    
    if (collation == DEFAULT_FONT_FAMILY)
      collation = "";

    columns->set_field(node, MySQLTableColumnsListBE::Collation, collation);
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::update_collation() {
//   Gtk::ComboBox* collation_combo;
//   _xml->get_widget("column_collation_combo", collation_combo);

  const bec::NodeId node = get_selected();
  if (node.is_valid()) {
    std::string has_charset;
    MySQLTableColumnsListBE* columns = _be->get_columns();

    columns->get_field(node, MySQLTableColumnsListBE::HasCharset, has_charset);
    if ("1" == has_charset) {
      std::string column_selected_charset;
      std::string column_selected_collation;
      
      columns->get_field(node, MySQLTableColumnsListBE::Charset, column_selected_charset);
      columns->get_field(node, MySQLTableColumnsListBE::Collation, column_selected_collation);

      if (column_selected_charset == "")
        column_selected_charset = DEFAULT_CHARSET_CAPTION;
      if(column_selected_collation == "")
        column_selected_collation = DEFAULT_COLLATION_CAPTION;
      
      _charset_combo->set_sensitive(true);
      _collation_combo->set_sensitive(true);
      set_selected_combo_item(_charset_combo, column_selected_charset);
      set_selected_combo_item(_collation_combo, column_selected_collation);
    } else {
      set_selected_combo_item(_charset_combo, DEFAULT_CHARSET_CAPTION);
      set_selected_combo_item(_collation_combo, DEFAULT_COLLATION_CAPTION);
      _charset_combo->set_sensitive(false);
      _collation_combo->set_sensitive(false);
    }
  } else {
    set_selected_combo_item(_charset_combo, DEFAULT_CHARSET_CAPTION);
    set_selected_combo_item(_collation_combo, DEFAULT_COLLATION_CAPTION);
    _charset_combo->set_sensitive(false);
    _collation_combo->set_sensitive(false);
  }
}
//--------------------------------------------------------------------------------
void DbMySQLTableEditorColumnPage::update_gc_storage_type() {
  const bec::NodeId node = get_selected();
  if (node.is_valid()) {
    std::string has_charset;
    MySQLTableColumnsListBE* columns = _be->get_columns();
    ssize_t isGenerated;
    columns->get_field(node, MySQLTableColumnsListBE::IsGenerated, isGenerated);
    Gtk::Box* box;
    _xml->get_widget("gc_storage_type_box", box);
    if (box)
      box->set_sensitive(isGenerated != 0);

    if (isGenerated != 0) {
      std::string storageType;
      columns->get_field(node, MySQLTableColumnsListBE::GeneratedStorageType, storageType);

      if (base::toupper(storageType) != "STORED")
        _radioVirtual->activate();
      else
        _radioStored->activate();
    }
  }
}

//--------------------------------------------------------------------------------

void DbMySQLTableEditorColumnPage::set_gc_storage_type() {
  const bec::NodeId node = get_selected();
  if (node.is_valid()) {
    MySQLTableColumnsListBE* columns = _be->get_columns();
    if (_radioVirtual->get_active())
      columns->set_field(node, MySQLTableColumnsListBE::GeneratedStorageType, "VIRTUAL");
    else
      columns->set_field(node, MySQLTableColumnsListBE::GeneratedStorageType, "STORED");
  }
}

//--------------------------------------------------------------------------------
bool DbMySQLTableEditorColumnPage::do_on_visible(GdkEventVisibility*) {
  if (!_auto_edit_pending && _be->get_columns()->count() == 1) {
    Glib::signal_idle().connect(
      sigc::bind_return(sigc::mem_fun(this, &DbMySQLTableEditorColumnPage::start_auto_edit), false));
    _auto_edit_pending = true;
  }
  return false;
}

void DbMySQLTableEditorColumnPage::start_auto_edit() {
  MySQLTableColumnsListBE* columns = _be->get_columns();
  ::bec::NodeId node(columns->get_node(0));
  _tv->set_cursor(node2path(node), *(_tv->get_column(0)), true);
}
