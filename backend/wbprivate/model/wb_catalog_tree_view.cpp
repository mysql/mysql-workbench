/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/menubar.h"

#include "workbench/wb_context.h"
#include "grt/icon_manager.h"
#include "wb_model_diagram_form.h"
#include "wb_catalog_tree_view.h"
#include <unordered_set>

using namespace wb;

//--------------------------------------------------------------------------------------------------

// Simple mapper of a tree node and a model node.
class CatalogData : public mforms::TreeNodeData {
public:
  bec::NodeId id;

  CatalogData(bec::NodeId theId) : mforms::TreeNodeData() {
    id = theId;
  }
};

//--------------------------------------------------------------------------------------------------
static std::string get_node_icon_path(NodeIcons icon) {
  bec::IconId iconid;

  switch (icon) {
    case IconTablesMany:
      iconid = bec::IconManager::get_instance()->get_icon_id("db.Table.many.$.png", bec::Icon16);
      break;
    case IconTable:
      iconid = bec::IconManager::get_instance()->get_icon_id("db.Table.$.png", bec::Icon16);
      break;
    case IconViewsMany:
      iconid = bec::IconManager::get_instance()->get_icon_id("db.View.many.$.png", bec::Icon16);
      break;
    case IconView:
      iconid = bec::IconManager::get_instance()->get_icon_id("db.View.$.png", bec::Icon16);
      break;
    case IconRoutineGroupsMany:
      iconid = bec::IconManager::get_instance()->get_icon_id("db.Routine.many.$.png", bec::Icon16);
      break;
    case IconRoutineGroup:
      iconid = bec::IconManager::get_instance()->get_icon_id("db.Routine.$.png", bec::Icon16);
      break;
    case IconSchema:
      iconid = bec::IconManager::get_instance()->get_icon_id("db.Schema.$.png", bec::Icon16);
      break;
    default:
      return "";
  }

  return bec::IconManager::get_instance()->get_icon_file(iconid);
}

CatalogTreeView::ObjectNodeData::ObjectNodeData(grt::ObjectRef obj_ref) : mforms::TreeNodeData(), _ref(obj_ref) {
}
grt::ObjectRef CatalogTreeView::ObjectNodeData::get_object_ref() {
  return _ref;
}

bool CatalogTreeView::get_drag_data(mforms::DragDetails &details, void **data, std::string &format) {
  std::list<mforms::TreeNodeRef> selection = get_selection();

  _dragged_objects.clear();
  for (std::list<mforms::TreeNodeRef>::const_iterator iterator = selection.begin(); iterator != selection.end();
       ++iterator) {
    ObjectNodeData *odata = dynamic_cast<ObjectNodeData *>((*iterator)->get_data());
    if (odata != NULL) {
      GrtObjectRef object;
      grt::ValueRef value = odata->get_object_ref();
      if (value.is_valid() &&
          (db_TableRef::can_wrap(value) || db_ViewRef::can_wrap(value) || db_RoutineGroupRef::can_wrap(value)))
        object = GrtObjectRef::cast_from(value);
      if (object.is_valid())
        _dragged_objects.push_back(object);
    }
  }

  if (_dragged_objects.empty())
    return false;

  details.allowedOperations = mforms::DragOperationCopy;
  *data = &_dragged_objects;
  format = WB_DBOBJECT_DRAG_TYPE;

  return true;
}

void CatalogTreeView::menu_action(const std::string &name, grt::ValueRef val) {
  if (name == "edit" && _activate_callback)
    _activate_callback(val);
}

