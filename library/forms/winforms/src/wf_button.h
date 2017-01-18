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

    /*
     * Implements a wrapper class around a .NET button control, so it can be used by the mforms library.
     */
  public
    class ButtonWrapper : public ViewWrapper {
    private:
      gcroot<System::Windows::Forms::Button ^> button;
      bool internal_padding;
      void enable_internal_padding(bool flag);

    protected:
      ButtonWrapper(mforms::Button *backend);

      static bool create(mforms::Button *backend, mforms::ButtonType btype);
      static void set_text(mforms::Button *backend, const std::string &text);
      static void set_icon(mforms::Button *backend, const std::string &path);
      static void button_click(System::Object ^ sender, System::EventArgs ^ e);
      static void enable_internal_padding(mforms::Button *backend, bool flag);

    public:
      static void init();
      virtual int set_text(const std::string &text);

      bool uses_internal_padding();
    };
  };
};
