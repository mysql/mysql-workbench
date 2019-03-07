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
#include "grtdb/editor_user_role.h"
#include "grtdb/db_object_helpers.h"
#include "treemodel_wrapper.h"
#include <gtkmm/notebook.h>
#include <gtkmm/image.h>
#include <gtkmm/combobox.h>
#include <gtkmm/builder.h>
#include "text_list_columns_model.h"
#include "workbench/wb_context.h"

//==============================================================================
class DbMySQLRoleEditor : public PluginEditorBase {
  bec::RoleEditorBE *_be;
  Glib::RefPtr<ListModelWrapper> _role_tree_model;
  Glib::RefPtr<ListModelWrapper> _role_objects_model;
  Glib::RefPtr<ListModelWrapper> _role_privs_model;
  Gtk::TreeView *_role_tree_tv;
  Gtk::TreeView *_role_objects_tv;
  Gtk::TreeView *_role_privs_tv;
  Gtk::ComboBox *_parent_combo;
  TextListColumnsModel _parent_combo_model;
  bool _refreshing;

  virtual bec::BaseEditor *get_be();

  void objects_tv_cursor_changed();

  void refresh_objects();
  void refresh_privileges();

  void check_all_privileges();
  void clear_privileges();
  void change_parent();

  void set_name(const std::string &nm) {
    _be->set_name(nm);
    _signal_title_changed.emit(_be->get_title());
  }

public:
  DbMySQLRoleEditor(grt::Module *m, const grt::BaseListRef &args);
  ~DbMySQLRoleEditor() {
    delete _be;
  }

  virtual void do_refresh_form_data();
  void onObjectDrop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                      const Gtk::SelectionData &selection_data, guint info, guint time);
  bool onKeyPressRoleObjects(GdkEventKey *ev);
  bool onKeyPressRolePrivs(GdkEventKey *ev);

  virtual bool switch_edited_object(const grt::BaseListRef &args);
};

DbMySQLRoleEditor::DbMySQLRoleEditor(grt::Module *m, const grt::BaseListRef &args)
  : PluginEditorBase(m, args, "modules/data/editor_role.glade"),
    _be(new bec::RoleEditorBE(db_RoleRef::cast_from(args[0]), get_rdbms_for_db_object(args[0]))),
    _refreshing(false) {
  xml()->get_widget("mysql_role_editor_notebook", _editor_notebook);

  _be->set_refresh_ui_slot(std::bind(&DbMySQLRoleEditor::refresh_form_data, this));

  _editor_notebook->reparent(*this);
  _editor_notebook->show();

  {
    Gtk::Image *image;
    xml()->get_widget("image1", image);

    image->set(ImageCache::get_instance()->image_from_filename("db.Role.editor.48x48.png", false));
  }

  bind_entry_and_be_setter("name_entry", this, &DbMySQLRoleEditor::set_name);

  xml()->get_widget("parent_combo", _parent_combo);
  setup_combo_for_string_list(_parent_combo);

  xml()->get_widget("roles_tv", _role_tree_tv);
  xml()->get_widget("objects_tv", _role_objects_tv);
  xml()->get_widget("privs_tv", _role_privs_tv);

  Gtk::Button *button;
  xml()->get_widget("clear_privs_button", button);
  button->signal_clicked().connect(sigc::mem_fun(this, &DbMySQLRoleEditor::clear_privileges));

  xml()->get_widget("checkall_privs_button", button);
  button->signal_clicked().connect(sigc::mem_fun(this, &DbMySQLRoleEditor::check_all_privileges));

  _role_tree_model = TreeModelWrapper::create(_be->get_role_tree(), _role_tree_tv, "RoleTree");
  _role_objects_model = ListModelWrapper::create(_be->get_object_list(), _role_objects_tv, "RoleObjectsTree");
  _role_privs_model = ListModelWrapper::create(_be->get_privilege_list(), _role_privs_tv, "RolePrivsTree");

  _role_tree_tv->set_model(_role_tree_model);
  _role_objects_tv->set_model(_role_objects_model);
  _role_privs_tv->set_model(_role_privs_model);

  _role_tree_model->model().append_string_column(bec::RoleTreeBE::Name, _("Role Hierarchy"), RO, NO_ICON);

  _role_privs_model->model().append_check_column(bec::RolePrivilegeListBE::Enabled, "", EDITABLE, TOGGLE_BY_GTKMM);
  _role_privs_model->model().append_string_column(bec::RolePrivilegeListBE::Name, _("Privileges for Selected Object"),
                                                  RO, NO_ICON);

  _role_objects_model->model().append_string_column(bec::RoleObjectListBE::Name, _("Objects"), RO, WITH_ICON);

  show_all();

  refresh_form_data();

  _parent_combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLRoleEditor::change_parent));

  _role_objects_tv->signal_cursor_changed().connect(sigc::mem_fun(this, &DbMySQLRoleEditor::objects_tv_cursor_changed));

  // Setup DnD
  std::vector<Gtk::TargetEntry> targets;
  targets.push_back(Gtk::TargetEntry(WB_DBOBJECT_DRAG_TYPE, Gtk::TARGET_SAME_APP));
  _role_objects_tv->drag_dest_set(targets, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_COPY);
  Glib::RefPtr<Gtk::TargetList> target_list = Glib::RefPtr<Gtk::TargetList>(Gtk::TargetList::create(targets));
  _role_objects_tv->drag_dest_set_target_list(target_list);  // need to explicitly specify target list, cause the one, set by drag_dest_set is not working :/
  _role_objects_tv->signal_drag_data_received().connect(sigc::mem_fun(this, &DbMySQLRoleEditor::onObjectDrop));
  _role_objects_tv->signal_key_release_event().connect(sigc::mem_fun(this, &DbMySQLRoleEditor::onKeyPressRoleObjects), false);
  _role_privs_tv->signal_key_release_event().connect(sigc::mem_fun(this, &DbMySQLRoleEditor::onKeyPressRolePrivs), false);

}

