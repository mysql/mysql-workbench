/*
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_sql_editor_form.h"
#include "wb_sql_editor_buffer.h"

#include "wb_sql_editor_panel.h"

#include "wb_sql_editor_tree_controller.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <errno.h>

#include "grt/common.h"

#include "base/util_functions.h"
#include "base/file_functions.h"
#include "base/string_utilities.h"
#include "base/log.h"

#include "mforms/utilities.h"
#include "mforms/filechooser.h"
#include "mforms/code_editor.h"

#include "query_side_palette.h"
#include "grtsqlparser/mysql_parser_services.h"

DEFAULT_LOG_DOMAIN("SqlEditor")

using namespace bec;
using namespace base;


void SqlEditorForm::auto_save()
{
  if (!_autosave_disabled && _startup_done)
  {
    try
    {
      save_workspace(sanitize_file_name(_connection->name()), true);
    }
    catch (std::exception &exc)
    {
      if (mforms::Utilities::show_error(_("Error on Auto-Save"),
                                        strfmt(_("An error occurred during auto-save:\n%s"),
                                               exc.what()), 
                                        _("Continue"), _("Skip Autosaving")) != mforms::ResultOk)
      {
        _autosave_disabled = true;
      }
    }
  }
}


// Save all script buffers, including scratch buffers
void SqlEditorForm::save_workspace(const std::string &workspace_name, bool is_autosave)
{
  std::string path;
  
  // if we're autosaving, just use the same path from previous saves
  if (!is_autosave || _autosave_path.empty())
  {
    std::string path_prefix = make_path(_grtm->get_user_datadir(),
                                        "sql_workspaces");
    if (!g_file_test(path_prefix.c_str(), G_FILE_TEST_EXISTS))
    {
      if (g_mkdir_with_parents(path_prefix.c_str(), 0700) < 0)
        throw std::runtime_error(strfmt("Could not create directory %s: %s", path_prefix.c_str(), g_strerror(errno)));
    }
    
    int i= 1;
    do
    {
      path = make_path(path_prefix, strfmt("%s-%i%s", workspace_name.c_str(), i++, (is_autosave ? ".autosave" : ".workspace")));
    }
    while (!create_directory(path, 0700)); // returns false if dir exists, exception on other errors
    
    if (is_autosave)
    {
      _autosave_lock = new base::LockFile(make_path(path, "lock"));
      _autosave_path = path;
    }
  }
  else
    path = _autosave_path;
  
  // save the real id of the connection
  if (_connection.is_valid())
    g_file_set_contents(make_path(path, "connection_id").c_str(), _connection->id().c_str(),
      (gssize)_connection->id().size(), NULL);

  // save some of the state of the schema tree
  {
    std::string info;
    info.append("active_schema=").append(active_schema()).append("\n");

    // save the expansion state of the active schema only, since saving everything could be very slow when restoring
    mforms::TreeNodeRef schema_node = _live_tree->get_schema_tree()->get_node_for_object(active_schema(), wb::LiveSchemaTree::Schema, "");
    if (schema_node)
    {
      std::string expand_state;
      if (schema_node->is_expanded())
      {
        expand_state = active_schema();
        expand_state.append(":schema");
        if (schema_node->get_child(0) && schema_node->get_child(0)->is_expanded())
          expand_state.append(",tables");
        if (schema_node->get_child(1) && schema_node->get_child(1)->is_expanded())
          expand_state.append(",views");
        if (schema_node->get_child(2) && schema_node->get_child(2)->is_expanded())
          expand_state.append(",procedures");
        if (schema_node->get_child(3) && schema_node->get_child(3)->is_expanded())
          expand_state.append(",functions");
      }
      else
        expand_state = "";
      info.append("expanded=").append(expand_state).append("\n");
    }

    g_file_set_contents(make_path(path, "schema_tree").c_str(), info.c_str(), info.size(), NULL);
  }

  if (_tabdock)
  {
    for (int c = _tabdock->view_count(), i = 0; i < c; i++)
    {
      SqlEditorPanel *editor = sql_editor_panel(i);
      if (!editor)
        continue;

      try
      {
        editor->auto_save(path);
      }
      catch (std::exception &e)
      {
        log_error("Could not auto-save editor %s\n", editor->get_title().c_str());
        mforms::Utilities::show_error("Auto save", base::strfmt("Could not save contents of tab %s.\n%s",
                                                                editor->get_title().c_str(),
                                                                e.what()),
                                      "OK");
      }
    }
  }
  save_workspace_order(path);
}

std::string SqlEditorForm::find_workspace_state(const std::string &workspace_name, std::auto_ptr<base::LockFile> &lock_file)
{
  std::string path_prefix = make_path(_grtm->get_user_datadir(), "sql_workspaces");

  // find workspaces on disk
  std::string workspace_path;
  bool restoring_autosave = false;
  {
    GDir *dir = g_dir_open(path_prefix.c_str(), 0, NULL);
    if (!dir)
      return "";
    int lowest_index = 9999999;
    const gchar *name;

    while ((name = g_dir_read_name(dir)) != NULL)
    {
      if (g_str_has_prefix(name, workspace_name.c_str()))
      {
        const char *end = strrchr(name, '.');
        int new_index = 0;
        if (end)
          new_index = atoi(std::string(name + workspace_name.size()+1, end-name-(workspace_name.size()+1)).c_str());

        if (g_str_has_suffix(name, ".autosave"))
        {
          if (LockFile::check(make_path(make_path(path_prefix, name), "lock")) != LockFile::NotLocked)
            continue;

          if (!restoring_autosave)
          {
            try
            {
              lock_file.reset(new base::LockFile(make_path(make_path(path_prefix, name), "lock")));
            }
            catch (const base::file_locked_error)
            {
              continue;
            }
            lowest_index = new_index;
            restoring_autosave = true;
            workspace_path = name;
          }
          else
            if (new_index < lowest_index)
            {
              try
              {
                lock_file.reset(new base::LockFile(make_path(make_path(path_prefix, name), "lock")));
              }
              catch (const base::file_locked_error)
              {
                continue;
              }
              lowest_index = new_index;
              workspace_path = name;
            }
        }
        else if (!restoring_autosave && g_str_has_suffix(name, ".workspace"))
        {
          if (new_index < lowest_index)
          {
            try
            {
              lock_file.reset(new base::LockFile(make_path(make_path(path_prefix, name), "lock")));
            }
            catch (const base::file_locked_error)
            {
              continue;
            }
            workspace_path = name;
            lowest_index = new_index;
          }
        }
      }
    }
    g_dir_close(dir);
  }
  return workspace_path;
}


struct GuardBoolFlag
{
  bool *flag;
  GuardBoolFlag(bool *ptr) : flag(ptr) { if (flag) *flag = true; }
  ~GuardBoolFlag() { if (flag) *flag = false; }
};

// Restore a previously saved workspace for this connection. The loaded data is deleted immediately after loading (unless its an autosave)
bool SqlEditorForm::load_workspace(const std::string &workspace_name)
{
  std::string path_prefix = make_path(_grtm->get_user_datadir(), "sql_workspaces");

  GuardBoolFlag flag(&_loading_workspace);

  std::auto_ptr<base::LockFile> lock_file;
  std::string workspace_path = find_workspace_state(workspace_name, lock_file);
  if (workspace_path.empty()) return false;
  workspace_path = make_path(path_prefix, workspace_path);

  if (base::file_exists(bec::make_path(workspace_path, "tab_order")))
  {
    // new WB 6.2 format workspace
    std::ifstream f(bec::make_path(workspace_path, "tab_order").c_str());

    std::vector<std::string> editor_files;
    while (!f.eof())
    {
      std::string suffix;
      f >> suffix;
      if (!suffix.empty())
        editor_files.push_back(suffix);
    }

    SqlEditorPanel *editor(add_sql_editor());

    BOOST_FOREACH(std::string file, editor_files)
    {
      std::string info_file = bec::make_path(workspace_path, file+".info");
      std::string text_file = bec::make_path(workspace_path, file+".scratch");
      SqlEditorPanel::AutoSaveInfo info(info_file);

      try
      {
        if (editor->load_autosave(info, text_file))
        {
          // editor load was successful, create tab for the next editor
          editor = add_sql_editor();
        }
      }
      catch (std::exception &e)
      {
        if (!text_file.empty())
        {
          int rc;
          rc = mforms::Utilities::show_error("Restore Workspace",
                                             strfmt("Could not read contents of '%s'.\n%s", text_file.c_str(), e.what()),
                                             "Ignore", "Cancel", "");
          if (rc != mforms::ResultOk)
            return false;
        }
      }

      // delete the autosaves
      try { base::remove(info_file); }
      catch (std::exception &e)
      {
        log_error("Could not delete autosave file %s\n", info_file.c_str());
      }
      try { base::remove(text_file); }
      catch (std::exception &e)
      {
        log_error("Could not delete autosave file %s\n", text_file.c_str());
      }
    }
    // remove the pre-created editor
    remove_sql_editor(editor);
  }
  else
  {
    typedef std::pair<std::string,SqlEditorPanel::AutoSaveInfo> FileItem;
    std::vector<FileItem> editor_files;

    // old format workspace
    {
      GDir *dir = g_dir_open(workspace_path.c_str(), 0, NULL);
      if (!dir)
        return false;
      const gchar *name;
      while ((name = g_dir_read_name(dir)) != NULL)
      {
        SqlEditorPanel::AutoSaveInfo info;
        std::string path = bec::make_path(workspace_path, name);

        if (bec::has_suffix(name, ".scratch"))
        {
          editor_files.push_back(std::make_pair(path, SqlEditorPanel::AutoSaveInfo::old_scratch(path)));
        }
        else if (bec::has_suffix(name, ".autosave"))
        {
          editor_files.push_back(std::make_pair(path, SqlEditorPanel::AutoSaveInfo::old_autosave(path)));
        }
      }
      g_dir_close(dir);
    }

    SqlEditorPanel *editor(add_sql_editor());

    BOOST_FOREACH(FileItem file, editor_files)
    {
      try
      {
        if (editor->load_autosave(file.second, file.first))
        {
          // editor load was successful, create tab for the next editor
          editor = add_sql_editor();
        }
      }
      catch (std::exception &e)
      {
        if (!file.first.empty())
        {
          int rc;
          rc = mforms::Utilities::show_error("Restore Workspace",
                                             strfmt("Could not read contents of '%s'.\n%s", file.first.c_str(), e.what()),
                                             "Ignore", "Cancel", "");
          if (rc != mforms::ResultOk)
            return false;
        }
      }

      // delete the autosaves
      try { base::remove(file.first); }
      catch (std::exception &e)
      {
        log_error("Could not delete autosave file %s\n", file.first.c_str());
      }
    }
    // remove the pre-created editor
    remove_sql_editor(editor);
  }


  // load schema tree state
  {
    gchar *data;
    gsize length;
    if (g_file_get_contents(make_path(workspace_path, "schema_tree").c_str(), &data, &length, NULL))
    {
      char *line = strtok(data, "\n");
      while (line)
      {
        if (base::starts_with(line, "expanded="))
        {
          char *value = strchr(line, '=');
          if (value)
          {
            _pending_expand_nodes = value+1; // expanded=<schema-name>:schema,tables,views,etc
            break;
          }
        }
        line = strtok(NULL, "\n");
      }
      g_free(data);
    }
  }
  
  if (has_suffix(workspace_path, ".autosave"))
  {
    _autosave_lock = lock_file.release();
    _autosave_path = workspace_path;
    
    _grtm->replace_status_text(_("Restored last session state"));
  }
  else
  {
    lock_file.reset(0);
    base_rmdir_recursively(workspace_path.c_str());
  }
  
  return true;
}

SqlEditorPanel* SqlEditorForm::add_sql_editor(bool scratch, bool start_collapsed)
{
  SqlEditorPanel* editor(mforms::manage(new SqlEditorPanel(this, scratch, start_collapsed)));
  editor->editor_be()->register_file_drop_for(this);

  editor->grtobj()->owner(grtobj());
  grtobj()->queryEditors().insert(editor->grtobj());

  _tabdock->dock_view(editor);
  _tabdock->select_view(editor);
  if (!scratch)
    editor->set_title(strfmt("SQL File %i", ++_sql_editors_serial));
  else
    editor->set_title(strfmt("Query %i", ++_scratch_editors_serial));

  if (!_loading_workspace)
    auto_save();
  
  return editor;
}

//--------------------------------------------------------------------------------------------------

mforms::DragOperation SqlEditorForm::drag_over(mforms::View *sender, base::Point p,
  mforms::DragOperation allowedOperations, const std::vector<std::string> &formats)
{
  // We can accept dropped files.
  if (std::find(formats.begin(), formats.end(), mforms::DragFormatFileName) != formats.end())
    return allowedOperations & mforms::DragOperationCopy; // Copy to indicate we don't do anything with the files.
  return mforms::DragOperationNone;
}

//--------------------------------------------------------------------------------------------------

mforms::DragOperation SqlEditorForm::files_dropped(mforms::View *sender, base::Point p,
  mforms::DragOperation allowedOperations, const std::vector<std::string> &file_names)
{
  if ((allowedOperations & mforms::DragOperationCopy) != mforms::DragOperationCopy)
    return mforms::DragOperationNone;

#ifdef _WIN32
  bool case_sensitive = false; // TODO: on Mac case sensitivity depends on the file system.
#else
  bool case_sensitive = true;
#endif
  std::vector<std::string> file_names_to_open;

  for (size_t i = 0; i < file_names.size(); ++i)
  {
    // First see if we have already a tab with that file open.
    bool found = false;

    for (size_t c = _tabdock->view_count(), j = 0; j < c; j++)
    {
      SqlEditorPanel *panel = sql_editor_panel((int)j);

      if (panel && base::same_string(panel->filename(), file_names[i], case_sensitive))
      {
        found = true;

        // Ignore this file name, but if this is the only one dropped the activate the particular
        // sql editor.
        if (file_names.size() == 1)
          _tabdock->select_view(panel);
        break;
      }
    }

    if (!found)
      file_names_to_open.push_back(file_names[i]);
  }

  for (std::vector<std::string>::const_iterator f = file_names_to_open.begin();
       f != file_names_to_open.end(); ++f)
    open_file(*f, true);

  return mforms::DragOperationCopy;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::remove_sql_editor(SqlEditorPanel *panel)
{
  panel->grtobj()->owner().clear();
  grtobj()->queryEditors().remove_value(panel->grtobj());

  //if we're removing editor, just cancel query side timer cause there is only one timer
  if (_side_palette)
    _side_palette->cancel_timer();

  if (!_closing && !_autosave_path.empty()) //if autosave_path is empty then it means we're not ready yet
  {
    panel->delete_auto_save(_autosave_path);
    save_workspace_order(_autosave_path);
  }

  _tabdock->undock_view(panel);

  // no need to delete, undock_view will release the reference and delete it because panel is managed
  //delete panel;

  if (_tabdock->view_count() == 0 && !_closing)
    new_sql_scratch_area();
}


SqlEditorPanel* SqlEditorForm::active_sql_editor_panel()
{
  if (_tabdock)
    return dynamic_cast<SqlEditorPanel*>(_tabdock->selected_view());
  return NULL;
}


void SqlEditorForm::sql_editor_panel_switched()
{
  SqlEditorPanel *panel = active_sql_editor_panel();
  if (panel)
    _grtm->run_once_when_idle((bec::UIForm*)panel, boost::bind(&mforms::View::focus, panel->editor_be()->get_editor_control()));

  validate_menubar();
}


void SqlEditorForm::save_workspace_order(const std::string &prefix)
{
  std::ofstream order_file;
  if (prefix.empty())
  {
    log_error("save with empty path\n");
  }

  order_file.open(bec::make_path(prefix, "tab_order").c_str(), std::ofstream::out);

  if (_tabdock)
  {
    for (int c = _tabdock->view_count(), i = 0; i < c; i++)
    {
      SqlEditorPanel *editor = sql_editor_panel(i);
      if (editor)
        order_file << editor->autosave_file_suffix() << "\n";
    }
  }
}

void SqlEditorForm::sql_editor_reordered(SqlEditorPanel *panel, int to)
{
  if (!panel || to < 0)
    return;

  /// Reorder the GRT lists
  int from_index = (int)grtobj()->queryEditors().get_index(panel->grtobj());
  if (from_index == (int)grt::BaseListRef::npos)
    log_fatal("Could not find reordered editor in GRT object list\n");

  // first build an array of result panel objects, in the same order as the tabview
  std::vector<std::pair<db_query_QueryEditorRef, int> > panels;
  for (int panel_order = 0, i = 0; i < sql_editor_count(); i++)
  {
    SqlEditorPanel *p = sql_editor_panel(i);
    if (p)
      panels.push_back(std::make_pair(p->grtobj(), panel_order++));
    else
      panels.push_back(std::make_pair(db_query_QueryEditorRef(), 0));
  }

  int to_index = -1;
  // now find out where we have to move to
  if ((int)from_index < to)
  {
    for (int i = to; i > (int)from_index; i--)
    {
      if (panels[i].first.is_valid())
      {
        to_index = panels[i].second;
        break;
      }
    }
  }
  else
  {
    for (int i = to; i < (int)from_index; i++)
    {
      if (panels[i].first.is_valid())
      {
        to_index = panels[i].second;
        break;
      }
    }
  }
  if (to_index < 0)
  {
    log_fatal("Unable to find suitable target index for reorder\n");
    return;
  }

  grtobj()->queryEditors()->reorder(from_index, to_index);

  if (!_autosave_path.empty())
  {
    /// Rename autosave files to keep the order
    save_workspace_order(_autosave_path);
  }
}

//--------------------------------------------------------------------------------------------------

SqlEditorPanel *SqlEditorForm::sql_editor_panel(int index)
{
  if (index >= 0 && index < _tabdock->view_count())
    return dynamic_cast<SqlEditorPanel*>(_tabdock->view_at_index(index));
  return NULL;
}


int SqlEditorForm::sql_editor_panel_index(SqlEditorPanel *panel)
{
  for (int c = _tabdock->view_count(), i = 0; i < c; i++)
  {
    if (sql_editor_panel(i) == panel)
      return i;
  }
  return -1;
}

int SqlEditorForm::sql_editor_count()
{
  if (_tabdock)
    return _tabdock->view_count();
  return 0;
}
//--------------------------------------------------------------------------------------------------

SqlEditorPanel *SqlEditorForm::new_sql_script_file()
{
  SqlEditorPanel *panel = add_sql_editor(false);
  _grtm->replace_status_text(_("Added new script editor"));
  update_menu_and_toolbar();
  return panel;
}

SqlEditorPanel *SqlEditorForm::new_sql_scratch_area(bool start_collapsed)
{
  SqlEditorPanel *panel = add_sql_editor(true, start_collapsed);
  _grtm->replace_status_text(_("Added new scratch query editor"));
  update_menu_and_toolbar();
  return panel;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::open_file(const std::string &path, bool in_new_tab)
{
  std::string file_path = path;

  _grtm->replace_status_text(base::strfmt(_("Opening %s..."), path.c_str()));

  if (file_path.empty())
  {
    mforms::FileChooser opendlg(mforms::OpenFile);
    opendlg.set_title(_("Open SQL Script"));
    opendlg.set_extensions("SQL Files (*.sql)|*.sql|Query Browser Files (*.qbquery)|*.qbquery", "sql");
    if (opendlg.run_modal())
      file_path = opendlg.get_path();
  }
  if (file_path.empty())
  {
    _grtm->replace_status_text(_("Cancelled open file"));
    return;
  }

  SqlEditorPanel *panel = NULL;
  if (!in_new_tab)
    panel = active_sql_editor_panel();

  if (!panel)
    panel = new_sql_script_file();

  if (panel->is_dirty())
  {
    int r = mforms::Utilities::show_warning(_("Open File"), 
                                            strfmt(_("SQL script %s has unsaved changes.\n"
                                                     "Would you like to Save these changes?"),
                                                   panel->get_title().c_str()), _("Save"), _("Cancel"), _("Don't Save"));
    if (r == mforms::ResultCancel)
      return;
    else if (r == mforms::ResultOk)
      if (!panel->save())
        return;
  }

  try
  {
    panel->load_from(file_path);
  }
  catch (std::exception &exc)
  {
    log_error("Cannot open file %s: %s\n", file_path.c_str(), exc.what());
    if (in_new_tab)
      remove_sql_editor(panel);
    mforms::Utilities::show_error(_("Open File"),
                                  strfmt(_("Could not open file %s\n%s"), file_path.c_str(), exc.what()),
                                  _("OK"));
    return;
  }

  {
    base::NotificationInfo info;
    info["opener"] = "SqlEditorForm";
    info["path"] = file_path;
    base::NotificationCenter::get()->send("GNDocumentOpened", this, info);
  }
  auto_save();
}

//--------------------------------------------------------------------------------------------------

std::string SqlEditorForm::restore_sql_from_history(int entry_index, std::list<int> &detail_indexes)
{
  return _history->restore_sql_from_history(entry_index, detail_indexes);
}

void SqlEditorForm::set_autosave_disabled(const bool autosave_disabled)
{
  _autosave_disabled = autosave_disabled;
}

bool SqlEditorForm::get_autosave_disabled(void)
{
  return _autosave_disabled;
}
