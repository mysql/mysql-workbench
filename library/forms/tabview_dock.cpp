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

#include "mforms/tabview_dock.h"
#include "mforms/appview.h"

using namespace mforms;

void TabViewDockingPoint::dock_view(mforms::AppView *view, const std::string &arg1, int arg2) {
  _tabview->add_page(view, view->get_title());
}

bool TabViewDockingPoint::select_view(mforms::AppView *view) {
  int i;
  if ((i = _tabview->get_page_index(view)) < 0)
    return false;
  _tabview->set_active_tab(i);
  return true;
}

mforms::AppView *TabViewDockingPoint::selected_view() {
  int i = _tabview->get_active_tab();
  if (i >= 0)
    return dynamic_cast<mforms::AppView *>(_tabview->get_page(i));
  return NULL;
}

void TabViewDockingPoint::undock_view(mforms::AppView *view) {
  _tabview->remove_page(view);
}

void TabViewDockingPoint::set_view_title(mforms::AppView *view, const std::string &title) {
  int i;
  if ((i = _tabview->get_page_index(view)) < 0)
    return;
  _tabview->set_tab_title(i, title);
}

std::pair<int, int> TabViewDockingPoint::get_size() {
  return std::make_pair(_tabview->get_width(), _tabview->get_height());
}

int TabViewDockingPoint::view_count() {
  return _tabview->page_count();
}

AppView *TabViewDockingPoint::view_at_index(int index) {
  return dynamic_cast<AppView *>(_tabview->get_page(index));
}
