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

#include <stdio.h>

#include "base/log.h"

#include "mforms/textbox.h"
#include "mforms/checkbox.h"
#include "mforms/label.h"
#include "mforms/splitter.h"
#include "mforms/checkbox.h"

#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_progress_page.h"
#include "grtui/wizard_schema_filter_page.h"
#include "grtui/wizard_object_filter_page.h"
#include "grtui/wizard_finished_page.h"
#include "grtui/wizard_view_text_page.h"
#include "grtui/grtdb_connect_panel.h"
#include "grtui/connection_page.h"

#include "fetch_schema_names_page.h"
#include "schema_matching_page.h"
#include "fetch_schema_contents_page.h"
#include "synchronize_differences_page.h"
#include "db_mysql_sql_script_sync.h"
#include "db_mysql_sql_sync.h"

#include "diff_tree.h"

using namespace grtui;
using namespace mforms;

class ConnectionPage;
class FetchSchemaNamesProgressPage;
class SchemaMatchingPage;
class FetchSchemaContentsProgressPage;
class ObjectSelectionPage;
class ReviewScriptPage;
class DBSynchronizeProgressPage;
class FinishPage;

//--------------------------------------------------------------------------------

class SyncOptionsPage : public WizardPage {
public:
  SyncOptionsPage(WizardForm *form, DbMySQLScriptSync *be)
    : WizardPage(form, "options"),
      _be(be),
      _options(mforms::TitledBoxPanel),
      _options_box(false),
      _db_options(mforms::TitledBoxPanel),
      _db_options_box(false) {
    set_title(_("Set Options for Synchronization Script"));
    set_short_title(_("Sync Options"));

    _options.set_title(_("Generation Options"));
    _options.add(&_options_box);
    _options_box.set_padding(12);
    _options_box.set_spacing(8);

    _db_options.set_title(_("Compare Options"));
    _db_options.add(&_db_options_box);
    _db_options_box.set_padding(12);
    _db_options_box.set_spacing(8);

    // DB Objects
    _skip_triggers_check.set_text(_("Skip synchronization of Triggers"));
    _db_options_box.add(&_skip_triggers_check, false, false);
    _skip_routines_check.set_text(_("Skip synchronization of Stored Procedures and Functions"));
    _db_options_box.add(&_skip_routines_check, false, false);

    _skip_routine_definer_check.set_text(_("Skip checking of Routine Definer"));
    _db_options_box.add(&_skip_routine_definer_check, false, false);

    // Script
    _omit_schema_qualifier_check.set_text(_("Omit Schema Qualifier in Object Names"));
    _options_box.add(&_omit_schema_qualifier_check, false, false);

    _generate_attached_scripts.set_text(_("Include SQL Scripts Attached to Model"));
    _options_box.add(&_generate_attached_scripts, false, false);

    add(&_db_options, false, true);
    add(&_options, false, true);

    scoped_connect(signal_leave(), std::bind(&SyncOptionsPage::gather_options, this, std::placeholders::_1));

    grt::Module *module = ((WizardPlugin *)_form)->module();
    _skip_routine_definer_check.set_active(module->document_int_data("SkipRoutineDefiner", 0) != 0);
    _skip_triggers_check.set_active(module->document_int_data("SkipTriggers", 0) != 0);
    _skip_routines_check.set_active(module->document_int_data("SkipRoutines", 0) != 0);
    _omit_schema_qualifier_check.set_active(module->document_int_data("OmitSchemata", 0) != 0);
    _generate_attached_scripts.set_active(module->document_int_data("GenerateAttachedScripts", 0) != 0);
  }

  void gather_options(bool advancing) {
    values().gset("SkipTriggers", _skip_triggers_check.get_active());
    values().gset("SkipRoutines", _skip_routines_check.get_active());
    values().gset("OmitSchemata", _omit_schema_qualifier_check.get_active());
    values().gset("GenerateAttachedScripts", _generate_attached_scripts.get_active());
    values().gset("SkipRoutineDefiner", _skip_routine_definer_check.get_active());

    grt::Module *module = ((WizardPlugin *)_form)->module();
    module->set_document_data("SkipTriggers", _skip_triggers_check.get_active());
    module->set_document_data("SkipRoutines", _skip_routines_check.get_active());
    module->set_document_data("OmitSchemata", _omit_schema_qualifier_check.get_active());
    module->set_document_data("GenerateAttachedScripts", _generate_attached_scripts.get_active());
    module->set_document_data("SkipRoutineDefiner", _skip_routine_definer_check.get_active());
  }

  virtual bool advance() {
    _be->set_options(values());

    return true;
  }

protected:
  DbMySQLScriptSync *_be;

  Panel _options;
  Box _options_box;

  Panel _db_options;
  Box _db_options_box;

  CheckBox _skip_triggers_check;
  CheckBox _skip_routines_check;
  CheckBox _skip_routine_definer_check;
  CheckBox _omit_schema_qualifier_check;
  CheckBox _generate_attached_scripts;
};

