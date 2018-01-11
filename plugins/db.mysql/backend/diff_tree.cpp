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

#include "base/log.h"

#include "diff/diffchange.h"
#include "diff/grtdiff.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"

#include "diff_tree.h"
#include "grts/structs.model.h"

#include <fstream>
#include <set>

#include "grtdb/catalog_templates.h"
#include "module_db_mysql_shared_code.h"

DEFAULT_LOG_DOMAIN("difftree");

// using namespace wb;
using namespace bec;

std::string utf_to_upper(const char *str) {
  char *n = g_utf8_strup(str, g_utf8_strlen(str, -1));
  std::string retval(n);
  g_free(n);
  return retval;
}

namespace {
  struct Output {
    std::ostream &ostream;
    Output(std::ostream &os) : ostream(os) {
    }
    void operator()(const DiffNode *node) {
      ostream << *node;
    }
  };
}

std::ostream &operator<<(std::ostream &os, const DiffNode &node) {
  os << "\n<diffnode is_modified='" << (node.is_modified() ? 1 : 0) << "'";

  if (node.get_model_part().is_valid_object())
    os << " model_name='" << node.get_model_part().get_name() << "'";
  if (node.get_db_part().is_valid_object())
    os << " db_name='" << node.get_db_part().get_name() << "'";
  if (node.get_application_direction() == DiffNode::ApplyToModel)
    os << "dir='model'";
  else if (node.get_application_direction() == DiffNode::ApplyToDb)
    os << "dir='db'";
  else if (node.get_application_direction() == DiffNode::DontApply)
    os << "dir='dontapply'";

  os << " >";

  std::for_each(node.get_children_begin(), node.get_children_end(), Output(os));

  os << "\n</diffnode>";
  return os;
}

namespace {
  struct CompareName {
    std::string name;
    const bool _case_sensitive;

    CompareName(const std::string &n, const bool case_sensitive) : name(n.c_str()), _case_sensitive(case_sensitive) {
    }

    bool operator()(const DiffNode *node) {
      if (node->get_db_part().is_valid_object()) {
        std::string node_name(get_old_object_name_for_key(node->get_db_part().get_object(), _case_sensitive));

        return (name.compare(node_name) == 0);
      } else if (node->get_model_part().is_valid_object()) {
        std::string node_name(get_old_object_name_for_key(node->get_model_part().get_object(), _case_sensitive));

        return (name.compare(node_name) == 0);
      }
      return false;
    }
  };
}

void DiffNode::dump(int depth) {
  const char *dir = NULL;
  switch (applyDirection) {
    case ApplyToModel:
      dir = "model->";
      break;
    case ApplyToDb:
      dir = "<-db";
      break;
    case DontApply:
      dir = "ignore";
      break;
    case CantApply:
      dir = "n/a";
      break;
  }

  logDebug("%*s: %s: %s | %s | %s\n", depth, "-", change ? change->get_type_name().c_str() : "nil",
           db_part.is_valid_object() ? db_part.get_name().c_str() : "nil", dir,
           model_part.is_valid_object() ? model_part.get_name().c_str() : "nil");
  for (DiffNodeVector::iterator It = children.begin(); It != children.end(); ++It)
    (*It)->dump(depth + 1);
}

DiffNode *DiffNode::find_child_by_db_part_name(const std::string &name) {
  DiffNodeVector::const_iterator b = children.begin();
  DiffNodeVector::const_iterator e = children.end();
  DiffNodeVector::const_iterator it = std::find_if(b, e, CompareName(name, true));
  if (it == e) {
    it = std::find_if(b, e, CompareName(base::toupper(name), false));
    if (it == e)
      return NULL;
  }
  return *it;
}

DiffNode *DiffNode::find_node_for_object(const grt::ObjectRef obj) {
  if (get_db_part().is_valid_object()) {
    if (get_db_part().get_object()->id() == obj->id())
      return this;
  } else if (get_model_part().is_valid_object()) {
    if (get_model_part().get_object()->id() == obj->id())
      return this;
  }
  for (DiffNodeVector::const_iterator It = children.begin(); It != children.end(); ++It) {
    DiffNode *node = (*It)->find_node_for_object(obj);
    if (node)
      return node;
  }
  return NULL;
};

