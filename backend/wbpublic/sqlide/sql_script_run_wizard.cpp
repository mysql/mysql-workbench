/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtdb/db_helpers.h"
#include "sql_script_run_wizard.h"

#include "mforms/code_editor.h"
#include "mforms/selector.h"
#include "mforms/button.h"

//--------------------------------------------------------------------------------------------------

SqlScriptReviewPage::SqlScriptReviewPage(grtui::WizardForm *form, GrtVersionRef version, std::string algorithm,
                                         std::string lock)
  : grtui::WizardPage(form, "review"), _box(false) {
  set_title(_("Review the SQL Script to be Applied on the Database"));
  set_short_title(_("Review SQL Script"));

  _box.set_spacing(5);
  add(&_box, true, true);

  _page_heading.set_text(
    _("Please review the following SQL script that will be applied to the database.\n"
      "Note that once applied, these statements may not be revertible without losing some of the data.\n"
      "You can also manually change the SQL statements before execution."));
  _page_heading.set_wrap_text(true);
  _box.add(&_page_heading, false, false);

  // Online DDL options. Structure is very similar as in the preferences, just on one line, instead 2.
  // Add them only if online DDL options were given (which only happens for meta data changes).
  // This is not supposed to be shown for pre 5.6 servers
  if (!algorithm.empty() && !lock.empty() &&
      (version.is_valid() && bec::is_supported_mysql_version_at_least(version, 5, 6))) {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Online DDL"));
    _box.add(frame, false);

    // The passed in values for online DDL algorithm and lock are used to properly
    // initialize the selectors.
    mforms::Box *line_box = mforms::manage(new mforms::Box(true));
    line_box->set_padding(20, 0, 20, 0);
    line_box->set_spacing(20);
    frame->add(line_box);

    mforms::Label *label = mforms::manage(new mforms::Label(_("Algorithm:")));
    line_box->add(label, false, false);

    _algorithm_selector = mforms::manage(new mforms::Selector());
    scoped_connect(_algorithm_selector->signal_changed(), std::bind(&SqlScriptReviewPage::option_changed, this));
    _algorithm_selector->add_item("Default");
    _algorithm_selector->add_item("In place");
    _algorithm_selector->add_item("Copy");
    if (algorithm == "INPLACE")
      _algorithm_selector->set_selected(1);
    else if (algorithm == "COPY")
      _algorithm_selector->set_selected(2); // else leave it at 0.
    _algorithm_selector->set_size(130, -1);
    _algorithm_selector->set_tooltip(
      _("If the currently connected server supports online DDL then use the selected "
        "algorithm as default. This setting can also be adjusted for each alter operation."));
    line_box->add(_algorithm_selector, false, false);

    label = mforms::manage(new mforms::Label(_("Lock Type:")));
    line_box->add(label, false, false);

    _lock_selector = mforms::manage(new mforms::Selector());
    scoped_connect(_lock_selector->signal_changed(), std::bind(&SqlScriptReviewPage::option_changed, this));
    _lock_selector->add_item("Default");
    _lock_selector->add_item("None");
    _lock_selector->add_item("Shared");
    _lock_selector->add_item("Exclusive");
    if (lock == "NONE")
      _lock_selector->set_selected(1);
    else if (lock == "SHARED")
      _lock_selector->set_selected(2);
    else if (lock == "EXCLUSIVE")
      _lock_selector->set_selected(3); // else leave it at 0.
    _lock_selector->set_size(130, -1);
    _lock_selector->set_tooltip(
      _("If the currently connected server supports online DDL then use the selected "
        "lock as default. This setting can also be adjusted for each alter operation."));
    line_box->add(_lock_selector, false, false);
  } else {
    _lock_selector = 0;
    _algorithm_selector = 0;
  }

  _sql_editor = mforms::manage(new mforms::CodeEditor());
  if (!version.is_valid() || version->majorNumber() < 5)
    _sql_editor->set_language(mforms::LanguageMySQL);
  else {
    switch (version->minorNumber()) {
      case 6:
        _sql_editor->set_language(mforms::LanguageMySQL56);
        break;

      case 7:
        _sql_editor->set_language(mforms::LanguageMySQL57);
        break;
        
      default:
        _sql_editor->set_language(mforms::LanguageMySQL);
    }
  }
  _box.add(_sql_editor, true, true);
}

//--------------------------------------------------------------------------------------------------

SqlScriptReviewPage::~SqlScriptReviewPage() {
  // No need to release _algorithm_selector and _lock_selector. They are managed with release_on_add.
  _sql_editor->release();
};

//--------------------------------------------------------------------------------------------------

void SqlScriptReviewPage::enter(bool advancing) {
  _sql_editor->set_value(values().get_string("sql_script"));
  grtui::WizardPage::enter(advancing);
}

//--------------------------------------------------------------------------------------------------

bool SqlScriptReviewPage::advance() {
  std::string sql = base::trim(_sql_editor->get_text(false));

  if (sql.empty())
    return false;
  else
    values().gset("sql_script", sql);

  return grtui::WizardPage::advance();
}

//--------------------------------------------------------------------------------------------------

std::string SqlScriptReviewPage::next_button_caption() {
  return _("Apply");
}

//--------------------------------------------------------------------------------------------------