//------------------------------------------------------------------------------
bool DbMySQLRoleEditor::switch_edited_object(const grt::BaseListRef &args) {
  bec::RoleEditorBE *old_be = _be;

  _be = new bec::RoleEditorBE(db_RoleRef::cast_from(args[0]), get_rdbms_for_db_object(args[0]));

  _be->set_refresh_ui_slot(std::bind(&DbMySQLRoleEditor::refresh_form_data, this));

  _role_tree_model = TreeModelWrapper::create(_be->get_role_tree(), _role_tree_tv, "RoleTree");
  _role_objects_model = ListModelWrapper::create(_be->get_object_list(), _role_objects_tv, "RoleObjectsTree");
  _role_privs_model = ListModelWrapper::create(_be->get_privilege_list(), _role_privs_tv, "RolePrivsTree");

  _role_tree_tv->set_model(_role_tree_model);
  _role_objects_tv->set_model(_role_objects_model);
  _role_privs_tv->set_model(_role_privs_model);

  _role_tree_tv->remove_all_columns();
  _role_objects_tv->remove_all_columns();
  _role_privs_tv->remove_all_columns();

  _role_tree_model->model().append_string_column(bec::RoleTreeBE::Name, _("Role Hierarchy"), RO, NO_ICON);

  _role_privs_model->model().append_check_column(bec::RolePrivilegeListBE::Enabled, "", EDITABLE, TOGGLE_BY_GTKMM);
  _role_privs_model->model().append_string_column(bec::RolePrivilegeListBE::Name, _("Privileges for Selected Object"),
                                                  RO, NO_ICON);

  _role_objects_model->model().append_string_column(bec::RoleObjectListBE::Name, _("Objects"), RO, WITH_ICON);

  refresh_form_data();

  delete old_be;

  return true;
}

//------------------------------------------------------------------------------
bec::BaseEditor *DbMySQLRoleEditor::get_be() {
  return _be;
}

//------------------------------------------------------------------------------
void DbMySQLRoleEditor::refresh_objects() {
  _role_objects_tv->unset_model();
  _role_objects_model->refresh();
  _role_objects_tv->set_model(_role_objects_model);
}

//------------------------------------------------------------------------------
void DbMySQLRoleEditor::refresh_privileges() {
  _role_privs_tv->unset_model();
  _role_privs_model->refresh();
  _role_privs_tv->set_model(_role_privs_model);
}

