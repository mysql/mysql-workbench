/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_overview_physical.h"
#include "wb_overview_privileges.h"

#include "workbench/wb_context.h"
#include "wb_component_physical.h"

#include "grt/icon_manager.h"
#include "grt/clipboard.h"

#include "grts/structs.db.h"
#include "base/string_utilities.h"

/**
 * @file  wb_overview_privileges.cpp
 * @brief
 */

using namespace bec;
using namespace wb;
using namespace wb::internal;
using namespace base;

#define USERLIST_DEFAULT_HEIGHT 150

class PrivilegeObjectNode : public OverviewBE::ObjectNode {
  boost::signals2::connection _changed_conn;

public:
  PrivilegeObjectNode(GrtObjectRef o,
                      const std::function<void(const std::string &, const grt::ValueRef &)> &refresh_slot) {
    object = o;

    if (refresh_slot)
      _changed_conn = o->signal_changed()->connect(refresh_slot);
  }

  virtual ~PrivilegeObjectNode() {
    _changed_conn.disconnect();
  }

  std::function<void(WBComponentPhysical *)> remove;

  virtual void delete_object(WBContext *wb) {
    remove(wb->get_component<WBComponentPhysical>());
  }

  virtual bool is_deletable() {
    return true;
  }

  virtual void copy_object(WBContext *wb, bec::Clipboard *clip) {
    clip->clear();
    clip->append_data(grt::copy_object(object));
    clip->set_content_description(label);
  }

  virtual bool is_copyable() {
    return true;
  }

  virtual bool is_renameable() {
    return true;
  }
};

/*
 * MySQL
 *
 * [+] Privileges
 * ------------------------------------------------------------
 * Users
 * ...
 * Roles
 * ...
 *
 * [+] Synchronized Databases
 * -------------------------------------------------------------
 *
 *
 */

class UserListNode : public OverviewBE::ContainerNode {
private:
  std::string id;
  grt::ListRef<GrtNamedObject> _list;
  std::function<void(WBComponentPhysical *, db_UserRef)> _remove;
  PhysicalOverviewBE *_owner;

public:
  UserListNode(const std::string &name, const db_CatalogRef &catalog, grt::ListRef<GrtNamedObject> list,
               const std::function<void(WBComponentPhysical *, db_UserRef)> &remove, PhysicalOverviewBE *owner)
    : ContainerNode(OverviewBE::OItem), _list(list), _remove(remove), _owner(owner) {
    id = catalog->id() + "/" + name;
    label = name;
    type = OverviewBE::OSection;
    small_icon = 0;
    large_icon = 0;
    expanded = false;

    refresh_children();
  }

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
  void refresh(const std::string &member, const grt::ValueRef &) {
    if (member == "name")
      _owner->send_refresh_users();
  }
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

  virtual void refresh_children() {
    Node *add_item = 0;
    if (!children.empty()) {
      add_item = children.front();
      children.erase(children.begin());
    }
    clear_children();

    if (add_item)
      children.push_back(add_item);

    for (size_t c = _list.count(), i = 0; i < c; i++) {
      PrivilegeObjectNode *node = new PrivilegeObjectNode(
        _list[i], std::bind(&UserListNode::refresh, this, std::placeholders::_1, std::placeholders::_2));

      node->type = OverviewBE::OItem;
      node->label = _list[i]->name();
      node->small_icon = IconManager::get_instance()->get_icon_id(_list[i]->get_metaclass(), Icon16);
      node->large_icon = IconManager::get_instance()->get_icon_id(_list[i]->get_metaclass(), Icon48);
      node->remove = std::bind(_remove, std::placeholders::_1, db_UserRef::cast_from(_list[i]));

      children.push_back(node);
    }
  }

  virtual std::string get_unique_id() {
    return id;
  }
};

class RoleListNode : public OverviewBE::ContainerNode {
private:
  std::string id;
  grt::ListRef<GrtNamedObject> _list;
  std::function<void(WBComponentPhysical *, db_RoleRef)> _remove;
  PhysicalOverviewBE *_owner;

public:
  RoleListNode(const std::string &name, const db_CatalogRef &catalog, grt::ListRef<GrtNamedObject> list,
               const std::function<void(WBComponentPhysical *, db_RoleRef)> &remove, PhysicalOverviewBE *owner)
    : ContainerNode(OverviewBE::OItem), _list(list), _remove(remove), _owner(owner) {
    id = catalog->id() + "/" + name;
    label = name;
    type = OverviewBE::OSection;
    small_icon = 0;
    large_icon = 0;
    expanded = false;

    refresh_children();
  }

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
  void refresh(const std::string &member, const grt::ValueRef &) {
    if (member == "name")
      _owner->send_refresh_roles();
  }
#ifndef _MSC_VER
#pragma GCC diagnostic push
#endif