mforms::TreeNodeRef CatalogTreeView::create_new_node(const ObjectType &otype, mforms::TreeNodeRef parent,
                                                     const std::string &name, grt::ObjectRef obj) {
  mforms::TreeNodeRef new_node;
  if (parent.is_valid()) {
    std::string icon_path;
    switch (otype) {
      case ObjTable: {
        new_node = parent->get_child(0)->add_child();
        icon_path = get_node_icon_path(IconTable);
        break;
      }
      case ObjView: {
        new_node = parent->get_child(1)->add_child();
        icon_path = get_node_icon_path(IconView);
        break;
      }
      case ObjRoutineGrp: {
        new_node = parent->get_child(2)->add_child();
        icon_path = get_node_icon_path(IconRoutineGroup);
        break;
      }
      case ObjSchema:
        new_node = parent->add_child();
        icon_path = get_node_icon_path(IconSchema);
        break;
      default:
        break;
    }

    if (new_node.is_valid()) {
      new_node->set_string(0, name);
      new_node->set_icon_path(0, icon_path);
      new_node->set_data(new ObjectNodeData(obj));
      new_node->set_tag(obj.id());
      if (otype == ObjSchema) // it's different we need to also create catalog nodes
      {
        mforms::TreeNodeRef child = new_node->add_child();
        new_node->expand();
        child->set_string(0, _("Tables"));
        child->set_icon_path(0, get_node_icon_path(IconTablesMany));
        child = new_node->add_child();
        child->set_string(0, _("Views"));
        child->set_icon_path(0, get_node_icon_path(IconViewsMany));
        child = new_node->add_child();
        child->set_string(0, _("Routine Groups"));
        child->set_icon_path(0, get_node_icon_path(IconRoutineGroupsMany));
      }
    }
  }

  return new_node;
}

CatalogTreeView::CatalogTreeView(ModelDiagramForm *owner)
  : mforms::TreeView(mforms::TreeNoBorder | mforms::TreeCanBeDragSource | mforms::TreeIndexOnTag | mforms::TreeTranslucent
#ifndef _MSC_VER
                     | mforms::TreeNoHeader
#endif
                     ),
    _owner(owner)

{
  _initialized = false;
  bool showHeaderText = true;
  set_selection_mode(mforms::TreeSelectMultiple);
#ifdef _MSC_VER
  set_row_height(19);
  showHeaderText = false;
#else
  set_row_height(17);
#endif
  add_column(mforms::IconStringColumnType, showHeaderText ? "Name" : "", 200);
  add_column(mforms::StringColumnType, showHeaderText ? "Presence" : "", 20);
  end_columns();

  _menu = new mforms::ContextMenu();
  _menu->signal_will_show()->connect(std::bind(&CatalogTreeView::context_menu_will_show, this, std::placeholders::_1));
  set_context_menu(_menu);
}

//--------------------------------------------------------------------------------------------------

CatalogTreeView::~CatalogTreeView() {
  delete _menu;
}

void CatalogTreeView::node_activated(mforms::TreeNodeRef row, int column) {
  ObjectNodeData *data = dynamic_cast<ObjectNodeData *>(row->get_data());
  if (data != NULL)
    _activate_callback(data->get_object_ref());
}

static bool compare_db_object(db_DatabaseObjectRef a, db_DatabaseObjectRef b) {
  return base::string_compare(a->name(), b->name()) < 0 ? true : false;
}

template <typename T>
static std::vector<grt::Ref<T> > sort_db_object(grt::ListRef<T> list) {
  std::vector<grt::Ref<T> > vec;
  for (size_t i = 0; i < list.count(); ++i) {
    vec.push_back(list[i]);
  }
  std::sort(vec.begin(), vec.end(), compare_db_object);
  return vec;
}