void SqlScriptReviewPage::option_changed() {
  SqlScriptRunWizard *wizard = dynamic_cast<SqlScriptRunWizard *>(_form);
  if (wizard != NULL && wizard->regenerate_script) {
    static std::string algorithms[] = {"DEFAULT", "INPLACE", "COPY"};
    std::string algorithm = algorithms[_algorithm_selector->get_selected_index()];

    static std::string locks[] = {"DEFAULT", "NONE", "SHARED", "EXCLUSIVE"};
    std::string lock = locks[_lock_selector->get_selected_index()];

    _sql_editor->set_value(wizard->regenerate_script(algorithm, lock));
  }
}

//----------------- SqlScriptApplyPage -------------------------------------------------------------

SqlScriptApplyPage::SqlScriptApplyPage(grtui::WizardForm *form)
  : grtui::WizardProgressPage(form, "apply", true), _err_count(0) {
  set_title(_("Applying SQL script to the database"));
  set_short_title(_("Apply SQL Script"));

  /*TaskRow *task=*/add_async_task(_("Execute SQL Statements"),
                                   std::bind(&SqlScriptApplyPage::execute_sql_script, this),
                                   _("Executing SQL Statements..."));

  end_adding_tasks(_("SQL script was successfully applied to the database."));

  {
    _abort_btn = mforms::manage(new mforms::Button());
    _abort_btn->set_text("Abort");
    _abort_btn->signal_clicked()->connect(std::bind(&SqlScriptApplyPage::abort_exec, this));
    _progress_bar_box->add_end(_abort_btn, false, true);
  }
  set_status_text("");
}

void SqlScriptApplyPage::abort_exec() {
  dynamic_cast<SqlScriptRunWizard *>(_form)->abort_apply();
}

int SqlScriptApplyPage::on_error(long long err_code, const std::string &err_msg, const std::string &err_sql) {
  std::string sql = base::strip_text(err_sql, true, true);
  _log += "ERROR";
  if (err_code >= 0)
    _log += base::strfmt(" %lli", err_code);
  _log += base::strfmt(": %s\n", err_msg.c_str());
  if (!err_sql.empty())
    _log += base::strfmt("SQL Statement:\n%s\n", sql.c_str());
  _log += "\n";
  return 0;
}

int SqlScriptApplyPage::on_exec_progress(float progress) {
  update_progress(progress, "");
  return 0;
}

int SqlScriptApplyPage::on_exec_stat(long success_count, long err_count) {
  _err_count = err_count;
  return 0;
}

grt::ValueRef SqlScriptApplyPage::do_execute_sql_script(const std::string &sql_script) {
  bec::GRTManager::get()->run_once_when_idle(
    this, std::bind(&SqlScriptApplyPage::add_log_text, this, "Executing:\n" + sql_script + "\n"));

  apply_sql_script(sql_script);

  if (_err_count) {
    values().gset("has_errors", 1);
    bec::GRTManager::get()->run_once_when_idle(this, std::bind(&SqlScriptApplyPage::add_log_text, this, _log));
    throw std::runtime_error(_("There was an error while applying the SQL script to the database."));
  } else {
    bec::GRTManager::get()->run_once_when_idle(
      this,
      std::bind(&SqlScriptApplyPage::add_log_text, this, _("SQL script was successfully applied to the database.")));
  }

  return grt::ValueRef();
}

bool SqlScriptApplyPage::execute_sql_script() {
  values().gset("applied", 1);
  values().gset("has_errors", 0);
  std::string sql_script = values().get_string("sql_script");

  // apply_sql_script(sql_script);

  execute_grt_task(std::bind(&SqlScriptApplyPage::do_execute_sql_script, this, sql_script), false);

  return true;
}

std::string SqlScriptApplyPage::next_button_caption() {
  return finish_button_caption();
}

bool SqlScriptApplyPage::allow_back() {
  return !_busy && !_done;
}

bool SqlScriptApplyPage::allow_next() {
  return !_busy && values().get_int("has_errors") == 0;
}

bool SqlScriptApplyPage::allow_cancel() {
  return values().get_int("has_errors") != 0;
}

void SqlScriptApplyPage::enter(bool advancing) {
  if (dynamic_cast<SqlScriptRunWizard *>(_form)->abort_apply)
    _abort_btn->show(true);
  else
    _abort_btn->show(false);

  if (advancing) {
    _log_text.set_value("");
  }
  WizardProgressPage::enter(advancing);
}

//-----------------------
// SqlScriptRunWizard
//-----------------------

SqlScriptRunWizard::SqlScriptRunWizard(GrtVersionRef version, std::string algorithm, std::string lock)
  : grtui::WizardForm() {
  set_name("Script Run Wizard");
  setInternalName("script_run_wizard");
  set_title(_("Apply SQL Script to Database"));
  review_page = new SqlScriptReviewPage(this, version, algorithm, lock);
  add_page(mforms::manage(review_page));
  apply_page = new SqlScriptApplyPage(this);
  add_page(mforms::manage(apply_page));
  values().gset("has_errors", 0);
  values().gset("applied", 0);
}

bool SqlScriptRunWizard::has_errors() {
  return values().get_int("has_errors") != 0;
}

bool SqlScriptRunWizard::applied() {
  return values().get_int("applied") != 0;
}
