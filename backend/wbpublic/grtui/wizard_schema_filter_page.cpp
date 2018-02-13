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

#include "wizard_schema_filter_page.h"

#include "grt/icon_manager.h"
#include "grt/common.h"
#include "grt/grt_manager.h"

/**
 * @file  wizard_schema_filter_page.cpp
 * @brief
 */

using namespace grtui;

WizardSchemaFilterPage::WizardSchemaFilterPage(WizardForm *form, const char *name)
  : WizardPage(form, name), _header(true) {
  _header.set_spacing(4);

  _image.set_image(bec::IconManager::get_instance()->get_icon_path("db.Schema.32x32.png"));
  _header.add(&_image, false, true);

  _label.set_text_align(mforms::MiddleLeft);
  _label.set_text(_("Select the schemas you want to include:"));
  _label.set_style(mforms::BoldStyle);
  _header.add(&_label, true, true);

  add(&_header, false, true);

  add(&_check_list, true, true);

  scoped_connect(_check_list.signal_changed(), std::bind(&WizardSchemaFilterPage::validate, this));
}

void WizardSchemaFilterPage::enter(bool advancing) {
  if (advancing)
    _check_list.set_strings(grt::StringListRef::cast_from(values().get("schemata")));
}

void WizardSchemaFilterPage::leave(bool advancing) {
  if (advancing) {
    grt::StringListRef list(grt::Initialized);
    std::vector<std::string> selection = _check_list.get_selection();

    for (std::vector<std::string>::const_iterator iter = selection.begin(); iter != selection.end(); ++iter)
      list.insert(*iter);

    values().set("selectedSchemata", list);
  }
}

bool WizardSchemaFilterPage::allow_next() {
  return _check_list.has_selection();
}
