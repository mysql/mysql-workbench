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

#include "wb_overview_physical_schema.h"

#include "workbench/wb_context.h"
#include "wb_component_physical.h"

#include "grt/icon_manager.h"
#include "grt/clipboard.h"
#include "base/ui_form.h"
#include "grt/exceptions.h"

#include "grts/structs.db.h"
#include "grts/structs.db.mysql.h"
#include "base/string_utilities.h"

/**
 * @file  wb_overview_physical_schema.cpp
 * @brief Schema specific panels for the overview window
 */

using namespace bec;
using namespace wb;
using namespace wb::internal;
using namespace base;

//-----------------------------------------------------------------------------

SchemaObjectNode::SchemaObjectNode(const db_DatabaseObjectRef &dbobject) : OverviewBE::ObjectNode() {
  object = dbobject;
  label = object->name();
}

void SchemaObjectNode::delete_object(WBContext *wb) {
  wb->get_component<WBComponentPhysical>()->delete_db_object(db_DatabaseObjectRef::cast_from(object));
}

bool SchemaObjectNode::is_deletable() {
  return true;
}

bool SchemaObjectNode::is_renameable() {
  return true;
}

void SchemaObjectNode::copy_object(WBContext *wb, bec::Clipboard *clip) {
  clip->append_data(grt::copy_object(object));
  clip->set_content_description(label);
}

bool SchemaObjectNode::is_copyable() {
  return true;
}

//-----------------------------------------------------------------------------

std::string SchemaTableNode::get_detail(int field) {
  switch (field) {
    case 0: // engine
      return db_mysql_TableRef::cast_from(object)->tableEngine();
    case 1: // created
      return db_TableRef::cast_from(object)->createDate();
    case 2: // modified
      return db_TableRef::cast_from(object)->lastChangeDate();
    case 3: // comment
      return db_TableRef::cast_from(object)->comment();
  }
  return "";
}

//-----------------------------------------------------------------------------

bool SchemaViewNode::is_renameable() {
  return false;
}

std::string SchemaViewNode::get_detail(int field) {
  switch (field) {
    case 0: // created
      return db_ViewRef::cast_from(object)->createDate();
    case 1: // modified
      return db_ViewRef::cast_from(object)->lastChangeDate();
    case 2: // comment
      return db_ViewRef::cast_from(object)->comment();
  }
  return "";
}

//-----------------------------------------------------------------------------

std::string SchemaRoutineGroupNode::get_detail(int field) {
  switch (field) {
    case 0: // created
      return db_RoutineGroupRef::cast_from(object)->createDate();
    case 1: // modified
      return db_RoutineGroupRef::cast_from(object)->lastChangeDate();
    case 2: // comment
      return db_RoutineGroupRef::cast_from(object)->comment();
  }
  return "";
}

//-----------------------------------------------------------------------------

std::string SchemaRoutineNode::get_detail(int field) {
  switch (field) {
    case 0: // created
      return db_RoutineRef::cast_from(object)->createDate();
    case 1: // modified
      return db_RoutineRef::cast_from(object)->lastChangeDate();
    case 2: // comment
      return db_RoutineRef::cast_from(object)->comment();
  }
  return "";
}

bool SchemaRoutineNode::is_renameable() {
  return false;
}

//-----------------------------------------------------------------------------

/*
 * MySQL
 *
 * [+] Physical Schemas
 * ------------------------------------------------------------
 *
 * | webshop       |  inventory      | Add Schema       |
 * |  MySQL schema |    MySQL schema |   Add new schema |
 * ------------------------------------------------------------
 * Tables
 * ...
 * Views
 * ...
 * Routines
 *
 * [+] Synchronized Databases
 * -------------------------------------------------------------
 *
 *
 */

static bool CompNodeLabel(OverviewBE::Node *a, OverviewBE::Node *b) {
  return g_utf8_collate(a->label.c_str(), b->label.c_str()) < 0;
}

//----------------------------------------------------------------------

class wb::internal::PhysicalSchemaContentNode : public OverviewBE::ContainerNode {
  std::vector<std::string> _fields;

private:
  std::string id;
  grt::ListRef<db_DatabaseObject> _list;
  std::function<SchemaObjectNode *(const db_DatabaseObjectRef &)> _create_node;

public:
  PhysicalSchemaContentNode(const std::string &name, const db_SchemaRef &owner, grt::ListRef<db_DatabaseObject> list,
                            std::function<SchemaObjectNode *(const db_DatabaseObjectRef &)> create_node)
    : ContainerNode(OverviewBE::OItem), _list(list), _create_node(create_node) {
    id = owner->id() + "/" + name;
    label = name;
    type = OverviewBE::OSection;
    small_icon = 0;
    large_icon = 0;
    expanded = false;
    display_mode = OverviewBE::MSmallIcon;

    refresh_children();
  }