DiffNodeController::DiffNodeController() {
  _directions_map[DiffNode::ApplyToModel] = DiffNode::ApplyToDb;
  _directions_map[DiffNode::ApplyToDb] = DiffNode::DontApply;
  _directions_map[DiffNode::DontApply] = DiffNode::ApplyToModel;
};

DiffNodeController::DiffNodeController(
  const std::map<DiffNode::ApplicationDirection, DiffNode::ApplicationDirection> directions_map)
  : _directions_map(directions_map){

    };

void DiffNodeController::set_next_apply_direction(DiffNode *node) const {
  std::map<DiffNode::ApplicationDirection, DiffNode::ApplicationDirection>::const_iterator newdir =
    _directions_map.find(node->apply_direction());
  node->apply_direction(newdir == _directions_map.end() ? DiffNode::DontApply : newdir->second);
};

void DiffNodeController::set_apply_direction(DiffNode *node, DiffNode::ApplicationDirection dir, bool recursive) const {
  node->apply_direction(dir);

  if (recursive)
    for (DiffNode::DiffNodeVector::const_iterator It = node->get_children_begin(); It != node->get_children_end(); ++It)
      set_apply_direction(*It, dir, recursive);
};

namespace {
  struct GetObjectListForScript {
    std::vector<grt::ValueRef> &vec;

    GetObjectListForScript(std::vector<grt::ValueRef> &v) : vec(v) {
    }
    void operator()(const DiffNode *node) {
      node->get_object_list_for_script(vec);
    }
  };

  struct NullModelPart {
    bool operator()(const DiffNode *node) {
      return node->get_model_part().get_object().is_valid() == 0;
    }
  };
}

void DiffNode::get_object_list_for_script(std::vector<grt::ValueRef> &vec) const {
  bool im_already_there = false;

  if (applyDirection == ApplyToDb) {
    if (this->get_model_part().get_object().is_valid()) {
      vec.push_back(this->get_model_part().get_object());
    } else {
      vec.push_back(this->get_db_part().get_object());
      return;
    }

    im_already_there = true;
  }

  DiffNodeVector::const_iterator b = children.begin();
  DiffNodeVector::const_iterator e = children.end();

  if (!im_already_there) {
    if (std::find_if(b, e, NullModelPart()) != e)
      vec.push_back(this->get_model_part().get_object());
  }

  std::for_each(b, e, GetObjectListForScript(vec));
}

namespace {
  struct GetObjectListToApplyToModel {
    std::vector<grt::ValueRef> &vec;
    std::vector<grt::ValueRef> &removal_vec;
    GetObjectListToApplyToModel(std::vector<grt::ValueRef> &v, std::vector<grt::ValueRef> &rv)
      : vec(v), removal_vec(rv) {
    }
    void operator()(const DiffNode *node) {
      node->get_object_list_to_apply_to_model(vec, removal_vec);
    }
  };
}

void DiffNode::get_object_list_to_apply_to_model(std::vector<grt::ValueRef> &vec,
                                                 std::vector<grt::ValueRef> &removal_vec) const {
  if (applyDirection == ApplyToModel) {
    grt::ValueRef val = this->get_db_part().get_object();
    if (val.is_valid())
      vec.push_back(val);
    else // object must be removed from model;
      removal_vec.push_back(this->get_model_part().get_object());
  }

  DiffNodeVector::const_iterator b = children.begin();
  DiffNodeVector::const_iterator e = children.end();
  std::for_each(b, e, GetObjectListToApplyToModel(vec, removal_vec));
}

void DiffNode::set_modified_and_update_dir(bool m, std::shared_ptr<grt::DiffChange> c) {
  change = c;
  modified = m;
  applyDirection = m ? ApplyToDb : CantApply;
}

std::string get_old_name_or_name(GrtNamedObjectRef obj) {
  if (!obj.is_valid())
    return "";

  if (strlen(obj->oldName().c_str()) && !db_mysql_SchemaRef::can_wrap(obj))
    return std::string(obj->oldName().c_str());
  return std::string(obj->name().c_str());
}