void CatalogTreeView::refill(bool force) {
  if (_initialized && !force)
    return;

  clear();
  model_ModelRef model = _owner->get_model_diagram()->owner();

  std::unordered_set<grt::internal::Value *> uset;
  grt::ListRef<model_Figure> figures(_owner->get_model_diagram()->figures());
  for (size_t c = figures.count(), i = 0; i < c; i++) {
    model_FigureRef f(figures[i]);

    if (f.has_member("table"))
      uset.insert(f.get_member("table").valueptr());
    else if (f.has_member("view"))
      uset.insert(f.get_member("view").valueptr());
    else if (f.has_member("routine"))
      uset.insert(f.get_member("routine").valueptr());
    else if (f.has_member("routineGroup"))
      uset.insert(f.get_member("routineGroup").valueptr());
  }

  freeze_refresh();
  grt::ListRef<db_Schema> schema_list = workbench_physical_ModelRef::cast_from(model)->catalog()->schemata();
  for (size_t i = 0; i < schema_list.count(); ++i) {
    mforms::TreeNodeRef node = add_node();
    node->set_string(0, schema_list[i]->name().c_str());
    node->set_icon_path(0, get_node_icon_path(IconSchema));
    ;
    node->set_tag(schema_list[i].id());
    node->set_data(new ObjectNodeData(schema_list[i]));
    mforms::TreeNodeRef child = node->add_child();
    if (i == 0) // we expand by default only first schema on the list
      node->expand();
    child->set_string(0, _("Tables"));
    child->set_icon_path(0, get_node_icon_path(IconTablesMany));

    std::vector<db_TableRef> table_list = sort_db_object<db_Table>(schema_list[i]->tables());

    for (size_t j = 0; j < table_list.size(); ++j) {
      db_TableRef table = table_list[j];
      mforms::TreeNodeRef subchild = child->add_child();
      subchild->set_string(0, table->name().c_str());
      subchild->set_icon_path(0, get_node_icon_path(IconTable));
      subchild->set_tag(table.id());
      subchild->set_data(new ObjectNodeData(table));
      if (uset.find(table.valueptr()) != uset.end())
        subchild->set_string(1, "\xe2\x97\x8f");
    }

    child = node->add_child();
    child->set_string(0, _("Views"));
    child->set_icon_path(0, get_node_icon_path(IconViewsMany));
    std::vector<db_ViewRef> view_list = sort_db_object<db_View>(schema_list[i]->views());
    for (size_t j = 0; j < view_list.size(); ++j) {
      db_ViewRef view = view_list[j];
      mforms::TreeNodeRef subchild = child->add_child();
      subchild->set_string(0, view->name().c_str());
      subchild->set_icon_path(0, get_node_icon_path(IconView));
      subchild->set_tag(view.id());
      subchild->set_data(new ObjectNodeData(view));
      if (uset.find(view.valueptr()) != uset.end())
        subchild->set_string(1, "\xe2\x97\x8f");
    }

    child = node->add_child();
    child->set_string(0, _("Routine Groups"));
    child->set_icon_path(0, get_node_icon_path(IconRoutineGroupsMany));
    std::vector<db_RoutineGroupRef> routine_group_list =
      sort_db_object<db_RoutineGroup>(schema_list[i]->routineGroups());
    for (size_t j = 0; j < routine_group_list.size(); ++j) {
      db_RoutineGroupRef routineGrp = routine_group_list[j];
      mforms::TreeNodeRef subchild = child->add_child();
      subchild->set_string(0, routineGrp->name().c_str());
      subchild->set_icon_path(0, get_node_icon_path(IconRoutineGroup));
      subchild->set_tag(routineGrp.id());
      subchild->set_data(new ObjectNodeData(routineGrp));
      if (uset.find(routineGrp.valueptr()) != uset.end())
        subchild->set_string(1, "\xe2\x97\x8f");
    }
  }
  thaw_refresh();
  _initialized = true;
}

