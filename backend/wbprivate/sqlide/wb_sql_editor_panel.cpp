/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
#include "wb_sql_editor_panel.h"
#include "wb_sql_editor_result_panel.h"
#include "sqlide/sql_editor_be.h"
#include "sqlide/recordset_cdbc_storage.h"
#include "grtpp_notifications.h"

#include "base/log.h"
#include "base/file_functions.h"

#include "mforms/toolbar.h"
#include "mforms/menubar.h"
#include "mforms/code_editor.h"
#include "mforms/find_panel.h"
#include "mforms/filechooser.h"

#include "workbench/wb_command_ui.h"

#include "base/boost_smart_ptr_helpers.h"

#include "objimpl/ui/mforms_ObjectReference_impl.h"
#include "objimpl/db.query/db_query_Resultset.h"

#include "grtui/file_charset_dialog.h"

#include <boost/lexical_cast.hpp>


DEFAULT_LOG_DOMAIN("SqlEditorPanel");

using namespace bec;
using namespace base;

//--------------------------------------------------------------------------------------------------

SqlEditorPanel::SqlEditorPanel(SqlEditorForm *owner, bool is_scratch, bool start_collapsed)
: mforms::AppView(false, "db.query.QueryBuffer", false), _form(owner),
  _editor_box(false), _splitter(false, true),
  _lower_tabview(mforms::TabViewEditorBottom), _lower_dock_delegate(&_lower_tabview, db_query_QueryEditor::static_class_name()),
  _lower_dock(&_lower_dock_delegate, false),
  _tab_action_box(true), _tab_action_apply(mforms::SmallButton), _tab_action_revert(mforms::SmallButton), _tab_action_info("Read Only"),
  _rs_sequence(0), _busy(false), _is_scratch(is_scratch)
{
  GRTManager *grtm = owner->grt_manager();
  db_query_QueryEditorRef grtobj(grtm->get_grt());

  grtobj->resultDockingPoint(mforms_to_grt(grtm->get_grt(), &_lower_dock));

  // In opposition to the object editors, each individual sql editor gets an own parser context
  // (and hence an own parser), to allow concurrent and multi threaded work.
  parser::MySQLParserServices::Ref services = parser::MySQLParserServices::get(grtm->get_grt());

  parser::ParserContext::Ref context = services->createParserContext(owner->rdbms()->characterSets(),
    owner->rdbms_version(), owner->lower_case_table_names() != 0);

  _editor = MySQLEditor::create(grtm->get_grt(), context, owner->work_parser_context(), grtobj);
  _editor->sql_check_progress_msg_throttle(grtm->get_app_option_int("DbSqlEditor:ProgressStatusUpdateInterval", 500)/(double)1000);
  _editor->set_auto_completion_cache(owner->auto_completion_cache());
  _editor->set_sql_mode(owner->sql_mode());
  _editor->set_current_schema(owner->active_schema());
  UIForm::scoped_connect(_editor->text_change_signal(),
                         boost::bind(&SqlEditorPanel::update_title, this));

  add(&_splitter, true, true);

  mforms::CodeEditor* code_editor = editor_be()->get_editor_control();

  _editor_box.add(setup_editor_toolbar(), false, true);
  _editor_box.add_end(code_editor, true, true);

  code_editor->set_font(grt::StringRef::cast_from(grtm->get_app_option("workbench.general.Editor:Font")));
  code_editor->set_status_text("");
  code_editor->set_show_find_panel_callback(boost::bind(&SqlEditorPanel::show_find_panel, this, _1, _2));

  if (start_collapsed)
    _editor->get_editor_control()->set_size(-1, 25);

  _splitter.add(&_editor_box);
  _splitter.add(&_lower_tabview);
  _splitter.set_position((int)mforms::App::get()->get_application_bounds().height());
  UIForm::scoped_connect(_splitter.signal_position_changed(), boost::bind(&SqlEditorPanel::splitter_resized, this));
  _tab_action_box.set_spacing(4);
  _tab_action_box.add_end(&_tab_action_info, false, true);
  _tab_action_box.add_end(&_tab_action_icon, false, false);
  _tab_action_box.add_end(&_tab_action_revert, false, true);
  _tab_action_box.add_end(&_tab_action_apply, false, true);
  _tab_action_icon.set_image(mforms::App::get()->get_resource_path("mini_notice.png"));
  _tab_action_icon.show(false);
  _tab_action_info.show(false);
  _tab_action_apply.enable_internal_padding(true);
  _tab_action_apply.set_text("Apply");
  _tab_action_apply.signal_clicked()->connect(boost::bind(&SqlEditorPanel::apply_clicked, this));
  _tab_action_revert.enable_internal_padding(true);
  _tab_action_revert.set_text("Revert");
  _tab_action_revert.signal_clicked()->connect(boost::bind(&SqlEditorPanel::revert_clicked, this));

  _tab_action_box.relayout();
  _tab_action_box.set_size(_tab_action_box.get_preferred_width(), _tab_action_box.get_preferred_height());

  _lower_tabview.set_aux_view(&_tab_action_box);
  _lower_tabview.set_allows_reordering(true);
  _lower_tabview.signal_tab_reordered()->connect(boost::bind(&SqlEditorPanel::lower_tab_reordered, this, _1, _2, _3));
  _lower_tabview.signal_tab_changed()->connect(boost::bind(&SqlEditorPanel::lower_tab_switched, this));
  _lower_tabview.signal_tab_closing()->connect(boost::bind(&SqlEditorPanel::lower_tab_closing, this, _1));
  _lower_tabview.signal_tab_closed()->connect(boost::bind(&SqlEditorPanel::lower_tab_closed, this, _1, _2));
  _lower_tabview.set_tab_menu(&_lower_tab_menu);

  set_on_close(boost::bind(&SqlEditorPanel::on_close_by_user, this));

  _lower_tab_menu.signal_will_show()->connect(boost::bind(&SqlEditorPanel::tab_menu_will_show, this));
  _lower_tab_menu.add_item_with_title("Rename Tab", boost::bind(&SqlEditorPanel::rename_tab_clicked, this), "rename");
  _lower_tab_menu.add_check_item_with_title("Pin Tab", boost::bind(&SqlEditorPanel::pin_tab_clicked, this), "pin");
  _lower_tab_menu.add_separator();
  _lower_tab_menu.add_item_with_title("Close Tab", boost::bind(&SqlEditorPanel::close_tab_clicked, this), "close");
  _lower_tab_menu.add_item_with_title("Close Other Tabs", boost::bind(&SqlEditorPanel::close_other_tabs_clicked, this), "close_others");
}

