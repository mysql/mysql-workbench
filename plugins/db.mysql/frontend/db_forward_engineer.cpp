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

#include "grt/grt_manager.h"
#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_progress_page.h"
#include "grtui/wizard_view_text_page.h"
#include "grtui/grtdb_connect_panel.h"
#include "grtui/wizard_object_filter_page.h"
#include "grtui/db_conn_be.h"

#include "db_frw_eng_be.h"

#include "catalog_validation_page.h"

using namespace grtui;
using namespace mforms;

namespace DBExport {

  class ExportInputPage;
  class ExportFilterPage;
  class ConnectionPage;
  class ExportProgressPage;
  class ExportFinishPage;
  class PreviewScriptPage;

  class WbPluginDbExport : public WizardPlugin {
    CatalogValidationPage *_validation_page;
    ExportInputPage *_input_page;
    ExportFilterPage *_selection_page;
    ConnectionPage *_connection_page;
    ExportProgressPage *_progress_page;
    PreviewScriptPage *_preview_page;

    Db_frw_eng _db_frw_eng;

  public:
    WbPluginDbExport(grt::Module *module);

    Db_frw_eng *be() {
      return &_db_frw_eng;
    }
  };

  //--------------------------------------------------------------------------------

  class ExportInputPage : public WizardPage {
  public:
    ExportInputPage(WizardForm *form)
      : WizardPage(form, "options"),
        _options(mforms::TitledBoxPanel),
        _options_box(false),
        _table_options(mforms::TitledBoxPanel),
        _table_options_box(false),
        _other_options(mforms::TitledBoxPanel),
        _other_options_box(false) {
      set_title(_("Set Options for Database to be Created"));
      set_short_title(_("Options"));

      _table_options.set_title(_("Tables"));
      _table_options.add(&_table_options_box);
      _table_options_box.set_padding(12);
      _table_options_box.set_spacing(8);

      _other_options.set_title(_("Other Objects"));
      _other_options.add(&_other_options_box);
      _other_options_box.set_padding(12);
      _other_options_box.set_spacing(8);

      _options.set_title(_("Code Generation"));
      _options.add(&_options_box);
      _options_box.set_padding(12);
      _options_box.set_spacing(8);

      // tables
      _skip_foreign_keys_check.set_text(_("Skip creation of FOREIGN KEYS"));
      _table_options_box.add(&_skip_foreign_keys_check, false, true);
      scoped_connect(_skip_foreign_keys_check.signal_clicked(), std::bind(&ExportInputPage::SkipFKToggled, this));
      _skip_FK_indexes_check.set_text(_("Skip creation of FK Indexes as well"));
      _table_options_box.add(&_skip_FK_indexes_check, false, true);
      _generate_create_index_check.set_text(_("Generate separate CREATE INDEX statements"));
      _table_options_box.add(&_generate_create_index_check, false, true);
      _generate_insert_check.set_text(_("Generate INSERT statements for tables"));
      _table_options_box.add(&_generate_insert_check, false, true);
      _no_FK_for_inserts.set_text(_("Disable FK checks for INSERTs"));
      _table_options_box.add(&_no_FK_for_inserts, false, true);
      //    _triggers_after_inserts.set_text(_("Create triggers after INSERTs"));
      //   _options_box.add(&_triggers_after_inserts, false, false);
      add(&_table_options, false, true);

      // other objects
      _no_view_placeholders.set_text(_("Don't create view placeholder tables"));
      _other_options_box.add(&_no_view_placeholders, false, true);
      _skip_users_check.set_text(_("Do not create users. Only create privileges (GRANTs)"));
      _other_options_box.add(&_skip_users_check, false, true);

      add(&_other_options, false, true);

      // code generation
      _generate_drop_check.set_text(_("DROP objects before each CREATE object"));
      _options_box.add(&_generate_drop_check, false, true);

      _generate_drop_schema_check.set_text(_("Generate DROP SCHEMA"));
      _options_box.add(&_generate_drop_schema_check, false, true);

      _omit_schema_qualifier_check.set_text(_("Omit schema qualifier in object names"));
      _options_box.add(&_omit_schema_qualifier_check, false, true);
      scoped_connect(_omit_schema_qualifier_check.signal_clicked(),
                     std::bind(&ExportInputPage::OmitSchemaToggled, this));
      _generate_use_check.set_text(_("Generate USE statements"));
      _options_box.add(&_generate_use_check, false, true);
      _generate_show_warnings_check.set_text(_("Add SHOW WARNINGS after every DDL statement"));
      _options_box.add(&_generate_show_warnings_check, false, true);

      _include_user_scripts.set_text(_("Include model attached scripts"));
      _options_box.add(&_include_user_scripts, false, true);

      add(&_options, false, true);

      scoped_connect(signal_leave(), std::bind(&ExportInputPage::gather_options, this, std::placeholders::_1));

      grt::Module *module = ((WizardPlugin *)_form)->module();
      _generate_drop_check.set_active(module->document_int_data("GenerateDrops", 0) != 0);
      _generate_drop_schema_check.set_active(module->document_int_data("GenerateSchemaDrops", 0) != 0);
      _skip_foreign_keys_check.set_active(module->document_int_data("SkipForeignKeys", 0) != 0);
      _skip_FK_indexes_check.set_active(module->document_int_data("SkipFKIndexes", 0) != 0);
      _generate_show_warnings_check.set_active(module->document_int_data("GenerateWarnings", 0) != 0);
      _generate_create_index_check.set_active(module->document_int_data("GenerateCreateIndex", 0) != 0);
      _no_view_placeholders.set_active(module->document_int_data("NoViewPlaceholders", 0) != 0);
      _skip_users_check.set_active(module->document_int_data("NoUsersJustPrivileges", 0) != 0);
      _generate_insert_check.set_active(module->document_int_data("GenerateInserts", 0) != 0);
      _no_FK_for_inserts.set_active(module->document_int_data("NoFKForInserts", 0) != 0);
      //    _triggers_after_inserts.set_active(module->document_int_data("TriggersAfterInserts", 0) != 0);
      _omit_schema_qualifier_check.set_active(module->document_int_data("OmitSchemata", 0) != 0);
      _generate_use_check.set_active(module->document_int_data("GenerateUse", 0) != 0);
      _generate_use_check.set_enabled(_omit_schema_qualifier_check.get_active());
      _skip_FK_indexes_check.set_enabled(_skip_foreign_keys_check.get_active());
      _include_user_scripts.set_active(module->document_int_data("GenerateAttachedScripts", 1) != 0);
    }

