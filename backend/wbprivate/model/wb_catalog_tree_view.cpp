/* 
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "stdafx.h"

#include "mforms/menubar.h"

#include "workbench/wb_context.h"

#include "wb_catalog_tree.h"
#include "grt/icon_manager.h"

#include "wb_catalog_tree_view.h"

using namespace wb;

//--------------------------------------------------------------------------------------------------

// Simple mapper of a tree node and a model node.
class CatalogData : public mforms::TreeNodeData
{
public:
  bec::NodeId id;

  CatalogData(bec::NodeId theId)
    : mforms::TreeNodeData()
  {
    id = theId;
  }
};

//--------------------------------------------------------------------------------------------------

CatalogTreeView::CatalogTreeView(CatalogTreeBE *model)
: mforms::TreeNodeView(mforms::TreeNoBorder | mforms::TreeNoHeader | mforms::TreeSizeSmall | mforms::TreeCanBeDragSource)
{
  _initialized = false;

  _model = model;
  scoped_connect(_model->tree_changed_signal(), boost::bind(&CatalogTreeView::model_changed, this, _1, _2));
  scoped_connect(_model->update_captions_signal(), boost::bind(&CatalogTreeView::update_captions, this));

  set_selection_mode(mforms::TreeSelectMultiple);
#ifdef _WIN32
  set_row_height(19);
#else
  set_row_height(17);
#endif
  add_column(mforms::IconStringColumnType, "Name", 200);
  add_column(mforms::StringColumnType, "Presence", 20);
  end_columns();

  _menu = new mforms::ContextMenu();
  _menu->signal_will_show()->connect(boost::bind(&CatalogTreeView::context_menu_will_show, this, _1));
  set_context_menu(_menu);
}

//--------------------------------------------------------------------------------------------------

CatalogTreeView::~CatalogTreeView()
{
  delete _menu;
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeView::context_menu_will_show(mforms::MenuItem *parent_item)
{
  std::list<mforms::TreeNodeRef> selection = get_selection();
  std::vector<bec::NodeId> nodes;
  for (std::list<mforms::TreeNodeRef>::const_iterator iterator = selection.begin(); iterator != selection.end(); ++iterator)
  {
    CatalogData *data = dynamic_cast<CatalogData *>((*iterator)->get_data());
    if (data != NULL)
      nodes.push_back(data->id);
  }

  if (parent_item == NULL)
    _model->update_menu_items_for_nodes(_menu, nodes);
  else
    _model->update_menu_items_for_nodes(parent_item, nodes);
}

//--------------------------------------------------------------------------------------------------

/**
 * Reads all child nodes of the given parent node (usually triggered after a model change).
 * do_default_expand is to indicated if we explicitly should expand all nodes of the first and 
 * second level.
 */
void CatalogTreeView::fill_node(mforms::TreeNodeRef parent_node, bec::NodeId parent_id)
{
  for (int count = _model->count_children(parent_id), i = 0; i < count; ++i)
  {
    bec::NodeId child = _model->get_child(parent_id, i);

    mforms::TreeNodeRef node = parent_node->add_child();
    CatalogData *data = new CatalogData(child);
    node->set_data(data);

    bec::IconId icon = _model->get_field_icon(child, 0, bec::Icon16);
    node->set_icon_path(0, bec::IconManager::get_instance()->get_icon_path(icon));

    std::string value;
    if (_model->get_field(child, 0, value))
      node->set_string(0, value);
    if (_model->get_field(child, 1, value))
      node->set_string(1, value);

    // If the model node is already expanded then also expand the tree node.
    // In order to not fill a node more than once we have to check if a node was already
    // expanded. This conflicts with the existing expand state, so we first collapse
    // (a very cheap operation) and then do the actual expand.
    // Also expand the first two node levels for easier access to the actual objects.
    if (_model->is_expanded(child) || child.depth() <= 3)
    {
      _model->collapse_node(child);
      node->expand();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeView::model_changed(bec::NodeId id, int old_child_count)
{
  refresh();
}

//--------------------------------------------------------------------------------------------------

/**
 * Updates the captions of all existing nodes. We have be tolerant here against model changes
 * we have not been notified about (e.g. expanding a node in one tree instance does not automatically
 * expand the same node in another). This is usually no problem as the catalog tree is refreshed also
 * when switching between instances. But until then model and treenodes may differ.
 */
void CatalogTreeView::update_parent_node(mforms::TreeNodeRef parent_node, bec::NodeId parent_id)
{
  for (int count = _model->count_children(parent_id), i = 0; i < count; ++i)
  {
    bec::NodeId child = _model->get_child(parent_id, i);
    mforms::TreeNodeRef node;
    if (i < parent_node->count())
      node = parent_node->get_child(i);
    if (node.is_valid())
    {
      std::string value;
      if (_model->get_field(child, 0, value))
        node->set_string(0, value);
      if (_model->get_field(child, 1, value))
        node->set_string(1, value);

      if (_model->is_expanded(child))
        update_parent_node(node, child);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeView::update_captions()
{
  if (_initialized)
    update_parent_node(root_node(), _model->get_root());
}

//--------------------------------------------------------------------------------------------------

bool CatalogTreeView::get_drag_data(mforms::DragDetails &details, void **data, std::string &format)
{
  std::list<mforms::TreeNodeRef> selection = get_selection();

  _dragged_objects.clear();
  for (std::list<mforms::TreeNodeRef>::const_iterator iterator = selection.begin();
    iterator != selection.end(); ++iterator)
  {
    CatalogData *cdata = dynamic_cast<CatalogData *>((*iterator)->get_data());
    if (cdata != NULL)
    {
      GrtObjectRef object;
      grt::ValueRef value = _model->get_node_value(cdata->id);
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

//--------------------------------------------------------------------------------------------------

void CatalogTreeView::refresh()
{
  clear();
  fill_node(root_node(), _model->get_root());
  _initialized = true;
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeView::node_activated(mforms::TreeNodeRef row, int column)
{
  CatalogData *data = dynamic_cast<CatalogData *>(row->get_data());
  if (data != NULL)
    _model->activate_node(data->id);
}

//--------------------------------------------------------------------------------------------------

bool CatalogTreeView::can_expand(mforms::TreeNodeRef row)
{
  CatalogData *data = dynamic_cast<CatalogData *>(row->get_data());
  if (data != NULL)
    return _model->is_expandable(data->id);

  return mforms::TreeNodeView::can_expand(row);
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeView::expand_toggle(mforms::TreeNodeRef row, bool expanded)
{
  TreeNodeView::expand_toggle(row, expanded);
  if (expanded)
  {
    CatalogData *data = dynamic_cast<CatalogData *>(row->get_data());
    if (data != NULL && !_model->is_expanded(data->id))
    {
      _model->expand_node(data->id);
      fill_node(row, data->id);
    }
  }
}

//--------------------------------------------------------------------------------------------------