//------------------------------------------------------------------------------
void DbMySQLRoleEditor::check_all_privileges() {
  _be->get_privilege_list()->add_all();
  refresh_privileges();
}

//------------------------------------------------------------------------------
void DbMySQLRoleEditor::clear_privileges() {
  _be->get_privilege_list()->remove_all();
  refresh_privileges();
}

//------------------------------------------------------------------------------
void DbMySQLRoleEditor::change_parent() {
  if (_refreshing)
    return;
  std::string old_parent = _be->get_parent_role();

  if (_parent_combo->get_active()) {
    Gtk::TreeRow row = *_parent_combo->get_active();
    _be->set_parent_role(row[_parent_combo_model.item]);
  } else
    _be->set_parent_role("");
  if (old_parent != _be->get_parent_role())
    do_refresh_form_data();
}

//------------------------------------------------------------------------------
void DbMySQLRoleEditor::do_refresh_form_data() {
  Gtk::Entry *entry;

  xml()->get_widget("name_entry", entry);
  entry->set_text(_be->get_name());

  _signal_title_changed.emit(_be->get_title());

  std::vector<std::string> c = _be->get_role_list();
  _refreshing = true;
  _parent_combo->set_model(model_from_string_list(c, &_parent_combo_model));
  _parent_combo->set_row_span_column(0);

  std::vector<std::string>::const_iterator i = std::find(c.begin(), c.end(), _be->get_parent_role());
  if (i != c.end())
    _parent_combo->set_active(i - c.begin());
  else
    _parent_combo->set_active(0);

  _role_tree_tv->unset_model();
  _be->get_role_tree()->refresh();
  _role_tree_model->refresh();
  _role_tree_tv->set_model(_role_tree_model);
  _role_tree_tv->expand_all();

  refresh_objects();
  refresh_privileges();
  _refreshing = false;
}

//------------------------------------------------------------------------------

void DbMySQLRoleEditor::onObjectDrop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                                       const Gtk::SelectionData &selection_data, guint info, guint time) {
  bool dnd_status = false;
  if (selection_data.get_target() == WB_DBOBJECT_DRAG_TYPE) {
    if (selection_data.get_length() > 0) {
      std::list<db_DatabaseObjectRef> objects;
      db_CatalogRef catalog(db_CatalogRef::cast_from(_be->get_role()->owner()));

      objects = bec::CatalogHelper::dragdata_to_dbobject_list(catalog, selection_data.get_data_as_string());

      for (std::list<db_DatabaseObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
        _be->add_object(*iter);
    }

    dnd_status = true;
    do_refresh_form_data();
  }

  context->drag_finish(dnd_status, false, time);
}

//------------------------------------------------------------------------------

bool DbMySQLRoleEditor::onKeyPressRoleObjects(GdkEventKey *ev) {
  if (ev->keyval == GDK_KEY_Delete) {
    auto list = _role_objects_model->get_selection();
    for (const auto &node: list)
      _be->remove_object(node);
  }

  return false;
}

//------------------------------------------------------------------------------

bool DbMySQLRoleEditor::onKeyPressRolePrivs(GdkEventKey *ev) {
  if (ev->keyval == GDK_KEY_space) {
    auto list = _role_privs_model->get_selection();
    for (const auto &node: list) {
      ssize_t val;
      _be->get_privilege_list()->get_field(node, bec::RolePrivilegeListBE::Enabled, val);
      if (val == 1)
        val = 0;
      else
        val = 1;
      _be->get_privilege_list()->set_field(node, bec::RolePrivilegeListBE::Enabled, val);
    }
  }

  return false;
}

//------------------------------------------------------------------------------

void DbMySQLRoleEditor::objects_tv_cursor_changed() {
  Gtk::TreeModel::iterator iter = _role_objects_tv->get_selection()->get_selected();
  bec::NodeId obj_nodeid = _role_objects_model->node_for_iter(iter);

  if (obj_nodeid.is_valid()) {
    _be->get_object_list()->set_selected_node(obj_nodeid);
    refresh_privileges();
  }
}

//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createDbMysqlRoleEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new DbMySQLRoleEditor(m, args));
}
};
