/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "stdafx.h"

#include <glib.h>
#include <glib/gstdio.h>

#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/util_functions.h"

#include "wb_sql_editor_snippets.h"
#include "sqlide/wb_sql_editor_form.h"

#include "mforms/utilities.h"
#include "mforms/filechooser.h"

using namespace mforms;

static DbSqlEditorSnippets *singleton= 0;

void DbSqlEditorSnippets::setup(wb::WBContextSQLIDE *sqlide, const std::string &path)
{
  if (singleton != 0)
    return;
  
  singleton = new DbSqlEditorSnippets(sqlide, path);
}


DbSqlEditorSnippets *DbSqlEditorSnippets::get_instance()
{
  return singleton;
}

//--------------------------------------------------------------------------------------------------

bool DbSqlEditorSnippets::activate_toolbar_item(const bec::NodeId &selected, const std::string &name)
{
  if (name == "restore_snippets")
  {
    DialogResult result = (DialogResult) Utilities::show_message(_("Restore snippet list"),
      base::strfmt(_("You are about to restore the \"%s\" snippet list to its original state. "
      "All changes will be lost.\n\nDo you want to continue?"), _selected_category.c_str()),
      _("Continue"), _("Cancel"));
    if (result == mforms::ResultOk)
    {
      copy_original_file(_selected_category + ".txt", true);
      load();
    }

    return true;
  }

  if (name == "add_snippet")
  {
    SqlEditorForm *editor_form = _sqlide->get_active_sql_editor();
    if (editor_form != NULL && editor_form->active_sql_editor() != NULL)
    {
      int start, end;
      std::string text = editor_form->active_sql_editor()->sql();
      if (editor_form->active_sql_editor()->selected_range(start, end))
        text = text.substr(start, end - start);
      if (editor_form->save_snippet(text))
        _sqlide->get_grt_manager()->replace_status_text("SQL saved to snippets list.");
    }
    return true;
  }
  
  if (name == "del_snippet" && selected.is_valid() && selected[0] >= 0 && selected[0] < (int) _entries.size())
  {
    delete_node(selected);
    return true;
  }

  else if (name == "exec_snippet")
  {
    SqlEditorForm *editor_form = _sqlide->get_active_sql_editor();
    std::string script;
    
    script = _entries[selected[0]].second;
    if (!script.empty())
    {
      editor_form->run_sql_in_scratch_tab(script, true, false);
    }
  } else if ((name == "replace_text" || name == "insert_text" || name == "copy_to_clipboard") &&
    selected.is_valid() && selected[0] >= 0 && selected[0] < (int) _entries.size())
  {
    std::string script = _entries[selected[0]].second;

    if (name == "copy_to_clipboard")
      mforms::Utilities::set_clipboard_text(script);
    else
    {
      SqlEditorForm *editor_form = _sqlide->get_active_sql_editor();
      if (editor_form != NULL && editor_form->active_sql_editor() != NULL)
      {
        if (name == "replace_text")
        {
          editor_form->active_sql_editor()->set_refresh_enabled(true);
          editor_form->active_sql_editor()->sql(script.c_str());
        }
        else if (name == "insert_text")
        {
          editor_form->active_sql_editor()->set_refresh_enabled(true);
          editor_form->active_sql_editor()->set_selected_text(script);
        }
      }
      else
        return false;
    }
  }
  else
    return false;

  return true;
}

static struct SnippetNameMapping {
  const char *file;
  const char *name;
} snippet_name_mapping[] = {
#if (defined(_WIN32) || defined(__APPLE__))
  {"DB Management", "DB Mgmt"},
  {"SQL DDL Statements", "SQL DDL"},
  {"SQL DML Statements", "SQL DML"},
  {"User Snippets", "My Snippets"},
#else
  {"DB_Management", "DB Mgmt"},
  {"SQL_DDL_Statements", "SQL DDL"},
  {"SQL_DML_Statements", "SQL DML"},
  {"User_Snippets", "My Snippets"},
#endif
  {NULL, NULL}
};

static std::string category_file_to_name(const std::string &file)
{
  for (int i= 0; snippet_name_mapping[i].file; i++)
  {
    if (strcmp(snippet_name_mapping[i].file, file.c_str()) == 0)
      return snippet_name_mapping[i].name;
  }
  return file;
}

static std::string category_name_to_file(const std::string &name)
{
  for (int i= 0; snippet_name_mapping[i].file; i++)
  {
    if (strcmp(snippet_name_mapping[i].name, name.c_str()) == 0)
      return snippet_name_mapping[i].file;
  }
  return name;
}