    void SkipFKToggled() {
      _skip_FK_indexes_check.set_enabled(_skip_foreign_keys_check.get_active());
    }

    void OmitSchemaToggled() {
      _generate_use_check.set_enabled(_omit_schema_qualifier_check.get_active());
    }

    void gather_options(bool advancing) {
      values().gset("GenerateDrops", _generate_drop_check.get_active());
      values().gset("GenerateSchemaDrops", _generate_drop_schema_check.get_active());
      values().gset("SkipForeignKeys", _skip_foreign_keys_check.get_active());
      values().gset("SkipFKIndexes", _skip_FK_indexes_check.get_active());
      values().gset("GenerateWarnings", _generate_show_warnings_check.get_active());
      values().gset("GenerateCreateIndex", _generate_create_index_check.get_active());
      values().gset("NoUsersJustPrivileges", _skip_users_check.get_active());
      values().gset("NoViewPlaceholders", _no_view_placeholders.get_active());
      values().gset("GenerateInserts", _generate_insert_check.get_active());
      values().gset("OmitSchemata", _omit_schema_qualifier_check.get_active());
      values().gset("GenerateUse", _generate_use_check.get_active());
      values().gset("NoFKForInserts", _no_FK_for_inserts.get_active());
      values().gset("GenerateAttachedScripts", _include_user_scripts.get_active());
      //    values().gset("TriggersAfterInserts", _triggers_after_inserts.get_active());

      grt::Module *module = ((WizardPlugin *)_form)->module();
      module->set_document_data("GenerateDrops", _generate_drop_check.get_active());
      module->set_document_data("GenerateSchemaDrops", _generate_drop_schema_check.get_active());
      module->set_document_data("SkipForeignKeys", _skip_foreign_keys_check.get_active());
      module->set_document_data("SkipFKIndexes", _skip_FK_indexes_check.get_active());
      module->set_document_data("GenerateWarnings", _generate_show_warnings_check.get_active());
      module->set_document_data("GenerateCreateIndex", _generate_create_index_check.get_active());
      module->set_document_data("NoUsersJustPrivileges", _skip_users_check.get_active());
      module->set_document_data("NoViewPlaceholders", _no_view_placeholders.get_active());
      module->set_document_data("GenerateInserts", _generate_insert_check.get_active());
      module->set_document_data("OmitSchemata", _omit_schema_qualifier_check.get_active());
      module->set_document_data("GenerateUse", _generate_use_check.get_active());
      module->set_document_data("NoFKForInserts", _no_FK_for_inserts.get_active());
      module->set_document_data("GenerateAttachedScripts", _include_user_scripts.get_active());
      //    module->set_document_data("TriggersAfterInserts", _triggers_after_inserts.get_active());
    }

