/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_PRINTING_WRAPPER_H_
#define _WB_PRINTING_WRAPPER_H_

#include "wb_printing.h"

namespace MySQL {
  namespace GUI {
    namespace Workbench {
      namespace Plugins {

      public
        ref class Printing {
        public:
          static int getPageCount(MySQL::Grt::GrtValue ^ view);
          static int printPageHDC(MySQL::Grt::GrtValue ^ view, int page, System::IntPtr ^ hdc, int width, int height);

          static System::Collections::Generic::Dictionary<System::String ^, System::Object ^> ^
            getPaperSettings(MySQL::Grt::GrtValue ^ value);
        };

      } // namespace MySQL
    }   // namespace GUI
  }     // namespace Workbench
} // namespace Plugins

#endif // _WB_PRINTING_WRAPPER_H_
