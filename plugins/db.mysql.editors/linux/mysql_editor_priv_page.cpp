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

#include "mysql_editor_priv_page.h"

#include "treemodel_wrapper.h"
#include "gtk_helpers.h"
#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>
#include "base/log.h"
DEFAULT_LOG_DOMAIN("Editor")

//------------------------------------------------------------------------------
DbMySQLEditorPrivPage::DbMySQLEditorPrivPage(::bec::DBObjectEditorBE *be)
  : _be(be),
    _object_roles_list_be(new bec::ObjectRoleListBE(_be, get_rdbms_for_db_object(be->get_dbobject()))),
    _role_tree_be(new bec::RoleTreeBE(_be->get_catalog())),
    _object_privilege_list_be(0),
    _reentrant(false) {
  _holder = new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8);

  Gtk::ScrolledWindow *scrolled = new Gtk::ScrolledWindow();
  scrolled->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
  _holder->pack_start(*scrolled, true, true);
  _roles_tv = new Gtk::TreeView();
  scrolled->add(*_roles_tv);
  scrolled->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  manage(scrolled); // add to auto-clean on exit list

  scrolled = new Gtk::ScrolledWindow();
  scrolled->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
  _holder->pack_start(*scrolled, true, true);
  _assigned_priv_tv = new Gtk::TreeView();
  scrolled->add(*_assigned_priv_tv);
  scrolled->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  manage(scrolled); // add to auto-clean on exit list

  Gtk::Box *vbox = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
  manage(vbox); // add to auto-clean on exit list

  _add_button = new Gtk::Button(" < ");
  _add_button->set_name("Add");
  vbox->pack_start(*_add_button, false, true, 4);
  _add_button->signal_clicked().connect(sigc::mem_fun(this, &DbMySQLEditorPrivPage::assign_privilege_handler));
  _remove_button = new Gtk::Button(" > ");
  _remove_button->set_name("Remove");
  vbox->pack_start(*_remove_button, false, true, 4);
  _remove_button->signal_clicked().connect(sigc::mem_fun(this, &DbMySQLEditorPrivPage::remove_privilege_handler));

  _holder->pack_start(*vbox, false, true);

  scrolled = new Gtk::ScrolledWindow();
  manage(scrolled); // add to auto-clean on exit list
  scrolled->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
  _holder->pack_start(*scrolled, true, true);
  _all_roles_tv = new Gtk::TreeView;
  scrolled->add(*_all_roles_tv);
  scrolled->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

  _all_roles_model = ListModelWrapper::create(_role_tree_be, _all_roles_tv, "PrivPageAllRoles");
  _all_roles_model->model().append_string_column(::bec::RoleTreeBE::Name, "All Roles", EDITABLE, NO_ICON);

  _roles_model = ListModelWrapper::create(_object_roles_list_be, _roles_tv, "PrivPageRoles");
  _roles_model->model().append_string_column(::bec::ObjectRoleListBE::Name, "Roles", EDITABLE, NO_ICON);

  _all_roles_tv->set_model(_all_roles_model);
  _roles_tv->set_model(_roles_model);

  _roles_tv->signal_cursor_changed().connect(sigc::mem_fun(*this, &DbMySQLEditorPrivPage::role_selected));

  _holder->show_all_children();
}

//------------------------------------------------------------------------------
DbMySQLEditorPrivPage::~DbMySQLEditorPrivPage() {
  delete _holder;
  delete _roles_tv;
  delete _assigned_priv_tv;
  delete _add_button;
  delete _remove_button;
  delete _all_roles_tv;
  delete _object_roles_list_be;
  delete _role_tree_be;
}

//------------------------------------------------------------------------------
void DbMySQLEditorPrivPage::refresh() {
  _all_roles_tv->unset_model();
  _roles_tv->unset_model();

  _object_roles_list_be->refresh();
  _role_tree_be->refresh();

  _all_roles_tv->set_model(_all_roles_model);
  _roles_tv->set_model(_roles_model);
}