  protected:
    Panel _options;
    Box _options_box;

    Panel _table_options;
    Box _table_options_box;

    Panel _other_options;
    Box _other_options_box;

    CheckBox _generate_drop_check;
    CheckBox _generate_drop_schema_check;
    CheckBox _generate_use_check;
    CheckBox _skip_foreign_keys_check;
    CheckBox _skip_FK_indexes_check;
    CheckBox _generate_create_index_check;
    CheckBox _generate_show_warnings_check;
    CheckBox _skip_users_check;
    CheckBox _no_view_placeholders;
    CheckBox _generate_insert_check;
    CheckBox _no_FK_for_inserts;
    //  CheckBox _triggers_after_inserts;
    CheckBox _omit_schema_qualifier_check;

    CheckBox _include_user_scripts;
  };

  //--------------------------------------------------------------------------------

  class ExportFilterPage : public WizardObjectFilterPage {
  public:
    ExportFilterPage(WizardForm *form, Db_frw_eng *plugin_be)
      : WizardObjectFilterPage(form, "filter"), _export_be(plugin_be) {
      set_short_title(_("Select Objects"));
      set_title(_("Select Objects to Forward Engineer"));

      _top_label.set_wrap_text(true);
      _top_label.set_text(
        _("To exclude objects of a specific type from the SQL Export, disable the corresponding checkbox. "
          "Press Show Filter and add objects or patterns to the ignore list to exclude them from the export."));
    }

  protected:
    virtual void enter(bool advancing) {
      if (advancing)
        setup_filters();
      std::vector<std::string> schemata;
      connected = true;
      try {
        _export_be->load_schemata(schemata);
      } catch (const std::exception &exc) {
        mforms::Utilities::show_error("Connect to Database",
                                      "Error connecting to database:\n" + std::string(exc.what()), "OK", "", "");
        connected = false;
        return;
      }

      WizardObjectFilterPage::enter(advancing);
    }

    void setup_filters() {
      reset();

      bec::GrtStringListModel *users_model;
      bec::GrtStringListModel *users_imodel;
      bec::GrtStringListModel *tables_model;
      bec::GrtStringListModel *tables_imodel;
      bec::GrtStringListModel *views_model;
      bec::GrtStringListModel *views_imodel;
      bec::GrtStringListModel *routines_model;
      bec::GrtStringListModel *routines_imodel;
      bec::GrtStringListModel *triggers_model;
      bec::GrtStringListModel *triggers_imodel;

      _export_be->setup_grt_string_list_models_from_catalog(&users_model, &users_imodel, &tables_model, &tables_imodel,
                                                            &views_model, &views_imodel, &routines_model,
                                                            &routines_imodel, &triggers_model, &triggers_imodel);

      _table_filter =
        add_filter(db_mysql_Table::static_class_name(), _("Export %s Objects"), tables_model, tables_imodel, NULL);

      _view_filter =
        add_filter(db_mysql_View::static_class_name(), _("Export %s Objects"), views_model, views_imodel, NULL);

      _routine_filter = add_filter(db_mysql_Routine::static_class_name(), _("Export %s Objects"), routines_model,
                                   routines_imodel, NULL);

      _trigger_filter = add_filter(db_mysql_Trigger::static_class_name(), _("Export %s Objects"), triggers_model,
                                   triggers_imodel, NULL);

      _user_filter = add_filter(db_User::static_class_name(), _("Export %s Objects"), users_model, users_imodel, NULL);
    }

