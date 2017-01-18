/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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
