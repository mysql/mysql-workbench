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

#pragma once

namespace MySQL {
  namespace Forms {

  private
    ref class TextBoxEx : public System::Windows::Forms::TextBox {
    private:
      mforms::ModifierKey modifiers; // Converted modifier keys for key down and key press events.
    protected:
      virtual bool ProcessCmdKey(System::Windows::Forms::Message % msg, System::Windows::Forms::Keys keyData) override;
      virtual void OnTextChanged(EventArgs ^ args) override;
      virtual void OnKeyDown(System::Windows::Forms::KeyEventArgs ^ args) override;
      virtual void OnKeyPress(System::Windows::Forms::KeyPressEventArgs ^ args) override;
    };

  public
    class TextBoxWrapper : public ViewWrapper {
    protected:
      TextBoxWrapper(mforms::TextBox *text);

      static bool create(mforms::TextBox *backend, mforms::ScrollBars scroll_bars);
      static void set_text(mforms::TextBox *backend, const std::string &text);
      static void append_text(mforms::TextBox *backend, const std::string &text, bool scroll_to_end);
      static std::string get_text(mforms::TextBox *backend);
      static void set_read_only(mforms::TextBox *backend, bool flag);
      static void set_padding(mforms::TextBox *backend, int pad);
      static void set_bordered(mforms::TextBox *backend, bool flag);
      static void set_monospaced(mforms::TextBox *backend, bool flag);
      static void get_selected_range(mforms::TextBox *backend, int &start, int &end);
      static void clear(mforms::TextBox *backend);

    public:
      static void init();
    };
  };
};
