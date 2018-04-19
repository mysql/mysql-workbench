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

#include "grtui/wizard_progress_page.h"
#include "db_plugin_be.h"

class FetchSchemaContentsProgressPage : public grtui::WizardProgressPage {
public:
  FetchSchemaContentsProgressPage(grtui::WizardForm *form, const char *name = "fetchSchema")
  : grtui::WizardProgressPage(form, name, true) {
    set_title(_("Retrieve and Reverse Engineer Schema Objects"));
    set_short_title(_("Retrieve Objects"));

    add_async_task(_("Retrieve Objects from Selected Schemas"),
                   std::bind(&FetchSchemaContentsProgressPage::perform_fetch, this),
                   _("Retrieving object lists from selected schemas..."));

    add_task(_("Check Results"), std::bind(&FetchSchemaContentsProgressPage::perform_check, this),
             _("Checking Retrieved data..."));

    end_adding_tasks(_("Retrieval Completed Successfully"));

    set_status_text("");
  }

  bool perform_fetch() {
    execute_grt_task(std::bind(&FetchSchemaContentsProgressPage::do_fetch, this), false);
    return true;
  }

  bool perform_check() {
    _finished = true;

    return true;
  }

  grt::ValueRef do_fetch() {
    grt::StringListRef selection(grt::StringListRef::cast_from(values().get("selectedSchemata")));
    std::vector<std::string> names;

    for (grt::StringListRef::const_iterator iter = selection.begin(); iter != selection.end(); ++iter)
      names.push_back(*iter);

    // tell the backend about the selection
    _dbplugin->schemata_selection(names, true);

    _dbplugin->load_db_objects(Db_plugin::dbotTable);
    _dbplugin->load_db_objects(Db_plugin::dbotView);
    if (!values().get_int("SkipRoutines"))
      _dbplugin->load_db_objects(Db_plugin::dbotRoutine);
    if (!values().get_int("SkipTriggers"))
      _dbplugin->load_db_objects(Db_plugin::dbotTrigger);

    return grt::ValueRef();
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      _finished = false;
      reset_tasks();
    }
    WizardProgressPage::enter(advancing);
  }

  virtual bool allow_next() {
    return _finished;
  }

  void set_db_plugin(Db_plugin *dbplugin) {
    _dbplugin = dbplugin;
  }

private:
  Db_plugin *_dbplugin;
  bool _finished;
};
