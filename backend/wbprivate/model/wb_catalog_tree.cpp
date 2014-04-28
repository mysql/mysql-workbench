/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"

#include "grt/common.h"
#include "grts/structs.db.h"
#include "grts/structs.workbench.physical.h"

#include "wb_catalog_tree.h"
#include "wb_component_physical.h"
#include "workbench/wb_context.h"

#include "mforms/menubar.h"

using namespace wb;
using namespace bec;

//--------------------------------------------------------------------------------------------------

CatalogTreeBE::CatalogTreeBE(grt::GRT *grt, WBComponentPhysical *owner)
  : ValueTreeBE(grt), _owner(owner)
{
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeBE::rescan_node(const NodeId &node_id, Node *node, const std::string &path, const grt::ObjectRef &value)
{
  node->reset_children();

  if (value.is_instance(db_Schema::static_class_name()))
  {
    Node *subnode;

    subnode= new Node();
    subnode->expandable= true;
    subnode->name= _("Tables");
    subnode->path= "tables";

    std::string struct_name= db_Table::static_class_name();
    subnode->small_icon= IconManager::get_instance()->get_icon_id(struct_name + ".$.png", bec::Icon16, "many");
    subnode->large_icon= IconManager::get_instance()->get_icon_id(struct_name + ".$.png", bec::Icon48, "many");
    node->subnodes.push_back(subnode);

    subnode= new Node();
    subnode->expandable= true;
    subnode->name= _("Views");
    subnode->path= "views";

    struct_name= db_View::static_class_name();
    subnode->small_icon= IconManager::get_instance()->get_icon_id(struct_name + ".$.png", bec::Icon16, "many");
    subnode->large_icon= IconManager::get_instance()->get_icon_id(struct_name + ".$.png", bec::Icon48, "many");
    node->subnodes.push_back(subnode);

    subnode= new Node();
    subnode->expandable= true;
    subnode->name= _("Routine Groups");
    subnode->path= "routineGroups";

    struct_name= db_Routine::static_class_name();
    subnode->small_icon= IconManager::get_instance()->get_icon_id(std::string(struct_name) + ".$.png", bec::Icon16, "many");
    subnode->large_icon= IconManager::get_instance()->get_icon_id(std::string(struct_name) + ".$.png", bec::Icon48, "many");
    node->subnodes.push_back(subnode);
  }
  else if (value.is_instance(db_Table::static_class_name()))
  {
    // list PKs from the table
    db_TableRef table(db_TableRef::cast_from(value));
    if (table->primaryKey().is_valid())
    {
      grt::ListRef<db_IndexColumn> columns(table->primaryKey()->columns());

      if (columns.is_valid())
      {
        for (size_t c= columns.count(), i= 0; i < c; i++)
        {
          db_IndexColumnRef column(columns.get(i));
          char buffer[50];
          snprintf(buffer, sizeof(buffer), "primaryKey/columns/%li/referencedColumn", (long)i);
          Node *child= new Node();
          child->path= buffer;
          child->name= column->referencedColumn()->name();
          child->expandable= false;
          child->small_icon= IconManager::get_instance()->get_icon_id("db.Column.pk.11x11.png");

          node->subnodes.push_back(child);
        }
      }
    }
  }
  else if (value.is_instance(db_View::static_class_name()))
  {
    ;
  }
  else if (value.is_instance(db_RoutineGroup::static_class_name()))
  {
    // list routines from group
    grt::ListRef<db_Routine> routines(db_RoutineGroupRef::cast_from(value)->routines());

    for (size_t c= routines.count(), i= 0; i < c; i++)
    {
      db_RoutineRef routine(routines.get(i));
      char buffer[30];
      snprintf(buffer, sizeof(buffer), "routines/%li", (long)i);
      Node *child= new Node();
      child->path= buffer;
      child->name= routine->name();
      child->expandable= false;
      child->small_icon= IconManager::get_instance()->get_icon_id(routine);

      node->subnodes.push_back(child);
    }
  }
  else
  {
    /*
    if (!db_RoutineRef::can_wrap(value) && !db_ViewRef::can_wrap(value))
    {
      printf("RESCAN %s\n",value.class_name().c_str());
      bec::ValueTreeBE::rescan_node(node_id, node, path, value);
    }
    else*/
      node->expandable= false;
  }
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeBE::rescan_node(const bec::NodeId &node_id, bec::ValueTreeBE::Node *node, const std::string &path, const grt::BaseListRef &value)
{
  super::rescan_node(node_id, node, path, value);

  grt::MetaClass *content= _grt->get_metaclass(value.content_class_name());

  if (content->is_a(db_Table::static_class_name()))
  {
    std::sort(node->subnodes.begin(), node->subnodes.end(), sortnode());
  }
  else if (content->is_a(db_Column::static_class_name())
           || content->is_a(db_Routine::static_class_name()))
  {
    for (std::vector<bec::ValueTreeBE::Node*>::iterator iter= node->subnodes.begin(); 
      iter != node->subnodes.end(); ++iter)
      (*iter)->expandable= false;

    std::sort(node->subnodes.begin(), node->subnodes.end(), sortnode());
  }
  else if (content->is_a(db_View::static_class_name())
           || content->is_a(db_RoutineGroup::static_class_name()))
  {
    for (std::vector<bec::ValueTreeBE::Node*>::iterator iter= node->subnodes.begin();
      iter != node->subnodes.end(); ++iter)
      (*iter)->expandable= false;

    std::sort(node->subnodes.begin(), node->subnodes.end(), sortnode());
  }
}

//--------------------------------------------------------------------------------------------------

bool CatalogTreeBE::get_field(const bec::NodeId &node_id, int column, std::string &value)
{
  if (column != 1)
    return super::get_field(node_id, column, value);
  else
  {
    grt::ValueRef v(get_node_value(node_id));
    if (_current_diagram_objects.find(v.valueptr()) != _current_diagram_objects.end())
      value= "\xe2\x97\x8f";
    else
      value= "";

    return true;
  }
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeBE::refresh_for_diagram(const workbench_physical_DiagramRef &view)
{
  _current_diagram_objects.clear();

  if (view.is_valid())
  {
    grt::ListRef<model_Figure> figures(view->figures());
    for (size_t c= figures.count(), i= 0; i < c; i++)
    {
      model_FigureRef f(figures[i]);

      if (f.has_member("table"))
        _current_diagram_objects.insert(f.get_member("table").valueptr());
      else if (f.has_member("view"))
        _current_diagram_objects.insert(f.get_member("view").valueptr());
      else if (f.has_member("routine"))
        _current_diagram_objects.insert(f.get_member("routine").valueptr());
      else if (f.has_member("routineGroup"))
        _current_diagram_objects.insert(f.get_member("routineGroup").valueptr());
    }

    update_captions();
  }
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeBE::update_captions()
{
  _update_captions_signal();
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeBE::update_menu_items_for_nodes(mforms::MenuBase *parent, const std::vector<bec::NodeId> &nodes)
{
  parent->remove_all();
  mforms::MenuItem *item;

  if (nodes.size() == 1)
  {
    grt::ValueRef value(get_node_value(nodes[0]));

    std::string caption = "";
    if (value.is_valid())
    {
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
    {
      item = parent->add_item_with_title(caption, boost::bind(&CatalogTreeBE::menu_action, this, "edit", nodes));
      parent->add_separator();
    }
  }

  item = parent->add_item_with_title(_("Refresh"), boost::bind(&CatalogTreeBE::menu_action, this, "refresh", nodes), "refresh");
  item->set_enabled(true);
}

//--------------------------------------------------------------------------------------------------

void CatalogTreeBE::menu_action(const std::string &name, const std::vector<bec::NodeId> &unsorted_nodes)
{
  if (name == "refresh")
  {
    refresh();
    return;
  }

  if (name == "edit")
  {
    GrtObjectRef object(GrtObjectRef::cast_from(get_node_value(unsorted_nodes[0])));
    if (object.is_valid())
      _owner->get_wb()->get_grt_manager()->open_object_editor(object);
    return;
  }
}

//--------------------------------------------------------------------------------------------------
