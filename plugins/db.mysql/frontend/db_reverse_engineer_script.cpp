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

#include "db_reverse_engineer_script.h"
#include "base/string_utilities.h"

using namespace ScriptImport;

//--------------------------------------------------------------------------------------------------

ImportInputPage::ImportInputPage(WizardPlugin *form) : WizardPage(form, "options"), _file_selector(true) {
  set_title(_("Input and Options"));
  set_short_title(_("Input and Options"));

  add(&_table, true, true);
  _table.set_row_count(5);
  _table.set_column_count(2);
  _table.set_row_spacing(14);
  _table.set_column_spacing(4);
  _table.set_padding(12);

  _heading.set_style(WizardHeadingStyle);
  _heading.set_text(_("Select the script containing the schemas to reverse engineer"));
  _table.add(&_heading, 0, 2, 0, 1, mforms::HFillFlag);

  _caption.set_text_align(mforms::TopLeft);
  _caption.set_text(_("Select SQL script file:"));
  _table.add(&_caption, 0, 1, 1, 2, mforms::HFillFlag | mforms::VFillFlag);
  _table.add(&_file_selector, 1, 2, 1, 2, mforms::HExpandFlag | mforms::HFillFlag | mforms::VFillFlag);
  _file_selector.set_size(-1, 24);

  std::string initial_filename = form->module()->document_string_data("input_filename", "");
  _file_selector.initialize(initial_filename, mforms::OpenFile, "SQL Files (*.sql)|*.sql", false,
                            std::bind(&WizardPage::validate, this));
  scoped_connect(_file_selector.signal_changed(), std::bind(&ImportInputPage::file_changed, this));

  _file_codeset_caption.set_text(_("File encoding:"));
  _file_codeset_caption.set_text_align(mforms::TopLeft);

  _table.add(&_file_codeset_caption, 0, 1, 2, 3, mforms::HFillFlag);
  _table.add(&_file_codeset_sel, 1, 2, 2, 3, mforms::HExpandFlag | mforms::HFillFlag);

  _file_codeset_sel.set_enabled(false); // Temporary stick this to the default (utf-8). We cannot convert encodings.

  // Fill the encoding selector with a useful list of encodings.
  fill_encodings_list();

  _table.add(&_autoplaceCheck, 1, 2, 3, 4, mforms::HExpandFlag | mforms::HFillFlag);
  _autoplaceCheck.set_text(_("Place imported objects on a diagram"));
  _autoplaceCheck.set_active(form->module()->document_int_data("place_figures", 0) != 0);

  _table.add(&_ansiQuotesCheck, 1, 2, 4, 5, mforms::HExpandFlag | mforms::HFillFlag);
  _ansiQuotesCheck.set_text(_("Use ANSI quotes"));
  _ansiQuotesCheck.set_active(false);

  scoped_connect(signal_leave(), std::bind(&ImportInputPage::gather_options, this, std::placeholders::_1));
}

//--------------------------------------------------------------------------------------------------

/**
 * Fills the selector (lookup) for file encodings with useful values.
 */
void ImportInputPage::fill_encodings_list() {
  const char *encodings[] = {"ARMSCII8", "ASCII",   "BIG5",   "BINARY", "CP1250", "CP1251",   "CP1256", "CP1257",
                             "CP850",    "CP852",   "CP866",  "CP932",  "DEC8",   "EUCJPMS",  "EUCKR",  "GB2312",
                             "GBK",      "GEOSTD8", "GREEK",  "HEBREW", "HP8",    "KEYBCS2",  "KOI8R",  "KOI8U",
                             "LATIN1",   "LATIN2",  "LATIN5", "LATIN7", "MACCE",  "MACROMAN", "SJIS",   "SWE7",
                             "TIS620",   "UCS2",    "UJIS",   "UTF8"};

  int encodings_count = sizeof(encodings) / sizeof(encodings[0]);
  for (int i = 0; i < encodings_count; ++i)
    _file_codeset_sel.add_item(encodings[i]);

  // As default find and select UTF8.
  const std::string default_encoding = "UTF8";
  int default_encoding_index = -1;
  for (int i = 0; i < encodings_count; ++i) {
    if (default_encoding == encodings[i]) {
      default_encoding_index = i;
      break;
    }
  }
  if (default_encoding_index > 0)
    _file_codeset_sel.set_selected(default_encoding_index);
}

//--------------------------------------------------------------------------------------------------

void ImportInputPage::file_changed() {
  validate();
}

//--------------------------------------------------------------------------------------------------

bool ImportInputPage::allow_next() {
  std::string name = _file_selector.get_filename();
  if (name.empty())
    return false;

  if (!g_file_test(name.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)))
    return false;

  return true;
}

//--------------------------------------------------------------------------------------------------

std::string ImportInputPage::next_button_caption() {
  return execute_caption();
}

//--------------------------------------------------------------------------------------------------

void ImportInputPage::gather_options(bool advancing) {
  values().gset("import.filename", _file_selector.get_filename());
  values().gset("import.file_codeset", _file_codeset_sel.get_string_value());
  values().gset("import.place_figures", _autoplaceCheck.get_active());
  values().gset("import.useAnsiQuotes", _ansiQuotesCheck.get_active());

  grt::Module *module = ((WizardPlugin *)_form)->module();

  module->set_document_data("input_filename", _file_selector.get_filename());
  module->set_document_data("place_figures", _autoplaceCheck.get_active());
}

//--------------------------------------------------------------------------------------------------