std::vector<std::string> DbSqlEditorSnippets::get_category_list()
{
  std::vector<std::string> categories;

  GDir *dir = g_dir_open(_path.c_str(), 0, NULL);
  if (dir)
  {
    const gchar *name;
    while ((name = g_dir_read_name(dir)))
    {
      if (g_str_has_suffix(name, ".txt"))
        categories.push_back(category_file_to_name(std::string(name, strlen(name)-4)));
    }
    g_dir_close(dir);
  }

  // Move up User Snippets to 1st entry.
  std::vector<std::string>::iterator iter;
  if ((iter = std::find(categories.begin(), categories.end(), "My Snippets")) != categories.end())
    categories.erase(iter);
  categories.insert(categories.begin(), "My Snippets");

  return categories;
}

void DbSqlEditorSnippets::select_category(const std::string &category)
{
  _selected_category = category_name_to_file(category);
  load();
}

std::string DbSqlEditorSnippets::selected_category()
{ 
  return category_file_to_name(_selected_category); 
}

void DbSqlEditorSnippets::load()
{
  _entries.clear();

  FILE *f = base_fopen(base::strfmt("%s/%s.txt", _path.c_str(), _selected_category.c_str()).c_str(), "r");
  if (f)
  {
    char line[1000];
    
    while (fgets(line, sizeof(line), f))
    {
      char *ptr = strchr(line, '\n');
      if (ptr)
        *ptr= 0;
      std::string name = line;
      std::string script = "";
      bool truncated_line = false;
      bool prev_truncated_line = false;

      while (fgets(line, sizeof(line)-1, f))
      {
        truncated_line = strchr(line, '\n') == 0;
        if (truncated_line || prev_truncated_line)
          script.append(line + (prev_truncated_line ? 0 : 1)); // skip preceding space if this is the beginning of line
        else
        {
          if (line[0] == '\n')
            break;
          script.append(line+1); // skip the preceding space
        }
        prev_truncated_line = truncated_line;
      }

      // Remove the last line break, we added that, not the user.
      if (script.size() > 0)
        script.erase(script.size() - 1, 1);
      _entries.push_back(std::make_pair(name, script));
    }
    
    fclose(f);
  }
}


void DbSqlEditorSnippets::save()
{
  FILE *f = base_fopen(base::strfmt("%s/%s.txt", _path.c_str(), _selected_category.c_str()).c_str(), "w+");
  if (f)
  {
    for (std::vector<std::pair<std::string, std::string> >::const_iterator i = _entries.begin();
         i != _entries.end(); ++i)
    {
      std::vector<std::string> lines = base::split(i->second, "\n");
      
      fprintf(f, "%s\n", i->first.c_str());
      for (std::vector<std::string>::const_iterator l = lines.begin(); l != lines.end(); ++l)
        fprintf(f, " %s\n", l->c_str());
      fprintf(f, "\n");
    }
    fclose(f);
  }  
}

//--------------------------------------------------------------------------------------------------