//--------------------------------------------------------------------------------

class ModelSchemaMatchingPage : public SchemaMatchingPage {
  DbMySQLSync *_db_be;

public:
  ModelSchemaMatchingPage(grtui::WizardForm *form, DbMySQLSync *be, const char *name = "selectSchemata",
                          const std::string &left_name = "Model", const std::string &right_name = "Database")
    : SchemaMatchingPage(form, name, left_name, right_name), _db_be(be) {
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      // Wee ned this information to set database if user is using for example Windows or Mac.
      if (_db_be && _db_be->db_conn())
        values().set(
          "server_is_case_sensitive",
          grt::IntegerRef((int)_db_be->db_conn()->get_dbc_connection()->getMetaData()->storesMixedCaseIdentifiers()));
      else
        values().set("server_is_case_sensitive",
                     grt::IntegerRef(1)); // By default set it to true, so we're on the safe side.

      values().set("targetSchemata", values().get("schemata"));

      grt::StringListRef schema_list(grt::Initialized);
      grt::ListRef<db_Schema> schemas = _db_be->model_catalog()->schemata();
      for (size_t i = 0; i < schemas.count(); i++)
        schema_list.insert(schemas[i]->name());
      values().set("schemata", schema_list);
    }

    SchemaMatchingPage::enter(advancing);
  }

  virtual void leave(bool advancing) {
    SchemaMatchingPage::leave(advancing);
    if (advancing) {
      // rename the schemas in the model so we can properly compare schemas with a different name
      // once the snc is done, the original name should be restored
      std::map<std::string, std::string> mapping = get_mapping();
      grt::ListRef<db_Schema> schemas = _db_be->model_catalog()->schemata();
      for (size_t i = 0; i < schemas.count(); i++) {
        db_SchemaRef schema(schemas[i]);
        if (mapping.find(schema->name()) != mapping.end()) {
          // save the original name before proceeding so it can be retored before the wizard is closed
          schema->customData().set("db.mysql.synchronize:originalName", schema->name());
          schema->customData().set("db.mysql.synchronize:originalOldName", schema->oldName());

          std::string sname = mapping[schema->name()];
          schema->name(sname);
          schema->oldName(sname);
        } else {
          // make sure to delete any dangling values
          schema->customData().remove("db.mysql.synchronize:originalName");
          schema->customData().remove("db.mysql.synchronize:originalOldName");
        }
      }
    }
  }
};

//--------------------------------------------------------------------------------

namespace DBSynchronize {
  class WbPluginDbSynchronize : public WizardPlugin {
    DbMySQLScriptSync _be;
    DbMySQLSync _db_be;

    std::vector<std::string> load_schemas() {
      std::vector<std::string> schema_names;
      _db_be.load_schemata(schema_names);
      _be.set_db_options(_db_be.load_db_options());
      _be.set_sync_profile_name(_db_be.db_conn()->get_connection()->hostIdentifier());
      return schema_names;
    }

  public:
    WbPluginDbSynchronize(grt::Module *module);
    virtual ~WbPluginDbSynchronize() {
      _be.restore_overriden_names();
    }

    DbMySQLScriptSync *get_be() {
      return &_be;
    }

    DbMySQLSync *get_db_be() {
      return &_db_be;
    }
  };

  //--------------------------------------------------------------------------------

  class PreviewScriptPage : public ViewTextPage {
    ::mforms::CheckBox _model_only;

  public:
    PreviewScriptPage(WizardForm *form)
      : ViewTextPage(form, "preview", (ViewTextPage::Buttons)(ViewTextPage::CopyButton | ViewTextPage::SaveButton),
                     "SQL Scripts (*.sql)|*.sql") {
      set_title(_("Preview Database Changes to be Applied"));
      set_short_title(_("Review DB Changes"));

      set_editable(true);

      _model_only.set_text("Skip DB changes and update model only");
      _button_box.add(&_model_only, true, true);

      scoped_connect(signal_leave(), std::bind(&PreviewScriptPage::apply_changes, this, std::placeholders::_1));
    }

    virtual void enter(bool advancing) {
      if (advancing)
        set_text(((WbPluginDbSynchronize *)_form)->get_be()->generate_diff_tree_script());
    }

    virtual bool advance() {
      // check if there is anything to be DROPPED and warn user
      // TODO XXX

      return ViewTextPage::advance();
    }

    void apply_changes(bool advancing) {
      values().gset("UpdateModelOnly", _model_only.get_active() ? 1 : 0);
      ((WbPluginDbSynchronize *)_form)
        ->get_db_be()
        ->set_option("ScriptToApply", get_text()); // TODO: does this seves any purpose any more?
      ((WbPluginDbSynchronize *)_form)->get_db_be()->sql_script(get_text());
    }

    virtual std::string next_button_caption() {
      return execute_caption();
    }
  };

  //--------------------------------------------------------------------------------

