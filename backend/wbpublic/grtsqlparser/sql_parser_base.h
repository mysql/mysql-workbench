/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wbpublic_public_interface.h"
#include "grt/grt_manager.h"
#include "grt.h"
#include "grts/structs.db.h"

#ifndef THROW
#ifdef _MSC_VER
#define THROW(...)
#else
#define THROW(...) throw(__VA_ARGS__)
#endif
#endif



/** Encapsulates generic DBMS agnostic functionality for processing of SQL statements/scripts:
 * <li>definition of enumeration types and callbacks
 * <li>handling of errors
 * <li>GRT messaging and reporting of progress
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_parser_base {
protected:
  Sql_parser_base();
  virtual ~Sql_parser_base() {
  }

public:
  const std::string &eol() const {
    return EOL;
  }
  void eol(const std::string &value) {
    EOL = value;
  }

protected:
  std::string EOL;

public:
  virtual void sql_mode(const std::string &value) {
  }

public:
  typedef std::function<int(int, int, int, const std::string &)> Parse_error_cb;
  void parse_error_cb(Parse_error_cb cb);
  Parse_error_cb &parse_error_cb();

public:
  typedef std::function<int(int, int, int, int)> Report_sql_statement_border;
  Report_sql_statement_border report_sql_statement_border;

protected:
  void do_report_sql_statement_border(int begin_lineno, int begin_line_pos, int end_lineno, int end_line_pos);

public:
  bool is_ast_generation_enabled() const {
    return _is_ast_generation_enabled;
  }
  void is_ast_generation_enabled(bool value) {
    _is_ast_generation_enabled = value;
  }

protected:
  bool _is_ast_generation_enabled;

public:
  bool stop() {
    return _stopped = _stop_cb ? _stop_cb() : false;
  }

protected:
  boost::function<bool()> _stop_cb;
  bool _stopped;

  template <class Slot>
  class SlotAutoDisconnector {
  private:
    Slot &_slot;

  public:
    SlotAutoDisconnector(Slot &slot) : _slot(slot) {
    }
    ~SlotAutoDisconnector() {
      _slot.clear();
    }
  };

private:
  Parse_error_cb _parse_error_cb;

public:
  void case_sensitive_identifiers(bool val) {
    _case_sensitive_identifiers = val;
  }
  bool case_sensitive_identifiers() {
    return _case_sensitive_identifiers;
  }

protected:
  bool _case_sensitive_identifiers;

protected:
  std::string normalize_identifier_case(const std::string &ident);

public:
  void messages_enabled(bool value);
  bool messages_enabled();

protected:
  // aux types
  enum Parse_result { pr_irrelevant = 0, pr_processed, pr_invalid };

  // initialization
  virtual void set_options(const grt::DictRef &options);

  // state monitoring
  void add_log_message(const std::string &text, int entry_type);
  void report_sql_error(int lineno, bool calc_abs_lineno, int err_tok_line_pos, int err_tok_len,
                        const std::string &err_msg, int entry_type, std::string resolution = "Statement skipped.");
  void step_progress(const std::string &text);
  void set_progress_state(float state, const std::string &text);

  // misc
  const std::string &sql_statement();
  virtual int total_line_count() = 0;

  // data members
  std::string _sql_statement;
  std::string _sql_script_preamble;
  std::size_t _processed_obj_count;
  std::size_t _warn_count;
  std::size_t _err_count;
  float _progress_state;
  bool _messages_enabled;
  db_DatabaseObjectRef _active_obj;

  class Parse_exception : public std::exception {
  public:
    Parse_exception(const std::string &msg_text) : _msg_text(msg_text), _flag(2){};
    Parse_exception(const char *msg_text) : _msg_text(msg_text), _flag(2){};
    virtual ~Parse_exception() throw() {
    }
    const char *what() const throw() {
      return _msg_text.c_str();
    }
    int flag() const {
      return _flag;
    }
    void flag(int val) {
      _flag = val;
    }

  private:
    std::string _msg_text;
    int _flag;
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC Null_state_keeper {
  public:
    Null_state_keeper(Sql_parser_base *sql_parser) : _sql_parser(sql_parser) {
    }
    virtual ~Null_state_keeper();

  protected:
    Sql_parser_base *_sql_parser;
  };
  friend class Null_state_keeper;
};
