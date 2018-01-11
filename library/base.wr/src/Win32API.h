/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

// Helper class to access native Windows functions. Avoids using P/Invoke from C#.
// In the mid term, remove P/Invoke code from Win32.cs and do native calls here.

namespace MySQL {
  namespace Utilities {
    namespace SysUtils {

    public
      ref class Win32Api {
      public:
        // Clipboard operations for editable combo boxes.
        static void Undo(System::Windows::Forms::ComboBox ^ box);
        static void Cut(System::Windows::Forms::ComboBox ^ box);
        static void Copy(System::Windows::Forms::ComboBox ^ box);
        static void Paste(System::Windows::Forms::ComboBox ^ box);
        static bool CanUndo(System::Windows::Forms::ComboBox ^ box);

        static bool UnblockWorkbenchFiles(System::String ^ folder);
        static bool UnblockFile(System::String ^ file);

        static bool RedirectConsole();
        static void ReleaseConsole();
      };

    } // SysUtilits
  }   // Utilities
} // MySQL
