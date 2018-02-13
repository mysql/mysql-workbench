/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtui/wizard_schema_filter_page.h"

class SchemaSelectionPage : public WizardSchemaFilterPage {
public:
  SchemaSelectionPage(WizardForm *form, const char *name = "pickSchemata")
    : WizardSchemaFilterPage(form, name), _dbplugin(0) {
    set_short_title(_("Select Schemas"));
    set_title(_("Select Schemas to Reverse Engineer"));
  }

  virtual void leave(bool advancing) {
    if (advancing) {
      grt::StringListRef list(grt::Initialized);
      std::vector<std::string> selection = _check_list.get_selection();

      for (std::vector<std::string>::const_iterator iter = _schemas.begin(); iter != _schemas.end(); ++iter)
        if (std::find(selection.begin(), selection.end(), *iter) == selection.end())
          list.insert(*iter);
      values().set("unSelectedSchemata", list);
    }
    WizardSchemaFilterPage::leave(advancing);
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      _schemas.clear();
      grt::ListRef<db_Schema> schemas(_dbplugin->model_catalog()->schemata());
      GRTLIST_FOREACH(db_Schema, schemas, schema)
      _schemas.push_back((*schema)->name());

      WizardSchemaFilterPage::enter(advancing);

      for (std::vector<std::string>::const_iterator iter = _schemas.begin(); iter != _schemas.end(); ++iter)
        _check_list.set_selected(*iter, true);
    }
  }

  void set_db_plugin(Db_plugin *pl) {
    _dbplugin = pl;
  }

private:
  std::vector<std::string> _schemas;
  Db_plugin *_dbplugin;
};
