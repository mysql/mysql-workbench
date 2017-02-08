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
#ifndef _LF_TABVIEW_H_
#define _LF_TABVIEW_H_

#include "lf_view.h"
#include "mforms/tabview.h"
namespace mforms {
  namespace gtk {

    class TabViewImpl : public ViewImpl {
      Gtk::Notebook *_nb;
      bool _reorderable;
      virtual Gtk::Widget *get_outer() const {
        return _nb;
      }

    protected:
      TabViewImpl(::mforms::TabView *self, ::mforms::TabViewType tabType);
      virtual ~TabViewImpl();

      void tab_changed(Gtk::Widget *, guint);
      void tab_reordered(Gtk::Widget *page, guint to);

      void close_tab_clicked(mforms::View *page);

      static bool create(::mforms::TabView *self, mforms::TabViewType tabtype);
      static void set_active_tab(::mforms::TabView *self, int index);
      static int get_active_tab(::mforms::TabView *self);
      static int add_page(::mforms::TabView *self, ::mforms::View *page, const std::string &caption,
                          bool hasCloseButton);
      static void remove_page(::mforms::TabView *self, ::mforms::View *page);
      static void set_tab_title(::mforms::TabView *self, int tab, const std::string &title);
      static void set_aux_view(mforms::TabView *self, mforms::View *view);

      static void set_allows_reordering(mforms::TabView *self, bool flag);

    public:
      static void init();
    };
  };
};

#endif