template <>
std::string get_catalog_map_key<db_mysql_Catalog>(db_mysql_CatalogRef cat) {
  if (!cat.is_valid())
    return "default";
  return std::string("`").append(cat->name()).append("`");
}

template <class _Object>
class ObjectAction {
protected:
  CatalogMap &map;

public:
  ObjectAction(CatalogMap &m) : map(m) {
  }

  virtual void operator()(_Object object) {
    map[get_catalog_map_key(object)] = object;
  }
};

namespace {
  struct TableAction : public ObjectAction<db_mysql_TableRef> {
  public:
    TableAction(CatalogMap &m) : ObjectAction<db_mysql_TableRef>(m) {
    }

    virtual void operator()(db_mysql_TableRef table) {
      ObjectAction<db_mysql_TableRef>::operator()(table);
      ObjectAction<db_mysql_ColumnRef> oa_column(map);
      ct::for_each<ct::Columns>(table, oa_column);

      ObjectAction<db_mysql_IndexRef> oa_index(map);
      ct::for_each<ct::Indices>(table, oa_index);

      ObjectAction<db_mysql_TriggerRef> oa_trigger(map);
      ct::for_each<ct::Triggers>(table, oa_trigger);

      ObjectAction<db_mysql_ForeignKeyRef> oa_fk(map);
      ct::for_each<ct::ForeignKeys>(table, oa_fk);
    }
  };

  struct SchemaAction : public ObjectAction<db_mysql_SchemaRef> {
  public:
    SchemaAction(CatalogMap &m) : ObjectAction<db_mysql_SchemaRef>(m) {
    }

    virtual void operator()(db_mysql_SchemaRef schema) {
      ObjectAction<db_mysql_SchemaRef>::operator()(schema);

      TableAction ta(map);
      ct::for_each<ct::Tables>(schema, ta);

      ObjectAction<db_mysql_ViewRef> oa_view(map);
      ct::for_each<ct::Views>(schema, oa_view);

      ObjectAction<db_mysql_RoutineRef> oa_routine(map);
      ct::for_each<ct::Routines>(schema, oa_routine);
    }
  };
}

WBPLUGINDBMYSQLBE_PUBLIC_FUNC
void build_catalog_map(db_mysql_CatalogRef catalog, CatalogMap &map) {
  SchemaAction sa(map);
  ct::for_each<ct::Schemata>(catalog, sa);
}

template <typename T>
T DiffTreeBE::find_object_in_catalog_map(T t, const CatalogMap &map) {
  if (strlen(t->oldName().c_str()) == 0)
    return T();
  CatalogMap::const_iterator iter = map.find(get_catalog_map_key(t));
  if (iter != map.end())
    return T::cast_from(iter->second);
  return T();
}

void DiffTreeBE::fill_tree(DiffNode *table_node, db_mysql_TableRef table, const CatalogMap &map, bool inverse) {
  // triggers
  for (size_t k = 0, triggers_count = table->triggers().count(); k < triggers_count; k++) {
    db_mysql_TriggerRef trigger = table->triggers().get(k);
    db_mysql_TriggerRef external_trigger = find_object_in_catalog_map(trigger, map);
    DiffNode *trigger_node = new DiffNode(trigger, external_trigger, inverse);
    table_node->append(trigger_node);
  }
}

void DiffTreeBE::fill_tree(DiffNode *schema_node, db_mysql_SchemaRef schema, const CatalogMap &map, bool inverse) {
  // tables
  for (size_t j = 0, tables_count = schema->tables().count(); j < tables_count; j++) {
    db_mysql_TableRef table = schema->tables().get(j);
    db_mysql_TableRef external_table = find_object_in_catalog_map(table, map);
    DiffNode *table_node = new DiffNode(table, external_table, inverse);
    schema_node->append(table_node);
    fill_tree(table_node, table, map, inverse);
  }

  // views
  for (size_t j = 0, views_count = schema->views().count(); j < views_count; j++) {
    db_mysql_ViewRef view = schema->views().get(j);
    db_mysql_ViewRef external_view = find_object_in_catalog_map(view, map);
    DiffNode *view_node = new DiffNode(view, external_view, inverse);
    schema_node->append(view_node);
  }

  // routines
  for (size_t j = 0, routines_count = schema->routines().count(); j < routines_count; j++) {
    db_mysql_RoutineRef routine = schema->routines().get(j);
    db_mysql_RoutineRef external_routine = find_object_in_catalog_map(routine, map);
    DiffNode *routine_node = new DiffNode(routine, external_routine, inverse);
    schema_node->append(routine_node);
  }
}

