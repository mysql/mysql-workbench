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

#include "editor_user_role.h"
#include "db_object_helpers.h"

#include "base/string_utilities.h"

using namespace grt;
using namespace bec;
using namespace base;

RolePrivilegeListBE::RolePrivilegeListBE(RoleEditorBE *owner) : _owner(owner) {
}

size_t RolePrivilegeListBE::count() {
  if (_privileges.is_valid())
    return _privileges.count();
  return 0;
}

void RolePrivilegeListBE::refresh() {
  _role_privilege = _owner->get_object_list()->get_selected_object_info();

  _privileges = grt::StringListRef();
  if (_role_privilege.is_valid()) {
    grt::ListRef<db_mgmt_PrivilegeMapping> mappings(_owner->get_rdbms()->privilegeNames());

    for (size_t c = mappings.count(), i = 0; i < c; i++) {
      if (_role_privilege->databaseObject().is_valid()) {
        if (_role_privilege->databaseObject().is_instance(mappings[i]->structName())) {
          _privileges = mappings[i]->privileges();
          break;
        }
      } else if (!_role_privilege->databaseObjectType().empty()) {
        std::string objectType;
        if (_role_privilege->databaseObjectType() == "SCHEMA")
          objectType = "db.mysql.Schema";
        else if (_role_privilege->databaseObjectType() == "TABLE")
          objectType = "db.mysql.Table";
        else if (_role_privilege->databaseObjectType() == "ROUTINE")
          objectType = "db.mysql.Routine";
        else if (_role_privilege->databaseObjectType() == "FUNCTION")
          objectType = "db.mysql.Routine";
        else if (_role_privilege->databaseObjectType() == "PROCEDURE")
          objectType = "db.mysql.Routine";

        if (objectType == *mappings[i]->structName()) {
          _privileges = mappings[i]->privileges();
          break;
        }
      }
    }
  }
}

bool RolePrivilegeListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  if (node[0] >= count() || !_role_privilege.is_valid())
    return false;

  switch ((Columns)column) {
    case Enabled:
      if (_role_privilege->privileges().get_index(_privileges.get(node[0])) == BaseListRef::npos)
        value = grt::IntegerRef(0);
      else
        value = grt::IntegerRef(1);
      return true;
    case Name:
      value = _privileges.get(node[0]);
      return true;
  }
  return false;
}

bool RolePrivilegeListBE::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  size_t index;

  if (node[0] >= count() || !_role_privilege.is_valid())
    return false;

  switch ((Columns)column) {
    case Enabled:
      if ((index = _role_privilege->privileges().get_index(_privileges.get(node[0]))) == BaseListRef::npos) {
        if (value) {
          // grt::AutoUndo undo;
          AutoUndoEdit undo(_owner);

          _role_privilege->privileges().insert(_privileges.get(node[0]));

          undo.end(strfmt(_("Add Object Privilege to Role '%s'"), _owner->get_name().c_str()));
        }
      } else {
        if (!value) {
          // grt::AutoUndo undo;
          AutoUndoEdit undo(_owner);

          _role_privilege->privileges().remove(index);

          undo.end(strfmt(_("Remove Object Privilege to Role '%s'"), _owner->get_name().c_str()));
        }
      }
      return true;

    case Name:
      return false;
  }
  return false;
}

void RolePrivilegeListBE::add_all() {
  if (_role_privilege.is_valid()) {
    AutoUndoEdit undo(_owner);

    for (size_t c = _privileges.count(), i = 0; i < c; i++)
      _role_privilege->privileges().insert(_privileges[i]);

    undo.end(
      strfmt(_("Add All Privileges for '%s' to Role '%s'"),
             _role_privilege->databaseObject().is_valid() ? _role_privilege->databaseObject()->name().c_str() : "*",
             _owner->get_name().c_str()));
  }
}

void RolePrivilegeListBE::remove_all() {
  if (_role_privilege.is_valid()) {
    // grt::AutoUndo undo;
    AutoUndoEdit undo(_owner);
    _role_privilege->privileges().remove_all();
    undo.end(
      strfmt(_("Remove Privileges for '%s' from Role '%s'"),
             _role_privilege->databaseObject().is_valid() ? _role_privilege->databaseObject()->name().c_str() : "*",
             _owner->get_name().c_str()));
  }
}

//-------------------------------------------------------------------------------------------

RoleObjectListBE::RoleObjectListBE(RoleEditorBE *owner) : _owner(owner) {
}

void RoleObjectListBE::set_selected_node(const NodeId &node) {
  _selection = node;
  _owner->get_privilege_list()->refresh();
}

db_RolePrivilegeRef RoleObjectListBE::get_selected_object_info() {
  if (_selection.is_valid() && _selection[0] < count())
    return _owner->get_role()->privileges().get(_selection[0]);

  return db_RolePrivilegeRef();
}

size_t RoleObjectListBE::count() {
  if (_owner->get_role().is_valid())
    return _owner->get_role()->privileges().count();
  return 0;
}

