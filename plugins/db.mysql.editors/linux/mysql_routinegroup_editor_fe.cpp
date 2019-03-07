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
#include "../backend/mysql_routinegroup_editor.h"
#include "grtdb/db_object_helpers.h"
#include "treemodel_wrapper.h"
#include "gtk/mforms_gtk.h"
#include <sstream>
#include "text_list_columns_model.h"
#include "workbench/wb_context.h"

//==============================================================================
//
//==============================================================================
class DbMySQLRoutineGroupEditor : public PluginEditorBase {
  MySQLRoutineGroupEditorBE *_be;
  Glib::RefPtr<Gtk::ListStore> _routines_model;
  Gtk::TreeView *_rg_list;
  TextListColumnsModel *_routines_columns;
  Gtk::Menu _context_menu;

  virtual bec::BaseEditor *get_be();

  void activate_row(const Gtk::TreePath &path, Gtk::TreeViewColumn *column);
  bool process_event(GdkEvent *event);
  void menu_action_on_node(const std::string &item_name, const Gtk::TreePath path);

  void set_group_name(const std::string &);

  void set_comment(const std::string &comm) {
    _be->set_comment(comm);
  }
  virtual bool can_close() {
    return _be->can_close();
  }

public:
  DbMySQLRoutineGroupEditor(grt::Module *m, const grt::BaseListRef &args);

  virtual ~DbMySQLRoutineGroupEditor();
  virtual void do_refresh_form_data();

  void on_routine_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                       const Gtk::SelectionData &selection_data, guint info, guint time);

  bool switch_edited_object(const grt::BaseListRef &args);
};

//------------------------------------------------------------------------------
DbMySQLRoutineGroupEditor::DbMySQLRoutineGroupEditor(grt::Module *m, const grt::BaseListRef &args)
  : PluginEditorBase(m, args, "modules/data/editor_rg.glade"),
    _be(new MySQLRoutineGroupEditorBE(db_mysql_RoutineGroupRef::cast_from(args[0]))),
    _routines_model(model_from_string_list(std::vector<std::string>(), &_routines_columns)) {
  xml()->get_widget("mysql_rg_editor_notebook", _editor_notebook);

  Gtk::Image *image;
  xml()->get_widget("rg_image", image);
  image->set(ImageCache::get_instance()->image_from_filename("db.RoutineGroup.editor.48x48.png", false));

  _be->set_refresh_ui_slot(std::bind(&DbMySQLRoutineGroupEditor::refresh_form_data, this));

  _editor_notebook->reparent(*this);
  _editor_notebook->show();

  bind_entry_and_be_setter("rg_name", this, &DbMySQLRoutineGroupEditor::set_group_name);

  Gtk::TextView *tv;
  xml()->get_widget("rg_comment", tv);
  add_text_change_timer(tv, sigc::mem_fun(this, &DbMySQLRoutineGroupEditor::set_comment));

  Gtk::Box *code_win;
  xml()->get_widget("rg_code_holder", code_win);
  embed_code_editor(_be->get_sql_editor()->get_container(), code_win);
  _be->load_routines_sql();

  refresh_form_data();

  xml()->get_widget("rg_list", _rg_list);

  _rg_list->set_model(_routines_model);
  _rg_list->append_column("Routine", _routines_columns->item);
  _rg_list->set_headers_visible(false);
  _rg_list->signal_row_activated().connect(sigc::mem_fun(this, &DbMySQLRoutineGroupEditor::activate_row));

  // Setup DnD
  std::vector<Gtk::TargetEntry> targets;

  targets.push_back(Gtk::TargetEntry(WB_DBOBJECT_DRAG_TYPE, Gtk::TARGET_SAME_APP));
  _rg_list->drag_dest_set(targets, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_COPY);
  _rg_list->signal_drag_data_received().connect(sigc::mem_fun(this, &DbMySQLRoutineGroupEditor::on_routine_drop));
  _rg_list->signal_event().connect(sigc::mem_fun(*this, &DbMySQLRoutineGroupEditor::process_event));

  show_all();
}

//------------------------------------------------------------------------------
DbMySQLRoutineGroupEditor::~DbMySQLRoutineGroupEditor() {
  delete _be;
}

