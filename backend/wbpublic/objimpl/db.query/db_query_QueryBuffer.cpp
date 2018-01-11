/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.db.query.h>

#include <grtpp_util.h>
#include "sqlide/sql_editor_be.h"
#include "db_query_Editor.h"
#include "db_query_QueryBuffer.h"

void db_query_QueryBuffer::init() {
  // _data init is delayed and done by grtwrap_sqleditor
}

db_query_QueryBuffer::~db_query_QueryBuffer() {
  delete _data;
}

void db_query_QueryBuffer::set_data(ImplData *data) {
  _data = data;
}

grt::IntegerRef db_query_QueryBuffer::insertionPoint() const {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    return grt::IntegerRef(editor->cursor_pos());
  }
  return grt::IntegerRef(0);
}

void db_query_QueryBuffer::insertionPoint(const grt::IntegerRef &value) {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    editor->set_cursor_pos(*value);
  }
}

grt::StringRef db_query_QueryBuffer::script() const {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    return grt::StringRef(editor->sql());
  }
  return grt::StringRef();
}

grt::StringRef db_query_QueryBuffer::currentStatement() const {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    return grt::StringRef(editor->current_statement());
  }
  return grt::StringRef();
}

grt::StringRef db_query_QueryBuffer::selectedText() const {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    return grt::StringRef(editor->selected_text());
  }
  return grt::StringRef();
}

void db_query_QueryBuffer::selectionEnd(const grt::IntegerRef &value) {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());

    // Read the old start value, as we pass an entire range to the editor.
    size_t start = 0, end = 0;
    editor->selected_range(start, end);
    editor->set_selected_range(start, *value);
  }
}

void db_query_QueryBuffer::selectionStart(const grt::IntegerRef &value) {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());

    // Read the old end value, as we pass an entire range to the editor.
    size_t start = 0, end = 0;
    editor->selected_range(start, end);
    editor->set_selected_range(*value, end);
  }
}

grt::IntegerRef db_query_QueryBuffer::selectionEnd() const {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    size_t start, end;
    if (editor->selected_range(start, end))
      return grt::IntegerRef(end);
  }
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_QueryBuffer::selectionStart() const {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    size_t start, end;
    if (editor->selected_range(start, end))
      return grt::IntegerRef(start);
  }
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_QueryBuffer::replaceContents(const std::string &text) {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    editor->set_refresh_enabled(true);
    editor->sql(text.c_str());
  }
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_QueryBuffer::replaceSelection(const std::string &text) {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    editor->set_selected_text(text);
  }
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_QueryBuffer::replaceCurrentStatement(const std::string &text) {
  if (_data) {
    MySQLEditor::Ref editor(_data->editor.lock());
    size_t start, end;

    if (editor->get_current_statement_range(start, end)) {
      editor->set_selected_range(start, end);
      editor->set_selected_text(text);
      return grt::IntegerRef(0);
    }
  }
  return grt::IntegerRef(-1);
}
