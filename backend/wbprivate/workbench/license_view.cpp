/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "license_view.h"

#include "wb_context.h"
#include "wb_context_ui.h"
#include "wb_version.h"

using namespace wb;
using namespace mforms;

//----------------------------------------------------------------------------------------------------------------------

LicenseView::LicenseView(WBContextUI *wbui)
: AppView(false, "License", "License", true), _wbui(wbui), _licenseText(ScrollBars::BothScrollBars) {
  add(&_licenseText, true, true);

  std::string path = App::get()->get_resource_path(
    _wbui->get_wb()->is_commercial() ? "License-commercial.txt" : "License.txt");

  if (path.empty()) { // is_commercial() also returns true for CE in development mode.
    std::string edition = base::tolower(APP_EDITION_NAME);
    if (edition == "development")
      path = App::get()->get_resource_path("License.txt");
  }

  std::string text = base::getTextFileContent(path);
  _licenseText.set_value(text);
  _licenseText.set_padding(20);
  _licenseText.set_name("License Text");
  _licenseText.setInternalName("licenseTextBox");
  _licenseText.set_read_only(true);
}

//----------------------------------------------------------------------------------------------------------------------
