/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

class FetchSchemaNamesProgressPage : public grtui::WizardProgressPage {
public:
  FetchSchemaNamesProgressPage(grtui::WizardForm *form, const char *name = "fetchNames")
    : WizardProgressPage(form, name, true), _dbconn(0) {
    set_title(_("Connect to DBMS and Fetch Information"));
    set_short_title(_("Connect to DBMS"));

    add_async_task(_("Connect to DBMS"), std::bind(&FetchSchemaNamesProgressPage::perform_connect, this),
                   _("Connecting to DBMS..."));

    add_async_task(_("Retrieve Schema List from Database"),
                   std::bind(&FetchSchemaNamesProgressPage::perform_fetch, this),
                   _("Retrieving schema list from database..."));

    add_async_task(_("Check Common Server Configuration Issues"),
                   std::bind(&FetchSchemaNamesProgressPage::perform_check_case, this),
                   _("Checking common server configuration issues..."));

    end_adding_tasks(_("Execution Completed Successfully"));

    set_status_text("");
  }

  void set_db_connection(DbConnection *dbc) {
    _dbconn = dbc;
  }

  void set_load_schemas_slot(const std::function<std::vector<std::string>()> &slot) {
    _load_schemas = slot;
  }

  void set_check_case_slot(const std::function<int()> &slot) {
    _check_case_problems = slot;
  }

protected:
  bool perform_connect() {
    db_mgmt_ConnectionRef conn = _dbconn->get_connection();

    execute_grt_task(std::bind(&FetchSchemaNamesProgressPage::do_connect, this), false);

    return true;
  }

  grt::ValueRef do_connect() {
    if (!_dbconn)
      throw std::logic_error("must call set_db_connection() 1st");
    _dbconn->test_connection();

    return grt::ValueRef();
  }

  bool perform_fetch() {
    execute_grt_task(std::bind(&FetchSchemaNamesProgressPage::do_fetch, this), false);
    return true;
  }

  static bool collate(const std::string &a, const std::string &b) {
    return g_utf8_collate(a.c_str(), b.c_str()) < 0;
  }

  grt::ValueRef do_fetch() {
    std::vector<std::string> schema_names = _load_schemas();

    // order the schema names alphabetically
    std::sort(schema_names.begin(), schema_names.end(),
              std::bind(&FetchSchemaNamesProgressPage::collate, std::placeholders::_1, std::placeholders::_2));
    grt::StringListRef list(grt::Initialized);
    for (std::vector<std::string>::const_iterator iter = schema_names.begin(); iter != schema_names.end(); ++iter)
      list.insert(*iter);

    values().set("schemata", list);

    return grt::ValueRef();
  }

  bool perform_check_case() {
    execute_grt_task(std::bind(&FetchSchemaNamesProgressPage::do_check_case, this), false);
    return true;
  }

  grt::ValueRef do_check_case() {
    if (_check_case_problems) {
      int resp = _check_case_problems();
      if (resp == -1)
        grt::GRT::get()->send_info(_("Server configuration check"),
                                   _("Unable to check for server case-sensitivity issues."));
      else if (resp == 1)
        grt::GRT::get()->send_warning(_("Server configuration check"),
                                      _("A server configuration problem was detected.\nThe server is in a system that "
                                        "does not properly support the selected lower_case_table_names option value. "
                                        "Some problems may occur.\nPlease consult the MySQL server documentation."));
    }
    _finished = true;

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

private:
  DbConnection *_dbconn;
  std::function<std::vector<std::string>()> _load_schemas;
  std::function<int()> _check_case_problems;
  bool _finished;
};