//------------------------------------------------------------------------------
void DbMySQLRoutineGroupEditor::activate_row(const Gtk::TreePath &path, Gtk::TreeViewColumn *column) {
  _be->open_editor_for_routine_at_index(path.front());
}

//------------------------------------------------------------------------------
bool DbMySQLRoutineGroupEditor::switch_edited_object(const grt::BaseListRef &args) {
  Gtk::Box *code_win;
  xml()->get_widget("rg_code_holder", code_win);
  delete _be;

  _be = new MySQLRoutineGroupEditorBE(db_mysql_RoutineGroupRef::cast_from(args[0]));
  embed_code_editor(_be->get_sql_editor()->get_container(), code_win);
  _be->load_routines_sql();
  _be->set_refresh_ui_slot(std::bind(&DbMySQLRoutineGroupEditor::refresh_form_data, this));

  refresh_form_data();

  return true;
}

//------------------------------------------------------------------------------

void DbMySQLRoutineGroupEditor::set_group_name(const std::string &name) {
  _be->set_name(name);
  _signal_title_changed.emit(_be->get_title());
}

//------------------------------------------------------------------------------
bec::BaseEditor *DbMySQLRoutineGroupEditor::get_be() {
  return _be;
}

//------------------------------------------------------------------------------
void DbMySQLRoutineGroupEditor::do_refresh_form_data() {
  Gtk::Entry *entry(0);
  xml()->get_widget("rg_name", entry);
  if (entry->get_text() != _be->get_name()) {
    entry->set_text(_be->get_name());
    _signal_title_changed.emit(_be->get_title());
  }
  Gtk::TextView *tv;
  xml()->get_widget("rg_comment", tv);
  tv->get_buffer()->set_text(_be->get_comment());

  _be->load_routines_sql();

  recreate_model_from_string_list(_routines_model, _be->get_routines_names());
}

//------------------------------------------------------------------------------
bool DbMySQLRoutineGroupEditor::process_event(GdkEvent *event) {
  if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
    Gtk::TreeModel::Path path;
    Gtk::TreeView::Column *column(0);
    int cell_x(-1);
    int cell_y(-1);

    if (_rg_list->get_path_at_pos((int)event->button.x, (int)event->button.y, path, column, cell_x, cell_y)) {
      bec::MenuItemList menuitems;
      bec::MenuItem item;
      item.caption = "Remove routine from the group";
      item.internalName = "remove_routine_from_the_group";
      item.accessibilityName = "Remove Routine From Group";
      menuitems.push_back(item);

      run_popup_menu(menuitems, event->button.time,
                     sigc::bind(sigc::mem_fun(this, &DbMySQLRoutineGroupEditor::menu_action_on_node), path),
                     &_context_menu);
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void DbMySQLRoutineGroupEditor::menu_action_on_node(const std::string &item_name, const Gtk::TreePath path) {
  if (item_name == "remove_routine_from_the_group") {
    const std::string name = (*(_routines_model->get_iter(path)))[_routines_columns->item];
    _be->delete_routine_with_name(name);
    do_refresh_form_data();
  }
}

//------------------------------------------------------------------------------
void DbMySQLRoutineGroupEditor::on_routine_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                                                const Gtk::SelectionData &selection_data, guint info, guint time) {
  bool dnd_status = false;

  if (selection_data.get_target() == WB_DBOBJECT_DRAG_TYPE) {
    std::list<db_DatabaseObjectRef> objects;

    const std::string selection = selection_data.get_data_as_string();

    objects = bec::CatalogHelper::dragdata_to_dbobject_list(_be->get_catalog(), selection);

    for (std::list<db_DatabaseObjectRef>::const_iterator obj = objects.begin(); obj != objects.end(); ++obj) {
      if (obj->is_instance<db_mysql_Routine>()) {
        db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(*obj);
        if (routine.is_valid())
          _be->append_routine_with_id(routine.id());
      }
    }

    recreate_model_from_string_list(_routines_model, _be->get_routines_names());

    dnd_status = true;
  }
  context->drag_finish(dnd_status, false, time);
}

//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createDbMysqlRoutineGroupEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new DbMySQLRoutineGroupEditor(m, args));
}
};
