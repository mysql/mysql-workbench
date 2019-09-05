/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <glib.h>
#include <glib/gstdio.h>

#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/util_functions.h"
#include "base/sqlstring.h"
#include "base/log.h"

#include "wb_sql_editor_snippets.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/wb_sql_editor_panel.h"
#include "workbench/wb_db_schema.h"

#include "mforms/utilities.h"
#include "mforms/filechooser.h"

DEFAULT_LOG_DOMAIN("SQLSnippets")

using namespace mforms;

static DbSqlEditorSnippets *singleton = 0;

void DbSqlEditorSnippets::setup(wb::WBContextSQLIDE *sqlide, const std::string &path) {
  if (singleton != 0)
    return;

  singleton = new DbSqlEditorSnippets(sqlide, path);
}

DbSqlEditorSnippets *DbSqlEditorSnippets::get_instance() {
  return singleton;
}

//--------------------------------------------------------------------------------------------------

bool DbSqlEditorSnippets::activate_toolbar_item(const bec::NodeId &selected, const std::string &name) {
  if (name == "restore_snippets") {
    DialogResult result = (DialogResult)Utilities::show_message(
      _("Restore snippet list"),
      base::strfmt(_("You are about to restore the \"%s\" snippet list to its original state. "
                     "All changes will be lost.\n\nDo you want to continue?"),
                   _selected_category.c_str()),
      _("Continue"), _("Cancel"));
    if (result == mforms::ResultOk) {
      copy_original_file(_selected_category + ".txt", true);
      load();
    }

    return true;
  }

  if (name == "add_snippet") {
    SqlEditorForm *editor_form = _sqlide->get_active_sql_editor();
    if (editor_form)
      editor_form->save_snippet();
    return true;
  }

  if (name == "del_snippet" && selected.is_valid() && selected[0] < _entries.size()) {
    delete_node(selected);
    return true;
  }

  else if (name == "exec_snippet") {
    SqlEditorForm *editor_form = _sqlide->get_active_sql_editor();
    std::string script;

    script = _entries[selected[0]].code;
    if (!script.empty()) {
      editor_form->run_sql_in_scratch_tab(script, true, false);
    }
  } else if ((name == "replace_text" || name == "insert_text" || name == "copy_to_clipboard") && selected.is_valid() &&
             selected[0] < _entries.size()) {
    std::string script = _entries[selected[0]].code;

    if (name == "copy_to_clipboard")
      mforms::Utilities::set_clipboard_text(script);
    else {
      SqlEditorForm *editor_form = _sqlide->get_active_sql_editor();
      SqlEditorPanel *panel;
      if (editor_form != NULL && (panel = editor_form->active_sql_editor_panel()) != NULL) {
        if (name == "replace_text") {
          panel->editor_be()->set_refresh_enabled(true);
          panel->editor_be()->sql(script.c_str());
        } else if (name == "insert_text") {
          panel->editor_be()->set_refresh_enabled(true);
          panel->editor_be()->set_selected_text(script);
        }
      } else
        return false;
    }
  } else
    return false;

  return true;
}

static struct SnippetNameMapping {
  const char *file;
  const char *name;
} snippet_name_mapping[] = {
#if (defined(_MSC_VER) || defined(__APPLE__))
  {"DB Management", "DB Mgmt"},
  {"SQL DDL Statements", "SQL DDL"},
  {"SQL DML Statements", "SQL DML"},
  {"User Snippets", USER_SNIPPETS},
  {"", SHARED_SNIPPETS},
#else
  {"DB_Management", "DB Mgmt"},
  {"SQL_DDL_Statements", "SQL DDL"},
  {"SQL_DML_Statements", "SQL DML"},
  {"User_Snippets", USER_SNIPPETS},
  {"", SHARED_SNIPPETS},
#endif
  {NULL, NULL}};

static std::string category_file_to_name(const std::string &file) {
  for (int i = 0; snippet_name_mapping[i].file; i++) {
    if (strcmp(snippet_name_mapping[i].file, file.c_str()) == 0)
      return snippet_name_mapping[i].name;
  }
  return file;
}

