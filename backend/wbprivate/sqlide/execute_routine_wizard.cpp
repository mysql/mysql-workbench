/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "execute_routine_wizard.h"

#include "base/string_utilities.h"

#include "mforms/button.h"
#include "mforms/box.h"
#include "mforms/table.h"
#include "mforms/label.h"
#include "mforms/textentry.h"
#include "mforms/scrollpanel.h"

#include "grtdb/db_object_helpers.h"
#include "grtsqlparser/mysql_parser_services.h"

using namespace base;

//--------------------------------------------------------------------------------------------------

ExecuteRoutineWizard::ExecuteRoutineWizard(db_mysql_RoutineRef routine, const std::string &sql_mode) : Form(NULL) {
  _sql_mode = sql_mode;
  _routine = routine;
  _catalog = db_mysql_CatalogRef::cast_from(_routine->owner()->owner());

  // set_managed();
  set_title(base::strfmt(_("Call stored %s %s.%s"), routine->routineType().c_str(), routine->owner()->name().c_str(),
                         routine->name().c_str()));

  mforms::Box *content = mforms::manage(new mforms::Box(false));
  content->set_padding(12);
  content->set_spacing(20);

  _cancel_button = mforms::manage(new mforms::Button());
  _cancel_button->set_text(_("Cancel"));

  _execxute_button = mforms::manage(new mforms::Button());
  _execxute_button->set_text(_("Execute"));

  mforms::Box *button_bar = mforms::manage(new mforms::Box(true));
  button_bar->set_spacing(12);
  mforms::Utilities::add_end_ok_cancel_buttons(button_bar, _execxute_button, _cancel_button);

  content->add_end(button_bar, false, true);

  mforms::Label *title = mforms::manage(new mforms::Label());
  title->set_text(base::strfmt(_("Enter values for parameters of your %s and click <Execute> to create "
                                 "an SQL editor and run the call:"),
                               routine->routineType().c_str()));
  title->set_wrap_text(true);
  content->add(title, false, true);

  mforms::ScrollPanel *scroll_box = mforms::manage(new mforms::ScrollPanel());
  content->add(scroll_box, true, true);

  // Create a table with a row for each IN and IN/OUT parameter.
  mforms::Table *table = mforms::manage(new mforms::Table());
  table->set_padding(5, 0, 5, 0);
  table->set_column_spacing(4);
  table->set_row_spacing(4);
  table->set_column_count(4);

  // Need an intermediate container as we have to make the scrollbox expand, but don't want
  // the table expand as well.
  mforms::Box *container = mforms::manage(new mforms::Box(false));
  container->add(table, false, true);
  scroll_box->add(container);

  grt::ListRef<db_mysql_RoutineParam> parameters = routine->params();
  table->set_row_count((int)parameters->count());
  for (int i = 0; i < (int)parameters->count(); ++i) {
    db_mysql_RoutineParamRef parameter = parameters[i];

    // Skip pure out parameters.
    if (routine->routineType() == "procedure" && parameter->paramType() == "out")
      continue;

    mforms::Label *text = mforms::manage(new mforms::Label(parameter->name()));
    text->set_style(mforms::BoldStyle);
    text->set_text_align(mforms::MiddleRight);
    table->add(text, 0, 1, i, i + 1);

    mforms::TextEntry *value_entry = mforms::manage(new mforms::TextEntry());
    _edits.push_back(value_entry);
    value_entry->set_size(100, -1);
    table->add(value_entry, 1, 2, i, i + 1, mforms::VFillFlag);

    if (!parameter->paramType().empty()) {
      text = mforms::manage(new mforms::Label("[" + base::toupper(parameter->paramType()) + "]"));
      text->set_text_align(mforms::MiddleLeft);
      text->set_color("#376BA5");
      table->add(text, 2, 3, i, i + 1, mforms::VFillFlag);
    }

    text = mforms::manage(new mforms::Label(parameter->datatype()));
    text->set_style(mforms::InfoCaptionStyle);
    text->set_text_align(mforms::MiddleLeft);
    table->add(text, 3, 4, i, i + 1);
  }

  set_content(content);
  set_size(500, std::min(800, 160 + (int)parameters->count() * 30));
}

//--------------------------------------------------------------------------------------------------

