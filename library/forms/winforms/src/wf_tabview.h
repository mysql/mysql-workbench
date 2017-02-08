/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

namespace MySQL {
  namespace Forms {
  public
    class TabViewWrapper : public ViewWrapper {
    private:
      mforms::TabViewType tabType;
      int activeIndex;
      std::vector<ViewWrapper *> pages; // Only used if the tab view is set to have no tabs.
    protected:
      TabViewWrapper(mforms::TabView *backend, mforms::TabViewType type);

      static bool create(mforms::TabView *backend, mforms::TabViewType type);
      static void set_active_tab(mforms::TabView *backend, int index);
      static int get_active_tab(mforms::TabView *backend);
      static int add_page(mforms::TabView *backend, mforms::View *page, const std::string &caption,
                          bool hasCloseButton);
      static void remove_page(mforms::TabView *backend, mforms::View *page);
      static void set_tab_title(mforms::TabView *backend, int tab, const std::string &caption);
      static void set_aux_view(mforms::TabView *backend, mforms::View *aux);
      static void set_allows_reordering(mforms::TabView *backend, bool flag);

    public:
      static void init();
    };
  };
};
