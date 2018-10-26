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

#include "db_alter_script_be.h"

#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_view_text_page.h"

#include "../backend/db_plugin_be.h"
#include "base/string_utilities.h"
#include "base/log.h"

using namespace grtui;
using namespace mforms;
using namespace base;

#include "grtui/connection_page.h"
#include "schema_matching_page.h"
#include "fetch_schema_contents_page.h"
#include "synchronize_differences_page.h"

class DescriptionPage : public WizardPage {
private:
  mforms::Label _description_text;
  mforms::CheckBox _show_description_check;

public:
  DescriptionPage(WizardForm *form) : WizardPage(form, "intro") {
    set_title(_("Introduction"));
    set_short_title(_("Introduction"));
    _description_text.set_wrap_text(true);
    _description_text.set_text(
      "This wizard allows you to compare a target database or script "
      "with the open model, external script or a second database and "
      "apply these changes back to the target.\n"
      "It's also possible to export the ALTER script generated to a "
      "file for executing it afterwards.\n"
      "The changes are applied one way only, to the target database "
      "and the source is left untouched.");
    add(&_description_text, false, true);
    _show_description_check.set_text("Always show this page");
    _show_description_check.set_active(
      bec::GRTManager::get()->get_app_option_int("db.mysql.synchronizeAny:show_sync_help_page", 1) != 0);
    add_end(&_show_description_check, false, true);
  }

  virtual void leave(bool advancing) {
    if (advancing)
      bec::GRTManager::get()->set_app_option("db.mysql.synchronizeAny:show_sync_help_page",
                                             grt::IntegerRef(_show_description_check.get_active()));
  }

  virtual void enter(bool advancing) {
    if (advancing)
      if (!bec::GRTManager::get()->get_app_option_int("db.mysql.synchronizeAny:show_sync_help_page", 1))
        _form->go_to_next();
  }
};

#include "multi_source_selector_page.h"
#include "fetch_schema_names_multi_page.h"
#include "fetch_schema_contents_multi_page.h"

//---------------------------------------------------------------------------------------------------

class AlterViewResultPage : public ViewTextPage {
public:
  AlterViewResultPage(WizardForm *form)
    : ViewTextPage(form, "viewdiff", (ViewTextPage::Buttons)(ViewTextPage::SaveButton | ViewTextPage::CopyButton),
                   "SQL Files (*.sql)|*.sql") {
    set_short_title(_("Detected Changes"));
    set_title(_("Detected Changes to be Applied to Destination"));
  }

  void set_generate_text_slot(const std::function<std::string()> &slot) {
    _generate = slot;
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      std::string sql = _generate();
      _text.set_value(sql);
      values().gset("script", sql);
    }
  }

  virtual bool advance() {
    if (values().get_int("result") == DataSourceSelector::FileSource) {
      std::string path = values().get_string("result_path");
      if (!path.empty())
        save_text_to(path);
    }
    return true;
  }

  virtual std::string next_button_caption() {
    return execute_caption();
  }

  virtual bool allow_cancel() {
    return false;
  }
  virtual bool next_closes_wizard() {
    return values().get_int("result") != DataSourceSelector::ServerSource;
  }

protected:
  std::function<std::string()> _generate;
};

//--------------------------------------------------------------------------------

class AlterApplyProgressPage : public WizardProgressPage {
  bool _finished;
  Db_plugin *_dbplugin;

public:
  AlterApplyProgressPage(WizardForm *form) : WizardProgressPage(form, "apply_progress", false) {
    set_title(_("Applying Alter Progress"));
    set_short_title(_("Alter Progress"));

    add_async_task(_("Connect to DBMS"), std::bind(&AlterApplyProgressPage::do_connect, this),
                   _("Connecting to DBMS..."));

    add_async_task(_("Execute Alter Script"), std::bind(&AlterApplyProgressPage::do_export, this),
                   _("Applying Alter engineered SQL script in DBMS..."));

    TaskRow *task =
      add_async_task(_("Read Back Changes Made by Server"), std::bind(&AlterApplyProgressPage::back_sync, this),
                     _("Fetching back object definitions reformatted by server..."));

    task->process_finish = std::bind(&AlterApplyProgressPage::export_finished, this, std::placeholders::_1);

    end_adding_tasks(_("Applying Alter Finished Successfully"));

    set_status_text("");
  }

  virtual void enter(bool advancing) {
    _finished = false;

    if (advancing)
      reset_tasks();

    WizardProgressPage::enter(advancing);
  }

  virtual bool allow_back() {
    return WizardProgressPage::allow_back() && !_finished;
  }

  virtual bool allow_cancel() {
    return WizardProgressPage::allow_cancel() && !_finished;
  }

