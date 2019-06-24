
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

#include "base/trackable.h"

#ifndef _MSC_VER
#include <memory>
#include <set>

#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.query.h"

#endif

#include "grtsqlparser/mysql_parser_services.h"
#include "grtdb/db_helpers.h"

#include "Scintilla.h"

namespace bec {
  class GRTManager;
}

namespace mforms {
  class CodeEditor;
  class FindPanel;
  class Menu;
  class View;
  class CodeEditorConfig;
  class ToolBar;
  class DropDelegate;
};

class MySQLRecognizer;

namespace parsers {
  class SymbolTable;
}

/**
 * The legacy MySQL editor class.
 */
class WBPUBLICBACKEND_PUBLIC_FUNC MySQLEditor : public base::trackable {
public:
  enum ContentType {
    ContentTypeGeneral,
    ContentTypeTrigger,
    ContentTypeView,
    ContentTypeFunction,
    ContentTypeProcedure,
    ContentTypeUdf,
    ContentTypeRoutine,
    ContentTypeEvent,
  };

  typedef std::shared_ptr<MySQLEditor> Ref;
  typedef std::weak_ptr<MySQLEditor> Ptr;

  static Ref create(parsers::MySQLParserContext::Ref syntaxCheckContext,
                    parsers::MySQLParserContext::Ref autocompleteContext,
                    std::vector<parsers::SymbolTable *> const &globalSymbols,
                    db_query_QueryBufferRef grtobj = db_query_QueryBufferRef());

  virtual ~MySQLEditor();

  db_query_QueryBufferRef grtobj();

  void set_base_toolbar(mforms::ToolBar *toolbar);

  mforms::View *get_container();
  mforms::ToolBar *get_toolbar(bool include_file_actions = true);
  mforms::CodeEditor *get_editor_control();
  mforms::FindPanel *get_find_panel();

  void show_special_chars(bool flag);
  void enable_word_wrap(bool flag);

  int int_option(std::string name);
  std::string string_option(std::string name);

  void set_current_schema(const std::string &schema);
  std::string sql();
  std::pair<const char *, size_t> text_ptr();
  void sql(const char *sql);

  bool empty();
  void append_text(const std::string &text);

  std::string current_statement();
  bool get_current_statement_range(size_t &start, size_t &end, bool strict = false);

  std::size_t cursor_pos();
  std::pair<std::size_t, std::size_t> cursor_pos_row_column(bool local);
  void set_cursor_pos(std::size_t position);

  bool selected_range(std::size_t &start, std::size_t &end);
  void set_selected_range(std::size_t start, std::size_t end);

  bool is_refresh_enabled() const;
  void set_refresh_enabled(bool val);
  bool is_sql_check_enabled() const;
  void set_sql_check_enabled(bool val);

  void show_auto_completion(bool auto_choose_single);
  std::vector<std::pair<int, std::string>> update_auto_completion(const std::string &typed_part);
  void cancel_auto_completion();

  std::string selected_text();
  void set_selected_text(const std::string &new_text);
  void insert_text(const std::string &new_text);

  boost::signals2::signal<void()> *text_change_signal();

  std::string sql_mode();
  void set_sql_mode(const std::string &value);
  void setServerVersion(GrtVersionRef version);

  void restrict_content_to(ContentType type);

  bool has_sql_errors() const;

  void stop_processing();

  void focus();

  void register_file_drop_for(mforms::DropDelegate *target);

protected:
  MySQLEditor(parsers::MySQLParserContext::Ref syntaxCheckContext,
              parsers::MySQLParserContext::Ref autocompleteContext);

private:
  class Private;
  Private *d;

  void set_grtobj(db_query_QueryBufferRef grtobj);

  void setup_auto_completion();
  void *run_code_completion();

  std::string getWrittenPart(size_t position);

  void text_changed(Sci_Position position, Sci_Position length, Sci_Position lines_changed, bool added);
  void char_added(int char_code);
  void dwell_event(bool started, size_t position, int x, int y);

  void setup_editor_menu();
  void editor_menu_opening();
  void activate_context_menu_item(const std::string &name);

  bool start_sql_processing();
  bool do_statement_split_and_check(int id); // Run in worker thread.

  int on_report_sql_statement_border(int begin_lineno, int begin_line_pos, int end_lineno, int end_line_pos, int tag);
  int on_sql_error(int lineno, int tok_line_pos, int tok_len, const std::string &msg, int tag);
  int on_sql_check_progress(float progress, const std::string &msg, int tag);

  void *splitting_done();
  void *update_error_markers();

  bool code_completion_enabled();
  bool auto_start_code_completion();
  bool make_keywords_uppercase();
};
