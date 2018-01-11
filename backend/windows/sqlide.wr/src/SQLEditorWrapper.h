/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "sqlide/sql_editor_be.h"

using namespace System;
using namespace System::Windows::Forms;

namespace mforms {
  class CodeEditor;
  class DockingPoint;
}

namespace MySQL {
  namespace GUI {
    namespace Workbench {

    public
      ref class SqlEditorWrapper {
      public:
        typedef MySQL::Grt::ManagedRef<MySQLEditor> ^ Ref;

      private:
        Ref _ref;
        mforms::DockingPoint *_result_docking_point; // Holder of the docking point delegate we get from the front end.
        MySQL::Forms::ManagedDockDelegate ^ _managed_result_dock_delegate;

      public:
        SqlEditorWrapper(IntPtr nref_ptr);
        ~SqlEditorWrapper();

        void set_result_docking_delegate(MySQL::Forms::ManagedDockDelegate ^ theDelegate);

        Ref ref() {
          return _ref;
        }
        System::Windows::Forms::Control ^ get_editor_container();
        System::Windows::Forms::Control ^ get_editor_control();

        bool is_refresh_enabled() {
          return _ref->is_refresh_enabled();
        }
        void set_refresh_enabled(bool val) {
          _ref->set_refresh_enabled(val);
        }
        bool is_sql_check_enabled() {
          return _ref->is_sql_check_enabled();
        }
        void set_sql_check_enabled(bool val) {
          _ref->set_sql_check_enabled(val);
        }

        void set_language(String ^ language);

        void append_text(String ^ text);
        void set_text(String ^ text);
        void focus();

        static SqlEditorWrapper ^ get_sql_editor(MySQL::Grt::BaseEditorWrapper ^ wrapper);
      };

    }; // namespace Workbench
  };   // namespace GUI
};     // namespace MySQL
