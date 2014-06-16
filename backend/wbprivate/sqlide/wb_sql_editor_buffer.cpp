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

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

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

// 20 MB max file size for auto-restoring
#define MAX_FILE_SIZE_FOR_AUTO_RESTORE 20000000

DEFAULT_LOG_DOMAIN("SqlEditor")

using namespace bec;
using namespace base;


void SqlEditorForm::auto_save()
{
  if (!_autosave_disabled)
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

  for (int c = _tabdock->view_count(), i = 0; i < c; i++)
  {
    SqlEditorPanel *editor = sql_editor_panel(i);
    if (!editor)
      continue;

    editor->auto_save(path, i);
  }
}


static bool check_if_file_too_big_to_restore(const std::string &path, const std::string &file_caption,
                                             bool allow_save_as = false)
{
  boost::int64_t length;
  if ((length = get_file_size(path.c_str())) > MAX_FILE_SIZE_FOR_AUTO_RESTORE)
  {
  again:
    std::string size = sizefmt(length, false);
    int rc = mforms::Utilities::show_warning("Restore Workspace",
      strfmt("The file %s has a size of %s. Are you sure you want to restore this file?",
      file_caption.c_str(), size.c_str()),
      "Restore", "Skip", allow_save_as ? "Save As..." : ""
                                             );
    if (rc == mforms::ResultCancel)
      return false;
    
    if (rc == mforms::ResultOther)
    {
      mforms::FileChooser fchooser(mforms::SaveFile);
      
      fchooser.set_title(_("Save File As..."));
      if (fchooser.run_modal())
      {
        if (!copy_file(path.c_str(), fchooser.get_path().c_str()))
        {
          if (mforms::Utilities::show_error("Save File",
                                            strfmt("File %s could not be saved.", fchooser.get_path().c_str()),
                                            _("Retry"), _("Cancel"), "") == mforms::ResultOk)
            goto again;
        }
      }
      else
        goto again;
      return false;
    }
  }
  return true;
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
  
  // find workspaces on disk
  std::string workspace_path;
  std::auto_ptr<base::LockFile> lock_file;
  bool restoring_autosave = false;
  {
    GDir *dir = g_dir_open(path_prefix.c_str(), 0, NULL);
    if (!dir)
      return false;
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
  if (workspace_path.empty()) return false;
  workspace_path = make_path(path_prefix, workspace_path);
  
  // get list of files
  std::vector<std::string> editor_files;
  {
    GDir *dir = g_dir_open(workspace_path.c_str(), 0, NULL);
    if (!dir)
      return false;
    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL)
    {
      editor_files.push_back(name);
    }
    g_dir_close(dir);
  }
  std::sort(editor_files.begin(), editor_files.end());
  
  BOOST_FOREACH(std::string file, editor_files)
  {
    if (g_str_has_suffix(file.c_str(), ".scratch"))
    {      
      if (!check_if_file_too_big_to_restore(make_path(workspace_path, file),
                                            strfmt("Saved scratch buffer '%s'", file.c_str())))
      {
        // if loading was skipped, delete it
        try { base::remove(make_path(workspace_path, file)); }
        catch (std::exception &e) 
        {
          log_error("Error deleting autosave file %s: %s", make_path(workspace_path, file).c_str(), e.what());
        }
        continue;
      }


      SqlEditorPanel* editor(add_sql_editor(true));
      try
      {
        editor->load_autosave(make_path(workspace_path, file), "");
      }
      catch (std::exception &e)
      {
        int rc;
        rc = mforms::Utilities::show_error("Restore Workspace",
                                           strfmt("Could not read contents of '%s'.\n%s", make_path(workspace_path, file).c_str(), e.what()),
                                           "Ignore", "Cancel", "");
        if (rc != mforms::ResultOk)
          return false;
      }
      
    }
    else if (g_str_has_suffix(file.c_str(), ".filename"))
    {
      std::string autosave_filename = file.substr(0, file.size()-strlen(".filename")).append(".autosave");
      std::string filename;
      std::string encoding;
      bool filename_was_empty = false;
      gchar *data;
      gsize length;
      
      if (!g_file_get_contents(make_path(workspace_path, file).c_str(), &data, &length, NULL))
        filename = "";
      else
      {
        char *line1 = strtok(data, "\n");
        char *line2 = strtok(NULL, "\n");

        if (line1)
          filename = line1;
        else
          filename_was_empty = true;

        if (line2)
          encoding = line2;
        g_free(data);
      }
      
      // check if an autosave of the edited file exists
      if (!g_file_test(make_path(workspace_path, autosave_filename).c_str(), G_FILE_TEST_EXISTS)
          && !filename.empty())
      {
        // no autosave exists, load the original
        if (check_if_file_too_big_to_restore(filename,
                                             strfmt("File '%s'", filename.c_str()),
                                             false))        
        {
          SqlEditorPanel* editor(add_sql_editor(false));
          try
          {
            editor->load_from(filename, encoding);
          }
          catch (std::exception &e)
          {
            int rc;
            rc = mforms::Utilities::show_error("Restore Workspace",
                                               strfmt("Could not read contents of '%s'.\n%s", make_path(workspace_path, file).c_str(), e.what()),
                                               "Ignore", "Cancel", "");
            if (rc != mforms::ResultOk)
              return false;
          }
        }
        else
        {
          // if loading was skipped, delete it
          try { base::remove(make_path(workspace_path, file)); }
          catch (std::exception &e) 
          {
            log_error("Error deleting autosave file %s: %s", make_path(workspace_path, file).c_str(), e.what());
          }          
        }
      }
      else
      {
        // Either it's a file that was never auto-saved or an auto-save of the file exists.
        // Load its contents if it does.
        std::string autosave_path = make_path(workspace_path, autosave_filename);
        
        if (base::file_exists(autosave_path))
        {
          if (!check_if_file_too_big_to_restore(autosave_path.c_str(),
                                                strfmt("Auto-saved file '%s'", filename.c_str())))
          {
            // if loading was skipped, delete it
            try { base::remove(autosave_path); }
            catch (std::exception &e) 
            {
              log_error("Error deleting autosave file %s: %s", autosave_path.c_str(), e.what());
            }
            continue;
          }

          SqlEditorPanel *panel = add_sql_editor(false);

          try
          {
            panel->load_autosave(autosave_path, filename, "");
          }
          catch (std::exception &e)
          {
            int rc;
            rc = mforms::Utilities::show_error("Restore Workspace",
                                               strfmt("Could not read auto-saved contents for '%s'.\n%s",
                                                      (filename.empty() ? "Unsaved Script" : filename.c_str()), e.what()),
                                               "Ignore", "Cancel", "");
            if (rc != mforms::ResultOk)
              return false;
          }

          if (!filename_was_empty) //it's only autosave file, we should load is a dirty
            panel->editor_be()->get_editor_control()->reset_dirty();
        }
        else if (filename.empty()) // An empty sql file.
        {
          add_sql_editor(false, false);
        }
      }
    }
  }
  
  if (restoring_autosave)
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
  const std::vector<std::string> &formats)
{
  // We can accept dropped files.
  if (std::find(formats.begin(), formats.end(), mforms::DragFormatFileName) != formats.end())
    return mforms::DragOperationCopy; // Copy to indicate we don't do anything with the files.
  return mforms::DragOperationNone;
}