  bool do_connect() {
    execute_grt_task(
      [this]() {
        _dbplugin->db_conn()->test_connection();
        return grt::ValueRef();
      },
      false);
    return true;
  }

  bool do_export() {
    _dbplugin->sql_script(values().get_string("script"));
    execute_grt_task(std::bind(&Db_plugin::apply_script_to_db, _dbplugin), false);

    return true;
  }

  bool back_sync() {
    execute_grt_task(std::bind(&AlterApplyProgressPage::back_sync_, this), false);
    return true;
  }

  grt::IntegerRef back_sync_() {
    _dbplugin->read_back_view_ddl();
    return grt::IntegerRef(0);
  }

  bool perform_sync_model() {
    if (!_got_error_messages) {
      //      _dbplugin->save_sync_profile(_dbplugin->model_catalog());
    }
    return true;
  }

  void export_finished(const grt::ValueRef &result) {
    _finished = true;
  }

  virtual bool next_closes_wizard() {
    return true;
  }

  void set_db_plugin(Db_plugin *pl) {
    _dbplugin = pl;
  }
};

//--------------------------------------------------------------------------------

class AlterScriptSynchronizeDifferencesPage : public SynchronizeDifferencesPage {
public:
  AlterScriptSynchronizeDifferencesPage(WizardForm *form, DbMySQLDiffAlter *be) : SynchronizeDifferencesPage(form, be) {
    _update_model.show(false);
    _update_source.set_text(_("Update Destination"));
    _update_source.set_tooltip(_("Update the database/script with changes detected in the source."));
    _heading.set_text("Double click arrows in the list to choose whether to ignore changes or update destination DB");
    _update_model.set_text(_("Source Database"));
    _update_model.set_tooltip(_("Source Database with detected changes."));
    _skip.set_text(_("Ignore"));
    _skip.set_tooltip(_("Ignore the change."));
    _update_source.set_text(_("Update Destination"));
    _update_source.set_tooltip(_("Update the database/script with changes."));
  };
};

class WbSynchronizeAnyWizard : public WizardPlugin {
public:
  WbSynchronizeAnyWizard(grt::Module *module) : WizardPlugin(module) {
    set_name("Synchronize With Any Source Wizard");
    // Start with intro page
    add_page(mforms::manage(_description_page = new DescriptionPage(this)));

    // Then pick source and target
    add_page(mforms::manage(_source_page = new MultiSourceSelectPage(this, true)));

    _left_db.grtm(true);
    _right_db.grtm(false);

    ConnectionPage *connect;
    // Pick source connection (optional)
    add_page(mforms::manage(
      connect = new ConnectionPage(this, "connect_source", "db.mysql.synchronizeAny:left_source_connection")));
    connect->set_db_connection(_left_db.db_conn());
    connect->set_title(std::string("Source Database: ").append(connect->get_title()));
    connect->set_short_title("Source Database");
    // Pick target connection (optional)
    add_page(mforms::manage(
      connect = new ConnectionPage(this, "connect_target", "db.mysql.synchronizeAny:right_source_connection")));
    connect->set_db_connection(_right_db.db_conn());
    connect->set_title(std::string("Target Database: ").append(connect->get_title()));
    connect->set_short_title("Target Database");

    // Fetch names from source and target if they're DBs, reveng script if they're files
    FetchSchemaNamesSourceTargetProgressPage *fetch_names_page;
    add_page(mforms::manage(fetch_names_page =
                              new FetchSchemaNamesSourceTargetProgressPage(this, _source_page, "fetch_names")));
    fetch_names_page->set_load_schemata_slot(
      _left_db.db_conn(), std::bind(&WbSynchronizeAnyWizard::load_schemata, this, &_left_db), _right_db.db_conn(),
      std::bind(&WbSynchronizeAnyWizard::load_schemata, this, &_right_db));
    fetch_names_page->set_model_catalog(_be.get_model_catalog());

    // Pick what to synchronize
    _schema_match_page = new SchemaMatchingPage(this, "pick_schemata", "Source Schema", "Target Schema", true);
    add_page(mforms::manage(_schema_match_page));

    // Fetch contents from source and target, if they come from the database.. otherwise the schemas are already loaded
    // (optional)
    FetchSchemaContentsSourceTargetProgressPage *fetch_schema_page;
    add_page(mforms::manage(fetch_schema_page =
                              new FetchSchemaContentsSourceTargetProgressPage(this, _source_page, "fetch_schema")));
    fetch_schema_page->set_db_plugin(&_left_db, &_right_db);

    // Show differences
    _diffs_page = new AlterScriptSynchronizeDifferencesPage(this, &_be);
    _diffs_page->set_title(_("Differences Found"));
    add_page(mforms::manage(_diffs_page));

    AlterViewResultPage *page;
    add_page(mforms::manage(page = new AlterViewResultPage(this)));
    page->set_generate_text_slot(std::bind(&WbSynchronizeAnyWizard::generate_alter, this));
    add_page(mforms::manage(_apply_page = new AlterApplyProgressPage(this)));
    _apply_page->set_db_plugin(&_right_db);

    set_title(_("Synchronize With Any Source"));
    set_size(920, 700);
  }

