/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <stack>

#include "grt/tree_model.h"
#include "grts/structs.db.h"
#include "grts/structs.db.mysql.h"
#include "grt.h"

#include "grtdb/catalog_templates.h"

std::string utf_to_upper(const char *str);

#include "diff/diffchange.h"
#include "db_mysql_public_interface.h"

class DiffNode;

std::ostream &operator<<(std::ostream &os, const DiffNode &);

std::string get_old_name_or_name(GrtNamedObjectRef obj);

template<typename T>
std::string get_catalog_map_key(grt::Ref<T> t) {
  typedef typename ct::Traits<T>::ParentType Parent;

  std::string parent_key(utf_to_upper(get_catalog_map_key(grt::Ref<Parent>::cast_from(t->owner())).c_str()));

  std::string obj_key(utf_to_upper(get_old_name_or_name(t).c_str()));

  return std::string(parent_key).append(".").append(T::static_class_name()).append(".`").append(obj_key).append("`");
}

template <>
std::string get_catalog_map_key<db_mysql_Catalog>(db_mysql_CatalogRef cat);

class DiffNodePart {
  GrtNamedObjectRef object;
  bool modified;

public:
  DiffNodePart(GrtNamedObjectRef obj) : object(obj), modified(false) {
  }

  bool is_modified() const {
    return modified;
  }
  void set_modified(bool mod) {
    modified = mod;
  }
  bool is_valid_object() const {
    return object.is_valid();
  }
  std::string get_name() const {
    return std::string(object->name().c_str());
  }
  GrtNamedObjectRef get_object() const {
    return object;
  }
};

class DiffNodeController;

class DiffNode {
public:
  enum ApplicationDirection { ApplyToModel = 20, ApplyToDb, DontApply, CantApply };

  typedef std::vector<DiffNode *> DiffNodeVector;

private:
  DiffNodePart model_part;
  DiffNodePart db_part;
  std::shared_ptr<grt::DiffChange> change;
  ApplicationDirection applyDirection;
  DiffNodeVector children;
  bool modified;

public:
  DiffNode(GrtNamedObjectRef model_object, GrtNamedObjectRef external_object, bool inverse,
           std::shared_ptr<grt::DiffChange> c = std::shared_ptr<grt::DiffChange>())
    : model_part(inverse ? external_object : model_object),
      db_part(inverse ? model_object : external_object),
      change(c),
      modified(false) {
    set_modified_and_update_dir(!model_object.is_valid() || !external_object.is_valid(), c);
  }

  ~DiffNode() {
    for (DiffNodeVector::iterator It = children.begin(); It != children.end(); ++It)
      delete *It;
  }

  void dump(int depth = 0);

  std::shared_ptr<grt::DiffChange> get_change() const {
    return change;
  };

  void apply_direction(const ApplicationDirection &d) {
    applyDirection = d;
  }

  ApplicationDirection apply_direction() const {
    return applyDirection;
  }

  ApplicationDirection get_application_direction() const {
    return applyDirection;
  }

  const DiffNodePart &get_model_part() const {
    return model_part;
  }
  const DiffNodePart &get_db_part() const {
    return db_part;
  }

  void append(DiffNode *child) {
    children.push_back(child);
  }

  size_t get_children_size() const {
    return children.size();
  }
  DiffNode *get_child(size_t idx) {
    return children[idx];
  }
  DiffNode *find_node_for_object(const grt::ObjectRef obj);
  DiffNode *find_child_by_db_part_name(const std::string &name);

  DiffNodeVector::const_iterator get_children_begin() const {
    return children.begin();
  }
  DiffNodeVector::const_iterator get_children_end() const {
    return children.end();
  }

  bool is_modified() const {
    return modified;
  }
  bool is_modified_recursive() const {
    if (modified)
      return true;
    for (DiffNodeVector::const_iterator i = children.begin(); i != children.end(); ++i)
      if ((*i)->is_modified_recursive())
        return true;
    return false;
  }
  void set_modified_and_update_dir(bool m, std::shared_ptr<grt::DiffChange> c);

  void get_object_list_for_script(std::vector<grt::ValueRef> &vec) const;
  void get_object_list_to_apply_to_model(std::vector<grt::ValueRef> &vec,
                                         std::vector<grt::ValueRef> &removal_vec) const;
};

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC DiffNodeController {
public:
  DiffNodeController();
  DiffNodeController(const std::map<DiffNode::ApplicationDirection, DiffNode::ApplicationDirection> directions_map);
  void set_next_apply_direction(DiffNode *node) const;
  void set_apply_direction(DiffNode *node, DiffNode::ApplicationDirection dir, bool recursive) const;

protected:
  std::map<DiffNode::ApplicationDirection, DiffNode::ApplicationDirection> _directions_map;
};

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC DiffTreeBE : public bec::TreeModel {
public:
  // icon, model-changed, model-object-name, apply-direction, icon, db-changed, db-object-name

  enum LayerColumns { ModelChanged = 10, ModelObjectName, ApplyDirection, DbChanged, DbObjectName };

private:
  DiffNodeController _node_controller;

  template <typename T>
  static T find_object_in_catalog_map(T cat, const CatalogMap &map);

  DiffNode *_root;
  bec::IconId change_nothing_icon, change_backward_icon, change_forward_icon, change_ignore_icon, alert_icon,
    create_alert_icon, drop_alert_icon;

  std::vector<std::string> _schemata;

  // static void build_catalog_map(db_mysql_CatalogRef catalog, CatalogMap& map);
  bool update_tree_with_changes(const std::shared_ptr<grt::DiffChange> diffchange);
  void apply_change(GrtObjectRef obj, std::shared_ptr<grt::DiffChange> change);

  void fill_tree(DiffNode *root, db_mysql_CatalogRef catalog, const CatalogMap &map, bool inverse);
  void fill_tree(DiffNode *schema_node, db_mysql_SchemaRef schema, const CatalogMap &map, bool inverse);
  void fill_tree(DiffNode *table_node, db_mysql_TableRef table, const CatalogMap &map, bool inverse);

public:
  DiffNode *get_node_with_id(const bec::NodeId &nodeid);
  DiffTreeBE(const std::vector<std::string> &schemata, db_mysql_CatalogRef model_catalogRef,
             db_mysql_CatalogRef external_catalog, std::shared_ptr<grt::DiffChange> diffchange,
             DiffNodeController controller = DiffNodeController());
  virtual ~DiffTreeBE() {
    delete _root;
  };

  virtual size_t count_children(const bec::NodeId &);
  virtual bec::NodeId get_child(const bec::NodeId &, size_t);
  virtual bool get_field(const bec::NodeId &node_id, ColumnId column, std::string &value);
  virtual bec::IconId get_field_icon(const bec::NodeId &node, ColumnId column, bec::IconSize size);
  virtual void refresh() {
  }

  void set_next_apply_direction(const bec::NodeId &node_id);
  void set_apply_direction(const bec::NodeId &node_id, DiffNode::ApplicationDirection dir, bool recursive);
  DiffNode::ApplicationDirection get_apply_direction(const bec::NodeId &node_id);

  void get_object_list_for_script(std::vector<grt::ValueRef> &vec) const;
  void get_object_list_to_apply_to_model(std::vector<grt::ValueRef> &vec,
                                         std::vector<grt::ValueRef> &removal_vec) const;
};