ImportProgressPage::ImportProgressPage(WizardForm *form, const std::function<void(bool, std::string)> &finished_cb)
  : WizardProgressPage(form, "progress", true) {
  set_title(_("Reverse Engineering Progress"));
  set_short_title(_("Reverse Engineer"));

  _finished_cb = finished_cb;

  _auto_place = false;
  _done = false;

  _import_be.grtm();

  // showAdvancedImportLogPage = false;
  // caseSensitiveIdentifiersCB.Checked = sqlImport.get_bool_option("SqlIdentifiersCS");

  TaskRow *task = add_async_task(_("Reverse Engineer SQL Script"), std::bind(&ImportProgressPage::import_objects, this),
                                 _("Reverse engineering and importing objects from script..."));
  task->process_finish = std::bind(&ImportProgressPage::import_objects_finished, this, std::placeholders::_1);

  add_task(_("Verify Results"), std::bind(&ImportProgressPage::verify_results, this),
           _("Verifying imported objects..."));

  _auto_place_task = add_async_task(_("Place Objects on Diagram"), std::bind(&ImportProgressPage::place_objects, this),
                                    _("Placing imported objects on a new diagram..."));

  end_adding_tasks(_("Import finished."));

  set_status_text("");
}

//--------------------------------------------------------------------------------------------------

void ImportProgressPage::import_objects_finished(grt::ValueRef value) {
  grt::GRT::get()->send_info(grt::StringRef::cast_from(value));
}

//--------------------------------------------------------------------------------------------------

bool ImportProgressPage::import_objects() {
  execute_grt_task(_import_be.get_task_slot(), false);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool ImportProgressPage::verify_results() {
  return true;
}

//--------------------------------------------------------------------------------------------------

bool ImportProgressPage::place_objects() {
  if (_auto_place) {
    execute_grt_task(_import_be.get_autoplace_task_slot(), false);
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ImportProgressPage::allow_back() {
  return false;
}

//--------------------------------------------------------------------------------------------------

void ImportProgressPage::enter(bool advancing) {
  if (advancing) {
    _import_be.sql_script(values().get_string("import.filename"));
    _import_be.encoding(values().get_string("import.file_codeset"));
    _import_be.sqlMode(values().get_int("import.useAnsiQuotes") != 0 ? "ANSI_QUOTES" : "");

    _auto_place = values().get_int("import.place_figures") != 0;

    _auto_place_task->set_enabled(_auto_place);

    // sqlImport.set_option("case_sensitive_identifiers", caseSensitiveIdentifiersCB.Checked);
    // sqlImport.set_option("processing_create_statements", procCreateStmtCB.Checked);
    // sqlImport.set_option("processing_alter_statements", procAlterStmtCB.Checked);
    // sqlImport.set_option("processing_drop_statements", procDropStmtCB.Checked);
  }
  WizardProgressPage::enter(advancing);
}

//--------------------------------------------------------------------------------------------------

std::string ImportProgressPage::get_summary() {
  std::string summary;
  int schemas = 0, tables = 0, views = 0, procedures = 0;

  grt::ListRef<GrtObject> created_objects = _import_be.get_created_objects();

  for (grt::ListRef<GrtObject>::const_iterator iter = created_objects.begin(); iter != created_objects.end(); ++iter) {
    if ((*iter).is_instance<db_Schema>())
      schemas++;
    else if ((*iter).is_instance<db_Table>())
      tables++;
    else if ((*iter).is_instance<db_View>())
      views++;
    else if ((*iter).is_instance<db_Routine>())
      procedures++;
  }

  summary = base::strfmt(_("Import of SQL script file '%s' has finished.\n\n"
                           "%i tables, %i views and %i stored procedures were imported in %i schemas.\n\n"),
                         _import_be.sql_script().c_str(), tables, views, procedures, schemas);

  if (_got_error_messages)
    summary.append(_("There were errors during the import.\n"));
  else if (_got_warning_messages)
    summary.append(_("There were warnings during the import.\n"));

  summary.append(_("Go Back to the previous page to review the logs."));

  return summary;
}

void ImportProgressPage::tasks_finished(bool success) {
  if (_finished_cb)
    _finished_cb(success, get_summary());
}

//--------------------------------------------------------------------------------------------------

WbPluginSQLImport::WbPluginSQLImport(grt::Module *module) : WizardPlugin(module) {
  set_name("SQL Import Wizard");
  _input_page = new ImportInputPage(this);
  _progress_page = new ImportProgressPage(
    this, std::bind(&WbPluginSQLImport::update_summary, this, std::placeholders::_1, std::placeholders::_2));

  _finish_page = new WizardFinishedPage(this, _("SQL Import Finished"));

  add_page(mforms::manage(_input_page));
  add_page(mforms::manage(_progress_page));
  add_page(mforms::manage(_finish_page));

  set_title(_("Reverse Engineer SQL Script"));
}

//--------------------------------------------------------------------------------------------------

void WbPluginSQLImport::update_summary(bool success, const std::string &summary) {
  _finish_page->set_title(success ? _("SQL Import Finished Successfully") : _("SQL Import Failed"));
  _finish_page->set_summary(summary);
}

//--------------------------------------------------------------------------------------------------

grtui::WizardPlugin *createImportScriptWizard(grt::Module *module, db_CatalogRef catalog) {
  return new ScriptImport::WbPluginSQLImport(module);
}

void deleteImportScriptWizard(grtui::WizardPlugin *plugin) {
  delete plugin;
}
