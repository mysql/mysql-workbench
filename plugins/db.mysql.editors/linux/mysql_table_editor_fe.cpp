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

#include "image_cache.h"
#include "grtdb/db_object_helpers.h"
#include "treemodel_wrapper.h"
#include "mysql_table_editor_column_page.h"
#include "mysql_table_editor_index_page.h"
#include "mysql_table_editor_fk_page.h"
#include "mysql_table_editor_trigger_page.h"
#include "mysql_table_editor_part_page.h"
#include "mysql_table_editor_opt_page.h"
//!#include "mysql_table_editor_insert_page.h"
#include "mysql_editor_priv_page.h"
#include "mysql_table_editor_fe.h"
#include "mforms/../gtk/lf_view.h"

extern const char *DEFAULT_CHARSET_CAPTION;
extern const char *DEFAULT_COLLATION_CAPTION;


//------------------------------------------------------------------------------
DbMySQLTableEditor::DbMySQLTableEditor(grt::Module *m, const grt::BaseListRef &args)
  //    : PluginEditorBase(m, grtm, args, "modules/data/editor_mysql_table.glade")
  : PluginEditorBase(m, args, 0),
    _be(new MySQLTableEditorBE(db_mysql_TableRef::cast_from(args[0]))),
    _part_page(0),
    _inserts_panel(0),
    _main_page_widget(0) {
  load_glade((_be->is_editing_live_object()) ? "modules/data/editor_mysql_table_live.glade"
                                             : "modules/data/editor_mysql_table.glade");

  xml()->get_widget("mysql_editor_notebook", _editor_notebook);
  _editor_notebook->signal_switch_page().connect(sigc::mem_fun(this, &DbMySQLTableEditor::page_changed));

  Gtk::Image *image(0);
  xml()->get_widget("table_editor_image", image);
  image->set(ImageCache::get_instance()->image_from_filename("db.Table.editor.48x48.png", false));
  image->set_data("is_large", (void *)1);

  if (!_be->is_editing_live_object())
    xml()->get_widget("table_page_box", _main_page_widget);

  set_border_width(0);

  _columns_page = new DbMySQLTableEditorColumnPage(this, _be, xml());
  _indexes_page = new DbMySQLTableEditorIndexPage(this, _be, xml());
  _fks_page = new DbMySQLTableEditorFKPage(this, _be, xml());
  _triggers_page = new DbMySQLTableEditorTriggerPage(this, _be, xml());
  _part_page = new DbMySQLTableEditorPartPage(this, _be, xml());
  _opts_page = new DbMySQLTableEditorOptPage(this, _be, xml());

  if (!is_editing_live_object()) {
    _inserts_panel = _be->get_inserts_panel();
    _editor_notebook->append_page(*mforms::widget_for_view(_inserts_panel), "Inserts");

    _privs_page = new DbMySQLEditorPrivPage(_be);
    _editor_notebook->append_page(_privs_page->page(), "Privileges");
  } else {
    _inserts_panel = NULL;
    _privs_page = NULL;

    Gtk::ComboBox *cbox = 0;
    xml()->get_widget("schema_combo", cbox);
    if (cbox) {
      setup_combo_for_string_list(cbox);
    }
  }

  create_table_page();

  add(*_editor_notebook);
  _editor_notebook->show();

  show_all();

  Gtk::Entry *entry(0);
  xml()->get_widget("table_name", entry);
  entry->signal_event().connect(sigc::mem_fun(this, &DbMySQLTableEditor::event_from_table_name_entry));

  refresh_form_data();

  focus_widget_when_idle(entry);

  _be->set_refresh_ui_slot(std::bind(&DbMySQLTableEditor::refresh_form_data, this));
  _be->set_partial_refresh_ui_slot(std::bind(&DbMySQLTableEditor::partial_refresh, this, std::placeholders::_1));

  _be->reset_editor_undo_stack();
  // Gtk::Paned* table_page_paned = 0;
  // xml()->get_widget("table_page_paned", table_page_paned);
  // gtk_paned_set_pos_ratio(table_page_paned, 0.2);
}