static std::string category_name_to_file(const std::string &name) {
  for (int i = 0; snippet_name_mapping[i].file; i++) {
    if (strcmp(snippet_name_mapping[i].name, name.c_str()) == 0)
      return snippet_name_mapping[i].file;
  }
  return name;
}

std::vector<std::string> DbSqlEditorSnippets::get_category_list() {
  std::vector<std::string> categories;

  GDir *dir = g_dir_open(_path.c_str(), 0, NULL);
  if (dir) {
    const gchar *name;
    while ((name = g_dir_read_name(dir))) {
      if (g_str_has_suffix(name, ".txt"))
        categories.push_back(category_file_to_name(std::string(name, strlen(name) - 4)));
    }
    g_dir_close(dir);
  }

  // Move up User Snippets to 1st entry.
  std::vector<std::string>::iterator iter;
  if ((iter = std::find(categories.begin(), categories.end(), USER_SNIPPETS)) != categories.end())
    categories.erase(iter);
  categories.insert(categories.begin(), USER_SNIPPETS);

  // DB stored snippets
  categories.push_back(SHARED_SNIPPETS);

  return categories;
}

void DbSqlEditorSnippets::select_category(const std::string &category) {
  _selected_category = category_name_to_file(category);
  if (_selected_category.empty())
    load_from_db();
  else
    load();
}

std::string DbSqlEditorSnippets::selected_category() {
  return category_file_to_name(_selected_category);
}

bool DbSqlEditorSnippets::shared_snippets_usable() {
  return _sqlide->get_active_sql_editor() != NULL && _sqlide->get_active_sql_editor()->connected();
}

void DbSqlEditorSnippets::load_from_db(SqlEditorForm *editor) {
  if (!editor)
    editor = _sqlide->get_active_sql_editor();

  _shared_snippets_enabled = false;
  _entries.clear();

  if (editor) {
    if (_snippet_db.empty())
      _snippet_db = bec::GRTManager::get()->get_app_option_string("workbench:InternalSchema");

    sql::Dbc_connection_handler::Ref conn;

    base::RecMutexLock aux_dbc_conn_mutex(editor->ensure_valid_aux_connection(conn));

    wb::InternalSchema internal_schema(_snippet_db, conn);

    if (!internal_schema.check_snippets_table_exist()) {
      // shared snippets are not enabled. They'll get prompted whether to enable it when they try to add a snippet
      return;
    }

    try {
      std::string s = base::sqlstring("SELECT id, title, code FROM !.snippet", 0) << _snippet_db;
      std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::unique_ptr<sql::ResultSet> result(stmt->executeQuery(s));

      while (result->next()) {
        Snippet snippet;
        snippet.db_snippet_id = result->getInt(1);
        snippet.title = result->getString(2);
        snippet.code = result->getString(3);
        _entries.push_back(snippet);
      }

      _shared_snippets_enabled = true;
    } catch (std::exception &e) {
      logError("Error querying snippets table: %s\n", e.what());
      mforms::Utilities::show_error("Shared Snippets",
                                    base::strfmt("Unable to load server stored snippets.\n%s", e.what()), "OK");
    }
  }
}

int DbSqlEditorSnippets::add_db_snippet(const std::string &name, const std::string &code) {
  if (_sqlide->get_active_sql_editor()) {
    sql::Dbc_connection_handler::Ref conn;
    base::RecMutexLock aux_dbc_conn_mutex(_sqlide->get_active_sql_editor()->ensure_valid_aux_connection(conn));
    wb::InternalSchema internal_schema(_snippet_db, conn);

    if (!internal_schema.check_snippets_table_exist()) {
      if (mforms::Utilities::show_message("Shared Snippets",
                                          base::strfmt("To enable shared snippets stored in the MySQL server, a new "
                                                       "schema called `%s` must be created in the connected server.",
                                                       internal_schema.schema_name().c_str()),
                                          "Create", "Cancel") != mforms::ResultOk)
        return 0;

      std::string error = internal_schema.create_snippets_table_exist();
      if (!error.empty()) {
        logWarning("Could not create table %s.snippet: %s\n", _snippet_db.c_str(), error.c_str());
        mforms::Utilities::show_error("Shared Snippets", "Unable to setup server stored snippets.\n" + error, "OK");
        return 0;
      }
    }

    try {
      return internal_schema.insert_snippet(name, code);
    } catch (std::exception &exc) {
      logError("Error saving snippet: %s\n", exc.what());
      mforms::Utilities::show_error("Shared Snippets", base::strfmt("Error adding new snippet: %s", exc.what()), "OK");
    }
  }
  return 0;
}

