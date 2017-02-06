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

#include "mforms/dockingpoint.h"
#include "mforms/tabview.h"

namespace mforms {

  class MFORMS_EXPORT TabViewDockingPoint : public mforms::DockingPointDelegate {
  protected:
    TabView *_tabview;
    std::string _type;

    void close_page(mforms::AppView *page);

  public:
    TabViewDockingPoint(TabView *tv, const std::string &type) : _tabview(tv), _type(type) {
    }

    virtual std::string get_type() {
      return _type;
    }

    virtual void dock_view(mforms::AppView *view, const std::string &arg1, int arg2);
    virtual bool select_view(mforms::AppView *view);
    virtual void undock_view(mforms::AppView *view);
    virtual void set_view_title(mforms::AppView *view, const std::string &title);
    virtual std::pair<int, int> get_size();

    virtual AppView *selected_view();
    virtual int view_count();
    virtual AppView *view_at_index(int index);
  };
};