MenuItemList RoleObjectListBE::get_popup_items_for_nodes(const std::vector<NodeId> &nodes) {
  MenuItemList items;

  MenuItem item;

  item.enabled = true;
  item.type = MenuAction;

  db_RoleRef role(_owner->get_role());
  if (role.is_valid() && role->owner().is_valid()) {
    db_CatalogRef catalog(db_CatalogRef::cast_from(role->owner()));

    GRTLIST_FOREACH(db_Schema, catalog->schemata(), schema) {
      item.caption = base::strfmt(_("Add Schema '%s'"), (*schema)->name().c_str());
      item.internalName = std::string("adds:") + (*schema)->name().c_str();
      item.accessibilityName = (*schema)->name();
      items.push_back(item);
      item.caption = base::strfmt(_("Add Tables Wildcard for '%s.*'"), (*schema)->name().c_str());
      item.internalName = std::string("addt:") + (*schema)->name().c_str();
      item.accessibilityName = (*schema)->name();
      items.push_back(item);
      item.caption = base::strfmt(_("Add All Tables in '%s'"), (*schema)->name().c_str());
      item.internalName = std::string("allt:") + (*schema)->name().c_str();
      item.accessibilityName = (*schema)->name();
      items.push_back(item);
    }
  }
  item.caption = "";
  item.internalName = "sep";
  item.accessibilityName = "Separator";
  item.type = MenuSeparator;
  items.push_back(item);

  item.type = MenuAction;
  item.caption = _("Delete Selected");
  item.internalName = "deleteObject";
  item.accessibilityName = "Delete Object";
  item.enabled = nodes.size() > 0;
  items.push_back(item);

  return items;
}

bool RoleObjectListBE::activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes) {
  if (name == "deleteObject") {
    for (std::vector<NodeId>::const_reverse_iterator node = nodes.rbegin(); node != nodes.rend(); ++node)
      _owner->remove_object(*node);
  } else if (name.substr(0, 5) == "adds:")
    _owner->add_object("SCHEMA", name.substr(5) + ".*");
  else if (name.substr(0, 5) == "addt:")
    _owner->add_object("TABLE", name.substr(5) + ".*");
  else if (name.substr(0, 5) == "allt:") {
    db_RoleRef role(_owner->get_role());
    if (role.is_valid() && role->owner().is_valid()) {
      db_CatalogRef catalog(db_CatalogRef::cast_from(role->owner()));
      std::string wanted_schema = name.substr(5);

      db_SchemaRef schema = grt::find_named_object_in_list(catalog->schemata(), wanted_schema);
      if (schema.is_valid()) {
        GRTLIST_FOREACH(db_Table, schema->tables(), table)
        _owner->add_object(*table);
      }
    }
  } else
    return false;

  return true;
}

bool RoleObjectListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  if (node[0] >= count())
    return false;

  switch ((Columns)column) {
    case Name:
      db_RolePrivilegeRef priv(_owner->get_role()->privileges().get(node[0]));
      if (priv.is_valid() && priv->databaseObject().is_valid())
        value = priv->databaseObject()->name();
      else
        value = grt::StringRef(base::strfmt("%s", priv->databaseObjectName().c_str()));
      return true;
  }
  return false;
}

IconId RoleObjectListBE::get_field_icon(const NodeId &node, ColumnId column, IconSize size) {
  db_RolePrivilegeRef priv(_owner->get_role()->privileges().get(node[0]));
  if (priv.is_valid()) {
    if (priv->databaseObject().is_valid())
      return IconManager::get_instance()->get_icon_id(priv->databaseObject());
    else if (priv->databaseObjectType().is_valid()) {
      if (priv->databaseObjectType() == "TABLE")
        return IconManager::get_instance()->get_icon_id("db.Table.many.$.png", Icon16, "");
      if (priv->databaseObjectType() == "SCHEMA")
        return IconManager::get_instance()->get_icon_id("db.Schema.$.png", Icon16, "");
      if (priv->databaseObjectType() == "ROUTINE")
        return IconManager::get_instance()->get_icon_id("db.Routine.many.$.png", Icon16, "");
      if (priv->databaseObjectType() == "VIEW")
        return IconManager::get_instance()->get_icon_id("db.View.many.$.png", Icon16, "");

      return 0;
    } else
      return 0;
  } else
    return 0;
}

//-------------------------------------------------------------------------------------------

RoleEditorBE::RoleEditorBE(const db_RoleRef &role, const db_mgmt_RdbmsRef &rdbms)
  : BaseEditor(role),
    _role(role),
    _rdbms(rdbms),
    _tree(db_CatalogRef::cast_from(role->owner())),
    _privilege_list(this),
    _object_list(this) {
}

void RoleEditorBE::set_name(const std::string &name) {
  if (get_name() != name) {
    AutoUndoEdit undo(this, get_role(), "name");
    std::string name_ = base::trim_right(name);
    get_role()->name(name_);
    undo.end(strfmt(_("Rename Role to '%s'"), name_.c_str()));
  }
}