void DiffTreeBE::fill_tree(DiffNode *root, db_mysql_CatalogRef model_catalog, const CatalogMap &map, bool inverse) {
  for (size_t i = 0, schemata_count = model_catalog->schemata().count(); i < schemata_count; i++) {
    db_mysql_SchemaRef schema = model_catalog->schemata().get(i);
    db_mysql_SchemaRef external_schema = find_object_in_catalog_map(schema, map);
    if (!external_schema.is_valid()) {
      std::string schema_name(schema->name().c_str());
      if (std::find(_schemata.begin(), _schemata.end(), schema_name) != _schemata.end())
        continue;
    }
    DiffNode *schema_node = new DiffNode(schema, external_schema, inverse);
    root->append(schema_node);
    fill_tree(schema_node, schema, map, inverse);
  }
}

bool is_node_object(const grt::ValueRef v) {
  return (db_SchemaRef::can_wrap(v) || db_TableRef::can_wrap(v) || db_ViewRef::can_wrap(v) ||
          db_RoutineRef::can_wrap(v) || db_TriggerRef::can_wrap(v));
}

bool DiffTreeBE::update_tree_with_changes(const std::shared_ptr<grt::DiffChange> diffchange) {
  if (!diffchange)
    return false;

  switch (diffchange->get_change_type()) {
    case grt::SimpleValue:
      return true;
      break;

    case grt::ListItemAdded: {
      grt::ValueRef v = static_cast<const grt::ListItemAddedChange *>(diffchange.get())->get_value();
      if (is_node_object(v))
        apply_change(GrtObjectRef::cast_from(v), diffchange);
      else
        return true;
    } break;

    case grt::ListItemRemoved: {
      grt::ValueRef v = static_cast<const grt::ListItemRemovedChange *>(diffchange.get())->get_value();
      if (is_node_object(v))
        apply_change(GrtObjectRef::cast_from(v), diffchange);
      else
        return true;
    } break;

    case grt::ListModified:
    case grt::ObjectModified: {
      bool flag = false;
      for (grt::ChangeList::const_iterator it = diffchange->subchanges()->begin(); it < diffchange->subchanges()->end();
           ++it) {
        if (update_tree_with_changes(*it))
          flag = true;
      }
      return flag;
    }
    case grt::ObjectAttrModified:
      return update_tree_with_changes(
        static_cast<const grt::ObjectAttrModifiedChange *>(diffchange.get())->get_subchange());

    case grt::ListItemModified: {
      grt::ValueRef v(static_cast<const grt::ListItemModifiedChange *>(diffchange.get())->get_old_value());

      bool push = is_node_object(v);

      bool flag =
        update_tree_with_changes(static_cast<const grt::ListItemModifiedChange *>(diffchange.get())->get_subchange());
      if (push && flag) {
        apply_change(GrtObjectRef::cast_from(v), diffchange);
        flag = false;
      }
      return flag;
    } break;

    case grt::ListItemOrderChanged: {
      const grt::ListItemOrderChange *oc = static_cast<const grt::ListItemOrderChange *>(diffchange.get());
      grt::ValueRef v(oc->get_old_value());
      // order changes matter for colums and indexcolumn... in anything else, it doesn't
      if (db_ColumnRef::can_wrap(v) || db_IndexColumnRef::can_wrap(v))
        return true;
      // the list item order change action can have a nested
      // object-modified action
      else if (oc->get_subchange())
        return update_tree_with_changes(oc->get_subchange());
    } break;

    default:
      break;
  }
  return false;
}

