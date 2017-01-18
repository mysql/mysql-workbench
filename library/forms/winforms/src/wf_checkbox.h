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
    class CheckBoxWrapper : public ButtonWrapper {
    protected:
      CheckBoxWrapper(mforms::CheckBox *cbox);

      static bool create(mforms::CheckBox *backend, bool square);
      static void click(System::Object ^ sender, System::EventArgs ^ e);
      static void set_active(mforms::CheckBox *backend, bool flag);
      static bool get_active(mforms::CheckBox *backend);

    public:
      static void init();
      virtual int set_text(const std::string &text);
      virtual void set_font(const std::string &fontDescription);
    };
  };
};
