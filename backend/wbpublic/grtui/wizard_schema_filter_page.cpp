/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
  _header.add(&_image, false);

  _label.set_text_align(mforms::MiddleLeft);
  _label.set_text(_("Select the schemas below you want to include:"));
  _label.set_style(mforms::BoldStyle);
  _header.add(&_label, true, true);

  add(&_header, false, false);

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