//--------------------------------------------------------------------------------------------------

mforms::DragOperation SqlEditorForm::files_dropped(mforms::View *sender, base::Point p,
  const std::vector<std::string> &file_names)
{
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
  //if we're removing editor, just cancel query side timer cause there is only one timer
  if (_side_palette)
    _side_palette->cancel_timer();

  panel->delete_auto_save();

  bool found = false;
  for (int c = _tabdock->view_count(), i = 0; i < c; i++)
  {
    SqlEditorPanel *p = sql_editor_panel(i);
    if (p)
    {
      // Rename the remaining editor auto saves to match their index.
      if (found)
        p->rename_auto_save(i-1);
      if (p == panel)
        found = true;
    }
  }
  _tabdock->undock_view(panel);

  delete panel;
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


void SqlEditorForm::sql_editor_reordered(SqlEditorPanel *panel, int to)
{
  int from = panel->autosave_index();
  if (!panel || to < 0 || from < 0)
    return;

  /// Reorder the GRT lists
  size_t from_index = grtobj()->queryEditors().get_index(panel->grtobj());
  if (from_index < 0)
    should_never_happen("Could not find reordered editor in GRT object list\n");

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
  if (from < to)
  {
    for (int i = to; i > from; i--)
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
    for (int i = to; i < from; i++)
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
    should_never_happen("Unable to find suitable target index for reorder\n");
    return;
  }

  grtobj()->queryEditors()->reorder(from_index, to_index);


  /// Rename autosave files to keep the order
  panel->rename_auto_save(-1); // temporary name
  if (from > to)
  {
    for (int i = from - 1; i >= to; i--)
    {
      SqlEditorPanel *p = sql_editor_panel(i);
      if (p)
        p->rename_auto_save(i+1);
    }
  }
  else
  {
    for (int i = from + 1; i <= to; i++)
    {
      SqlEditorPanel *p = sql_editor_panel(i);
      if (p)
        p->rename_auto_save(i-1);
    }
  }
  panel->rename_auto_save(to);
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