// TODO check how new DiffNode being deleted
void DiffTreeBE::apply_change(GrtObjectRef obj, std::shared_ptr<grt::DiffChange> change) {
  DiffNode *obj_node = _root->find_node_for_object(obj);

#if 0
  // Uncomment to see what are the differences detected for a given object
  if (obj->name() == "")
    change->dump_log(0);
#endif

  if (obj_node)
    obj_node->set_modified_and_update_dir(true, change);
  else {
    DiffNode *owner_node = _root->find_node_for_object(obj->owner());
    if (!owner_node)
      owner_node = _root;

    DiffNode *newnode = new DiffNode(GrtNamedObjectRef(), GrtNamedObjectRef::cast_from(obj), false, change);
    owner_node->append(newnode);
  }
}

DiffTreeBE::DiffTreeBE(const std::vector<std::string> &schemata, db_mysql_CatalogRef model_catalogRef,
                       db_mysql_CatalogRef external_catalog, std::shared_ptr<grt::DiffChange> diffchange,
                       DiffNodeController controller)
  : _node_controller(controller) {
  _root = new DiffNode(model_catalogRef, external_catalog, false);
  _schemata.assign(schemata.begin(), schemata.end());

  CatalogMap map;
  if (external_catalog.is_valid())
    build_catalog_map(external_catalog, map);
  fill_tree(_root, model_catalogRef, map, false);
  update_tree_with_changes(diffchange);

  change_nothing_icon = bec::IconManager::get_instance()->get_icon_id("change_nothing.png");
  change_backward_icon = bec::IconManager::get_instance()->get_icon_id("change_backward.png");
  change_forward_icon = bec::IconManager::get_instance()->get_icon_id("change_forward.png");
  change_ignore_icon = bec::IconManager::get_instance()->get_icon_id("change_ignore.png");
  alert_icon = bec::IconManager::get_instance()->get_icon_id("change_alert_thin.png");
  create_alert_icon = bec::IconManager::get_instance()->get_icon_id("change_alert_create.png");
  drop_alert_icon = bec::IconManager::get_instance()->get_icon_id("change_alert_drop.png");
}

DiffNode *DiffTreeBE::get_node_with_id(const NodeId &nodeid) {
  DiffNode *n = _root;

  if (!n)
    return NULL;

  if (nodeid.depth() == 0)
    return n;

  for (size_t i = 0; i < nodeid.depth(); i++) {
    if (nodeid[i] >= n->get_children_size())
      throw std::logic_error("Invalid node id");

    n = n->get_child(nodeid[i]);
  }
  return n;
}

size_t DiffTreeBE::count_children(const bec::NodeId &nodeid) {
  DiffNode *node = get_node_with_id(nodeid);
  if (node)
    return (int)node->get_children_size();
  return 0;
}

bec::NodeId DiffTreeBE::get_child(const bec::NodeId &parentid, size_t child_idx) {
  DiffNode *node = get_node_with_id(parentid);

  if (node && child_idx < node->get_children_size())
    return NodeId(parentid).append(child_idx);

  if (node)
    throw std::logic_error("invalid index");

  return NodeId();
}

bool DiffTreeBE::get_field(const bec::NodeId &node_id, ColumnId column, std::string &value) {
  if ((column != ModelObjectName) && (column != DbObjectName)) {
    return false;
  }

  DiffNode *node = dynamic_cast<DiffNode *>(get_node_with_id(node_id));
  if (!node)
    return false;

  switch (column) {
    case ModelObjectName:
      if (node->get_model_part().is_valid_object()) {
        value = node->get_model_part().get_name();
        if (db_SchemaRef::can_wrap(node->get_model_part().get_object())) {
          // put the actual name of the schema if it's being synchronized with a differently named schema
          std::string real_name = db_SchemaRef::cast_from(node->get_model_part().get_object())
                                    ->customData()
                                    .get_string("db.mysql.synchronize:originalName");
          if (!real_name.empty())
            value.append(" (" + real_name + ")");
        }
      } else
        value = "N/A";
      break;
    case DbObjectName:
      if (node->get_db_part().is_valid_object())
        value = node->get_db_part().get_name();
      else
        value = "N/A";
      break;
    default:
      value = "";
      return false;
  }

  return true;
}

