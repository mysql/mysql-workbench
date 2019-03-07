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

#include "plugin_editor_base.h"
#include "../backend/mysql_routine_editor.h"
#include "grtdb/db_object_helpers.h"
#include "treemodel_wrapper.h"
#include "mysql_editor_priv_page.h"
#include "gtk/mforms_gtk.h"

//==============================================================================
//
//==============================================================================
class DbMySQLRoutineEditor : public PluginEditorBase {
  MySQLRoutineEditorBE *_be;
  DbMySQLEditorPrivPage *_privs_page;

  virtual bec::BaseEditor *get_be();

  virtual bool can_close() {
    return _be->can_close();
  }

public:
  DbMySQLRoutineEditor(grt::Module *m, const grt::BaseListRef &args);

  virtual ~DbMySQLRoutineEditor();
  virtual void do_refresh_form_data();

  virtual bool switch_edited_object(const grt::BaseListRef &args);

  bool comment_lost_focus(GdkEventFocus *ev, Gtk::TextView *view);
};

DbMySQLRoutineEditor::DbMySQLRoutineEditor(grt::Module *m, const grt::BaseListRef &args)
  : PluginEditorBase(m, args, "modules/data/editor_routine.glade"),
    _be(new MySQLRoutineEditorBE(db_mysql_RoutineRef::cast_from(args[0]))) {
  xml()->get_widget("mysql_routine_editor_notebook", _editor_notebook);

  Gtk::Image *image;
  xml()->get_widget("routine_editor_image", image);
  image->set(ImageCache::get_instance()->image_from_filename("db.Routine.editor.48x48.png", false));
  xml()->get_widget("routine_editor_image2", image);
  image->set(ImageCache::get_instance()->image_from_filename("db.Routine.editor.48x48.png", false));

  _be->set_refresh_ui_slot(std::bind(&DbMySQLRoutineEditor::refresh_form_data, this));

  _editor_notebook->reparent(*this);
  _editor_notebook->show();

  Gtk::Box *ddl_win;
  xml()->get_widget("routine_ddl", ddl_win);
  embed_code_editor(_be->get_sql_editor()->get_container(), ddl_win);
  _be->load_routine_sql();

  if (!is_editing_live_object()) {
    _privs_page = new DbMySQLEditorPrivPage(_be);
    _editor_notebook->append_page(_privs_page->page(), "Privileges");

    Gtk::TextView *tview(0);
    xml()->get_widget("comment", tview);
    tview->get_buffer()->set_text(_be->get_comment());

    tview->signal_focus_out_event().connect(
      sigc::bind(sigc::mem_fun(this, &DbMySQLRoutineEditor::comment_lost_focus), tview), false);
  } else {
    _privs_page = NULL;
    _editor_notebook->remove_page(*_editor_notebook->get_nth_page(1));
  }

  refresh_form_data();

  _be->reset_editor_undo_stack();

  show_all();
}

//------------------------------------------------------------------------------
DbMySQLRoutineEditor::~DbMySQLRoutineEditor() {
  delete _privs_page;
  delete _be;
}

bool DbMySQLRoutineEditor::comment_lost_focus(GdkEventFocus *ev, Gtk::TextView *view) {
  if (_be) {
    _be->set_comment(view->get_buffer()->get_text());
  }
  return false;
}

//------------------------------------------------------------------------------
bool DbMySQLRoutineEditor::switch_edited_object(const grt::BaseListRef &args) {
  Gtk::Box *ddl_win;
  xml()->get_widget("routine_ddl", ddl_win);

  delete _be;

  _be = new MySQLRoutineEditorBE(db_mysql_RoutineRef::cast_from(args[0]));

  embed_code_editor(_be->get_sql_editor()->get_container(), ddl_win);
  _be->load_routine_sql();

  if (!_be->is_editing_live_object()) {
    Gtk::TextView *tview(0);
    xml()->get_widget("comment", tview);
    tview->get_buffer()->set_text(_be->get_comment());
  }

  _be->set_refresh_ui_slot(std::bind(&DbMySQLRoutineEditor::refresh_form_data, this));

  if (!is_editing_live_object())
    _privs_page->switch_be(_be);

  refresh_form_data();

  return true;
}

//------------------------------------------------------------------------------

bec::BaseEditor *DbMySQLRoutineEditor::get_be() {
  return _be;
}

//------------------------------------------------------------------------------
void DbMySQLRoutineEditor::do_refresh_form_data() {
  Gtk::Entry *entry(0);
  xml()->get_widget("routine_name", entry);
  if (entry->get_text() != _be->get_name()) {
    entry->set_text(_be->get_name());
    _signal_title_changed.emit(_be->get_title());
  }

  _be->load_routine_sql();

  if (!is_editing_live_object())
    _privs_page->refresh();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createDbMysqlRoutineEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new DbMySQLRoutineEditor(m, args));
}
};
