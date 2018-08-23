/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _STUB_TOOLBAR_H_
#define _STUB_TOOLBAR_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class ToolBarWrapper : public ViewWrapper {
    protected:
      ToolBarWrapper(::mforms::ToolBar *self)
        : ViewWrapper(self)

      {
      }

      static bool create_tool_bar(ToolBar *item, ToolBarType type) {
        return true;
      }

      static void insert_item(ToolBar *toolbar, int index, ToolBarItem *item) {
      }

      static void remove_item(ToolBar *toolbar, ToolBarItem *item) {
      }

      static bool create_tool_item(ToolBarItem *item, ToolBarItemType type) {
        return true;
      }

      static void set_item_icon(ToolBarItem *item, const std::string &) {
      }

      static void set_item_alt_icon(ToolBarItem *item, const std::string &) {
      }

      static void set_item_text(ToolBarItem *item, const std::string &) {
      }

      static std::string get_item_text(ToolBarItem *item) {
        return "";
      }

      static void set_item_enabled(ToolBarItem *item, bool) {
      }

      static bool get_item_enabled(ToolBarItem *item) {
        return true;
      }

      static void set_item_checked(ToolBarItem *item, bool) {
      }

      static bool get_item_checked(ToolBarItem *item) {
        return true;
      }

      static void set_item_tooltip(ToolBarItem *item, const std::string &) {
      }

      static void set_selector_items(ToolBarItem *item, const std::vector<std::string> &values) {
      }

      static void setItemName(ToolBarItem *item, const std::string &) {

      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_tool_bar_impl.create_tool_bar = &ToolBarWrapper::create_tool_bar;
        f->_tool_bar_impl.create_tool_item = &ToolBarWrapper::create_tool_item;
        f->_tool_bar_impl.get_item_checked = &ToolBarWrapper::get_item_checked;
        f->_tool_bar_impl.get_item_enabled = &ToolBarWrapper::get_item_enabled;
        f->_tool_bar_impl.get_item_text = &ToolBarWrapper::get_item_text;
        f->_tool_bar_impl.insert_item = &ToolBarWrapper::insert_item;
        f->_tool_bar_impl.remove_item = &ToolBarWrapper::remove_item;
        f->_tool_bar_impl.set_item_alt_icon = &ToolBarWrapper::set_item_alt_icon;
        f->_tool_bar_impl.set_item_checked = &ToolBarWrapper::set_item_checked;
        f->_tool_bar_impl.set_item_enabled = &ToolBarWrapper::set_item_enabled;
        f->_tool_bar_impl.set_item_icon = &ToolBarWrapper::set_item_icon;
        f->_tool_bar_impl.set_item_text = &ToolBarWrapper::set_item_text;
        f->_tool_bar_impl.set_item_tooltip = &ToolBarWrapper::set_item_tooltip;
        f->_tool_bar_impl.set_selector_items = &ToolBarWrapper::set_selector_items;
        f->_tool_bar_impl.set_item_name = &ToolBarWrapper::setItemName;
      }
    };
  }
}

#endif
