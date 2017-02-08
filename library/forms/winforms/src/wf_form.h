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

  private
    ref class FormFillLayout : public System::Windows::Forms::Layout::LayoutEngine {
    public:
      virtual bool Layout(Object ^ container, System::Windows::Forms::LayoutEventArgs ^ arguments) override;
      System::Drawing::Size GetPreferredSize(System::Windows::Forms::Control ^ container,
                                             System::Drawing::Size proposedSize);
    };

  public
    class FormWrapper : public ViewWrapper {
    private:
      bool hideOnClose;

    protected:
      gcroot<System::Windows::Forms::Form ^> _owner;

      FormWrapper(mforms::Form *form, mforms::Form *owner, mforms::FormFlag flag);

      static bool create(mforms::Form *backend, mforms::Form *owner, mforms::FormFlag flag);
      static void set_title(mforms::Form *backend, const std::string &title);
      static void show_modal(mforms::Form *backend, mforms::Button *accept, mforms::Button *cancel);
      static bool run_modal(mforms::Form *backend, mforms::Button *accept, mforms::Button *cancel);
      static void close(mforms::Form *backend);
      static void set_content(mforms::Form *backend, mforms::View *view);
      static void center(mforms::Form *backend);
      static void flush_events(mforms::Form *backend);
      static void end_modal(mforms::Form *backend, bool result);
      static void set_menubar(mforms::Form *backend, mforms::MenuBar *menubar);

    public:
      bool hide_on_close();

      static void init();
    };
  };
};
