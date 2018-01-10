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

//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef _OVERVIEW_PANEL_H_
#define _OVERVIEW_PANEL_H_

#include <map>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/box.h>
#include <gtkmm/menu.h>

#include "workbench/wb_overview.h"
#include "workbench/wb_context_ui.h"

class OverviewDivision;
class OverviewSection;
class OverviewItemContainer;
class OverviewGroupContainer;

class OverviewGroupContainer;
class MultiView;

class OverviewPanel : public Gtk::ScrolledWindow {
public:
  OverviewPanel(wb::OverviewBE *overview);

  void reset();
  void rebuild_all();
  void update_for_resize();

  void select_node(const bec::NodeId &node);
  void refresh_node(const bec::NodeId &node);
  void refresh_children(const bec::NodeId &node);

  wb::OverviewBE *get_be() {
    return _overview_be;
  }

  void item_popup_menu(const Gtk::TreeModel::Path &path, guint32 time, OverviewItemContainer *sender);

  virtual bool on_close() {
    if (!_overview_be->can_close())
      return false;
    _overview_be->close();
    return false;
  }

  void select_default_group_page();
  void refresh_active_group_node_children();

private:
  Gtk::Box *_container;
  OverviewGroupContainer *_groups;
  Gtk::Menu _context_menu;

  std::map<std::string, OverviewGroupContainer *> _group_containers_by_id;
  std::map<std::string, OverviewItemContainer *> _item_containers_by_id;

  wb::OverviewBE *_overview_be;

  bool _freeze;
  bool _rebuilding;

  void pre_refresh_groups();

  void update_group_note(OverviewGroupContainer *group_container, const bec::NodeId &node);

  void build_division(Gtk::Box *container, const bec::NodeId &node);
  void build_group(OverviewDivision *division, OverviewGroupContainer *group_container, const bec::NodeId &node,
                   int position = -1);
  void build_group_contents(OverviewDivision *division, Gtk::Box *page, const bec::NodeId &node);

  void item_list_selection_changed(const std::vector<bec::NodeId> &nodes, MultiView *mview);
};

#endif /* _OVERVIEW_PANEL_H_ */

//!
//! @}
//!
