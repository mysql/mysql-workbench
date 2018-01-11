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

#include "grtui/wizard_progress_page.h"

class FetchSchemaContentsSourceTargetProgressPage : public WizardProgressPage {
public:
  FetchSchemaContentsSourceTargetProgressPage(WizardForm *form, MultiSourceSelectPage *source_page,
                                              const char *name = "fetchSchema")
    : WizardProgressPage(form, name, true), _source_page(source_page) {
    set_title(_("Retrieve and Reverse Engineer Schema Objects"));
    set_short_title(_("Fetch Objects"));

    set_status_text("");
  }

  bool perform_fetch(bool source) {
    execute_grt_task(std::bind(&FetchSchemaContentsSourceTargetProgressPage::do_fetch, this, source), false);
    return true;
  }

  grt::ValueRef do_fetch(bool source) {
    grt::StringListRef selection(
      grt::StringListRef::cast_from(values().get(source ? "selectedOriginalSchemata" : "selectedSchemata")));
    std::vector<std::string> names;
    for (grt::StringListRef::const_iterator iter = selection.begin(); iter != selection.end(); ++iter)
      names.push_back(*iter);

    Db_plugin *dbplugin = source ? _source_dbplugin : _target_dbplugin;

    // tell the backend about the selection
    dbplugin->schemata_selection(names, true);

    dbplugin->load_db_objects(Db_plugin::dbotTable);
    dbplugin->load_db_objects(Db_plugin::dbotView);
    dbplugin->load_db_objects(Db_plugin::dbotRoutine);
    dbplugin->load_db_objects(Db_plugin::dbotTrigger);

    _finished++;

    return grt::ValueRef();
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      _finished = 0;
      clear_tasks();

      if (_source_page->get_left_source() == DataSourceSelector::ServerSource)
        add_async_task(_("Retrieve Source Objects from Selected Schemas"),
                       std::bind(&FetchSchemaContentsSourceTargetProgressPage::perform_fetch, this, true),
                       _("Retrieving object lists from selected schemata..."));

      if (_source_page->get_right_source() == DataSourceSelector::ServerSource)
        add_async_task(_("Retrieve Target Objects from Selected Schemas"),
                       std::bind(&FetchSchemaContentsSourceTargetProgressPage::perform_fetch, this, false),
                       _("Retrieving object lists from selected schemata..."));

      end_adding_tasks(_("Retrieval Completed Successfully"));

      reset_tasks();
    }
    WizardProgressPage::enter(advancing);
  }

  virtual bool allow_next() {
    int count = 0;
    if (_source_page->get_left_source() == DataSourceSelector::ServerSource)
      count++;
    if (_source_page->get_right_source() == DataSourceSelector::ServerSource)
      count++;
    return _finished == count;
  }

  void set_db_plugin(Db_plugin *source_dbplugin, Db_plugin *target_dbplugin) {
    _source_dbplugin = source_dbplugin;
    _target_dbplugin = target_dbplugin;
  }

private:
  MultiSourceSelectPage *_source_page;
  Db_plugin *_source_dbplugin;
  Db_plugin *_target_dbplugin;
  int _finished;
};