void CatalogTreeView::set_activate_callback(const std::function<void(grt::ValueRef)> &active_callback) {
  _activate_callback = active_callback;
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeView::context_menu_will_show(mforms::MenuItem *parent_item) {
  std::list<mforms::TreeNodeRef> selection = get_selection();

  mforms::MenuBase *parent;
  if (parent_item)
    parent = parent_item;
  else
    parent = _menu;

  parent->remove_all();

  if (!selection.empty()) {
    ObjectNodeData *odata = dynamic_cast<ObjectNodeData *>((*selection.begin())->get_data());
    if (odata != NULL) {
      grt::ValueRef value(odata->get_object_ref());
      std::string caption = "";
      if (value.is_valid()) {
        if (db_SchemaRef::can_wrap(value))
          caption = _("Edit Schema...");
        else if (db_TableRef::can_wrap(value))
          caption = _("Edit Table...");
        else if (db_ViewRef::can_wrap(value))
          caption = _("Edit View...");
        else if (db_RoutineRef::can_wrap(value))
          caption = _("Edit Routine...");
        else if (db_RoutineGroupRef::can_wrap(value))
          caption = _("Edit Routine Group...");
      }

      if (!caption.empty())
        parent->add_item_with_title(caption, std::bind(&CatalogTreeView::menu_action, this, "edit", value), "", "");
      else {
        parent->add_separator();
      }
    } else {
      parent->add_separator();
    }

  }
}
//--------------------------------------------------------------------------------------------------
void CatalogTreeView::mark_node(grt::ValueRef val, bool mark) {
  db_DatabaseObjectRef obj;
  if (db_DatabaseObjectRef::can_wrap(val))
    obj = db_DatabaseObjectRef::cast_from(val);

  if (!obj.is_valid())
    return;

  mforms::TreeNodeRef node = node_with_tag(obj.id());
  if (node.is_valid())
    node->set_string(1, mark ? "\xe2\x97\x8f" : "");
}

void CatalogTreeView::add_update_node_caption(grt::ValueRef val) {
  ObjectType otype = ObjNone;

  db_DatabaseObjectRef obj;
  if (db_DatabaseObjectRef::can_wrap(val))
    obj = db_DatabaseObjectRef::cast_from(val);

  if (!obj.is_valid())
    return;

  std::string new_name = obj->name().c_str();
  if (db_TableRef::can_wrap(val))
    otype = ObjTable;
  else if (db_RoutineGroupRef::can_wrap(val))
    otype = ObjRoutineGrp;
  else if (db_ViewRef::can_wrap(val))
    otype = ObjView;
  else if (db_SchemaRef::can_wrap(val))
    otype = ObjSchema;
  else
    return;

  mforms::TreeNodeRef node = node_with_tag(obj.id());
  if (node.is_valid()) {
    node->set_string(0, new_name);
  } else // if node is invalid it means that new object was added on diagram
  {
    node = node_with_tag(obj->owner().id());
    if (node.is_valid()) {
      mforms::TreeNodeRef prnt = node;
      node = create_new_node(otype, prnt, new_name, obj);
      workbench_physical_DiagramRef view(workbench_physical_DiagramRef::cast_from(_owner->get_model_diagram()));
      if (view->getFigureForDBObject(obj).is_valid())
        node->set_string(1, "\xe2\x97\x8f");

    } else if (db_SchemaRef::can_wrap(obj)) // check if it's schemaref
      node = create_new_node(otype, root_node(), new_name, obj);
  }

  if (node.is_valid() &&
      node->get_parent()->count() >
        1) // We check if node is valid, and if so we need to rearrange this into different position.
  {
    mforms::TreeNodeRef next_node = node->next_sibling();
    mforms::TreeNodeRef parent = node->get_parent();
    mforms::TreeNodeRef prev_node = node->previous_sibling();
    bool moved = false;
    if (next_node.is_valid()) {
      int new_idx = -1;
      while (next_node.is_valid()) {
        int ret = base::string_compare(node->get_string(0), next_node->get_string(0), false);
        if (ret > 0)
          new_idx = parent->get_child_index(next_node);
        else {
          next_node = next_node->previous_sibling(); // wee need to get back to the prev node
          break;
        }

        if (next_node->next_sibling().is_valid())
          next_node = next_node->next_sibling();
        else
          break;
      }
      if (new_idx > -1 && next_node.is_valid()) {
        node->move_node(next_node, false);
        moved = true;
      }
    }
    if (prev_node.is_valid() && !moved) {
      int new_idx = -1;
      while (prev_node.is_valid()) {
        int ret = base::string_compare(node->get_string(0), prev_node->get_string(0), false);
        if (ret < 0)
          new_idx = parent->get_child_index(prev_node);
        else {
          prev_node = prev_node->next_sibling(); // wee need to get back to the prev node
          break;
        }

        if (prev_node->previous_sibling().is_valid())
          prev_node = prev_node->previous_sibling();
        else
          break;
      }
      if (new_idx > -1 && prev_node.is_valid()) {
        node->move_node(prev_node, true);
        moved = true;
      }
    }
  }
}

void CatalogTreeView::remove_node(grt::ValueRef val) {
  db_DatabaseObjectRef obj;
  if (db_DatabaseObjectRef::can_wrap(val))
    obj = db_DatabaseObjectRef::cast_from(val);

  if (!obj.is_valid())
    return;

  mforms::TreeNodeRef node = node_with_tag(obj.id());
  if (node.is_valid())
    node->remove_from_parent();
}
//--------------------------------------------------------------------------------------------------
