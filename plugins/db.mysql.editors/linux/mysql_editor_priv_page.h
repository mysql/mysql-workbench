/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef __WB_MYSQL_EDITOR_INSERT_PAGE_H__
#define __WB_MYSQL_EDITOR_INSERT_PAGE_H__

#include "grtdb/editor_dbobject.h"
#include "grtdb/dbobject_roles.h"
#include "grtdb/role_tree_model.h"
#include <gtkmm/treemodel.h>
#include "widgets_auto_cleaner.h"
#include "grtdb/db_object_helpers.h"

class ListModelWrapper;
namespace Gtk {
  class TreeView;
  class ListStore;
  class Box;
  class Button;
  class Widget;
}

//==============================================================================
//
//==============================================================================
class DbMySQLEditorPrivPage : private WidgetsAutoCleaner {
public:
  DbMySQLEditorPrivPage(::bec::DBObjectEditorBE *be);
  ~DbMySQLEditorPrivPage();
  void refresh();

  void switch_be(bec::DBObjectEditorBE *be);

  Gtk::Box &page() const {
    return *_holder;
  }

private:
  void assign_privilege_handler();
  void assign_privilege(const Gtk::TreeModel::iterator &iter);
  void remove_privilege_handler();
  void remove_privilege(const Gtk::TreeModel::Path &path);

  void role_selected();

  ::bec::DBObjectEditorBE *_be;

  ::bec::ObjectRoleListBE *_object_roles_list_be;
  ::bec::RoleTreeBE *_role_tree_be;
  ::bec::ObjectPrivilegeListBE *_object_privilege_list_be;

  Glib::RefPtr<ListModelWrapper> _roles_model;
  Glib::RefPtr<ListModelWrapper> _all_roles_model;
  Glib::RefPtr<ListModelWrapper> _assigned_priv_model;

  Gtk::Box *_holder;
  Gtk::TreeView *_roles_tv;
  Gtk::TreeView *_assigned_priv_tv;
  Gtk::Button *_add_button;
  Gtk::Button *_remove_button;
  Gtk::TreeView *_all_roles_tv;
  std::vector<Gtk::TreePath> _selected;

  bool _reentrant;
};

#endif
