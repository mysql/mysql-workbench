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

#include "lf_view.h"
#include "lf_button.h"
#include <mforms/radiobutton.h>

namespace mforms {
  namespace gtk {

    class RadioButtonImpl : public ButtonImpl {
    protected:
      Gtk::RadioButton *_radio;
      int _group_id;
      virtual Gtk::Widget *get_outer() const;

      RadioButtonImpl(::mforms::RadioButton *self, int group_id);

      static void *unregister_group(void *data);

      static void toggled(::mforms::RadioButton *self);

      static bool create(::mforms::RadioButton *self, int group_id);

      static bool get_active(::mforms::RadioButton *self);

      static void set_active(::mforms::RadioButton *self, bool flag);

      virtual void set_text(const std::string &text);

    public:
      static void init();
    };
  }
}