//------------------------------------------------------------------------------
DbMySQLTableEditor::~DbMySQLTableEditor() {
  // Notebook is not attached to any widget, we need to release it manualy
  if (_editor_notebook->is_managed_())
    _editor_notebook->unreference();

  delete _columns_page;
  delete _indexes_page;
  delete _fks_page;
  delete _triggers_page;
  delete _part_page;
  delete _opts_page;
  delete _inserts_panel;
  delete _privs_page;
  delete _be;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::decorate_object_editor() {
  if (is_editing_live_object()) {
    PluginEditorBase::decorate_object_editor();
    Gtk::Box *header_part = 0;
    xml()->get_widget("header_part", header_part);

    if (header_part->get_parent() == NULL) {
      decorator_control()->pack_start(*header_part, false, true);
      decorator_control()->reorder_child(*header_part, 0);

      Gtk::Button *hide_button = 0;
      xml()->get_widget("hide_button", hide_button);
      Gtk::Image *hide_image =
        Gtk::manage(new Gtk::Image(ImageCache::get_instance()->image_from_filename("EditorExpanded.png", false)));
      Gtk::Image *show_image =
        Gtk::manage(new Gtk::Image(ImageCache::get_instance()->image_from_filename("EditorCollapsed.png", false)));
      hide_image->show();
      Gtk::Box *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
      box->pack_start(*hide_image, false, false);
      box->pack_start(*show_image, false, false);
      box->show();
      show_image->hide();
      hide_button->set_image(*box);
      hide_button->signal_clicked().connect(sigc::mem_fun(this, &DbMySQLTableEditor::toggle_header_part));
      toggle_header_part();
    }
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::toggle_header_part() {
  Gtk::Button *hide_button = 0;
  xml()->get_widget("hide_button", hide_button);

  Gtk::Image *image = 0;
  xml()->get_widget("table_editor_image", image);
  const bool make_image_small = image->get_data("is_large");
  image->set(ImageCache::get_instance()->image_from_filename(
    make_image_small ? "db.Table.editor.24x24.png" : "db.Table.editor.48x48.png", false));
  image->set_data("is_large", (void *)(!make_image_small));

  Gtk::Box *image_box = dynamic_cast<Gtk::Box *>(hide_button->get_image());
  if (image_box) {
    const std::vector<Gtk::Widget *> images = image_box->get_children();
    for (int i = ((int)images.size()) - 1; i >= 0; --i) {
      if (images[i]->is_visible())
        images[i]->hide();
      else
        images[i]->show();
    }

    const char *const names[] = {"collation_label", "charset_combo", "collation_combo", "engine_label", "engine_combo", "comment_box"};
    const int names_size = sizeof(names) / sizeof(const char **);
    for (int i = 0; i < names_size; ++i) {
      Gtk::Widget *w = 0;
      xml()->get_widget(names[i], w);
      if (w) {
        if (w->is_visible())
          w->hide();
        else
          w->show();
      }
    }
  }
}

//------------------------------------------------------------------------------
bool DbMySQLTableEditor::switch_edited_object(const grt::BaseListRef &args) {
  MySQLTableEditorBE *old_be = _be;
  _be = new MySQLTableEditorBE(db_mysql_TableRef::cast_from(args[0]));

  _columns_page->switch_be(_be);
  _indexes_page->switch_be(_be);
  _fks_page->switch_be(_be);
  _triggers_page->switch_be(_be);
  _part_page->switch_be(_be);
  _opts_page->switch_be(_be);
  if (!is_editing_live_object()) {
    int index = _editor_notebook->page_num(*mforms::widget_for_view(_inserts_panel));
    bool active = _editor_notebook->get_current_page() == index;
    _editor_notebook->remove_page(*mforms::widget_for_view(_inserts_panel));

    _inserts_panel = _be->get_inserts_panel();
    _editor_notebook->insert_page(*mforms::widget_for_view(_inserts_panel), "Inserts", index);
    if (active)
      _editor_notebook->set_current_page(index);

    _privs_page->switch_be(_be);
  }
  _be->set_refresh_ui_slot(std::bind(&DbMySQLTableEditor::refresh_form_data, this));
  _be->set_partial_refresh_ui_slot(std::bind(&DbMySQLTableEditor::partial_refresh, this, std::placeholders::_1));

  delete old_be;

  do_refresh_form_data();

  if (_editor_notebook->get_nth_page(_editor_notebook->get_current_page()) == _main_page_widget) {
    Gtk::Entry *entry(0);
    xml()->get_widget("table_name", entry);
    focus_widget_when_idle(entry);
  }
  return true;
}

//------------------------------------------------------------------------------
bec::BaseEditor *DbMySQLTableEditor::get_be() {
  return _be;
}

//------------------------------------------------------------------------------

void DbMySQLTableEditor::set_table_name(const std::string &name) {
  _be->set_name(name);
  _signal_title_changed.emit(_be->get_title());
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::set_table_option_by_name(const std::string &name, const std::string &value) {
    if (!_be)
      return;
    
    if (name == "CHARACTER SET" and value == DEFAULT_CHARSET_CAPTION)
      _be->set_table_option_by_name(name, "");
    else if (name == "COLLATE" and value == DEFAULT_COLLATION_CAPTION)
      _be->set_table_option_by_name(name, "");
    else
      _be->set_table_option_by_name(name, value); 
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::create_table_page() {
  // Connect Table tab widgets
  bind_entry_and_be_setter("table_name", this, &DbMySQLTableEditor::set_table_name);

  Gtk::ComboBox *combo = nullptr;
  xml()->get_widget("engine_combo", combo);
  setup_combo_for_string_list(combo);
  fill_combo_from_string_list(combo, _be->get_engines_list());
  add_option_combo_change_handler(combo, "ENGINE", sigc::mem_fun(this, &DbMySQLTableEditor::set_table_option_by_name));

  Gtk::ComboBox *charset_combo = nullptr;
  xml()->get_widget("charset_combo", charset_combo);
  setup_combo_for_string_list(charset_combo);
  fill_combo_from_string_list(charset_combo, _be->get_charset_list());
  add_option_combo_change_handler(charset_combo, "CHARACTER SET",
                                  sigc::mem_fun(this, &DbMySQLTableEditor::set_table_option_by_name));
  add_option_combo_change_handler(charset_combo, "CHARACTER SET",
                                  sigc::mem_fun(this, &DbMySQLTableEditor::charset_combo_changed));
  
  Gtk::ComboBox *collation_combo = nullptr;
  xml()->get_widget("collation_combo", collation_combo);
  setup_combo_for_string_list(collation_combo);

  add_option_combo_change_handler(collation_combo, "COLLATE",
                                  sigc::mem_fun(this, &DbMySQLTableEditor::set_table_option_by_name));

  Gtk::TextView *tview = 0;
  xml()->get_widget("table_comments", tview);

  add_text_change_timer(tview, sigc::mem_fun(this, &DbMySQLTableEditor::set_comment));
}

//------------------------------------------------------------------------------

void DbMySQLTableEditor::charset_combo_changed(const std::string &name, const std::string &value) {
  if (name != "CHARACTER SET")
    return;
  
  Gtk::ComboBox *collation_combo;
  xml()->get_widget("collation_combo", collation_combo);
  
  std::vector<std::string> vec = _be->get_charset_collation_list(value);
  fill_combo_from_string_list(collation_combo, vec);
  
  set_selected_combo_item(collation_combo, DEFAULT_COLLATION_CAPTION);
}

//------------------------------------------------------------------------------

bool DbMySQLTableEditor::can_close() {
  return _be->can_close();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::set_comment(const std::string &cmt) {
  _be->set_comment(cmt);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::partial_refresh(const int what) {
  switch (what) {
    case ::bec::TableEditorBE::RefreshColumnCollation:
    case ::bec::TableEditorBE::RefreshColumnMoveUp:
    case ::bec::TableEditorBE::RefreshColumnMoveDown: {
      _columns_page->partial_refresh(what);
      break;
    }
    default: { g_message("DbMySQLTableEditor: unsupported partial refresh"); }
  }
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::refresh_table_page() {
  Gtk::Entry *entry(0);
  xml()->get_widget("table_name", entry);

  if (_be->get_name() != entry->get_text()) {
    entry->set_text(_be->get_name());
    _signal_title_changed.emit(_be->get_title());
  }

  Gtk::TextView *tview;
  xml()->get_widget("table_comments", tview);
  if (_be->get_comment() != tview->get_buffer()->get_text())
    tview->get_buffer()->set_text(_be->get_comment());

  Gtk::ComboBox *combo = 0;
  xml()->get_widget("engine_combo", combo);
  set_selected_combo_item(combo, _be->get_table_option_by_name("ENGINE"));

  std::string charset = _be->get_table_option_by_name("CHARACTER SET");
  std::string collate = _be->get_table_option_by_name("COLLATE");  
  
  xml()->get_widget("charset_combo", combo);
  set_selected_combo_item(combo, charset == "" ? DEFAULT_CHARSET_CAPTION : charset);

  xml()->get_widget("collation_combo", combo);
  fill_combo_from_string_list(combo, _be->get_charset_collation_list(charset));
  set_selected_combo_item(combo, collate == "" ? DEFAULT_COLLATION_CAPTION : collate);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditor::do_refresh_form_data() {
  refresh_table_page();

  _columns_page->refresh();
  _indexes_page->refresh();
  _fks_page->refresh();
  _triggers_page->refresh();
  _part_page->refresh();
  _opts_page->refresh();

  if (!is_editing_live_object()) {
    Gtk::Notebook *notebook;
    xml()->get_widget("mysql_editor_notebook", notebook);

    _privs_page->refresh();
  } else {
    Gtk::ComboBox *cbox = 0;
    xml()->get_widget("schema_combo", cbox);
    if (cbox) {
      fill_combo_from_string_list(cbox, _be->get_all_schema_names());
      cbox->set_active(0);
    }
  }
}

// TESTING
//--------------------------------------------------------------------------------
void DbMySQLTableEditor::refresh_indices() {
  _indexes_page->refresh();
}
//\TESTING

//--------------------------------------------------------------------------------
void DbMySQLTableEditor::page_changed(Gtk::Widget *page, guint page_num) {
  switch (page_num) {
    case 0: // general stuff
      break;

    case 1: // columns
      break;

    case 2: // indexes
      _indexes_page->refresh();
      break;

    case 3:                 // fks
      _fks_page->refresh(); // Note! Currently the call to refresh is mandatory to show/hide FK page content depending
                            // on engine capabilities
      break;

    case 4: // triggers
      _triggers_page->refresh();
      break;

    case 5: // partition
      _part_page->refresh();
      break;

    case 6: // options
      _opts_page->refresh();
      break;

    case 7: // inserts
      break;

    case 8: // privs
      _privs_page->refresh();
      break;
  }
}

//--------------------------------------------------------------------------------
bool DbMySQLTableEditor::event_from_table_name_entry(GdkEvent *event) {
  if (event->type == GDK_KEY_RELEASE &&
      (event->key.keyval == GDK_KEY_Return || event->key.keyval == GDK_KEY_KP_Enter)) {
    Gtk::Notebook *editor_window(0);
    xml()->get_widget("mysql_editor_notebook", editor_window);

    editor_window->set_current_page(1);
  }

  return false;
}

//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createDbMysqlTableEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new DbMySQLTableEditor(m, args));
}
};
