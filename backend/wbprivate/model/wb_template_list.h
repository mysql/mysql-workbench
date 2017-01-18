/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "snippet_list.h"
#include "mforms/box.h"
#include "grts/structs.workbench.physical.h"

namespace wb {
  class WBContextModel;
};

namespace mforms {
  class ToolBar;
  class ToolBarItem;
  class ScrollPanel;
};

class TableTemplatePanel;

class TableTemplateList : public BaseSnippetList, public bec::ListModel {
  TableTemplatePanel *_owner;

  void prepare_context_menu();
  void menu_will_show();

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);

  virtual size_t count();
  virtual bool get_field(const bec::NodeId &node, ColumnId column, std::string &value);
  virtual void refresh();

public:
  std::string get_selected_template();
  TableTemplateList(TableTemplatePanel *owner);
  ~TableTemplateList();
};

class TableTemplatePanel : public mforms::Box {
  TableTemplateList _templates;
  mforms::ToolBar *_toolbar;
  mforms::ScrollPanel *_scroll_panel;
  wb::WBContextModel *_context;

  void toolbar_item_activated(mforms::ToolBarItem *item);

public:
  TableTemplatePanel(wb::WBContextModel *cmodel);

  void on_action(const std::string &action);
};
