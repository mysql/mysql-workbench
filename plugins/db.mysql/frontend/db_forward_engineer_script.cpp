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

#include "base/util_functions.h"

#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_progress_page.h"
#include "grtui/wizard_finished_page.h"
#include "grtui/wizard_view_text_page.h"
#include "grtui/wizard_object_filter_page.h"

#include "grtui/grtdb_object_filter.h"

#include "db_mysql_sql_export.h"
#include "base/string_utilities.h"
#include "mforms/fs_object_selector.h"
#include "mforms/app.h"
#include "grts/structs.workbench.h"
#include "grtdb/db_object_helpers.h"

using namespace grtui;
using namespace mforms;

class ExportInputPage : public WizardPage {
public:
  ExportInputPage(WizardPlugin *form)
    : WizardPage(form, "options"), _options(mforms::TitledBoxPanel), _options_box(false) {
    set_title(_("SQL Export Options"));
    set_short_title(_("SQL Export Options"));

    _contents.set_row_count(2);
    _contents.set_column_count(3);
    _contents.set_column_spacing(8);
    _contents.set_row_spacing(8);

    _contents.add(&_caption, 0, 1, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
    _contents.add(&_filename, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
    _contents.add(&_browse_button, 2, 3, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
    _contents.add(&_skip_out_label, 1, 2, 1, 2, 0);

    _skip_out_label.set_text(_("Leave blank to view generated script but not save to a file."));
    _skip_out_label.set_style(SmallHelpTextStyle);

    _file_selector = mforms::manage(new FsObjectSelector(&_browse_button, &_filename));
    std::string initial_value = form->module()->document_string_data("create_sql_output_filename", "");
    _file_selector->initialize(initial_value, mforms::SaveFile, "SQL Files (*.sql)|*.sql", false,
                               std::bind(&WizardPage::validate, this));
    scoped_connect(_file_selector->signal_changed(), std::bind(&ExportInputPage::file_changed, this));

    _caption.set_text(_("Output SQL Script File:"));

    add(&_contents, false, true);

    _options.set_title(_("SQL Options"));

    _options.add(&_options_box);
    _options_box.set_padding(12);
    _options_box.set_spacing(8);

    _generate_drop_check.set_text(_("Generate DROP Statements Before Each CREATE Statement"));
    _options_box.add(&_generate_drop_check, false, true);
    _generate_drop_schema_check.set_text(_("Generate DROP SCHEMA"));
    _options_box.add(&_generate_drop_schema_check, false, true);

    _sortTablesAlphabeticallyCheck.set_text(_("Sort Tables Alphabetically"));
    _sortTablesAlphabeticallyCheck.set_name("Sort Tables Alphabetically");
    auto box = mforms::manage(new Box(true));
    box->add(&_sortTablesAlphabeticallyCheck, false, true);
    auto img = mforms::manage(new ImageBox());
    img->set_image(mforms::App::get()->get_resource_path("mini_notice.png"));
    img->set_tooltip("When this is unchecked, tables will be sorted according to foreign key references.");
    box->add(img, false, true);

    _options_box.add(box, false, true);

    _skip_foreign_keys_check.set_text(_("Skip Creation of FOREIGN KEYS"));
    _options_box.add(&_skip_foreign_keys_check, false, true);
    scoped_connect(_skip_foreign_keys_check.signal_clicked(), std::bind(&ExportInputPage::SkipFKToggled, this));
    _skip_FK_indexes_check.set_text(_("Skip creation of FK Indexes as well"));
    _options_box.add(&_skip_FK_indexes_check, false, true);

    _omit_schema_qualifier_check.set_text(_("Omit Schema Qualifier in Object Names"));
    _options_box.add(&_omit_schema_qualifier_check, false, true);
    scoped_connect(_omit_schema_qualifier_check.signal_clicked(), std::bind(&ExportInputPage::OmitSchemaToggled, this));
    _generate_use_check.set_text(_("Generate USE statements"));
    _options_box.add(&_generate_use_check, false, true);

    _generate_create_index_check.set_text(_("Generate Separate CREATE INDEX Statements"));
    _options_box.add(&_generate_create_index_check, false, true);
    _generate_show_warnings_check.set_text(_("Add SHOW WARNINGS After Every DDL Statement"));
    _options_box.add(&_generate_show_warnings_check, false, true);
    _skip_users_check.set_text(_("Do Not Create Users. Only Export Privileges"));
    _options_box.add(&_skip_users_check, false, true);
    _no_view_placeholders.set_text(_("Don't create view placeholder tables."));
    _options_box.add(&_no_view_placeholders, false, true);

    _generate_insert_check.set_text(_("Generate INSERT Statements for Tables"));
    _options_box.add(&_generate_insert_check, false, true);
    _no_FK_for_inserts.set_text(_("Disable FK checks for inserts"));
    _options_box.add(&_no_FK_for_inserts, false, true);
    _triggers_after_inserts.set_text(_("Create triggers after inserts"));
    _options_box.add(&_triggers_after_inserts, false, true);

    add(&_options, false, true);

    _generate_drop_check.set_active(form->module()->document_int_data("generate_drop", false) != 0);
    _generate_drop_schema_check.set_active(form->module()->document_int_data("generate_schema_drop", 0) != 0);
    _sortTablesAlphabeticallyCheck.set_active(form->module()->document_int_data("SortTablesAlphabetically", 0) != 0);
    _skip_foreign_keys_check.set_active(form->module()->document_int_data("skip_foreign_keys", false) != 0);
    _skip_FK_indexes_check.set_active(form->module()->document_int_data("SkipFKIndexes", false) != 0);
    _omit_schema_qualifier_check.set_active(form->module()->document_int_data("omit_schema_qualifier", false) != 0);
    _generate_create_index_check.set_active(form->module()->document_int_data("generate_create_index", false) != 0);
    _generate_show_warnings_check.set_active(form->module()->document_int_data("generate_show_warnings", false) != 0);
    _skip_users_check.set_active(form->module()->document_int_data("skip_users", false) != 0);
    _no_view_placeholders.set_active(form->module()->document_int_data("no_vew_placeholders", false) != 0);
    _generate_insert_check.set_active(form->module()->document_int_data("generate_insert", false) != 0);
    _generate_use_check.set_active(form->module()->document_int_data("generate_use", false) != 0);
    _generate_use_check.set_enabled(_omit_schema_qualifier_check.get_active());
    _skip_FK_indexes_check.set_enabled(_skip_foreign_keys_check.get_active());
  }

  void SkipFKToggled() {
    _skip_FK_indexes_check.set_enabled(_skip_foreign_keys_check.get_active());
  }

  void OmitSchemaToggled() {
    _generate_use_check.set_enabled(_omit_schema_qualifier_check.get_active());
  }

  void file_changed() {
    validate();
  }

  virtual bool allow_next() {
    return true;
    // return !_filename.get_string_value().empty();
  }

  virtual bool advance() {
    std::string filename = _file_selector->get_filename();

    if (_confirmed_overwrite_for != filename && !_file_selector->check_and_confirm_file_overwrite())
      return false;
    _confirmed_overwrite_for = filename;

    return WizardPage::advance();
  }

  virtual void leave(bool advancing) {
    if (advancing) {
      values().gset("OutputFileName", _file_selector->get_filename());

      values().gset("GenerateDrops", _generate_drop_check.get_active());
      values().gset("GenerateSchemaDrops", _generate_drop_schema_check.get_active());
      values().gset("SortTablesAlphabetically", _sortTablesAlphabeticallyCheck.get_active());
      values().gset("SkipForeignKeys", _skip_foreign_keys_check.get_active());
      values().gset("SkipFKIndexes", _skip_FK_indexes_check.get_active());
      values().gset("GenerateWarnings", _generate_show_warnings_check.get_active());
      values().gset("GenerateCreateIndex", _generate_create_index_check.get_active());
      values().gset("NoUsersJustPrivileges", _skip_users_check.get_active());
      values().gset("NoViewPlaceholders", _no_view_placeholders.get_active());
      values().gset("GenerateInserts", _generate_insert_check.get_active());
      values().gset("NoFKForInserts", _no_FK_for_inserts.get_active());
      values().gset("TriggersAfterInserts", _triggers_after_inserts.get_active());
      values().gset("OmitSchemata", _omit_schema_qualifier_check.get_active());
      values().gset("GenerateUse", _generate_use_check.get_active());

      grt::Module *module = ((WizardPlugin *)_form)->module();

      module->set_document_data("create_sql_output_filename", _file_selector->get_filename());
      module->set_document_data("generate_drop", _generate_drop_check.get_active());
      module->set_document_data("generate_schema_drop", _generate_drop_schema_check.get_active());
      module->set_document_data("SortTablesAlphabetically", _sortTablesAlphabeticallyCheck.get_active());
      module->set_document_data("skip_foreign_keys", _skip_foreign_keys_check.get_active());
      module->set_document_data("SkipFKIndexes", _skip_FK_indexes_check.get_active());
      module->set_document_data("omit_schema_qualifier", _omit_schema_qualifier_check.get_active());
      module->set_document_data("generate_create_index", _generate_create_index_check.get_active());
      module->set_document_data("generate_show_warnings", _generate_show_warnings_check.get_active());
      module->set_document_data("skip_users", _skip_users_check.get_active());
      module->set_document_data("no_vew_placeholders", _no_view_placeholders.get_active());
      module->set_document_data("generate_insert", _generate_insert_check.get_active());
      module->set_document_data("generate_use", _generate_use_check.get_active());
    }
  }

protected:
  std::string _confirmed_overwrite_for;

  Table _contents;
  Label _caption;
  TextEntry _filename;
  Button _browse_button;
  FsObjectSelector *_file_selector;
  Label _skip_out_label;

  Panel _options;
  Box _options_box;

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
  CheckBox _triggers_after_inserts;
  CheckBox _omit_schema_qualifier_check;
  CheckBox _sortTablesAlphabeticallyCheck;
};

//--------------------------------------------------------------------------------

class ExportFilterPage : public WizardObjectFilterPage {
public:
  ExportFilterPage(WizardPlugin *form, DbMySQLSQLExport *export_be)
    : WizardObjectFilterPage(form, "filter"), _export_be(export_be) {
    _table_filter = 0;
    _view_filter = 0;
    _routine_filter = 0;
    _trigger_filter = 0;
    _user_filter = 0;

    set_title(_("SQL Object Export Filter"));
    set_short_title(_("Filter Objects"));

    _top_label.set_wrap_text(true);
    _top_label.set_text(
      _("To exclude objects of a specific type from the SQL Export, disable the corresponding checkbox. "
        "Press Show Filter and add objects or patterns to the ignore list to exclude them from the export."));
  }

protected:
  virtual void enter(bool advancing) {
    if (advancing && !_table_filter)
      setup_filters();

    WizardObjectFilterPage::enter(advancing);
  }

  void setup_filters() {
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

    _routine_filter =
      add_filter(db_mysql_Routine::static_class_name(), _("Export %s Objects"), routines_model, routines_imodel, NULL);

    _trigger_filter =
      add_filter(db_mysql_Trigger::static_class_name(), _("Export %s Objects"), triggers_model, triggers_imodel, NULL);

    _user_filter = add_filter(db_User::static_class_name(), _("Export %s Objects"), users_model, users_imodel, NULL);
  }

  virtual bool advance() {
    std::string header;

    db_CatalogRef catalog(_export_be->get_catalog());
    if (catalog.is_valid() && catalog->owner().is_valid() && catalog->owner()->owner().is_valid()) {
      workbench_DocumentRef doc(workbench_DocumentRef::cast_from(catalog->owner()->owner()));
      std::string description = doc->info()->description();

      base::replaceStringInplace(description, "\n", "\n-- ");

      header = "-- MySQL Script generated by MySQL Workbench\n";
      header += "-- " + base::fmttime(0, "%c") + "\n";
      header += "-- Model: " + *doc->info()->caption() + "    Version: " + *doc->info()->version() + "\n";
      if (!description.empty())
        header += "\n-- " + description + "\n\n";
    }

    _export_be->set_option("OutputFileName", values().get_string("OutputFileName"));
    _export_be->set_option("GenerateDrops", values().get_int("GenerateDrops") != 0);
    _export_be->set_option("GenerateSchemaDrops", values().get_int("GenerateSchemaDrops") != 0);
    _export_be->set_option("SkipForeignKeys", values().get_int("SkipForeignKeys") != 0);
    _export_be->set_option("SkipFKIndexes", values().get_int("SkipFKIndexes") != 0);
    _export_be->set_option("GenerateWarnings", values().get_int("GenerateWarnings") != 0);
    _export_be->set_option("GenerateCreateIndex", values().get_int("GenerateCreateIndex") != 0);
    _export_be->set_option("NoUsersJustPrivileges", values().get_int("NoUsersJustPrivileges") != 0);
    _export_be->set_option("NoViewPlaceholders", values().get_int("NoViewPlaceholders") != 0);
    _export_be->set_option("GenerateInserts", values().get_int("GenerateInserts") != 0);
    _export_be->set_option("NoFKForInserts", values().get_int("NoFKForInserts") != 0);
    _export_be->set_option("TriggersAfterInserts", values().get_int("TriggersAfterInserts") != 0);
    _export_be->set_option("OmitSchemata", values().get_int("OmitSchemata") != 0);
    _export_be->set_option("GenerateUse", values().get_int("GenerateUse") != 0);
    _export_be->set_option("SortTablesAlphabetically", values().get_int("SortTablesAlphabetically") != 0);

    _export_be->set_option("TablesAreSelected", _table_filter->get_active());
    _export_be->set_option("TriggersAreSelected", _trigger_filter->get_active());
    _export_be->set_option("RoutinesAreSelected", _routine_filter->get_active());
    _export_be->set_option("ViewsAreSelected", _view_filter->get_active());
    _export_be->set_option("UsersAreSelected", _user_filter->get_active());

    _export_be->set_option("OutputScriptHeader", header);

    // take target version from the version in the model

    _export_be->set_db_options_for_version(
        GrtVersionRef::cast_from(bec::getModelOption(workbench_physical_ModelRef::cast_from(_export_be->get_catalog()->owner()), "CatalogVersion")));

    return true;
  }

protected:
  DbMySQLSQLExport *_export_be;
  DBObjectFilterFrame *_table_filter;
  DBObjectFilterFrame *_view_filter;
  DBObjectFilterFrame *_routine_filter;
  DBObjectFilterFrame *_trigger_filter;
  DBObjectFilterFrame *_user_filter;
};

class PreviewScriptPage : public ViewTextPage {
  DbMySQLSQLExport *_export_be;
  mforms::Label _label;

public:
  PreviewScriptPage(WizardPlugin *form, DbMySQLSQLExport *export_be)
    : ViewTextPage(form, "preview", (ViewTextPage::Buttons)(ViewTextPage::CopyButton | ViewTextPage::SaveButton),
                   "SQL Scripts (*.sql)|*.sql"),
      _export_be(export_be) {
    set_title(_("Review Generated Script"));
    set_short_title(_("Review SQL Script"));

    _save_button.set_text(_("Save to Other File..."));
    _save_button.set_tooltip(_("Save the script to a file."));

    add(&_label, false, true);
    _label.set_style(mforms::WizardHeadingStyle);

    set_editable(true);
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      if (_export_be->get_output_filename().empty())
        _label.set_text(_("Review the generated script."));
      else
        _label.set_text(_("Review and edit the generated script and press Finish to save."));

      try {
        _export_be->start_export(true);

        set_text(_export_be->export_sql_script());

        _form->clear_problem();
      } catch (std::exception &exc) {
        set_text(std::string(_("Could not generate CREATE script.")).append("\n").append(exc.what()));
        _form->set_problem(_("Error generating script."));
      }
    }
  }

  virtual bool advance() {
    std::string path = values().get_string("OutputFileName");
    if (!path.empty()) {
      save_text_to(path);

      bec::GRTManager::get()->push_status_text(base::strfmt(_("Wrote CREATE Script to '%s'"), path.c_str()));
      grt::GRT::get()->send_info(base::strfmt(_("Wrote CREATE Script to '%s'"), path.c_str()));
    }
    return true;
  }

  virtual std::string next_button_caption() {
    return finish_caption();
  }

  virtual bool next_closes_wizard() {
    return true;
  }
};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class WbPluginSQLExport : public WizardPlugin {
  DbMySQLSQLExport _export_be;

public:
  WbPluginSQLExport(grt::Module *module) : WizardPlugin(module) {
    set_name("SQL Export Wizard");
    add_page(mforms::manage(new ExportInputPage(this)));
    add_page(mforms::manage(new ExportFilterPage(this, &_export_be)));
    add_page(mforms::manage(new PreviewScriptPage(this, &_export_be)));

    set_title(_("Forward Engineer SQL Script"));
  }
};

grtui::WizardPlugin *createExportCREATEScriptWizard(grt::Module *module, db_CatalogRef catalog) {
  return new WbPluginSQLExport(module);
}

void deleteExportCREATEScriptWizard(grtui::WizardPlugin *plugin) {
  delete plugin;
}