void DbSqlEditorSnippets::delete_db_snippet(int snippet_id) {
  if (_sqlide->get_active_sql_editor()) {
    sql::Dbc_connection_handler::Ref conn;
    base::RecMutexLock aux_dbc_conn_mutex(_sqlide->get_active_sql_editor()->ensure_valid_aux_connection(conn));
    wb::InternalSchema internal_schema(_snippet_db, conn);
    try {
      internal_schema.delete_snippet(snippet_id);
    } catch (std::exception &exc) {
      logError("Error saving snippet: %s\n", exc.what());
      mforms::Utilities::show_error("Shared Snippets", base::strfmt("Error deleting snippet: %s", exc.what()), "OK");
    }
  }
}

void DbSqlEditorSnippets::load() {
  _entries.clear();

  FILE *f = base_fopen(base::strfmt("%s/%s.txt", _path.c_str(), _selected_category.c_str()).c_str(), "r");
  if (f) {
    char line[1000];

    while (fgets(line, sizeof(line), f)) {
      char *ptr = strchr(line, '\n');
      if (ptr)
        *ptr = 0;
      std::string name = line;
      std::string script = "";
      bool truncated_line = false;
      bool prev_truncated_line = false;

      while (fgets(line, sizeof(line) - 1, f)) {
        truncated_line = strchr(line, '\n') == 0;
        if (truncated_line || prev_truncated_line)
          script.append(line + (prev_truncated_line ? 0 : 1)); // skip preceding space if this is the beginning of line
        else {
          if (line[0] == '\n')
            break;
          script.append(line + 1); // skip the preceding space
        }
        prev_truncated_line = truncated_line;
      }

      // Remove the last line break, we added that, not the user.
      if (script.size() > 0)
        script.erase(script.size() - 1, 1);

      Snippet snippet;
      snippet.db_snippet_id = 0;
      snippet.title = name;
      snippet.code = script;
      _entries.push_back(snippet);
    }

    fclose(f);
  }
  
  std::sort(_entries.begin(), _entries.end(), [](Snippet& a, Snippet& b) { return a.title < b.title; });
}

void DbSqlEditorSnippets::save() {
  if (_selected_category.empty()) {
    // nothing to do here
  } else {
    FILE *f = base_fopen(base::strfmt("%s/%s.txt", _path.c_str(), _selected_category.c_str()).c_str(), "w+");
    if (f) {
      for (std::deque<Snippet>::const_iterator i = _entries.begin(); i != _entries.end(); ++i) {
        std::vector<std::string> lines = base::split(i->code, "\n");

        fprintf(f, "%s\n", i->title.c_str());
        for (std::vector<std::string>::const_iterator l = lines.begin(); l != lines.end(); ++l)
          fprintf(f, " %s\n", l->c_str());
        fprintf(f, "\n");
      }
      fclose(f);
    }
  }
  std::sort(_entries.begin(), _entries.end(), [](Snippet& a, Snippet& b) { return a.title < b.title; });
}

//--------------------------------------------------------------------------------------------------