  class DBSynchronizeProgressPage : public WizardProgressPage {
    TaskRow *db_task;
    TaskRow *read_back_task;

  public:
    DBSynchronizeProgressPage(WbPluginDbSynchronize *form) : WizardProgressPage(form, "importProgress", true) {
      set_title(_("Progress of Model and Database Synchronization"));
      set_short_title(_("Synchronize Progress"));

      db_task =
        add_async_task(_("Apply Changes to Database"), std::bind(&DBSynchronizeProgressPage::perform_sync_db, this),
                       _("Applying selected changes from model to the database..."));

      read_back_task =
        add_async_task(_("Read Back Changes Made by Server"), std::bind(&DBSynchronizeProgressPage::back_sync, this),
                       _("Fetching back object definitions reformatted by server..."));

      add_task(_("Apply Changes to Model"), std::bind(&DBSynchronizeProgressPage::perform_sync_model, this),
               _("Applying selected changes from database to the model..."));

      end_adding_tasks(_("Synchronization Completed Successfully"));

      set_status_text("");
    }

    virtual void enter(bool advancing) {
      if (values().get_int("UpdateModelOnly")) {
        db_task->set_enabled(false);
        read_back_task->set_enabled(false);
      } else {
        db_task->set_enabled(true);
        read_back_task->set_enabled(true);
      }
      WizardProgressPage::enter(advancing);
    }

    bool perform_sync_db() {
      grt::GRT::get()->send_info("Applying synchronization scripts to server...");

      execute_grt_task(std::bind(&Db_plugin::apply_script_to_db, ((WbPluginDbSynchronize *)_form)->get_db_be()), false);

      return true;
    }

    bool back_sync() {
      execute_grt_task(std::bind(&DBSynchronizeProgressPage::back_sync_, this), false);
      return true;
    }

    grt::IntegerRef back_sync_() {
      ((WbPluginDbSynchronize *)_form)->get_db_be()->read_back_view_ddl();
      return grt::IntegerRef(0);
    }

    bool perform_sync_model() {
      grt::GRT::get()->send_info("Updating model...");
      if (!_got_error_messages) {
        ((WbPluginDbSynchronize *)_form)->get_be()->save_sync_profile();
      }
      ((WbPluginDbSynchronize *)_form)->get_be()->apply_changes_to_model();

      return true;
    }

    virtual bool allow_back() {
      return false;
    }
    virtual bool allow_cancel() {
      return false;
    }

    virtual bool next_closes_wizard() {
      return true;
    }
  };

  //--------------------------------------------------------------------------------

  WbPluginDbSynchronize::WbPluginDbSynchronize(grt::Module *module) : WizardPlugin(module) {
    // add validation here
    set_name("DB Synchronize Wizard");

    ConnectionPage *connection_page = new ConnectionPage(this);
    connection_page->set_db_connection(_db_be.db_conn());
    add_page(mforms::manage(connection_page));

    SyncOptionsPage *options_page = new SyncOptionsPage(this, &_be);
    add_page(mforms::manage(options_page));

    FetchSchemaNamesProgressPage *fetch_progress_page = new FetchSchemaNamesProgressPage(this);
    fetch_progress_page->set_db_connection(_db_be.db_conn());
    fetch_progress_page->set_load_schemas_slot(std::bind(&WbPluginDbSynchronize::load_schemas, this));
    fetch_progress_page->set_check_case_slot(std::bind(&Db_plugin::check_case_sensitivity_problems, &_db_be));
    add_page(mforms::manage(fetch_progress_page));

    SchemaMatchingPage *schema_selection_page =
      new ModelSchemaMatchingPage(this, &_db_be, "pickSchemata", "Model Schema", "RDBMS Schema");
    add_page(mforms::manage(schema_selection_page));

    FetchSchemaContentsProgressPage *fetch_contents_page = new FetchSchemaContentsProgressPage(this);
    fetch_contents_page->set_db_plugin(&_db_be);
    add_page(mforms::manage(fetch_contents_page));

    SynchronizeDifferencesPage *diffs_page = new SynchronizeDifferencesPage(this, &_be);
    diffs_page->set_title(_("Model and Database Differences"));
    diffs_page->set_catalog_getter_slot(std::bind(&Db_plugin::model_catalog, &_db_be),
                                        std::bind(&Db_plugin::db_catalog, &_db_be));
    add_page(mforms::manage(diffs_page));

    add_page(mforms::manage(new PreviewScriptPage(this)));
    add_page(mforms::manage(new DBSynchronizeProgressPage(this)));

    set_title("Synchronize Model with Database");
    set_size(920, 700);
  }

}; // namespace DBSynchronize

grtui::WizardPlugin *createDbSynchronizeWizard(grt::Module *module, db_CatalogRef catalog) {
  return new DBSynchronize::WbPluginDbSynchronize(module);
}

void deleteDbSynchronizeWizard(grtui::WizardPlugin *plugin) {
  delete plugin;
}