std::string RoleEditorBE::get_name() {
  return get_role()->name();
}

std::string RoleEditorBE::get_title() {
  return strfmt("%s - Role", get_name().c_str());
}

void RoleEditorBE::set_parent_role(const std::string &name) {
  if (name != get_parent_role()) {
    grt::ListRef<db_Role> roles(db_CatalogRef::cast_from(get_role()->owner())->roles());
    db_RoleRef new_parent_role(grt::find_named_object_in_list(roles, name));

    if (!name.empty()) {
      db_RoleRef some_parent_role(new_parent_role);
      // check for a role loop
      while (some_parent_role.is_valid()) {
        // if the new parent somehow reaches back to this role, then there's a loop
        if (some_parent_role == get_role())
          throw std::runtime_error("Cannot set the parent role to a sub-role.");
        some_parent_role = some_parent_role->parentRole();
      }
    }

    AutoUndoEdit undo(this);

    if (name.empty())
      get_role()->parentRole(db_RoleRef());
    else {
      grt::ListRef<db_Role> roles(db_CatalogRef::cast_from(get_role()->owner())->roles());

      get_role()->parentRole(new_parent_role);
    }

    get_role_tree()->refresh();

    undo.end(strfmt(_("Set Parent Role of '%s'"), get_name().c_str()));
  }
}

std::string RoleEditorBE::get_parent_role() {
  if (get_role()->parentRole().is_valid())
    return *get_role()->parentRole()->name();

  return "";
}

std::vector<std::string> RoleEditorBE::get_role_list() {
  grt::ListRef<db_Role> roles(db_CatalogRef::cast_from(get_role()->owner())->roles());
  std::vector<std::string> names;

  names.push_back("");
  for (grt::ListRef<db_Role>::const_iterator iter = roles.begin(); iter != roles.end(); ++iter) {
    db_RoleRef role((*iter));

    // do not include the role itself or descendants of ourselves
    while (role.is_valid()) {
      if (role == get_role())
        break;
      role = role->parentRole();
    }
    if (role != get_role())
      names.push_back((*iter)->name().c_str());
  }
  return names;
}

bool RoleEditorBE::add_dropped_objectdata(const std::string &data) {
  std::list<db_DatabaseObjectRef> objects;
  bool flag = false;

  objects = bec::CatalogHelper::dragdata_to_dbobject_list(db_CatalogRef::cast_from(get_role()->owner()), data);

  for (std::list<db_DatabaseObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
    if (add_object(*iter))
      flag = true;
  }
  return flag;
}

bool RoleEditorBE::add_object(const std::string &type, const std::string &name) {
  db_RolePrivilegeRef priv(grt::Initialized);
  priv->databaseObjectType(type);
  priv->databaseObjectName(name);
  priv->owner(get_role());

  AutoUndoEdit undo(this);

  get_role()->privileges().insert(priv);

  undo.end(strfmt(_("Add Object %s '%s' to Role '%s'"), type.c_str(), name.c_str(), get_name().c_str()));
  return true;
}

bool RoleEditorBE::add_object(db_DatabaseObjectRef object) {
  grt::ListRef<db_mgmt_PrivilegeMapping> mappings(get_rdbms()->privilegeNames());
  bool ok = false;

  for (size_t c = mappings.count(), i = 0; i < c; i++) {
    if (object.is_instance(*mappings[i]->structName())) {
      ok = true;
      break;
    }
  }

  if (!ok)
    return false;

  // check if already in list
  for (size_t c = get_role()->privileges().count(), i = 0; i < c; i++) {
    if (get_role()->privileges().get(i)->databaseObject() == object) {
      ok = false;
      break;
    }
  }

  if (ok) {
    db_RolePrivilegeRef priv(grt::Initialized);
    priv->databaseObject(object);
    priv->owner(get_role());

    AutoUndoEdit undo(this);

    get_role()->privileges().insert(priv);

    undo.end(strfmt(_("Add Object '%s' to Role '%s'"), object->name().c_str(), get_name().c_str()));
  }
  return true;
}

//!
//! Removes object with index object_node_id from the Role's object list.
//! As it is hard to get to the db_DatabaseObjectRef to remove
//! via
//!
void RoleEditorBE::remove_object(const bec::NodeId &object_node_id) {
  // object_node_id is an id of the object in the role
  size_t object_idx = -1;
  try {
    object_idx = object_node_id.end();
  } catch (const std::logic_error &e) {
    throw std::logic_error(std::string("RoleEditorBE: while removing object from role") + e.what());
  }
  if (object_idx < get_role()->privileges().count()) {
    AutoUndoEdit undo(this);
    get_role()->privileges().remove(object_idx);
    undo.end(strfmt(_("Remove object from Role '%s'"), get_name().c_str()));
  }
}