bool ExecuteRoutineWizard::needs_quoting(const std::string &type) {
  // Parse type to see if it needs quoting.
  grt::ListRef<db_SimpleDatatype> default_type_list;
  grt::ListRef<db_SimpleDatatype> type_list;
  GrtVersionRef target_version;
  if (_catalog.is_valid()) {
    default_type_list = _catalog->simpleDatatypes();
    type_list = default_type_list;
    target_version = _catalog->version();
  }

  db_UserDatatypeRef userType;
  db_SimpleDatatypeRef simpleType;
  int precision = bec::EMPTY_COLUMN_PRECISION;
  int scale = bec::EMPTY_COLUMN_SCALE;
  int length = bec::EMPTY_COLUMN_LENGTH;
  std::string datatypeExplicitParams;

  // Since we work with code directly from the server parsing should always succeed.
  // But just in case there's an unexpected error assume quoting is needed.
  parsers::MySQLParserServices *services = parsers::MySQLParserServices::get();
  if (!services->parseTypeDefinition(type, target_version, type_list, grt::ListRef<db_UserDatatype>(),
    default_type_list, simpleType, userType, precision, scale, length, datatypeExplicitParams))
    return true;

  return simpleType->needsQuotes() != 0;
}

//--------------------------------------------------------------------------------------------------

bool is_quoted(const std::string &text) {
  std::string text_ = base::trim(text);
  if (text_.size() < 2)
    return false;

  if (text_[0] == '"' || text_[0] == '\'') {
    char quote_char = text_[0];
    if (text_[text.size() - 1] == quote_char)
      return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

std::string ExecuteRoutineWizard::run() {
  // Generate sql for the caller, so it can be run in an editor.
  std::string result;

  // If there are no input parameters, we don't need to ask the user for anything.
  grt::ListRef<db_mysql_RoutineParam> parameters = _routine->params();
  if (!_edits.empty()) {
    if (!run_modal(_execxute_button, _cancel_button))
      return "";
  }

  GrtVersionRef version;
  if (_catalog.is_valid()) {
    version = _catalog->version();
  }
  MySQLVersion versionEnum = bec::versionToEnum(version);

  std::string schema_name = base::quoteIdentifierIfNeeded(*_routine->owner()->name(), '`', versionEnum);
  std::string routine_name = base::quoteIdentifierIfNeeded(*_routine->name(), '`', versionEnum);
  if (base::tolower(_routine->routineType()) == "procedure") {
    std::string parameters_list;
    std::string variables_list;
    int edit_index = 0;

    for (size_t i = 0; i < parameters->count(); ++i) {
      db_mysql_RoutineParamRef parameter = parameters[i];
      bool quote = needs_quoting(parameter->datatype());
      if (base::tolower(parameter->paramType()) == "in") {
        // A pure input parameter. Just add it to the parameter list.
        if (!parameters_list.empty())
          parameters_list += ", ";

        std::string value = _edits[edit_index++]->get_string_value();

        // Don't quote if the user already did.
        if (quote && is_quoted(value))
          quote = false;
        if (quote)
          parameters_list += "'" + value + "'";
        else
          parameters_list += value;
      } else {
        // Out or in/out parameter.
        // Since we cannot use DECLARE outside stored programs we use SET to define a variable
        // that can take the output of the call. Need to set a dummy value, however.
        std::string parameter_name = base::quoteIdentifierIfNeeded(*parameter->name(), '`', versionEnum);
        result += "set @" + parameter_name + " = ";

        std::string value = "0";
        if (base::tolower(parameter->paramType()) == "inout")
          value = _edits[edit_index++]->get_string_value();

        if (quote && is_quoted(value))
          quote = false;
        if (quote)
          result += "'" + value + "';\n";
        else
          result += value + ";\n";

        if (!parameters_list.empty())
          parameters_list += ", ";
        parameters_list += "@" + parameter_name;

        if (!variables_list.empty())
          variables_list += ", ";
        variables_list += "@" + parameter_name;
      }
    }

    result += "call " + schema_name + "." + routine_name + "(" + parameters_list + ");\n";
    if (!variables_list.empty())
      result += "select " + variables_list + ";\n";

  } else {
    std::string parameter_list;

    for (size_t i = 0; i < _edits.size(); ++i) {
      // For functions there's a 1:1 relationship between input edits and parameters.
      db_mysql_RoutineParamRef parameter = parameters[i];
      if (!parameter_list.empty())
        parameter_list += ", ";

      if (needs_quoting(parameter->datatype()))
        parameter_list += "'" + _edits[i]->get_string_value() + "'";
      else
        parameter_list += _edits[i]->get_string_value();
    }

    result = "select " + schema_name + "." + routine_name + "(" + parameter_list + ");\n";
  }

  for (size_t i = 0; i < _edits.size(); ++i)
    _edits[i]->release();

  return result;
}

//--------------------------------------------------------------------------------------------------
