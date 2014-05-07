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

#include "stdafx.h"
#include "wb_sql_editor_form.h"
#include "wb_sql_editor_buffer.h"

#include "grtui/file_charset_dialog.h"

#include <boost/foreach.hpp>
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

#include <boost/lexical_cast.hpp>

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
  MutexLock sql_editors_mutex(_sql_editors_mutex);
  
  if (_sql_editors.size() < 1 || (_sql_editors.size() == 1 && _sql_editors[0]->is_scratch && _sql_editors[0]->editor->empty()))
    return;
  
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
    g_file_set_contents(make_path(path, "connection_id").c_str(), 
                        _connection->id().c_str(), _connection->id().size(), NULL);

  //XXX The editor contents need to be up-to-date. Since we can only request a refresh of the backend
  // for the active tab, the frontend must automatically refresh tabs when switching away from them
  // This can be removed once backend copy of text is dropped and getting data from frontend editor happens
  // through a callback
  
  int editor_index = 0;
  BOOST_FOREACH (Sql_editor_info::Ref sql_editor_info, _sql_editors)
  {
    editor_index++;
    
    // if the editor is saved to a file, we store the path. if the file is unsaved, we save an updated copy
    // for scrath areas, the file is always kept as an autosave
    std::string filename;
    
    if (!sql_editor_info->is_scratch)
    {
      GError *error = 0;
      std::string contents = sql_editor_info->filename + "\n" + sql_editor_info->orig_encoding;
      // save the path to the real file
      if (!g_file_set_contents(make_path(path, strfmt("%i.filename", editor_index)).c_str(),
                               contents.data(), contents.size(), &error))
      {
        std::string msg(strfmt("Could not save snapshot of editor contents to %s: %s", path.c_str(), error->message));
        g_error_free(error);
        base_rmdir_recursively(path.c_str());
        throw msg;
      }
    }
    
    if (sql_editor_info->is_scratch)
    {
      if (!base::starts_with(sql_editor_info->caption, "Query "))
        filename = strfmt("%s.scratch", sql_editor_info->caption.c_str());
      else
        filename = strfmt("%i.scratch", editor_index);
      sql_editor_info->autosave_filename = filename;
    }
    else
    {
      if (sql_editor_info->editor->get_editor_control()->is_dirty())
      {
        if (extension(sql_editor_info->autosave_filename) == ".scratch")
        {
          try
          {
            base::remove(make_path(path, sql_editor_info->autosave_filename));
          }
          catch (...)
          {
          }
        }

        filename = strfmt("%i.autosave", editor_index);
        sql_editor_info->autosave_filename = filename;
      }
    }
    
    // only save editor contents for scratch areas and unsaved editors
    if (!filename.empty())
    {
      // We don't need to lock the editor as we are in the main thread here
      // and directly set the file content without detouring to anything that could change the text.
      GError *error = 0;
      std::string fn = make_path(path, filename);
      
      std::pair<const char*, size_t> text = sql_editor_info->editor->text_ptr();
      if (!g_file_set_contents(fn.c_str(), text.first, text.second, &error))
      {
        std::string msg(strfmt("Could not save snapshot of editor contents to %s: %s", fn.c_str(), error->message));
        g_error_free(error);
        base_rmdir_recursively(path.c_str());
        throw std::runtime_error(msg);
      }
    }
    else
    {
      // delete the autosave file if the file was saved
      try
      {
        if (!sql_editor_info->autosave_filename.empty())
        {
          base::remove(make_path(path, sql_editor_info->autosave_filename));
        }
      }
      catch (...)
      {
      }
    }
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
      GError *error = NULL;
      gchar *data;
      gsize length;
      
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
      
      if (!g_file_get_contents(make_path(workspace_path, file).c_str(), &data, &length, &error))
      {
        int rc;
        rc = mforms::Utilities::show_error("Restore Workspace", 
                                           strfmt("Could not read contents of '%s'.\n%s", make_path(workspace_path, file).c_str(), error->message),
                                           "Ignore", "Cancel", "");
        if (rc != mforms::ResultOk)
          return false;
      }
      
      int i = add_sql_editor(true);
      Sql_editor_info::Ref info(_sql_editors[i]);
      sql_editor_new_ui(i);
      info->editor->sql(data ? data : "");
      info->editor->get_editor_control()->reset_dirty();

      _active_sql_editor_index = i;

      std::string tab_name = base::strip_extension(file);
      try
      {
        boost::lexical_cast<int>(tab_name);
      }
      catch(const boost::bad_lexical_cast &)
      {
        info->caption =  tab_name;
        do_partial_ui_refresh(RefreshEditorTitle);
      }
      g_free(data);
    }
    else if (g_str_has_suffix(file.c_str(), ".filename"))
    {
      std::string autosave_filename = file.substr(0, file.size()-strlen(".filename")).append(".autosave");
      std::string filename;
      std::string encoding;
      bool filename_was_empty = false;
      GError *error = NULL;
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
          new_sql_script_file();
          // load the saved file
          sql_editor_open_file(_active_sql_editor_index, filename, encoding);
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
          // load auto-saved file and set the filename to the original path
          if (!g_file_get_contents(autosave_path.c_str(), &data, &length, &error))
          {
            int rc;
            rc = mforms::Utilities::show_error("Restore Workspace", 
                                               strfmt("Could not read auto-saved contents for '%s'.\n%s", 
                                                      (filename.empty() ? "Unsaved Script" : filename.c_str()), error->message),
                                               "Ignore", "Cancel", "");
            if (rc != mforms::ResultOk)
              return false;
          }
          else
          {
            int i = add_sql_editor(false);
            Sql_editor_info::Ref info(_sql_editors[i]);
            
            // set active editor now so that any callbacks triggered now go to the right editor
            _active_sql_editor_index = i;
            
            sql_editor_new_ui(_active_sql_editor_index);
            info->editor->sql(data ? data : "");
            if (!filename_was_empty && data) //it's only autosave file, we should load is a dirty
              info->editor->get_editor_control()->reset_dirty();

            info->filename = filename;
            if (!filename.empty())
              info->caption = base::strip_extension(base::basename(filename));
            
            g_free(data);
          }
        }
        else if (filename.empty()) // An empty sql file.
        {
          int index = add_sql_editor(false, false);
          sql_editor_new_ui(index);
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

int SqlEditorForm::add_sql_editor(bool scratch, bool start_collapsed)
{
  db_query_QueryEditorRef grtobj(grt_manager()->get_grt());
  Sql_editor::Ref sql_editor= Sql_editor::create(rdbms(), wbsql()->get_grt_editor_object(this)->serverVersion(), grtobj);
  sql_editor->sql_check_progress_msg_throttle(_progress_status_update_interval);
  sql_editor->set_auto_completion_cache(_auto_completion_cache);
  sql_editor->sql_mode(_sql_mode);
  sql_editor->set_current_schema(active_schema());
  sql_editor->register_file_drop_for(this);
  scoped_connect(sql_editor->text_change_signal(),
    boost::bind(&SqlEditorForm::do_partial_ui_refresh, this, (int)RefreshEditorTitle));
  int sql_editor_index;
  {
    MutexLock sql_editors_mutex(_sql_editors_mutex);
    Sql_editor_info::Ref info(new Sql_editor_info());
    info->toolbar = setup_editor_toolbar(sql_editor);
    info->editor = sql_editor;
    info->is_scratch = scratch;
    info->start_collapsed = start_collapsed;
    if (!scratch)
      info->caption = strfmt("SQL File %i", ++_sql_editors_serial);
    else
      info->caption = strfmt("Query %i", ++_scratch_editors_serial);
    
    _sql_editors.push_back(info);
    sql_editor_index= _sql_editors.size() - 1;    
  }
  
  sql_editor_list_changed(sql_editor, true);
  
  if (!_loading_workspace)
    auto_save();
  
  return sql_editor_index;
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
#if _WIN32
  bool case_sensitive = false;
#else
  bool case_sensitive = true;
#endif
  Sql_editors editors_copy;
  {
    MutexLock sql_editors_mutex(_sql_editors_mutex);
    editors_copy = _sql_editors;
  }
  for (size_t i = 0; i < file_names.size(); ++i)
  {
    // First see if we have already a tab with that file open.
    bool found = false;
    for (size_t j = 0; j < editors_copy.size(); ++j)
    {
      if (base::same_string(editors_copy[j]->filename, file_names[i], case_sensitive))
      {
        found = true;

        // Ignore this file name, but if this is the only one dropped the activate the particular
        // sql editor.
        if (file_names.size() == 1)
          active_sql_editor_index(j); // TODO: signal the UI that we changed the active sql editor.
        break;
      }
    }

    if (!found)
      open_file(file_names[i], true);
  }

  return mforms::DragOperationCopy;
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper to rename all auto-save files under the from index to the to index, provided that:
 * - A file with that name exists and
 * - the target name isn't already used.
 * 
 * The given indices are one-based.
 */
void SqlEditorForm::rename_autosave_files(int from, int to)
{
  try
  {
    if (file_exists(make_path(_autosave_path, strfmt("%i.autosave", from))))
      rename(make_path(_autosave_path, strfmt("%i.autosave", from)),
             make_path(_autosave_path, strfmt("%i.autosave", to)));
    else
      if (file_exists(make_path(_autosave_path, strfmt("%i.scratch", from))))
        rename(make_path(_autosave_path, strfmt("%i.scratch", from)),
               make_path(_autosave_path, strfmt("%i.scratch", to)));
  }
  catch (std::exception &exc)
  {
    if (from > 0)
    {
      log_warning("Could not rename auto-save file %s: %s\n",
        _sql_editors[from - 1]->autosave_filename.c_str(), exc.what());
    }
    else
    {
      log_warning("Could not rename temporary auto-save file: %s\n", exc.what());
    }
  }

  try
  {
    if (file_exists(make_path(_autosave_path, strfmt("%i.filename", from))))
      rename(make_path(_autosave_path, strfmt("%i.filename", from)), 
      make_path(_autosave_path, strfmt("%i.filename", to)));
  }
  catch (std::exception &exc)
  {
    log_warning("Could not rename auto-save file %s: %s\n", strfmt("%i.filename", from).c_str(),
      exc.what());
  }
}

void SqlEditorForm::remove_sql_editor(int editor_index)
{
  //if we're removing editor, just cancel query side timer cause there is only one timer
  if (_side_palette)
    _side_palette->cancel_timer();

  Sql_editor_info::Ref info(_sql_editors.at(editor_index));
  info->editor->stop_processing();
  {
    // Notify the UI that recordsets must be closed.
    MutexLock  lock(info->recordset_mutex);
    RecordsetsRef rsets = info->recordsets;
    if (rsets != NULL)
    {
      // Keep in mind the list_changed callback will modify the recordset list.
      while (rsets->size() > 0)
      {
        recordset_list_changed(editor_index, *rsets->begin(), false);
        rsets->erase(rsets->begin());
      }
    }
  }
  
  Sql_editor::Ref active_sql_editor;
  {
    MutexLock sql_editors_mutex(_sql_editors_mutex);
    if ((editor_index < 0) || (editor_index >= (int)_sql_editors.size()))
      return;
    
    try
    {
      base::remove(make_path(_autosave_path, strfmt("%i.autosave", editor_index+1)));
    } catch (std::exception &exc) { log_warning("Could not delete auto-save file: %s\n", exc.what()); }
    try
    {
      base::remove(make_path(_autosave_path, strfmt("%i.filename", editor_index+1)));
    } catch (std::exception &exc) { log_warning("Could not delete auto-save file: %s\n", exc.what()); }
    try
    {
      base::remove(make_path(_autosave_path, strfmt("%i.scratch", editor_index+1)));
    } catch (std::exception &exc) { log_warning("Could not delete auto-save file: %s\n", exc.what()); }
    
    active_sql_editor= _sql_editors[editor_index]->editor;
    _sql_editors.erase(_sql_editors.begin() + editor_index);
    
    // Rename the remaining editor auto saves to match their index.
    for (size_t i= editor_index; i < _sql_editors.size(); i++)
      rename_autosave_files(i + 2, i + 1);
  }
  
  if (_active_sql_editor_index >= (int) _sql_editors.size())
    _active_sql_editor_index= _sql_editors.size()-1;
  
  sql_editor_list_changed(active_sql_editor, false);
}


int SqlEditorForm::sql_editor_count()
{
  MutexLock sql_editors_mutex(_sql_editors_mutex);
  return _sql_editors.size();
}


void SqlEditorForm::active_sql_editor_index(int val) 
{ 
  _active_sql_editor_index= val; 

  validate_menubar();
}


Sql_editor::Ref SqlEditorForm::active_sql_editor()
{
  if (_active_sql_editor_index < 0 || _active_sql_editor_index >= (int) _sql_editors.size())
    return Sql_editor::Ref();
  return _sql_editors.at(_active_sql_editor_index)->editor;
}


Sql_editor::Ref SqlEditorForm::active_sql_editor_or_new_scratch()
{
  if (_active_sql_editor_index < 0)
    new_sql_scratch_area(false);
  return active_sql_editor();
}

Sql_editor::Ref SqlEditorForm::sql_editor(int new_index)
{
  MutexLock sql_editors_mutex(_sql_editors_mutex);
  if (new_index >= 0 && new_index < (int)_sql_editors.size())
    return _sql_editors.at(new_index)->editor;
  return Sql_editor::Ref();
}


bool SqlEditorForm::sql_editor_reorder(Sql_editor::Ref editor, int new_index)
{
  int old_index = sql_editor_index(editor);
  if (old_index < 0 || old_index == new_index || new_index < 0)
    return false;

  MutexTryLock editor_lock(_sql_editors_mutex);
  if (editor_lock.locked())
  {
    // First reorder auto-save files so file names are correct in case of errors.
    rename_autosave_files(old_index + 1, 0); // A name not used otherwise.
    if (old_index > new_index)
    {
      for (int i = old_index - 1; i >= new_index; i--)
        rename_autosave_files(i + 1, i + 2);
    }
    else
    {
      for (int i = old_index + 1; i <= new_index; i++)
        rename_autosave_files(i + 1, i);
    }
    rename_autosave_files(0, new_index + 1);

    // Then reorder the editors.
    Sql_editor_info::Ref info(_sql_editors[old_index]);
    _sql_editors.erase(_sql_editors.begin()+old_index);
    if (new_index >= (int)_sql_editors.size())
      _sql_editors.push_back(info);
    else
      _sql_editors.insert(_sql_editors.begin() + new_index, info);
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if this is an editor created from context menu (so it is supposed to maximize the resultset).
 */
bool SqlEditorForm::sql_editor_start_collapsed(int index)
{
  bool result = _sql_editors[index]->start_collapsed;

  // Reset the collapsed flag in case it is queried multiple times even though the editor has already
  // been set up. This is to avoid frequently resetting the collapsed state when rerunning the query
  // in that particular sql editor.
  _sql_editors[index]->start_collapsed = false;
  return result;
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorForm::sql_editor_will_close(int new_index)
{
  int edited_recordsets = 0;
  {
    MutexLock sql_editors_mutex(_sql_editors_mutex);
    if (new_index < 0 || new_index >= (int)_sql_editors.size() || _sql_editors.at(new_index)->busy)
      return false;
  }
  
  // the recordset lock will prevent the editor from being closed elsewhere
  MutexTryLock recordset_mutex(_sql_editors[new_index]->recordset_mutex);
  if (!recordset_mutex.locked())
    return false;
  {
    RecordsetsRef rsets(sql_editor_recordsets(new_index));
    if (rsets)
    {
      for (Recordsets::const_iterator end = rsets->end(), iter = rsets->begin(); iter != end; ++iter)
        if ((*iter)->has_pending_changes())
          edited_recordsets++;
    }
  }
  
  bool check_scratch_editors = true;

  // if Save of workspace on close is enabled, we don't need to check whether there are unsaved scratch 
  // SQL editors but other stuff should be checked.
  grt::ValueRef option(_grtm->get_app_option("workbench:SaveSQLWorkspaceOnClose"));
  if (option.is_valid() && *grt::IntegerRef::cast_from(option))
    check_scratch_editors = false;

  if (!sql_editor_is_scratch(new_index) || check_scratch_editors)
  {
    if (sql_editor(new_index)->get_editor_control()->is_dirty())
    {
      int result = mforms::Utilities::show_warning(_("Close SQL Tab"), 
        strfmt(_("SQL script %s has unsaved changes.\n"
        "Would you like to Save these changes, discard them or cancel closing the page?"),
        sql_editor_caption(new_index).c_str()), _("Save"), _("Cancel"), _("Don't Save"));

      if (result == mforms::ResultCancel)
        return false;
      else if (result == mforms::ResultOk)
      {
        if (!save_sql_script_file(sql_editor_path(new_index), new_index))
          return false;
      }
      else
        sql_editor(new_index)->get_editor_control()->reset_dirty();
    }
  }
  
  int r = -1;
  if (edited_recordsets == 1)
    r = mforms::Utilities::show_warning(_("Close SQL Tab"),
                                        strfmt(_("An edited recordset has unsaved changes in SQL tab %s.\n"
                                                 "Would you like to save these changes, discard them or cancel closing the page?"),
                                               sql_editor_caption(new_index).c_str()),
                                        _("Save Changes"), _("Cancel"), _("Don't Save"));
  else
    if (edited_recordsets > 0)
      r = mforms::Utilities::show_warning(_("Close SQL Tab"),
                                          strfmt(_("There are %i recordsets with unsaved changes in SQL tab %s.\n"
                                                   "Would you like to save these changes, discard them or cancel closing to review them manually?"),
                                                 edited_recordsets, sql_editor_caption(new_index).c_str()),
                                          _("Save All"), _("Cancel"), _("Don't Save"));
  
  bool success = true;
  if (r == mforms::ResultCancel)
    success = false;
  else
  {
    RecordsetsRef rsets(sql_editor_recordsets(new_index));
    
    for (Recordsets::const_iterator end = rsets->end(), iter = rsets->begin(); iter != end; ++iter)
    {
      if ((*iter)->has_pending_changes())
      {
        try
        {
          if (r == mforms::ResultOk)
            (*iter)->apply_changes();
          else
            (*iter)->rollback();
        }
        catch (const std::exception &exc)
        {
          if (mforms::Utilities::show_error(_("Save Changes"),
                                            strfmt(_("An error occurred while saving changes to the recordset %s\n%s"),
                                                   (*iter)->caption().c_str(), exc.what()),
                                            _("Ignore"), _("Cancel"), "") == mforms::ResultCancel)
          {
            success = false;
            break;
          }
        }        
      }
    }
  }
  return success;
}

//--------------------------------------------------------------------------------------------------

std::string SqlEditorForm::sql_editor_caption(int new_index)
{
  if (new_index < 0)
    new_index = _active_sql_editor_index;
  
  if (new_index >= 0 && new_index < (int) _sql_editors.size())
  {
    if (_sql_editors.at(new_index)->editor->get_editor_control()->is_dirty() && !_sql_editors.at(new_index)->is_scratch)
      return _sql_editors.at(new_index)->caption+"*";
    else
      return _sql_editors.at(new_index)->caption;
  }
  return "";
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::sql_editor_caption(int new_index, std::string caption)
{
  if (new_index < 0)
    new_index = _active_sql_editor_index;
  
  if (new_index >= 0 && new_index < (int) _sql_editors.size())
    _sql_editors.at(new_index)->caption = caption;
}

//--------------------------------------------------------------------------------------------------

int SqlEditorForm::sql_editor_index_for_recordset(long rset)
{
  int i = 0;
  for (Sql_editors::iterator iter = _sql_editors.begin(); iter != _sql_editors.end(); ++iter)
  {
    RecordsetsRef rsets = (*iter)->recordsets;
    if (rsets)
    {
      for (Recordsets::iterator rend = rsets->end(), rec = rsets->begin(); rec != rend; ++rec)
      {
        if ((*rec)->key() == rset)
          return i;
      }
    }
    ++i;
  }
  return -1;
}

RecordsetsRef SqlEditorForm::sql_editor_recordsets(const int index)
{
  Sql_editor_info::Ref editor_info = _sql_editors[index];
  RecordsetsRef rsets = editor_info->recordsets;
  
  if (!rsets)
  {
    rsets = RecordsetsRef(new Recordsets());
    editor_info->recordsets = rsets;
  }
  
  return rsets;
}

//--------------------------------------------------------------------------------------------------

int SqlEditorForm::sql_editor_index(Sql_editor::Ref editor)
{
  MutexLock ed_lock(_sql_editors_mutex);
  for (size_t i = 0; i < _sql_editors.size(); i++)
  {
    if (_sql_editors[i]->editor == editor)
      return (int)i;
  }
  return -1;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::set_sql_editor_text(const char *sql, int editor_index)
{
  if (editor_index < 0)
    editor_index = _active_sql_editor_index;

  if (editor_index >= 0 && editor_index < (int)_sql_editors.size())
  {
    _sql_editors[editor_index]->editor->set_refresh_enabled(true);
    _sql_editors[editor_index]->editor->sql(sql);
  }
}

void SqlEditorForm::new_sql_script_file()
{
  _active_sql_editor_index= add_sql_editor(false);
  sql_editor_new_ui(_active_sql_editor_index);
  _grtm->replace_status_text(_("Added new script editor"));
  update_menu_and_toolbar();
}


void SqlEditorForm::new_sql_scratch_area(bool start_collapsed)
{
  _active_sql_editor_index= add_sql_editor(true, start_collapsed);
  sql_editor_new_ui(_active_sql_editor_index);
  _grtm->replace_status_text(_("Added new scratch query editor"));
  update_menu_and_toolbar();
}


void SqlEditorForm::revert_sql_script_file()
{
  Sql_editor_info::Ref info(_sql_editors[_active_sql_editor_index]);
  
  info->editor->sql("");
  sql_editor_open_file(_active_sql_editor_index, info->filename, info->orig_encoding);
  
  _grtm->replace_status_text(strfmt(_("Reverted to saved '%s'"), info->filename.c_str()));
}


void SqlEditorForm::open_file(const std::string &path, bool in_new_tab)
{
  std::string file_path = path;
  
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
  
  if (in_new_tab)
    new_sql_script_file();
  
  if (sql_editor(_active_sql_editor_index)->get_editor_control()->is_dirty())
  {
    int r = mforms::Utilities::show_warning(_("Open File"), 
                                            strfmt(_("SQL script %s has unsaved changes.\n"
                                                     "Would you like to Save these changes?"),
                                                   sql_editor_path(_active_sql_editor_index).c_str()), _("Save"), _("Cancel"), _("Don't Save"));
    if (r == mforms::ResultCancel)
      return;
    else if (r == mforms::ResultOk)
      if (!save_sql_script_file(sql_editor_path(_active_sql_editor_index), _active_sql_editor_index))
        return;
  }
  
  sql_editor_open_file(_active_sql_editor_index, file_path, "");
}

#define EDITOR_TEXT_LIMIT 100 * 1024 * 1024

void SqlEditorForm::sql_editor_open_file(int index, const std::string &file_path, const std::string &encoding)
{
  if (index < 0)
    index = _active_sql_editor_index;
  
  if (index < 0 || index >= (int)_sql_editors.size())
    return;
  
  gchar *contents = NULL;
  gsize length = base_get_file_size(file_path.c_str());
  GError *error = NULL;
  
  if (length > EDITOR_TEXT_LIMIT)
  {
    // File is larger than 100 MB. Tell the user we are going to switch off code folding and
    // auto completion.
    int result = mforms::Utilities::show_warning(_("Large File"), strfmt(_("The file \"%s\" has a size "
      "of %.2f MB. Are you sure you want to load this large file?\n\nNote: code folding "
      "will be disabled for this file."), file_path.c_str(), length / 1024.0 / 1024.0), _("OK, load it"), _("No, better not"));
    if (result != mforms::ResultOk)
      return;
  }
  _grtm->replace_status_text(strfmt(_("Loading SQL script file '%s'..."), file_path.c_str()));
  
  if (!g_file_get_contents(file_path.c_str(), &contents, &length, &error))
  {
    _grtm->replace_status_text(strfmt(_("Error loading SQL script file '%s'."), file_path.c_str()));
    mforms::Utilities::show_error(strfmt(_("Error opening file %s"), file_path.c_str()), error->message, _("OK"));
    g_error_free(error);
    return;
  }
  
  // XXX: the handling here is ineffective. Switch to const char* instead copying around data.
  std::string utf8_contents;
  std::string original_encoding;
  if (!FileCharsetDialog::ensure_filedata_utf8(contents, length, encoding,
                                               file_path, utf8_contents, &original_encoding))
  {
    g_free(contents);
    _grtm->replace_status_text(_("Cancelled"));
    return;
  }
  g_free(contents);
  
  //if (run_only_skip_editor)
  if (0)
  {    
    _grtm->replace_status_text(strfmt(_("Executing SQL script file '%s'..."), file_path.c_str()));
    
    exec_sql_retaining_editor_contents(utf8_contents, Sql_editor::Ref(), true); 
    
    _grtm->replace_status_text(strfmt(_("Finished executing SQL script file '%s'."), file_path.c_str()));
    
    if (_exec_sql_error_count == 0)
      mforms::Utilities::show_message(_("Execute Script File"),
                                      _("Script file execution finished with no errors."), "OK", "", "");
    else
      mforms::Utilities::show_message(_("Execute Script File"),
                                      strfmt(_("Script file execution finished with %i errors."), _exec_sql_error_count),
                                      "OK", "", "");
    return;
  }  
  
  _updating_sql_editor++;
  set_sql_editor_text(utf8_contents.c_str(), index);
  _updating_sql_editor--;
  
  Sql_editor_info::Ref editor(_sql_editors[index]);
  editor->editor->get_editor_control()->reset_dirty();

  editor->filename = file_path;
  editor->orig_encoding = original_encoding;
  editor->is_scratch = false;
  editor->caption = base::strip_extension(base::basename(file_path));
  base::file_mtime(file_path, editor->file_timestamp);
  //_context_ui->get_wb()->add_recent_file(file_path);
  {
    NotificationInfo info;
    info["opener"] = "SqlEditorForm";
    info["path"] = file_path;
    NotificationCenter::get()->send("GNDocumentOpened", this, info);
  }
  do_partial_ui_refresh(RefreshEditorTitle);
  
  if (!_loading_workspace)
    auto_save();
  
  _grtm->replace_status_text(strfmt(_("Loaded SQL script file '%s'"), file_path.c_str()));
}


void SqlEditorForm::save_file()
{
  if (_active_sql_editor_index >= 0 && _active_sql_editor_index < (int)_sql_editors.size())
    save_sql_script_file(sql_editor_path(_active_sql_editor_index), _active_sql_editor_index);
}

// file_name - filename to save to or empty if it should be asked to user
// editor_index - index of the editor which contents should be saved
// Returns true if file was saved or false if it was cancelled or failed.
bool SqlEditorForm::save_sql_script_file(const std::string &file_name, int editor_index)
{  
  std::string path = file_name;
  
  if (path.empty())
  {
    mforms::FileChooser dlg(mforms::SaveFile);
    
    dlg.set_title(_("Save SQL Script"));
    dlg.set_extensions("SQL Files (*.sql)|*.sql", "sql");
    if (!dlg.run_modal())
      return false;
    
    path = dlg.get_path();
  }
  
  if (!path.empty())
  {
    GError *error= NULL;
    
    // this is already done in FileChooser  
    //    if (!g_str_has_suffix(path.c_str(), ".sql"))
    //      path.append(".sql");
    
    _grtm->replace_status_text(strfmt(_("Saving SQL script to '%s'..."), path.c_str()));
    
    std::pair<const char*, size_t> text = sql_editor(editor_index)->text_ptr();
    if (!g_file_set_contents(path.c_str(), text.first, text.second, &error))
    {
      _grtm->replace_status_text(strfmt(_("Error saving SQL script to '%s'."), path.c_str()));
      
      mforms::Utilities::show_error(strfmt(_("Error writing file %s"), path.c_str()),
                                    error->message, _("OK"));
      g_error_free(error);
      return false;
    }
    
    Sql_editor_info::Ref editor(_sql_editors[editor_index]);
    editor->editor->get_editor_control()->reset_dirty();
    editor->filename = path;
    editor->is_scratch = false;
    editor->caption = base::strip_extension(base::basename(path));
    base::file_mtime(path, editor->file_timestamp);

    do_partial_ui_refresh(RefreshEditorTitle);
    
    //_context_ui->get_wb()->add_recent_file(path);
    {
      NotificationInfo info;
      info["opener"] = "SqlEditorForm";
      info["path"] = path;
      NotificationCenter::get()->send("GNDocumentOpened", this, info);
    }    
    
    _grtm->replace_status_text(strfmt(_("SQL script saved to '%s'"), path.c_str()));
    
    auto_save();
    return true;
  }
  return false;
}


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
