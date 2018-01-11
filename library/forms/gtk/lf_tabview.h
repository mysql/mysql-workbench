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