DbSqlEditorSnippets::DbSqlEditorSnippets(wb::WBContextSQLIDE *sqlide, const std::string &path)
  : _sqlide(sqlide), _path(path) {
  _shared_snippets_enabled = false;

  // check if the old snippets file exist and move it to the new location
  if (g_file_test(std::string(_path + "/../sql_snippets.txt").c_str(), G_FILE_TEST_EXISTS)) {
    g_mkdir_with_parents(_path.c_str(), 0700);
    g_rename(std::string(_path + "/../sql_snippets.txt").c_str(), std::string(_path + "/User Snippets.txt").c_str());
  } else
    g_mkdir_with_parents(_path.c_str(), 0700);

  // copy the standard files
  std::string datadir = bec::GRTManager::get()->get_data_file_path("snippets");
  {
    GDir *dir = g_dir_open(datadir.c_str(), 0, NULL);
    if (dir) {
      const gchar *name;
      while ((name = g_dir_read_name(dir)))
        copy_original_file(name, false);
      g_dir_close(dir);
    }
  }

  load();
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorSnippets::copy_original_file(const std::string &name, bool overwrite) {
  std::string datadir = bec::GRTManager::get()->get_data_file_path("snippets");
  std::string dest = base::makePath(_path, name);
  bool target_exists = g_file_test(dest.c_str(), G_FILE_TEST_EXISTS) == TRUE;
  if (!target_exists || overwrite) {
    if (target_exists)
      g_unlink(dest.c_str());

    std::string source = std::string(datadir).append("/").append(name);
    base::copyFile(source.c_str(), dest.c_str());
  }
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorSnippets::add_snippet(const std::string &name, const std::string &code, bool edit) {
  Snippet snippet;
  snippet.db_snippet_id = 0;
  snippet.title = base::trim_left(name);
  snippet.code = code;

  if (_selected_category.empty()) {
    snippet.db_snippet_id = add_db_snippet(name, code);
    if (snippet.db_snippet_id != 0)
      _entries.push_front(snippet);
  } else {
    _entries.push_front(snippet);
    save();
  }
}

size_t DbSqlEditorSnippets::count() {
  return _entries.size();
}

bool DbSqlEditorSnippets::get_field(const bec::NodeId &node, ColumnId column, std::string &value) {
  if (node.is_valid() && node[0] < _entries.size()) {
    switch ((Column)column) {
      case Description:
        value = _entries[node[0]].title;
        break;
      case Script:
        value = _entries[node[0]].code;
        if (value.empty())
          return false;
        break;
    }
    return true;
  }
  return false;
}

bool DbSqlEditorSnippets::set_field(const bec::NodeId &node, ColumnId column, const std::string &value) {
  if (node.is_valid() && node[0] < _entries.size()) {
    switch ((Column)column) {
      case Description:
        _entries[node[0]].title = value;
        break;
      case Script:
        _entries[node[0]].code = value;
        break;
    }
    if (_selected_category.empty() && _shared_snippets_enabled && _sqlide->get_active_sql_editor()) {
      sql::Dbc_connection_handler::Ref conn;
      base::RecMutexLock aux_dbc_conn_mutex(_sqlide->get_active_sql_editor()->ensure_valid_aux_connection(conn));
      wb::InternalSchema internal_schema(_snippet_db, conn);
      try {
        switch ((Column)column) {
          case Description:
            internal_schema.set_snippet_title(_entries[node[0]].db_snippet_id, value);
            break;
          case Script:
            internal_schema.set_snippet_code(_entries[node[0]].db_snippet_id, value);
            break;
        }
      } catch (std::exception &exc) {
        logError("Error saving snippet: %s\n", exc.what());
        mforms::Utilities::show_error("Shared Snippets", base::strfmt("Error deleting snippet: %s", exc.what()), "OK");
      }
    } else
      save();
    std::sort(_entries.begin(), _entries.end(), [](Snippet& a, Snippet& b) { return a.title < b.title; });
    return true;
  }

  return false;
}

bool DbSqlEditorSnippets::can_delete_node(const bec::NodeId &node) {
  return node.is_valid() && node[0] < _entries.size();
}

bool DbSqlEditorSnippets::delete_node(const bec::NodeId &node) {
  if (node.is_valid() && node[0] < _entries.size()) {
    int entry_id = _entries[node[0]].db_snippet_id;

    _entries.erase(_entries.begin() + node[0]);

    if (_selected_category.empty()) {
      if (_shared_snippets_enabled && entry_id > 0) {
        delete_db_snippet(entry_id);
      }
    } else
      save();
    return true;
  }
  return false;
}