bec::IconId DiffTreeBE::get_field_icon(const bec::NodeId &node_id, ColumnId column, IconSize size) {
  if ((column != ModelObjectName) && (column != ModelChanged) && (column != ApplyDirection) && (column != DbChanged) &&
      (column != DbObjectName)) {
    return -1;
  }

  DiffNode *node = dynamic_cast<DiffNode *>(get_node_with_id(node_id));
  if (!node)
    return -1;

  IconId object_icon = 1;
  if (node->get_db_part().is_valid_object())
    object_icon = bec::IconManager::get_instance()->get_icon_id(node->get_db_part().get_object());
  else if (node->get_model_part().is_valid_object())
    object_icon = bec::IconManager::get_instance()->get_icon_id(node->get_model_part().get_object());

  switch (column) {
    case ModelChanged:
      if (node->is_modified_recursive()) {
        bool model_valid = node->get_model_part().is_valid_object();
        bool db_valid = node->get_db_part().is_valid_object();
        DiffNode::ApplicationDirection dir = node->get_application_direction();

        if (model_valid && !db_valid) {
          // exists in model, doesn't in DB
          if (dir == DiffNode::ApplyToModel)
            return drop_alert_icon;
          else
            return alert_icon;
        } else if (!model_valid && db_valid) {
          // exists in model, doesn't in DB
          if (dir == DiffNode::ApplyToDb)
            return create_alert_icon;
          else
            return alert_icon;
        } else
          return alert_icon;
      } else
        return 0;
    // return (node->is_modified()) ? ((!node->get_db_part().is_valid_object() ||
    // !node->get_model_part().is_valid_object())? drop_alert_icon : alert_icon) : 0;
    case ModelObjectName:
      return object_icon;
    case ApplyDirection:
      if (node->is_modified()) {
        switch (node->get_application_direction()) {
          case DiffNode::ApplyToModel:
            return change_backward_icon;
          case DiffNode::ApplyToDb:
            return change_forward_icon;
          case DiffNode::DontApply:
            return change_ignore_icon;
          case DiffNode::CantApply:
            return change_nothing_icon;
        }
      } else
        return change_nothing_icon;
      return -1;
    case DbObjectName:
      if (node->is_modified()) {
        bool model_valid = node->get_model_part().is_valid_object();
        bool db_valid = node->get_db_part().is_valid_object();
        DiffNode::ApplicationDirection dir = node->get_application_direction();

        if (model_valid && !db_valid) {
          // exists in model, doesn't in DB
          if (dir == DiffNode::ApplyToDb)
            return create_alert_icon;
          else
            return alert_icon;
        } else if (!model_valid && db_valid) {
          // exists in model, doesn't in DB
          if (dir == DiffNode::ApplyToDb)
            return drop_alert_icon;
          else
            return alert_icon;
        } else
          return alert_icon;
        // return (node->is_modified()) ? ((!node->get_db_part().is_valid_object() ||
        // !node->get_model_part().is_valid_object())? drop_alert_icon : alert_icon) : 0;
      } else
        return 0;
    default:
      return -1;
  }
}

void DiffTreeBE::set_next_apply_direction(const bec::NodeId &node_id) {
  DiffNode *node = dynamic_cast<DiffNode *>(get_node_with_id(node_id));
  if (node)
    _node_controller.set_next_apply_direction(node);
}

void DiffTreeBE::set_apply_direction(const bec::NodeId &node_id, DiffNode::ApplicationDirection dir, bool recursive) {
  DiffNode *node = dynamic_cast<DiffNode *>(get_node_with_id(node_id));
  if (node)
    _node_controller.set_apply_direction(node, dir, recursive);
}

DiffNode::ApplicationDirection DiffTreeBE::get_apply_direction(const bec::NodeId &node_id) {
  DiffNode *node = dynamic_cast<DiffNode *>(get_node_with_id(node_id));
  if (node)
    return node->get_application_direction();
  return DiffNode::CantApply;
}

void DiffTreeBE::get_object_list_for_script(std::vector<grt::ValueRef> &vec) const {
  _root->get_object_list_for_script(vec);
}

void DiffTreeBE::get_object_list_to_apply_to_model(std::vector<grt::ValueRef> &vec,
                                                   std::vector<grt::ValueRef> &removal_vec) const {
  _root->get_object_list_to_apply_to_model(vec, removal_vec);
}

// void DiffTreeBE::sync_old_name() const
//{
//  _root->sync_old_name();
//}