    virtual bool advance() {
      // this is done by _export_be->load_schemata(schemata); in enter()
      //_export_be->set_db_options( _export_be->load_db_options());
      _export_be->set_option("OutputFileName", values().get_string("OutputFileName"));
      _export_be->set_option("GenerateDrops", values().get_int("GenerateDrops") != 0);
      _export_be->set_option("SkipForeignKeys", values().get_int("SkipForeignKeys") != 0);
      _export_be->set_option("SkipFKIndexes", values().get_int("SkipFKIndexes") != 0);
      _export_be->set_option("GenerateSchemaDrops", values().get_int("GenerateSchemaDrops") != 0);
      _export_be->set_option("GenerateWarnings", values().get_int("GenerateWarnings") != 0);
      _export_be->set_option("GenerateCreateIndex", values().get_int("GenerateCreateIndex") != 0);
      _export_be->set_option("NoUsersJustPrivileges", values().get_int("NoUsersJustPrivileges") != 0);
      _export_be->set_option("NoViewPlaceholders", values().get_int("NoViewPlaceholders") != 0);
      _export_be->set_option("GenerateInserts", values().get_int("GenerateInserts") != 0);
      _export_be->set_option("NoFKForInserts", values().get_int("NoFKForInserts") != 0);
      //    _export_be->set_option("TriggersAfterInserts", values().get_int("TriggersAfterInserts") != 0);
      _export_be->set_option("OmitSchemata", values().get_int("OmitSchemata") != 0);
      _export_be->set_option("GenerateUse", values().get_int("GenerateUse") != 0);
      _export_be->set_option("GenerateAttachedScripts", values().get_int("GenerateAttachedScripts") != 0);

      _export_be->set_option("TablesAreSelected", _table_filter->get_active());
      _export_be->set_option("TriggersAreSelected", _trigger_filter->get_active());
      _export_be->set_option("RoutinesAreSelected", _routine_filter->get_active());
      _export_be->set_option("ViewsAreSelected", _view_filter->get_active());
      _export_be->set_option("UsersAreSelected", _user_filter->get_active());

      _export_be->set_up_dboptions();

      return true;
    }
    
    virtual bool allow_next() {
      return connected;
    };

  protected:
    Db_frw_eng *_export_be;
    DBObjectFilterFrame *_table_filter;
    DBObjectFilterFrame *_view_filter;
    DBObjectFilterFrame *_routine_filter;
    DBObjectFilterFrame *_trigger_filter;
    DBObjectFilterFrame *_user_filter;
    bool connected;
  };

  //--------------------------------------------------------------------------------

  class PreviewScriptPage : public ViewTextPage {
  public:
    PreviewScriptPage(WizardForm *form)
      : ViewTextPage(form, "preview", (ViewTextPage::Buttons)(ViewTextPage::CopyButton | ViewTextPage::SaveButton),
                     "SQL Scripts (*.sql)|*.sql"), script_ready(false) {
      set_short_title(_("Review SQL Script"));
      set_title(_("Review the SQL Script to be Executed"));

      set_editable(true);

      _heading.set_wrap_text(true);
      _heading.set_style(mforms::WizardHeadingStyle);
      _heading.set_text(
        _("This script will now be executed on the DB server to create your databases.\nYou may make changes before "
          "executing."));
      add(&_heading, false, true);
    }

    virtual void enter(bool advancing) {
      if (advancing) {
        set_text(""); // Clear the output area.
        std::string script;
        script_ready = false;
        _form->update_buttons();
        Db_frw_eng *backend = ((WbPluginDbExport *)_form)->be();

        backend->export_task_finish_cb(std::bind(&PreviewScriptPage::export_task_finished, this));
        backend->start_export();
      }
    }

    int export_task_finished() {
      set_text(((WbPluginDbExport *)_form)->be()->export_sql_script());
      script_ready = true;
      _form->update_buttons();
      return 0;
    }

    virtual void leave(bool advancing) {
      if (advancing)
        ((WbPluginDbExport *)_form)->be()->sql_script(_text.get_string_value());
    }

    virtual bool allow_next() {
      return script_ready;
    };

  protected:
    bool script_ready;
    mforms::Label _heading;
  };

//--------------------------------------------------------------------------------

#include "grtui/connection_page.h"

  class MyConnectionPage : public ConnectionPage {
  public:
    MyConnectionPage(WizardForm *form, const char *name = "connect") : ConnectionPage(form, name) {
    }
    /*
    virtual std::string next_button_caption()
    {
      return execute_caption();
    }
    */