  virtual void refresh_children() {
    Node *add_node = 0;

    focused = 0;

    if (!children.empty()) {
      add_node = children.front();
      children.erase(children.begin());
    }
    clear_children();

    if (add_node)
      children.push_back(add_node);

    for (size_t c = _list.count(), i = 0; i < c; i++) {
      db_DatabaseObjectRef object(_list[i]);

      SchemaObjectNode *node = _create_node(object);

      node->type = OverviewBE::OItem;
      node->label = object->name();
      node->small_icon = IconManager::get_instance()->get_icon_id(object->get_metaclass(), Icon16);
      node->large_icon = IconManager::get_instance()->get_icon_id(object->get_metaclass(), Icon48);

      children.push_back(node);
    }
    // sort items after add_node
    std::sort(children.begin() + (add_node ? 1 : 0), children.end(), CompNodeLabel);
  }

  void set_detail_fields(const std::vector<std::string> &fields) {
    _fields = fields;
  }

  virtual int count_detail_fields() {
    return (int)_fields.size();
  }

  virtual std::string get_detail_name(int field) {
    return _fields[field];
  }

  virtual std::string get_unique_id() {
    return id;
  }
};

//----------------------------------------------------------------------

PhysicalSchemaNode::PhysicalSchemaNode(db_SchemaRef schema)
  : ContainerNode(OverviewBE::OSection), _is_routine_group_enabled(true) {
  object = schema;
  type = OverviewBE::OGroup;
  label = schema->name();
  description = _("MySQL Schema");
  small_icon = IconManager::get_instance()->get_icon_id("db.Schema.$.png", Icon16);
  large_icon = IconManager::get_instance()->get_icon_id("db.Schema.$.png", Icon32);
}

void PhysicalSchemaNode::init() {
  db_SchemaRef schema = db_SchemaRef::cast_from(object);
  std::vector<std::string> fields;

  PhysicalSchemaContentNode *node;

  node = new PhysicalSchemaContentNode("Tables", schema, grt::ListRef<db_DatabaseObject>::cast_from(schema->tables()),
                                       std::bind(&PhysicalSchemaNode::create_table_node, this, std::placeholders::_1));
  fields.clear();
  fields.push_back(_("Engine"));
  fields.push_back(_("Created"));
  fields.push_back(_("Modified"));
  fields.push_back(_("Comment"));
  node->set_detail_fields(fields);
  children.push_back(node);

  OverviewBE::AddObjectNode *add_node =
    new OverviewBE::AddObjectNode(std::bind(&PhysicalSchemaNode::add_new_db_table, this, std::placeholders::_1));
  add_node->label = _("Add Table");
  add_node->type = OverviewBE::OItem;
  add_node->small_icon = IconManager::get_instance()->get_icon_id("db.Table.$.png", Icon16, "add");
  add_node->large_icon = IconManager::get_instance()->get_icon_id("db.Table.$.png", Icon48, "add");
  node->children.insert(node->children.begin(), add_node); // Add special node in front of all others.

  node = new PhysicalSchemaContentNode("Views", schema, grt::ListRef<db_DatabaseObject>::cast_from(schema->views()),
                                       std::bind(&PhysicalSchemaNode::create_view_node, this, std::placeholders::_1));
  fields.clear();
  fields.push_back(_("Created"));
  fields.push_back(_("Modified"));
  fields.push_back(_("Comment"));
  node->set_detail_fields(fields);
  children.push_back(node);

  add_node =
    new OverviewBE::AddObjectNode(std::bind(&PhysicalSchemaNode::add_new_db_view, this, std::placeholders::_1));
  add_node->label = _("Add View");
  add_node->type = OverviewBE::OItem;
  add_node->small_icon = IconManager::get_instance()->get_icon_id("db.View.$.png", Icon16, "add");
  add_node->large_icon = IconManager::get_instance()->get_icon_id("db.View.$.png", Icon48, "add");
  node->children.insert(node->children.begin(), add_node);

  node =
    new PhysicalSchemaContentNode("Routines", schema, grt::ListRef<db_DatabaseObject>::cast_from(schema->routines()),
                                  std::bind(&PhysicalSchemaNode::create_routine_node, this, std::placeholders::_1));
  fields.clear();
  fields.push_back(_("Created"));
  fields.push_back(_("Modified"));
  fields.push_back(_("Comment"));
  node->set_detail_fields(fields);
  children.push_back(node);

  add_node =
    new OverviewBE::AddObjectNode(std::bind(&PhysicalSchemaNode::add_new_db_routine, this, std::placeholders::_1));
  add_node->label = _("Add Routine");
  add_node->type = OverviewBE::OItem;
  add_node->small_icon = IconManager::get_instance()->get_icon_id("db.Routine.$.png", Icon16, "add");
  add_node->large_icon = IconManager::get_instance()->get_icon_id("db.Routine.$.png", Icon48, "add");
  node->children.insert(node->children.begin(), add_node);

  if (_is_routine_group_enabled) {
    node = new PhysicalSchemaContentNode(
      "Routine Groups", schema, grt::ListRef<db_DatabaseObject>::cast_from(schema->routineGroups()),
      std::bind(&PhysicalSchemaNode::create_routine_group_node, this, std::placeholders::_1));
    fields.clear();
    fields.push_back(_("Created"));
    fields.push_back(_("Modified"));
    fields.push_back(_("Comment"));
    node->set_detail_fields(fields);
    children.push_back(node);

    add_node = new OverviewBE::AddObjectNode(
      std::bind(&PhysicalSchemaNode::add_new_db_routine_group, this, std::placeholders::_1));
    add_node->label = _("Add Group");
    add_node->type = OverviewBE::OItem;
    add_node->small_icon = IconManager::get_instance()->get_icon_id("db.RoutineGroup.$.png", Icon16, "add");
    add_node->large_icon = IconManager::get_instance()->get_icon_id("db.RoutineGroup.$.png", Icon48, "add");
    node->children.insert(node->children.begin(), add_node);
  }
}

