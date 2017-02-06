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
#ifndef _LF_NATIVE_H_
#define _LF_NATIVE_H_

#include "mforms/native.h"

#include "lf_view.h"

namespace mforms {
  namespace gtk {

    class NativeContainerImpl : public ViewImpl {
      Gtk::Widget *_widget;

    public:
      virtual Gtk::Widget *get_outer() const {
        return _widget;
      }

      NativeContainerImpl(::mforms::NativeContainer *self, Gtk::Widget *w);
    };
  };

  mforms::NativeContainer *native_from_widget(Gtk::Widget *w);
};

#endif
