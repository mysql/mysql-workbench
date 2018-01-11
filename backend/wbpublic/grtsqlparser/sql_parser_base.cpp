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

#include "sql_parser_base.h"
#include "module_utils.h"
#include "grtdb/charset_utils.h"
#include "base/string_utilities.h"
#include <sstream>

#include "base/log.h"

using namespace grt;
using namespace bec;

DEFAULT_LOG_DOMAIN(DOMAIN_SQL_PARSER)

Sql_parser_base::Null_state_keeper::~Null_state_keeper() {
  //_sql_parser->_grt= NULL;
  //_sql_parser->_parse_error_cb.disconnect();
  _sql_parser->_active_obj = db_DatabaseObjectRef();
  _sql_parser->_messages_enabled = true;
  _sql_parser->_progress_state = 0.f;
  _sql_parser->_processed_obj_count = 0;
  _sql_parser->_warn_count = 0;
  _sql_parser->_err_count = 0;
  _sql_parser->_sql_script_preamble = std::string();
  _sql_parser->_sql_statement = std::string();
  _sql_parser->_stopped = false;
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Sql_parser_base::Sql_parser_base()
  : EOL(base::EolHelpers::eol(base::EolHelpers::eol_lf)),
    _is_ast_generation_enabled(true),
    _stopped(false),
    _processed_obj_count(0),
    _warn_count(0),
    _err_count(0),
    _progress_state(0.0),
    _messages_enabled(false) {
  {
    NULL_STATE_KEEPER // reset all members to null-values
  }

  grt::DictRef options = grt::DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
  _case_sensitive_identifiers = options.is_valid() ? (options.get_int("SqlIdentifiersCS", 1) != 0) : true;
}

void Sql_parser_base::set_options(const grt::DictRef &options) {
}

void Sql_parser_base::messages_enabled(bool value) {
  _messages_enabled = value;
}

bool Sql_parser_base::messages_enabled() {
  return _messages_enabled;
}

std::string Sql_parser_base::normalize_identifier_case(const std::string &ident) {
  return _case_sensitive_identifiers ? ident : base::toupper(ident);
}

void Sql_parser_base::add_log_message(const std::string &text, int entry_type) {
  // Keep it that way (don't always write to log file). Messages are usually disabled during
  // syntax checks and the like. Simple SQL errors shouldn't go into the log file.
  if (_messages_enabled) {
    bool send_to_frontend = (!bec::GRTManager::get()->in_main_thread());

    // TODO: The entry types needs a review. It should be streamlined to the logger levels.
    switch (entry_type) {
      case 0: {
        logDebug2("%s", (text + "\n").c_str());

        if (send_to_frontend)
          grt::GRT::get()->send_info(text);
        break;
      }
      case 1: {
        ++_warn_count;
        logDebug("%s", (text + "\n").c_str());

        if (send_to_frontend)
          grt::GRT::get()->send_warning(text);
        break;
      }
      case 2: {
        logDebug("%s", (text + "\n").c_str());

        if (send_to_frontend)
          grt::GRT::get()->send_error(text);
        break;
      }
      default:
        logDebug3("%s", (text + "\n").c_str());
        break;
    }
  }
}

void Sql_parser_base::report_sql_error(int lineno, bool calc_abs_lineno, int err_tok_line_pos, int err_tok_len,
                                       const std::string &err_msg, int entry_type, std::string resolution) {
  ++_err_count;

  if (calc_abs_lineno) {
    // count statement line count
    int stmt_lc = base::EolHelpers::count_lines(_sql_statement);
    int sql_script_preamble_line_count = base::EolHelpers::count_lines(_sql_script_preamble);
    lineno = total_line_count() - stmt_lc - sql_script_preamble_line_count + lineno;
  }

  if (_parse_error_cb)
    _parse_error_cb(lineno, err_tok_line_pos, err_tok_len, err_msg);

  std::ostringstream oss;
  if (_active_obj.is_valid())
    oss << _active_obj.get_metaclass()->get_attribute("caption") << " " << *_active_obj->name() << ". ";
  oss << "Line " << lineno << ": " << err_msg << "." << (resolution.empty() ? "" : " ") << resolution;
  add_log_message(oss.str(), entry_type);
}

void Sql_parser_base::step_progress(const std::string &text) {
  if (!_messages_enabled)
    return;

  //! cycling progress state for now. statement count estimation is needed.
  _progress_state = (float)(div((int)(_progress_state * 10) + 1, 10).rem) / 10;

  grt::GRT::get()->send_progress(_progress_state, _("Processing object"), text);
}

void Sql_parser_base::set_progress_state(float state, const std::string &text) {
  if (!_messages_enabled)
    return;
  grt::GRT::get()->send_progress(state, text);
}

const std::string &Sql_parser_base::sql_statement() {
  return _sql_statement;
}

void Sql_parser_base::parse_error_cb(Parse_error_cb cb) {
  _parse_error_cb = cb;
}

Sql_parser_base::Parse_error_cb &Sql_parser_base::parse_error_cb() {
  return _parse_error_cb;
}

void Sql_parser_base::do_report_sql_statement_border(int begin_lineno, int begin_line_pos, int end_lineno,
                                                     int end_line_pos) {
  // calculate lineno shift to get absolute line numbers
  int lineno_shift;
  {
    int stmt_lc = base::EolHelpers::count_lines(_sql_statement);
    int sql_script_preamble_line_count = base::EolHelpers::count_lines(_sql_script_preamble);
    lineno_shift = total_line_count() - stmt_lc - sql_script_preamble_line_count;
  }
  if (report_sql_statement_border)
    report_sql_statement_border(begin_lineno + lineno_shift, begin_line_pos, end_lineno + lineno_shift, end_line_pos);
}