void PhysicalSchemaNode::paste_object(WBContext *wb, bec::Clipboard *clip) {
  std::list<grt::ObjectRef> objects(clip->get_data());
  db_SchemaRef schema(db_SchemaRef::cast_from(object));
  WBComponentPhysical *pc = wb->get_component<WBComponentPhysical>();
  grt::CopyContext context;

  grt::AutoUndo undo;
  for (std::list<grt::ObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
    if (iter->is_instance(db_DatabaseObject::static_class_name()))
      pc->clone_db_object_to_schema(schema, db_DatabaseObjectRef::cast_from(*iter), context);
  }
  context.finish();
  undo.end(strfmt(_("Paste %s"), clip->get_content_description().c_str()));
}

bool PhysicalSchemaNode::is_renameable() {
  return false;
}

bool PhysicalSchemaNode::rename(WBContext *wb, const std::string &name) {
  return false;
}

bool PhysicalSchemaNode::is_pasteable(bec::Clipboard *clip) {
  std::string prefix = object.get_metaclass()->name();

  prefix = prefix.substr(0, prefix.length() - strlen(".Schema"));

  std::list<grt::ObjectRef> objects(clip->get_data());
  for (std::list<grt::ObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
    if (!(*iter).is_instance(db_Table::static_class_name()) && !(*iter).is_instance(db_View::static_class_name()) &&
        !(*iter).is_instance(db_Routine::static_class_name()) &&
        !(*iter).is_instance(db_RoutineGroup::static_class_name()))
      return false;

    if (!base::hasPrefix((*iter).get_metaclass()->name(), prefix))
      return false;
  }
  return !objects.empty();
}

void PhysicalSchemaNode::delete_object(WBContext *wb) {
  wb->get_component<WBComponentPhysical>()->delete_db_schema(db_SchemaRef::cast_from(object));
}

bool PhysicalSchemaNode::activate(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(object);
  return true;
}

bool PhysicalSchemaNode::is_deletable() {
  return true;
}

void PhysicalSchemaNode::focus(OverviewBE *sender) {
  db_SchemaRef schema(db_SchemaRef::cast_from(object));

  if (schema->owner().is_valid())
    db_CatalogRef::cast_from(schema->owner())->defaultSchema(schema);
}

void PhysicalSchemaNode::refresh() {
  label = object->name();
}

bool PhysicalSchemaNode::add_new_db_table(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(
    wb->get_component<WBComponentPhysical>()->add_new_db_table(db_SchemaRef::cast_from(object)));
  return true;
}

bool PhysicalSchemaNode::add_new_db_view(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(
    wb->get_component<WBComponentPhysical>()->add_new_db_view(db_SchemaRef::cast_from(object)));
  return true;
}

bool PhysicalSchemaNode::add_new_db_routine_group(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(
    wb->get_component<WBComponentPhysical>()->add_new_db_routine_group(db_SchemaRef::cast_from(object)));
  return true;
}

bool PhysicalSchemaNode::add_new_db_routine(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(
    wb->get_component<WBComponentPhysical>()->add_new_db_routine(db_SchemaRef::cast_from(object)));
  return true;
}

SchemaObjectNode *PhysicalSchemaNode::create_table_node(const db_DatabaseObjectRef &dbobject) {
  return new SchemaTableNode(dbobject);
}

SchemaObjectNode *PhysicalSchemaNode::create_view_node(const db_DatabaseObjectRef &dbobject) {
  return new SchemaViewNode(dbobject);
}

SchemaObjectNode *PhysicalSchemaNode::create_routine_node(const db_DatabaseObjectRef &dbobject) {
  return new SchemaRoutineNode(dbobject);
}

SchemaObjectNode *PhysicalSchemaNode::create_routine_group_node(const db_DatabaseObjectRef &dbobject) {
  return new SchemaRoutineGroupNode(dbobject);
}
