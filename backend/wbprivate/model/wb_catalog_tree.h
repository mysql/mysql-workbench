/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_CATALOG_TREE_H_
#define _WB_CATALOG_TREE_H_

#include "grt/grt_value_tree.h"
#ifndef _WIN32
#include <set>
#endif

namespace wb 
{
  class WBComponentPhysical;

  class CatalogTreeBE : public bec::ValueTreeBE
  {
  private:
    typedef bec::ValueTreeBE super;
    #ifdef _WIN32
    typedef stdext::hash_set<grt::internal::Value*> HashSet;
    #else
    typedef std::set<grt::internal::Value*> HashSet;
    #endif

    struct sortnode 
    {
      bool operator ()(Node *a, Node *b) const
      {
        return a->name < b->name;
      }
    };

    HashSet _current_diagram_objects;
    WBComponentPhysical *_owner;
    boost::signals2::signal<void ()> _update_captions_signal;

    virtual void rescan_node(const bec::NodeId &node_id, bec::ValueTreeBE::Node *node, const std::string &path, const grt::ObjectRef &value);
    virtual void rescan_node(const bec::NodeId &node_id, bec::ValueTreeBE::Node *node, const std::string &path, const grt::BaseListRef &value);

    void menu_action(const std::string &name, const std::vector<bec::NodeId> &nodes);

  public:
    CatalogTreeBE(grt::GRT *grt, WBComponentPhysical *owner);

    void refresh_for_diagram(const workbench_physical_DiagramRef &view);
    void update_captions();

    virtual bool get_field(const bec::NodeId &node_id, int column, std::string &value);

    virtual void update_menu_items_for_nodes(mforms::MenuBase *parent, const std::vector<bec::NodeId> &nodes);

    boost::signals2::signal<void ()>* update_captions_signal() { return &_update_captions_signal; }
  };
};

#endif