//------------------------------------------------------------------------------
void DbMySQLEditorPrivPage::role_selected() {
  if (_reentrant)
    return;
  _reentrant = true;
  Gtk::TreeModel::iterator iter = _roles_tv->get_selection()->get_selected();
  bec::NodeId selected_role = _roles_model->node_for_iter(iter);

  _selected = _roles_tv->get_selection()->get_selected_rows();

  if (selected_role.is_valid()) {
    _object_roles_list_be->select_role(selected_role);
    _object_roles_list_be->refresh();

    _assigned_priv_tv->remove_all_columns();
    _assigned_priv_tv->unset_model();

    _object_privilege_list_be = _object_roles_list_be->get_privilege_list();

    _assigned_priv_model =
      ListModelWrapper::create(_object_privilege_list_be, _assigned_priv_tv, "PrivPageAssignedPrivs");

    _assigned_priv_model->model().append_check_column(::bec::ObjectPrivilegeListBE::Enabled, "", EDITABLE);
    _assigned_priv_model->model().append_string_column(::bec::ObjectPrivilegeListBE::Name, "", RO, NO_ICON);

    _assigned_priv_tv->set_model(_assigned_priv_model);
  } else {
    _assigned_priv_tv->remove_all_columns();
    _assigned_priv_tv->unset_model();

    _object_roles_list_be->select_role(bec::NodeId());
    refresh();
  }
  _reentrant = false;
}

//------------------------------------------------------------------------------
void DbMySQLEditorPrivPage::assign_privilege(const Gtk::TreeModel::iterator &iter) {
  ::bec::NodeId node = _all_roles_model->node_for_iter(iter);
  if (node.is_valid()) {
    _object_roles_list_be->add_role_for_privileges(_role_tree_be->get_role_with_id(node));
  }
}

//------------------------------------------------------------------------------
void DbMySQLEditorPrivPage::assign_privilege_handler() {
  Glib::RefPtr<Gtk::TreeSelection> selection = _all_roles_tv->get_selection();
  selection->selected_foreach_iter(sigc::mem_fun(this, &DbMySQLEditorPrivPage::assign_privilege));
  refresh();
}

//------------------------------------------------------------------------------
void DbMySQLEditorPrivPage::remove_privilege(const Gtk::TreeModel::Path &path) {
  bec::NodeId node(_roles_model->get_node_for_path(path));
  if (node.is_valid())
    _object_roles_list_be->remove_role_from_privileges(_role_tree_be->get_role_with_id(node));
}

//------------------------------------------------------------------------------
void DbMySQLEditorPrivPage::remove_privilege_handler() {
  std::for_each(_selected.begin(), _selected.end(), sigc::mem_fun(this, &DbMySQLEditorPrivPage::remove_privilege));

  refresh();

  role_selected();
}

//------------------------------------------------------------------------------
void DbMySQLEditorPrivPage::switch_be(bec::DBObjectEditorBE *be) {
  logDebug("Switching BE for table editor privileges page\n");
  _be = be;

  ::bec::ObjectRoleListBE *object_roles_list_be = _object_roles_list_be;
  ::bec::RoleTreeBE *role_tree_be = _role_tree_be;

  _object_roles_list_be = new bec::ObjectRoleListBE(_be, get_rdbms_for_db_object(be->get_dbobject()));
  _role_tree_be = new bec::RoleTreeBE(_be->get_catalog());

  _assigned_priv_tv->remove_all_columns();
  _assigned_priv_tv->unset_model();
  _object_privilege_list_be = 0;

  _roles_tv->remove_all_columns();
  _all_roles_tv->remove_all_columns();

  _all_roles_model = ListModelWrapper::create(_role_tree_be, _all_roles_tv, "PrivPageAllRoles");
  _all_roles_model->model().append_string_column(::bec::RoleTreeBE::Name, "All Roles", EDITABLE, NO_ICON);

  _roles_model = ListModelWrapper::create(_object_roles_list_be, _roles_tv, "PrivPageRoles");
  _roles_model->model().append_string_column(::bec::ObjectRoleListBE::Name, "Roles", EDITABLE, NO_ICON);

  _all_roles_tv->set_model(_all_roles_model);
  _roles_tv->set_model(_roles_model);

  delete object_roles_list_be;
  delete role_tree_be;
}
