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

    class AppViewWrapper;

  public
    ref class AppViewDockContent : MySQL::Controls::TabDocument {
    private:
      mforms::AppView *appview;

    public:
      AppViewDockContent();
      ~AppViewDockContent();

      void SetBackend(mforms::AppView *backend);

      System::String ^ GetAppViewIdentifier();
      System::String ^ GetContextName();

      mforms::AppView *GetBackend();
      System::Windows::Forms::MenuStrip ^ GetMenuBar();
      System::Windows::Forms::ToolStrip ^ GetToolBar();

      String ^ GetTitle();
      void SetTitle(String ^ title);

      bool CanCloseDocument();
      void CloseDocument();

      void UpdateColors();
    };

  public
    class AppViewWrapper : public BoxWrapper {
    private:
      mforms::AppView *appview;
      gcroot<AppViewDockContent ^> host;

    protected:
      AppViewWrapper(mforms::AppView *backend);
      ~AppViewWrapper();

      static bool create(mforms::AppView *backend, bool horizontal);

    public:
      AppViewDockContent ^ GetHost();
      static void init();
    };
  };
};