DbSqlEditorSnippets::DbSqlEditorSnippets(wb::WBContextSQLIDE *sqlide, const std::string &path)
: _sqlide(sqlide), _path(path)
{
  // check if the old snippets file exist and move it to the new location
  if (g_file_test(std::string(_path+"/../sql_snippets.txt").c_str(), G_FILE_TEST_EXISTS))
  {
    g_mkdir_with_parents(_path.c_str(), 0700);
    g_rename(std::string(_path+"/../sql_snippets.txt").c_str(), std::string(_path+"/User Snippets.txt").c_str());
  }
  else
    g_mkdir_with_parents(_path.c_str(), 0700);
  
  // copy the standard files
  std::string datadir = _sqlide->get_grt_manager()->get_data_file_path("snippets");
  {
    GDir *dir = g_dir_open(datadir.c_str(), 0, NULL);
    if (dir)
    {
      const gchar *name;
      while ((name = g_dir_read_name(dir)))
        copy_original_file(name, false);
      g_dir_close(dir);
    }
  }

  load();
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorSnippets::copy_original_file(const std::string& name, bool overwrite)
{
  std::string datadir = _sqlide->get_grt_manager()->get_data_file_path("snippets");
  std::string dest = bec::make_path(_path, name);
  bool target_exists = g_file_test(dest.c_str(), G_FILE_TEST_EXISTS) == TRUE;
  if (!target_exists || overwrite)
  {
    if (target_exists)
      g_unlink(dest.c_str());

    std::string source = std::string(datadir).append("/").append(name);
    copy_file(source.c_str(), dest.c_str());
  }
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorSnippets::add_snippet(const std::string &name, const std::string &snippet, bool edit)
{
  _entries.push_back(std::make_pair(base::trim_left(name), snippet));
  save();
}



int DbSqlEditorSnippets::count()
{
  return (int)_entries.size();
}


bool DbSqlEditorSnippets::get_field(const bec::NodeId &node, int column, std::string &value)
{
  if (node.is_valid() && node[0] >= 0 && node[0] < (int)_entries.size())
  {
    switch ((Column)column)
    {
      case Description:
        value = _entries[node[0]].first;
        break;
      case Script:
        value = _entries[node[0]].second;
        break;
    }
    return true;
  }
  return false;
}

bool DbSqlEditorSnippets::set_field(const bec::NodeId &node, int column, const std::string &value)
{
  if (node.is_valid() && node[0] >= 0 && node[0] < (int)_entries.size())
  {
    switch ((Column)column)
    {
    case Description:
      _entries[node[0]].first = value;
      break;
    case Script:
      _entries[node[0]].second = value;
      break;
    }
    return true;
  }

  return false;
}
/*
bool DbSqlEditorSnippets::activate_node(const bec::NodeId &node)
{
  if (node.is_valid())
  {
    std::string script = _entries[node[0]].second;
    
    SqlEditorForm *editor = _sqlide->get_active_sql_editor();
    if (editor)
    {
      editor->active_sql_editor()->is_refresh_enabled(true);
      editor->active_sql_editor()->sql(script);
      editor->do_partial_ui_refresh(SqlEditorForm::RefreshEditor);
      return true;
    }
  }
  return false;
}


bec::MenuItemList DbSqlEditorSnippets::get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes)
{
  bec::MenuItemList items;
  
  bec::MenuItem item;
  
  item.caption = "Replace Text in SQL Area";
  item.enabled = nodes.size() > 0;
  item.name = "replace_text";
  item.type = bec::MenuAction;
  items.push_back(item);

  item.caption = "Insert Text into SQL Area";
  item.enabled = nodes.size() > 0;
  item.name = "insert_text";
  item.type = bec::MenuAction;
  items.push_back(item);
  
  item.caption = "Copy to Clipboard";
  item.enabled = nodes.size() > 0;
  item.name = "copy_to_clipboard";
  item.type = bec::MenuAction;
  items.push_back(item);
  
  item.caption = "";
  item.enabled = true;
  item.name = "";
  item.type = bec::MenuSeparator;
  items.push_back(item);
  
  item.caption = "Delete";
  item.enabled = nodes.size() > 0;
  item.name = "delete";
  item.type = bec::MenuAction;
  items.push_back(item);
  
  return items;
}


bool DbSqlEditorSnippets::activate_popup_item_for_nodes(const std::string &name, const std::vector<bec::NodeId> &nodes)
{  
  if (name == "delete")
  {
    for (std::vector<bec::NodeId>::const_reverse_iterator n = nodes.rbegin(); n != nodes.rend(); ++n)
      delete_node(*n);
  }
  else if (name == "exec_snippet")
  {
    SqlEditorForm *editor = _sqlide->get_active_sql_editor();
    std::string script;
    
    for (std::vector<bec::NodeId>::const_iterator n = nodes.begin(); n != nodes.end(); ++n)
    {
      script = _entries[(*n)[0]].second;
      if (!script.empty())
      {
        editor->run_sql_in_scratch_tab(script, true, false);
      }
    }
  }
  else if (name == "replace_text" || name == "copy_to_clipboard" || name == "insert_text")
  {
    std::string script;
    
    for (std::vector<bec::NodeId>::const_iterator n = nodes.begin(); n != nodes.end(); ++n)
    {
      script.append(_entries[(*n)[0]].second);
      script.append("\n");
    }

    if (name == "copy_to_clipboard")
    {
      mforms::Utilities::set_clipboard_text(script);
    }
    else
    {
      SqlEditorForm *editor = _sqlide->get_active_sql_editor();
      if (editor)
      {
        if (name == "replace_text")
        {
          editor->active_sql_editor()->is_refresh_enabled(true);
          editor->active_sql_editor()->sql(script);
        }
        else if (name == "insert_text")
        {
          editor->active_sql_editor()->is_refresh_enabled(true);
          editor->active_sql_editor()->set_selected_text(script);
        }
      }
      else
        return false;
    }
  }
  else
    return false;
  return true;
}
*/

bool DbSqlEditorSnippets::can_delete_node(const bec::NodeId &node)
{
  return node.is_valid() && node[0] < (int)_entries.size();
}


bool DbSqlEditorSnippets::delete_node(const bec::NodeId &node)
{
  if (node.is_valid() && node[0] >= 0 && node[0] < (int)_entries.size())
  {
    _entries.erase(_entries.begin() + node[0]);
    save();
    return true;
  }
  return false;
}