    void load_saved_connection() {
      if (_dbconn) {
        grt::ListRef<db_mgmt_Connection> list(_dbconn->get_db_mgmt()->storedConns());
        grt::ListRef<db_mgmt_Connection>::const_iterator iter = list.begin();

        const std::string saved_conn_name = bec::GRTManager::get()->get_app_option_string("LastUsedConnectionName");

        for (; iter != list.end(); ++iter) {
          if ((*iter)->name() == saved_conn_name) {
            _connect.set_connection(*iter);
            break;
          }
        }
      }
    }

    void save_used_connection() {
      if (_dbconn && _dbconn->get_connection().is_valid()) {
        bec::GRTManager::get()->set_app_option("LastUsedConnectionName",
                                               grt::StringRef(_dbconn->get_connection()->name()));
      }
    }
  };

  //--------------------------------------------------------------------------------

  class ExportProgressPage : public WizardProgressPage {
    bool _finished;
    MyConnectionPage *_conn_page;

  public:
    ExportProgressPage(WizardForm *form)
      : WizardProgressPage(form, "progress", false), _finished(false), _conn_page(0) {
      set_title(_("Forward Engineering Progress"));
      set_short_title(_("Commit Progress"));

      add_async_task(_("Connect to DBMS"), std::bind(&ExportProgressPage::do_connect, this),
                     _("Connecting to DBMS..."));

      add_async_task(_("Execute Forward Engineered Script"), std::bind(&ExportProgressPage::do_export, this),
                     _("Executing forward engineered SQL script in DBMS..."));

      add_async_task(_("Read Back Changes Made by Server"), std::bind(&ExportProgressPage::back_sync, this),
                     _("Fetching back object definitions reformatted by server..."));

      TaskRow *task = add_task(_("Save Synchronization State"), std::bind(&ExportProgressPage::save_sync_profile, this),
                               _("Storing state information to synchronization profile..."));

      task->process_finish = std::bind(&ExportProgressPage::export_finished, this, std::placeholders::_1);

      end_adding_tasks(_("Forward Engineer Finished Successfully"));

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
          ((WbPluginDbExport *)_form)->be()->db_conn()->test_connection();
          return grt::ValueRef();
        },
        false);
      return true;
    }

    bool do_export() {
      execute_grt_task(std::bind(&Db_plugin::apply_script_to_db, ((WbPluginDbExport *)_form)->be()), false);

      return true;
    }

    bool back_sync() {
      execute_grt_task(std::bind(&ExportProgressPage::back_sync_, this), false);
      return true;
    }

    grt::IntegerRef back_sync_() {
      ((WbPluginDbExport *)_form)->be()->read_back_view_ddl();
      return grt::IntegerRef(0);
    }

    bool save_sync_profile() {
      //#warning  TODO
      //    ((WbPluginDbExport*)_form)->be()->save_
      return true;
    }

    void export_finished(const grt::ValueRef &result) {
      _finished = true;
      if (_conn_page)
        _conn_page->save_used_connection();
    }

    virtual bool next_closes_wizard() {
      return true;
    }

    void set_connection_page(MyConnectionPage *page) {
      _conn_page = page;
    }
  };

  WbPluginDbExport::WbPluginDbExport(grt::Module *module) : WizardPlugin(module) {
    set_name("DB Export Wizard");
    if (CatalogValidationPage::has_modules())
      _validation_page = new CatalogValidationPage(this);
    else
      _validation_page = 0;
    _input_page = new ExportInputPage(this);
    _connection_page = new MyConnectionPage(this);
    _connection_page->set_db_connection(be()->db_conn());
    static_cast<MyConnectionPage *>(_connection_page)->load_saved_connection();
    _preview_page = new PreviewScriptPage(this);
    _selection_page = new ExportFilterPage(this, &_db_frw_eng);
    _progress_page = new ExportProgressPage(this);
    _progress_page->set_connection_page((MyConnectionPage *)_connection_page);
    add_page(mforms::manage(_connection_page));
    if (_validation_page)
      add_page(mforms::manage(_validation_page));
    add_page(mforms::manage(_input_page));
    add_page(mforms::manage(_selection_page));
    add_page(mforms::manage(_preview_page));
    add_page(mforms::manage(_progress_page));

    set_title(_("Forward Engineer to Database"));
    set_size(900, 700);
  }
};

grtui::WizardPlugin *createDbExportWizard(grt::Module *module, db_CatalogRef catalog) {
  return new DBExport::WbPluginDbExport(module);
}

void deleteDbExportWizard(grtui::WizardPlugin *plugin) {
  delete plugin;
}
