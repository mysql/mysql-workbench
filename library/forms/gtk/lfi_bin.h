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
#ifndef _LFI_BIN_H_
#define _LFI_BIN_H_

#include "lf_view.h"

namespace mforms {
  namespace gtk {
    class BinImpl {
      ViewImpl *_view;

    protected:
      BinImpl(ViewImpl *view) : _view(view) {
      }

      void add(::mforms::View *child) {
        ((Gtk::Container *)_view->get_inner())->add(*child->get_data<ViewImpl>()->get_outer());
        child->show();
      }

      void remove() {
        ((Gtk::Bin *)_view->get_inner())->remove();
      }

      void remove(::mforms::View *child) {
        ((Gtk::Container *)_view->get_inner())->remove(*child->get_data<ViewImpl>()->get_outer());
      }
    };
  };
};

#endif /* _LFI_BIN_H_ */