//--------------------------------------------------------------------------------------------------

SqlEditorPanel::~SqlEditorPanel()
{
  _editor->stop_processing();
  _editor->cancel_auto_completion();
}

//--------------------------------------------------------------------------------------------------

db_query_QueryEditorRef SqlEditorPanel::grtobj()
{
  return db_query_QueryEditorRef::cast_from(_editor->grtobj());
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::on_close_by_user()
{
  if (can_close())
  {
    close();
    return false; // return false because we'll do the undocking ourselves
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::can_close()
{
  if (_busy)
    return false;

  bool check_scratch_editors = true;
  // if Save of workspace on close is enabled, we don't need to check whether there are unsaved scratch
  // SQL editors but other stuff should be checked.
  grt::ValueRef option(_form->grt_manager()->get_app_option("workbench:SaveSQLWorkspaceOnClose"));
  if (option.is_valid() && *grt::IntegerRef::cast_from(option))
    check_scratch_editors = false;

  if (!_is_scratch || check_scratch_editors)
  {
    if (is_dirty())
    {
      int result = mforms::Utilities::show_warning(_("Close SQL Tab"),
                                                   strfmt(_("SQL script %s has unsaved changes.\n"
                                                            "Would you like to Save these changes before closing?"),
                                                          get_title().c_str()), _("Save"), _("Cancel"), _("Don't Save"));

      if (result == mforms::ResultCancel)
        return false;
      else if (result == mforms::ResultOk)
      {
        if (!save())
          return false;
      }
      else
        _editor->get_editor_control()->reset_dirty();
    }
  }


  // check if there are unsaved recordset changes
  int edited_recordsets = 0;
  for (int c = _lower_tabview.page_count(), i = 0; i < c; i++)
  {
    SqlEditorResult* result = dynamic_cast<SqlEditorResult*>(_lower_tabview.get_page(i));
    if (result && result->has_pending_changes())
      edited_recordsets++;
  }

  int r = -999;
  if (edited_recordsets == 1)
    r = mforms::Utilities::show_warning(_("Close SQL Tab"),
                                        strfmt(_("An edited recordset has unsaved changes in %s.\n"
                                                       "Would you like to save these changes, discard them or cancel closing the page?"),
                                                     get_title().c_str()),
                                        _("Save Changes"), _("Cancel"), _("Don't Save"));
  else if (edited_recordsets > 0)
    r = mforms::Utilities::show_warning(_("Close SQL Tab"),
                                        strfmt(_("There are %i recordsets with unsaved changes in %s.\n"
                                                       "Would you like to save these changes, discard them or cancel closing to review them manually?"),
                                                     edited_recordsets, get_title().c_str()),
                                        _("Save All"), _("Cancel"), _("Don't Save"));

  bool success = true;
  if (r != -999)
  {
    if (r == mforms::ResultCancel)
      success = false;
    else
    {
      for (int c = _lower_tabview.page_count(), i = 0; i < c; i++)
      {
        SqlEditorResult* result = dynamic_cast<SqlEditorResult*>(_lower_tabview.get_page(i));
        if (result && result->has_pending_changes())
        {
          try
          {
            if (r == mforms::ResultOk)
              result->apply_changes();
            else
              result->discard_changes();
          }
          catch (const std::exception &exc)
          {
            if (mforms::Utilities::show_error(_("Save Changes"),
                                              strfmt(_("An error occurred while saving changes to the recordset %s\n%s"),
                                                     result->recordset()->caption().c_str(), exc.what()),
                                              _("Ignore"), _("Cancel"), "") == mforms::ResultCancel)
            {
              success = false;
              break;
            }
          }
        }
      }
    }
  }

  // close everything now (recordsets should already have their dirty flag cleared)
  if (success && !_lower_dock.close_all_views())
    return false;

  return success;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::apply_clicked()
{
  SqlEditorResult *result = active_result_panel();
  if (result)
    result->apply_changes();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::revert_clicked()
{
  SqlEditorResult *result = active_result_panel();
  if (result)
    result->discard_changes();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::resultset_edited()
{
  SqlEditorResult *result = active_result_panel();
  Recordset::Ref rset;
  if (result && (rset = result->recordset()))
  {
    bool edited = rset->has_pending_changes();
    _tab_action_apply.set_enabled(edited);
    _tab_action_revert.set_enabled(edited);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::splitter_resized()
{
  if (_lower_tabview.page_count() > 0)
  {
    _form->grt_manager()->set_app_option("DbSqlEditor:ResultSplitterPosition",
                                         grt::IntegerRef(_splitter.get_position()));
  }
}

//--------------------------------------------------------------------------------------------------

#define EDITOR_TEXT_LIMIT 100 * 1024 * 1024

bool SqlEditorPanel::load_autosave(const std::string &file, const std::string &real_filename, const std::string &encoding)
{
  if (!load_from(file, encoding, true))
    return false;
  _filename = real_filename;
  if (real_filename.empty())
    _file_timestamp = 0;
  else
    base::file_mtime(_filename, _file_timestamp);

  std::string tab_name = base::strip_extension(base::basename(file));
  try
  {
    // check if the filename is just a number
    boost::lexical_cast<int>(tab_name);
  }
  catch(const boost::bad_lexical_cast &)
  {
    set_title(tab_name);
  }
  _autosave_file_path = file;

  return false;
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::load_from(const std::string &file, const std::string &encoding, bool keep_dirty)
{
  GError *error = NULL;
  gchar *data;
  gsize length;
  gsize file_size = base_get_file_size(file.c_str());

  if (file_size > EDITOR_TEXT_LIMIT)
  {
    // File is larger than 100 MB. Tell the user we are going to switch off code folding and
    // auto completion.
    int result = mforms::Utilities::show_warning(_("Large File"),
                                                 strfmt(_("The file \"%s\" has a size "
                                                                "of %.2f MB. Are you sure you want to open this large file?\n\nNote: code folding "
                                                                "will be disabled for this file."), file.c_str(), file_size / 1024.0 / 1024.0),
                                                 _("Open"), _("Cancel"));
    if (result != mforms::ResultOk)
      return false;
  }

  _orig_encoding = encoding;
  
  if (!g_file_get_contents(file.c_str(), &data, &length, &error))
  {
    log_error("Could not read file %s: %s\n", file.c_str(), error->message);
    std::string what = error->message;
    g_error_free(error);
    throw std::runtime_error(what);
  }

  char *utf8_data;
  std::string original_encoding;
  if (!FileCharsetDialog::ensure_filedata_utf8(data, length, encoding, file,
                                               utf8_data, &original_encoding))
  {
    g_free(data);
    return false;
  }
  // if original data was in utf8, utf8_data comes back NULL
  if (!utf8_data)
    utf8_data = data;
  else
    g_free(data);

  _editor->set_refresh_enabled(true);
  _editor->sql(utf8_data ? utf8_data : "");

  g_free(utf8_data);

  if (!keep_dirty)
  {
    _editor->get_editor_control()->reset_dirty();

    _filename = file;
    _orig_encoding = original_encoding;
    
    set_title(strip_extension(basename(file)));
  }

  if (!file_mtime(file, _file_timestamp))
  {
    log_warning("Can't get timestamp for %s\n", file.c_str());
    _file_timestamp = 0;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::close()
{
  _form->remove_sql_editor(this);
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::save_as(const std::string &path)
{
  if (path.empty())
  {
    mforms::FileChooser dlg(mforms::SaveFile);

    dlg.set_title(_("Save SQL Script"));
    dlg.set_extensions("SQL Files (*.sql)|*.sql", "sql");
    if (!_filename.empty())
      dlg.set_path(_filename);
    if (!dlg.run_modal())
      return false;
    _filename = dlg.get_path();
  }

  if (save())
  {
    set_title(strip_extension(basename(_filename)));
    {
      NotificationInfo info;
      info["opener"] = "SqlEditorForm";
      info["path"] = _filename;
      NotificationCenter::get()->send("GNDocumentOpened", this, info);
    }
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::save()
{
  if (_filename.empty())
    return save_as("");

  GError *error= NULL;

  // File extension check is already done in FileChooser.

  _form->grt_manager()->replace_status_text(strfmt(_("Saving SQL script to '%s'..."), _filename.c_str()));

  std::pair<const char*, size_t> text = text_data();
  if (!g_file_set_contents(_filename.c_str(), text.first, text.second, &error))
  {
    log_error("Could not save script %s: %s\n", _filename.c_str(), error->message);
    _form->grt_manager()->replace_status_text(strfmt(_("Error saving SQL script to '%s'."), _filename.c_str()));

    mforms::Utilities::show_error(strfmt(_("Error writing file %s"), _filename.c_str()),
                                  error->message, _("OK"));
    g_error_free(error);
    return false;
  }

  _editor->get_editor_control()->reset_dirty();
  _is_scratch = false; // saving a file makes it not a scratch buffer anymore
  file_mtime(_filename, _file_timestamp);

  _form->grt_manager()->replace_status_text(strfmt(_("SQL script saved to '%s'"), _filename.c_str()));

  // update autosave state
  _form->auto_save();

  return true;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::revert_to_saved()
{
  _editor->sql("");
  if (load_from(_filename, _orig_encoding))
  {
    {
      NotificationInfo info;
      info["opener"] = "SqlEditorForm";
      info["path"] = _filename;
      NotificationCenter::get()->send("GNDocumentOpened", this, info);
    }
    _form->auto_save();
    _form->grt_manager()->replace_status_text(strfmt(_("Reverted to saved '%s'"), _filename.c_str()));
  }
}

//--------------------------------------------------------------------------------------------------

int SqlEditorPanel::autosave_index()
{
  if (_autosave_file_path.empty())
    return -1;
  return atoi(strip_extension(basename(_autosave_file_path)).c_str());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::rename_auto_save(int to)
{
  if (!_autosave_file_path.empty())
  {
    std::string dir = dirname(_autosave_file_path);
    std::string prefix = strip_extension(_autosave_file_path);
    std::string new_prefix = make_path(dir, strfmt("%i", to));

    if (prefix != new_prefix)
    {
      try
      {
        if (file_exists(prefix + ".autosave"))
        {
          rename(prefix + ".autosave", new_prefix + ".autosave");
          _autosave_file_path = new_prefix + ".autosave";
        }
        else
        {
          if (file_exists(prefix + ".scratch"))
          {
            rename(prefix + ".scratch", new_prefix + ".scratch");
            _autosave_file_path = new_prefix + ".scratch";
          }
        }
      }
      catch (std::exception &exc)
      {
        log_warning("Could not rename temporary auto-save file: %s\n", exc.what());
      }

      try
      {
        if (file_exists(prefix + ".filename"))
          rename(prefix + ".filename", new_prefix + ".filename");
      }
      catch (std::exception &exc)
      {
        log_warning("Could not rename auto-save file %s: %s\n", (prefix + ".filename").c_str(),
                    exc.what());
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::auto_save(const std::string &directory, int order)
{
  // if the editor is saved to a file, we store the path. if the file is unsaved, we save an updated copy
  // for scrath areas, the file is always kept as an autosave
  std::string filename;

  if (!_is_scratch)
  {
    GError *error = 0;
    std::string contents = _filename + "\n" + _orig_encoding;
    // save the path to the real file
    if (!g_file_set_contents(make_path(directory, strfmt("%i.filename", order)).c_str(),
                             contents.data(), contents.size(), &error))
    {
      log_error("Could not save snapshot of editor contents to %s: %s\n", directory.c_str(), error->message);
      std::string msg(strfmt("Could not save snapshot of editor contents to %s: %s", directory.c_str(), error->message));
      g_error_free(error);
      base_rmdir_recursively(directory.c_str());
      throw msg;
    }
  }

  if (_is_scratch)
  {
    if (!starts_with(get_title(), "Query "))
      filename = strfmt("%s.scratch", get_title().c_str());
    else
      filename = strfmt("%i.scratch", order);
    _autosave_file_path = make_path(directory, filename);
  }
  else
  {
    if (is_dirty())
    {
      if (extension(_autosave_file_path) == ".scratch")
      {
        try
        {
          base::remove(make_path(directory, _autosave_file_path));
        }
        catch (std::exception &e)
        {
          log_warning("Error deleting autosave file %s: %s\n", make_path(directory, _autosave_file_path).c_str(), e.what());
        }
      }

      filename = strfmt("%i.autosave", order);
      _autosave_file_path = make_path(directory, filename);
    }
  }

  // only save editor contents for scratch areas and unsaved editors
  if (!filename.empty())
  {
    // We don't need to lock the editor as we are in the main thread here
    // and directly set the file content without detouring to anything that could change the text.
    GError *error = 0;
    std::string fn = make_path(directory, filename);

    std::pair<const char*, size_t> text = text_data();
    if (!g_file_set_contents(fn.c_str(), text.first, text.second, &error))
    {
      log_error("Could not save snapshot of editor contents to %s: %s\n", fn.c_str(), error->message);
      std::string msg(strfmt("Could not save snapshot of editor contents to %s: %s", fn.c_str(), error->message));
      g_error_free(error);
      base_rmdir_recursively(directory.c_str());
      throw std::runtime_error(msg);
    }
  }
  else
  {
    // delete the autosave file if the file was saved
    try
    {
      if (!_autosave_file_path.empty())
      {
        base::remove(_autosave_file_path);
      }
    }
    catch (std::exception &e)
    {
      log_warning("Error deleting autosave file %s: %s\n", make_path(directory, _autosave_file_path).c_str(), e.what());
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::delete_auto_save()
{
  if (!_autosave_file_path.empty())
  {
    std::string prefix = strip_extension(_autosave_file_path);
    // delete the autosave related files
    try
    {
      base::remove(_autosave_file_path);
    } catch (std::exception &exc) { log_warning("Could not delete auto-save file: %s\n", exc.what()); }
    try
    {
      base::remove(strfmt("%s.filename", prefix.c_str()));
    } catch (std::exception &exc) { log_warning("Could not delete auto-save file: %s\n", exc.what()); }
  }
}

//--------------------------------------------------------------------------------------------------

// Toolbar handling.

void SqlEditorPanel::show_find_panel(mforms::CodeEditor *editor, bool show)
{
  mforms::FindPanel *panel = editor->get_find_panel();
  if (show && !panel->get_parent())
    _editor_box.add(panel, false, true);
  panel->show(show);
}

//--------------------------------------------------------------------------------------------------

static void toggle_continue_on_error(SqlEditorForm *sql_editor_form)
{
  sql_editor_form->continue_on_error(!sql_editor_form->continue_on_error());
}

//--------------------------------------------------------------------------------------------------

static void toggle_limit(mforms::ToolBarItem *item, SqlEditorForm *sql_editor_form)
{
  bool do_limit = item->get_checked();

  sql_editor_form->grt_manager()->set_app_option("SqlEditor:LimitRows", do_limit ? grt::IntegerRef(1) : grt::IntegerRef(0));

  std::string limit = do_limit ? strfmt("%li", sql_editor_form->grt_manager()->get_app_option_int("SqlEditor:LimitRowsCount")) : "0";

  mforms::MenuItem *menu = sql_editor_form->get_menubar()->find_item("limit_rows");
  int c = menu->item_count();
  for (int i = 0; i < c; i++)
  {
    mforms::MenuItem *item = menu->get_item(i);
    if (item->get_type() != mforms::SeparatorMenuItem)
    {
      if (item->get_name() == limit)
        item->set_checked(true);
      else
        item->set_checked(false);
    }
  }
}

//--------------------------------------------------------------------------------------------------

mforms::ToolBar *SqlEditorPanel::setup_editor_toolbar()
{
  mforms::ToolBar *tbar(new mforms::ToolBar(mforms::SecondaryToolBar));
#ifdef _WIN32
  tbar->set_size(-1, 27);
#endif
  mforms::ToolBarItem *item;

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.openFile");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_open.png"));
  item->set_tooltip(_("Open a script file in this editor"));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind((void (SqlEditorForm::*)(const std::string&,bool))&SqlEditorForm::open_file, _form, "", false));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.saveFile");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_save.png"));
  item->set_tooltip(_("Save the script to a file."));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorPanel::save, this));
  tbar->add_item(item);

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.execute");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_execute.png"));
  item->set_tooltip(_("Execute the selected portion of the script or everything, if there is no selection"));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::run_editor_contents, _form, false));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.execute_current_statement");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_execute-current.png"));
  item->set_tooltip(_("Execute the statement under the keyboard cursor"));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::run_editor_contents, _form, true));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.explain_current_statement");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_explain.png"));
  item->set_tooltip(_("Execute the EXPLAIN command on the statement under the cursor"));
  _form->wbsql()->get_cmdui()->scoped_connect(item->signal_activated(),
                                      boost::bind((void (wb::CommandUI::*)(const std::string&))&wb::CommandUI::activate_command,
                                                  _form->wbsql()->get_cmdui(), "builtin:query.explain_current_statement"));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.cancel");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop.png"));
  item->set_tooltip(_("Stop the query being executed (the connection to the DB server will not be restarted and any open transactions will remain open)"));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::cancel_query, _form));
  tbar->add_item(item);

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.stopOnError");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop-on-error-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop-on-error-off.png"));
  item->set_tooltip(_("Toggle whether execution of SQL script should continue after failed statements"));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(toggle_continue_on_error, _form));

  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.toggleLimit");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_row-limit-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_row-limit-off.png"));
  item->set_tooltip(_("Toggle limiting of number of rows returned by queries.\nWorkbech will automatically add the LIMIT clause with the configured number of rows to SELECT queries.\nYou can change the limit number in Preferences or in the Query -> Limit Rows menu."));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(toggle_limit, item, _form));
  tbar->add_item(item);

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.commit");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_commit.png"));
  item->set_tooltip(_("Commit the current transaction.\nNOTE: all query tabs in the same connection share the same transaction. To have independent transactions, you must open a new connection."));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::commit, _form));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.rollback");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_rollback.png"));
  item->set_tooltip(_("Rollback the current transaction.\nNOTE: all query tabs in the same connection share the same transaction. To have independent transactions, you must open a new connection."));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::rollback, _form));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.autocommit");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_autocommit-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_autocommit-off.png"));
  item->set_tooltip(_("Toggle autocommit mode. When enabled, each statement will be committed immediately.\nNOTE: all query tabs in the same connection share the same transaction. To have independent transactions, you must open a new connection."));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::toggle_autocommit, _form));
  tbar->add_item(item);

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("add_snippet");
  item->set_icon(IconManager::get_instance()->get_icon_path("snippet_add.png"));
  item->set_tooltip(_("Save current statement or selection to the snippet list."));
  bec::UIForm::scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::save_snippet, _form));
  tbar->add_item(item);
  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  // adds generic SQL editor toolbar buttons
  _editor->set_base_toolbar(tbar);

  return tbar;
}