  virtual void refresh_children() {
    Node *add_item = 0;

    if (!children.empty()) {
      add_item = children.front();
      children.erase(children.begin());
    }
    clear_children();
    if (add_item)
      children.push_back(add_item);

    for (size_t c = _list.count(), i = 0; i < c; i++) {
      PrivilegeObjectNode *node = new PrivilegeObjectNode(
        _list[i], std::bind(&RoleListNode::refresh, this, std::placeholders::_1, std::placeholders::_2));

      node->type = OverviewBE::OItem;
      node->label = _list[i]->name();
      node->small_icon = IconManager::get_instance()->get_icon_id(_list[i]->get_metaclass(), Icon16);
      node->large_icon = IconManager::get_instance()->get_icon_id(_list[i]->get_metaclass(), Icon48);
      node->remove = std::bind(_remove, std::placeholders::_1, db_RoleRef::cast_from(_list[i]));

      children.push_back(node);
    }
  }

  virtual std::string get_unique_id() {
    return id;
  }
};

//----------------------------------------------------------------------

void wb::internal::PrivilegeInfoNode::paste_object(WBContext *wb, bec::Clipboard *clip) {
  std::list<grt::ObjectRef> objects(clip->get_data());
  db_CatalogRef catalog(db_CatalogRef::cast_from(object));

  for (std::list<grt::ObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
    if ((*iter).is_instance(db_User::static_class_name())) {
      db_UserRef dbuser(db_UserRef::cast_from(grt::copy_object(*iter)));

      grt::AutoUndo undo;

      if (grt::find_named_object_in_list(catalog->users(), dbuser->name()).is_valid())
        dbuser->name(grt::get_name_suggestion_for_list_object(catalog->users(), *dbuser->name() + "_copy"));

      catalog->users().insert(dbuser);
      undo.end(strfmt(_("Paste '%s'"), dbuser->name().c_str()));
    } else if ((*iter).is_instance(db_Role::static_class_name())) {
      db_RoleRef dbrole(db_RoleRef::cast_from(grt::copy_object(*iter)));

      grt::AutoUndo undo;

      if (grt::find_named_object_in_list(catalog->roles(), dbrole->name()).is_valid())
        dbrole->name(grt::get_name_suggestion_for_list_object(catalog->roles(), *dbrole->name() + "_copy"));

      catalog->roles().insert(dbrole);
      undo.end(strfmt(_("Paste '%s'"), dbrole->name().c_str()));
    }
  }
}

bool wb::internal::PrivilegeInfoNode::is_pasteable(bec::Clipboard *clip) {
  std::list<grt::ObjectRef> objects(clip->get_data());
  for (std::list<grt::ObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
    if (!(*iter).is_instance(db_User::static_class_name()) && !(*iter).is_instance(db_Role::static_class_name()))
      return false;
  }
  return !objects.empty();
}

bool wb::internal::PrivilegeInfoNode::add_new_user(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(
    wb->get_component<WBComponentPhysical>()->add_new_user(workbench_physical_ModelRef::cast_from(object->owner())));
  return true;
}

bool wb::internal::PrivilegeInfoNode::add_new_role(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(
    wb->get_component<WBComponentPhysical>()->add_new_role(workbench_physical_ModelRef::cast_from(object->owner())));
  return true;
}

wb::internal::PrivilegeInfoNode::PrivilegeInfoNode(const db_CatalogRef &catalog, PhysicalOverviewBE *owner)
  : ContainerNode(OverviewBE::OSection) {
  object = catalog;
  type = OverviewBE::ODivision;
  label = _("Schema Privileges");
  description = "Privileges";
  display_mode = OverviewBE::MSmallIcon;

  OverviewBE::AddObjectNode *add_node =
    new OverviewBE::AddObjectNode(std::bind(&PrivilegeInfoNode::add_new_user, this, std::placeholders::_1));
  add_node->label = _("Add User");
  add_node->type = OverviewBE::OItem;
  add_node->small_icon = IconManager::get_instance()->get_icon_id("db.User.$.png", Icon16, "add");
  add_node->large_icon = IconManager::get_instance()->get_icon_id("db.User.$.png", Icon48, "add");

  {
    UserListNode *node;
    node = new UserListNode("Users", catalog, grt::ListRef<GrtNamedObject>::cast_from(catalog->users()),
                            std::bind(&WBComponentPhysical::remove_user, std::placeholders::_1, std::placeholders::_2),
                            owner);
    children.push_back(node);
    node->children.insert(node->children.begin(), add_node);
  }

  add_node = new OverviewBE::AddObjectNode(std::bind(&PrivilegeInfoNode::add_new_role, this, std::placeholders::_1));
  add_node->label = _("Add Role");
  add_node->type = OverviewBE::OItem;
  add_node->small_icon = IconManager::get_instance()->get_icon_id("db.Role.$.png", Icon16, "add");
  add_node->large_icon = IconManager::get_instance()->get_icon_id("db.Role.$.png", Icon48, "add");

  {
    RoleListNode *node = new RoleListNode(
      "Roles", catalog, grt::ListRef<GrtNamedObject>::cast_from(catalog->roles()),
      std::bind(&WBComponentPhysical::remove_role, std::placeholders::_1, std::placeholders::_2), owner);
    children.push_back(node);

    node->children.insert(node->children.begin(), add_node);
  }
}
