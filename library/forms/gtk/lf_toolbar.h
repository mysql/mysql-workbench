/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MFORMS_LF_TOOLBAR_H_
#define _MFORMS_LF_TOOLBAR_H_

#include "mforms/toolbar.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <gtkmm.h>
#pragma GCC diagnostic pop

namespace mforms {
  Gtk::Widget *widget_for_toolbar(mforms::ToolBar *);
  Gtk::Widget *widget_for_toolbar_item_named(mforms::ToolBar *, const std::string &);

  namespace gtk {

    void lf_toolbar_init();
    struct ToolBarImpl {
      static bool create_tool_bar(ToolBar *item, ToolBarType type);
      static void insert_item(ToolBar *toolbar, int index, ToolBarItem *item);
      static void remove_item(ToolBar *toolbar, ToolBarItem *item);

      static bool create_tool_item(ToolBarItem *item, ToolBarItemType type);
      static void set_item_icon(ToolBarItem *item, const std::string &);
      static void set_item_alt_icon(ToolBarItem *item, const std::string &);
      static void set_item_text(ToolBarItem *item, const std::string &);
      static std::string get_item_text(ToolBarItem *item);
      static void set_item_name(ToolBarItem *item, const std::string &);
      static void set_item_enabled(ToolBarItem *item, bool);
      static bool get_item_enabled(ToolBarItem *item);
      static void set_item_checked(ToolBarItem *item, bool);
      static bool get_item_checked(ToolBarItem *item);
      static void set_item_tooltip(ToolBarItem *item, const std::string &);

      static void set_selector_items(ToolBarItem *item, const std::vector<std::string> &values);
    };

  } // ns gtk
} // ns mforms

#endif