//--------------------------------------------------------------------------------------------------

mforms::ToolBar *SqlEditorPanel::get_toolbar()
{
  return _editor->get_toolbar();
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::is_dirty() const
{
  return _editor->get_editor_control()->is_dirty();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::check_external_file_changes()
{
  time_t ts;
  if (!_filename.empty() && file_mtime(_filename, ts))
  {
    if (ts > _file_timestamp)
    {
      // File was changed externally. For now we ignore local changes if the user chooses to reload.
      std::string connection_description = _form->connection_descriptor().is_valid() ?
          strfmt("(from connection to %s) ", _form->connection_descriptor()->name().c_str()) : "";
      if (mforms::Utilities::show_warning("File Changed",
                                          strfmt(_("File %s %swas changed from outside MySQL Workbench.\nWould you like to discard your changes and reload it?"),
                                                       _filename.c_str(), connection_description.c_str()),
                                          "Reload File", "Ignore", "") == mforms::ResultOk
          )
      {
        revert_to_saved();
      }
      else
      {
        _file_timestamp = ts;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::pair<const char*, size_t> SqlEditorPanel::text_data() const
{
  return _editor->text_ptr();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::set_title(const std::string &title)
{
  _title = title;
  grtobj()->name(_title);
  mforms::AppView::set_title(title);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::set_filename(const std::string &filename)
{
  _filename = filename;
  if (!filename.empty())
    set_title(strip_extension(basename(filename)));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::update_title()
{
  if (!_is_scratch)
    mforms::AppView::set_title(_title+(is_dirty() ? "*" : ""));
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts the auto completion list in the currently active editor. The content of this list is
 * determined from various sources + the current query context.
 */
void SqlEditorPanel::list_members()
{
  editor_be()->show_auto_completion(true, owner()->work_parser_context()->recognizer());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::jump_to_placeholder()
{
  _editor->get_editor_control()->jump_to_next_placeholder();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::query_started(bool retain_old_recordsets)
{
  _busy = true;

  _form->set_busy_tab(_form->sql_editor_panel_index(this));

  // disable tabview reordering because we can get new tabs added at odd times if a query is running
  _lower_tabview.set_allows_reordering(false);

  // if we're already running the query, it's obvious there's no more need to autocomplete what we typed
  _editor->cancel_auto_completion();

  if (!retain_old_recordsets)
  {
    // close recordsets that were opened previously (unless they're pinned or something)
    for (int i = _lower_tabview.page_count() - 1; i >= 0; --i)
    {
      SqlEditorResult *result = dynamic_cast<SqlEditorResult*>(_lower_tabview.get_page(i));
      if (result)
      {
        if (result->pinned())
          continue;

        if (result->has_pending_changes())
          continue;

        // make sure that the result is docked here
        int i = _lower_tabview.get_page_index(result);
        if (i >= 0)
        {
          _lower_dock.close_view(result);
          result_removed();
        }
      }
    }
  }

  _was_empty = (_lower_tabview.page_count() == 0);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::query_finished()
{
  _busy = false;

  _form->set_busy_tab(-1);
  _lower_tabview.set_allows_reordering(true);

  _form->post_query_slot();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::query_failed(const std::string &message)
{
  log_error("Unhandled error during query: %s\n", message.c_str());
  _busy = false;

  _form->set_busy_tab(-1);
  _lower_tabview.set_allows_reordering(true);

  _form->post_query_slot();
}

//--------------------------------------------------------------------------------------------------

// Resultset management.

SqlEditorResult *SqlEditorPanel::active_result_panel()
{
  return result_panel(_lower_tabview.get_active_tab());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::lower_tab_switched()
{
  _lower_dock.view_switched();

  db_query_QueryEditorRef qeditor(grtobj());
  SqlEditorResult *result = active_result_panel();
  Recordset::Ref rset;
  if (result && (rset = result->recordset()))
  {
    bool found = false;
    for (size_t c = qeditor->resultPanels().count(), i = 0; i < c; i++)
    {
      if (mforms_from_grt(qeditor->resultPanels()[i]->dockingPoint()) == result->dock())
      {
        found = true;
        qeditor->activeResultPanel(qeditor->resultPanels()[i]);
        break;
      }
    }
    if (!found)
      qeditor->activeResultPanel(db_query_ResultPanelRef());

    bool readonly = rset->is_readonly();
    _tab_action_apply.show(!readonly);
    _tab_action_revert.show(!readonly);
    _tab_action_icon.show(readonly);
    _tab_action_info.show(readonly);
    bool edited = result->has_pending_changes();
    _tab_action_apply.set_enabled(edited);
    _tab_action_revert.set_enabled(edited);
    if (readonly)
    {
      _tab_action_info.set_tooltip(rset->readonly_reason());
      _tab_action_icon.set_tooltip(rset->readonly_reason());
    }
  }
  else
  {
    qeditor->activeResultPanel(db_query_ResultPanelRef());

    _tab_action_apply.show(true);
    _tab_action_revert.show(true);
    _tab_action_icon.show(false);
    _tab_action_info.show(false);

    _tab_action_apply.set_enabled(false);
    _tab_action_revert.set_enabled(false);
  }

  mforms::MenuBar *menu;
  if ((menu = _form->get_menubar()))
  {
    Recordset::Ref rset(result ? result->recordset(): Recordset::Ref());

    menu->set_item_enabled("query.save_edits", rset && rset->has_pending_changes());
    menu->set_item_enabled("query.discard_edits", rset && rset->has_pending_changes());
    menu->set_item_enabled("query.export", (bool)rset);
  }

  // if a lower tab view selection has changed, we make sure it's visible
  if (!_busy && _lower_tabview.page_count() > 0) // if we're running a query, then let dock_result handle this
  {
    int position = _form->grt_manager()->get_app_option_int("DbSqlEditor:ResultSplitterPosition", 200);
    if (position > _splitter.get_height()-100)
      position = _splitter.get_height()-100;
    _splitter.set_position(position);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::on_recordset_context_menu_show(Recordset::Ptr rs_ptr)
{
  Recordset::Ref rs(rs_ptr.lock());
  if (rs)
  {
    grt::DictRef info(rs->grtm()->get_grt());

    std::vector<int> selection(rs->selected_rows());
    grt::IntegerListRef rows(info.get_grt());
    for (std::vector<int>::const_iterator i = selection.begin(); i != selection.end(); ++i)
      rows.insert(*i);

    info.set("selected-rows", rows);
    info.gset("selected-column", rs->selected_column());
    info.set("menu", mforms_to_grt(info.get_grt(), rs->get_context_menu()));

    db_query_QueryBufferRef qbuffer(grtobj());
    if (qbuffer.is_valid() && db_query_QueryEditorRef::can_wrap(qbuffer))
    {
      db_query_QueryEditorRef qeditor(db_query_QueryEditorRef::cast_from(qbuffer));
      for (size_t c = qeditor->resultPanels().count(), i = 0; i < c; i++)
      {
        db_query_ResultsetRef rset(qeditor->resultPanels()[i]->resultset());

        if (rset.is_valid() && dynamic_cast<WBRecordsetResultset*>(rset->get_data())->recordset == rs)
        {
          grt::GRTNotificationCenter::get()->send_grt("GRNSQLResultsetMenuWillShow", rset, info);
          break;
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

SqlEditorResult *SqlEditorPanel::result_panel(int i)
{
  if (i >= 0 && i < _lower_tabview.page_count())
    return dynamic_cast<SqlEditorResult*>(_lower_tabview.get_page(i));
  return NULL;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::add_panel_for_recordset_from_main(Recordset::Ref rset)
{
  if (_form->grt_manager()->in_main_thread())
  {
    SqlEditorForm::RecordsetData *rdata = dynamic_cast<SqlEditorForm::RecordsetData*>(rset->client_data());
    
    rdata->result_panel = add_panel_for_recordset(rset);
  }
  else
    _form->grt_manager()->run_once_when_idle(dynamic_cast<bec::UIForm*>(this), 
      boost::bind(&SqlEditorPanel::add_panel_for_recordset_from_main, this, rset));
}

//--------------------------------------------------------------------------------------------------

SqlEditorResult* SqlEditorPanel::add_panel_for_recordset(Recordset::Ref rset)
{
  SqlEditorResult *result = new SqlEditorResult(this);
  if (rset)
    result->set_recordset(rset);
  dock_result_panel(result);

  return result;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::dock_result_panel(SqlEditorResult *result)
{
  result->grtobj()->owner(grtobj());
  grtobj()->resultPanels().insert(result->grtobj());

  if (Recordset::Ref rset = result->recordset())
    result->set_title(rset->caption());

  _lower_dock.dock_view(result);
  _lower_dock.select_view(result);
  if (_was_empty)
  {
    int position = _form->grt_manager()->get_app_option_int("DbSqlEditor:ResultSplitterPosition", 200);
    if (position > _splitter.get_height()-100)
      position = _splitter.get_height()-100;
    _splitter.set_position(position);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::lower_tab_reordered(mforms::View *view, int from, int to)
{
  if (from == to || dynamic_cast<SqlEditorResult*>(view) == NULL)
    return;

  // not all tabs will have a SqlEditorResult
  // so the reordering gets more complicated, because actual reordering only happens if the relative
  // position between the result objects changes...
  // relative result object order changes always mean that a tab was reordered, but the other way around is
  // not always true

  size_t from_index = grtobj()->resultPanels().get_index(dynamic_cast<SqlEditorResult*>(view)->grtobj());
  if (from_index == grt::BaseListRef::npos)
  {
    log_fatal("Result panel is not in resultPanels() list\n");
    return;
  }

  // first build an array of result panel objects, in the same order as the tabview
  std::vector<std::pair<db_query_ResultPanelRef, int> > panels;
  for (int result_order = 0, i = 0; i < _lower_tabview.page_count(); i++)
  {
    SqlEditorResult *p = result_panel(i);
    if (p)
      panels.push_back(std::make_pair(p->grtobj(), result_order++));
    else
      panels.push_back(std::make_pair(db_query_ResultPanelRef(), 0));
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
    log_fatal("Unable to find suitable target index for reorder\n");
    return;
  }

  grtobj()->resultPanels()->reorder(from_index, to_index);
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::lower_tab_closing(int tab)
{
  mforms::AppView *view = _lower_dock.view_at_index(tab);
  if (view)
  {
    if (_lower_dock.close_view(view))
    {
      result_removed();
      return true;
    }
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::lower_tab_closed(mforms::View *page, int tab)
{
  SqlEditorResult* rpage = dynamic_cast<SqlEditorResult*>(page);
  if (rpage)
  {
    db_query_ResultPanelRef closed_panel(rpage->grtobj());
    grtobj()->resultPanels().remove_value(closed_panel);
    if (closed_panel->resultset().is_valid())
      closed_panel->resultset()->reset_references();
    closed_panel->reset_references();
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::result_removed()
{
  if (_lower_tabview.page_count() == 0)
    _splitter.set_position(_splitter.get_height());
  lower_tab_switched();
}

//--------------------------------------------------------------------------------------------------

std::list<SqlEditorResult*> SqlEditorPanel::dirty_result_panels()
{
  std::list<SqlEditorResult*> results;

  for (int c = _lower_tabview.page_count(), i = 0; i < c; i++)
  {
    SqlEditorResult *result = result_panel(i);
    if (result && result->has_pending_changes())
      results.push_back(result);
  }
  return results;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::tab_menu_will_show()
{
  SqlEditorResult *result(result_panel(_lower_tabview.get_menu_tab()));

  _lower_tab_menu.set_item_enabled("rename", result != NULL);
  _lower_tab_menu.set_item_enabled("pin", result != NULL);
  _lower_tab_menu.set_item_checked("pin", result && result->pinned());

  if (_lower_tabview.page_count() > 1)
    _lower_tab_menu.set_item_enabled("close_others", true); // close others
  else
    _lower_tab_menu.set_item_enabled("close_others", false); // close others
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::rename_tab_clicked()
{
  int tab = _lower_tabview.get_menu_tab();
  SqlEditorResult *result = result_panel(tab);
  if (result)
  {
    std::string title;
    if (mforms::Utilities::request_input(_("Rename Result Tab"), "Enter a new name for the result tab:", result->caption().c_str(), title))
      _lower_tabview.set_tab_title(tab, title);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::pin_tab_clicked()
{
  int tab = _lower_tabview.get_menu_tab();
  SqlEditorResult *result = result_panel(tab);
  if (result)
    result->set_pinned(!result->pinned());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::close_tab_clicked()
{
  lower_tab_closing(_lower_tabview.get_menu_tab());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::close_other_tabs_clicked()
{
  int tab = _lower_tabview.get_menu_tab();
  for (int i = _lower_tabview.page_count() - 1; i >= 0; --i)
  {
    if (i != tab)
      lower_tab_closing(i);
  }
}

//--------------------------------------------------------------------------------------------------
