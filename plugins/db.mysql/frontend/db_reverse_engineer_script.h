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

#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_progress_page.h"
#include "grtui/wizard_finished_page.h"

#include "db_rev_eng_be.h"

#include "mforms/fs_object_selector.h"
#include "mforms/table.h"
#include "mforms/label.h"
#include "mforms/selector.h"
#include "mforms/checkbox.h"

using namespace grtui;
using namespace mforms;

namespace ScriptImport {

  /**
   * Wizard page for setting up the import.
   */
  class ImportInputPage : public WizardPage {
  private:
    Table _table;
    Label _heading;
    Label _caption;
    FsObjectSelector _file_selector;
    Label _file_codeset_caption;
    Selector _file_codeset_sel;

    CheckBox _autoplaceCheck;
    CheckBox _ansiQuotesCheck;

    void fill_encodings_list();

  public:
    ImportInputPage(WizardPlugin *form);
    void file_changed();
    virtual bool allow_next();
    virtual std::string next_button_caption();
    void gather_options(bool advancing);
  };

  /**
   * Wizard page that shows the progress of the current import operation.
   */
  class ImportProgressPage : public WizardProgressPage {
  private:
    Sql_import _import_be;
    TaskRow *_auto_place_task;
    std::function<void(bool, std::string)> _finished_cb;
    bool _auto_place;
    bool _done;

  public:
    ImportProgressPage(WizardForm *form, const std::function<void(bool, std::string)> &finished_cb);
    void import_objects_finished(grt::ValueRef value);
    bool import_objects();
    bool verify_results();
    bool place_objects();
    virtual bool allow_back();
    virtual void enter(bool advancing);
    virtual void tasks_finished(bool success);
    std::string get_summary();
  };

  /**
   * The actual import wizard comprising the pages declared above and some additional stuff.
   */

  class WbPluginSQLImport : public WizardPlugin {
  private:
    ImportInputPage *_input_page;
    ImportProgressPage *_progress_page;
    WizardFinishedPage *_finish_page;

  public:
    WbPluginSQLImport(grt::Module *module);
    void update_summary(bool success, const std::string &summary);
  };

}; // namespace ScriptImport

grtui::WizardPlugin *createImportScriptWizard(grt::Module *module, db_CatalogRef catalog);