  virtual ~WbSynchronizeAnyWizard() {
    _be.restore_overriden_names();
  }

  std::string generate_alter() {
    std::string report;
    try {
      //  TODO: Check if this is best place to set the options. For now it fixes a crash
      _be.set_db_options(_left_db.load_db_options());
      report = _be.generate_alter();
    } catch (const std::exception &exc) {
      report = base::strfmt("Error generating alter script: %s", exc.what());
    }
    return report;
  }

  virtual WizardPage *get_next_page(WizardPage *current) {
    std::string curid = current ? current->get_id() : "";
    std::string nextid;

    if (curid == "source") {
      if (_source_page->get_left_source() == DataSourceSelector::ServerSource)
        nextid = "connect_source";
      else {
        if (_source_page->get_right_source() == DataSourceSelector::ServerSource)
          nextid = "connect_target";
        else
          nextid = "fetch_names";
      }
    } else if (curid == "connect_source") {
      if (_source_page->get_right_source() == DataSourceSelector::ServerSource)
        nextid = "connect_target";
      else
        nextid = "fetch_names";
    } else if (curid == "pick_schemata") {
      if (_source_page->get_left_source() == DataSourceSelector::ServerSource ||
          _source_page->get_right_source() == DataSourceSelector::ServerSource)
        nextid = "fetch_schema";
      else
        nextid = "diffs";
    }

    if (nextid.empty())
      nextid = WizardForm::get_next_page(current)->get_id();

    if (nextid == "diffs") {
      db_CatalogRef left_catalog, right_catalog;

      if (_source_page->get_left_source() == DataSourceSelector::ServerSource)
        left_catalog = _left_db.db_catalog();
      else if (_source_page->get_left_source() == DataSourceSelector::FileSource)
        left_catalog = db_CatalogRef::cast_from(values().get("left_file_catalog"));
      else if (_source_page->get_left_source() == DataSourceSelector::ModelSource)
        left_catalog = _be.get_model_catalog();

      if (_source_page->get_right_source() == DataSourceSelector::ServerSource)
        right_catalog = _right_db.db_catalog();
      else if (_source_page->get_right_source() == DataSourceSelector::FileSource)
        right_catalog = db_CatalogRef::cast_from(values().get("right_file_catalog"));
      else if (_source_page->get_right_source() == DataSourceSelector::ModelSource)
        right_catalog = _be.get_model_catalog();

      // Fix the names from original schemata so the comparison works correctly
      std::map<std::string, std::string> mapping(_schema_match_page->get_mapping());
      grt::ListRef<db_Schema> schemata = left_catalog->schemata();
      for (size_t i = 0; i < schemata.count(); i++) {
        db_SchemaRef schema(schemata[i]);
        if (mapping.find(schema->name()) != mapping.end()) {
          // save the original name before proceeding so it can be retored before the wizard is closed
          schema->customData().set("db.mysql.synchronize:originalName", schema->name());
          schema->customData().set("db.mysql.synchronize:originalOldName", schema->oldName());

          std::string sname = mapping[schema->name()];
          schema->name(sname);
          schema->oldName(sname);
        }
      }
      _diffs_page->set_src(left_catalog);
      _diffs_page->set_dst(right_catalog);
    }
    return get_page_with_id(nextid);
  }

protected:
  DbMySQLDiffAlter _be;
  Db_plugin _left_db;
  Db_plugin _right_db;
  DescriptionPage *_description_page;
  FetchSchemaNamesSourceTargetProgressPage *_fetch_names_page;
  SchemaMatchingPage *_schema_match_page;
  MultiSourceSelectPage *_source_page;
  AlterScriptSynchronizeDifferencesPage *_diffs_page;
  AlterApplyProgressPage *_apply_page;
  bool connect1_apply;

  std::vector<std::string> load_schemata(Db_plugin *db) {
    std::vector<std::string> names;
    db->load_schemata(names);
    _be.set_db_options(db->load_db_options());
    return names;
  }
};

WizardPlugin *createWbSynchronizeAnyWizard(grt::Module *module, db_CatalogRef catalog) {
  return new WbSynchronizeAnyWizard(module);
}

void deleteWbSynchronizeAnyWizard(WizardPlugin *plugin) {
  delete plugin;
}
