/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_sql_editor_form.h"
#include "wb_sql_editor_panel.h"
#include "wb_sql_editor_result_panel.h"
#include "sqlide/sql_editor_be.h"
#include "sqlide/recordset_cdbc_storage.h"
#include "grtpp_notifications.h"

#include "base/log.h"
#include "base/file_functions.h"
#include "base/util_functions.h"

#include "mforms/toolbar.h"
#include "mforms/menubar.h"
#include "mforms/code_editor.h"
#include "mforms/find_panel.h"
#include "mforms/filechooser.h"

#include "workbench/wb_command_ui.h"

#include "base/boost_smart_ptr_helpers.h"

#include "objimpl/wrapper/mforms_ObjectReference_impl.h"
#include "objimpl/db.query/db_query_Resultset.h"

#include "grtui/file_charset_dialog.h"
#include "grtsqlparser/mysql_parser_services.h"

#include "mysql/MySQLRecognizerCommon.h"

#include <fstream>
#include <sstream>

// 20 MB max file size for auto-restoring
#define MAX_FILE_SIZE_FOR_AUTO_RESTORE 20000000

DEFAULT_LOG_DOMAIN("SqlEditorPanel");

using namespace bec;
using namespace base;

//--------------------------------------------------------------------------------------------------

SqlEditorPanel::SqlEditorPanel(SqlEditorForm *owner, bool is_scratch, bool start_collapsed)
  : mforms::AppView(false, "Query Buffer", "db.query.QueryBuffer", false),
    _form(owner),
    _editor_box(false),
    _splitter(false, false),
#ifdef __APPLE__
    _lower_tabview(mforms::TabViewEditorBottomPinnable), // TODO: Windows, Linux
#else
    _lower_tabview(mforms::TabViewEditorBottom),
#endif
    _lower_dock_delegate(&_lower_tabview, db_query_QueryEditor::static_class_name()),
    _lower_dock(&_lower_dock_delegate, false),
    _tab_action_box(true),
    _tab_action_apply(mforms::SmallButton),
    _tab_action_revert(mforms::SmallButton),
    _tab_action_info("Read Only"),
    _rs_sequence(0),
    _busy(false),
    _is_scratch(is_scratch) {
  db_query_QueryEditorRef grtobj(grt::Initialized);

  grtobj->resultDockingPoint(mforms_to_grt(&_lower_dock));

  _autosave_file_suffix = grtobj.id();

  // In opposition to the object editors, each individual sql editor gets an own parser context
  // (and hence an own parser), to allow concurrent and multi threaded work.
  parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();

  parsers::MySQLParserContext::Ref context = services->createParserContext(
    owner->rdbms()->characterSets(), owner->rdbms_version(), owner->sql_mode(), owner->lower_case_table_names() != 0);

  parsers::SymbolTable *functionSymbols = parsers::functionSymbolsForVersion(bec::versionToEnum(owner->rdbms_version()));
  _editor = MySQLEditor::create(context, owner->work_parser_context(), { functionSymbols, owner->databaseSymbols() }, grtobj);
  _editor->set_sql_mode(owner->sql_mode());
  _editor->set_current_schema(owner->active_schema());
  UIForm::scoped_connect(_editor->text_change_signal(), std::bind(&SqlEditorPanel::update_title, this));

  add(&_splitter, true, true);

  mforms::CodeEditor *code_editor = editor_be()->get_editor_control();
  code_editor->set_name("Code Editor");
  code_editor->setInternalName("code editor");
  _editor_box.add(setup_editor_toolbar(), false, true);
  _editor_box.add_end(code_editor, true, true);

  code_editor->set_font(
    grt::StringRef::cast_from(bec::GRTManager::get()->get_app_option("workbench.general.Editor:Font")));
  code_editor->set_status_text("");
  code_editor->set_show_find_panel_callback(
    std::bind(&SqlEditorPanel::show_find_panel, this, std::placeholders::_1, std::placeholders::_2));

  if (start_collapsed)
    _editor->get_editor_control()->set_size(-1, 25);

  _splitter.add(&_editor_box, 150);
  _splitter.add(&_lower_tabview, 150);
  _editor_box.set_name("Editor Area");
  _editor_box.setInternalName("Editor area");
  _lower_tabview.set_name("Resultset Placeholder");
  _lower_tabview.setInternalName("Resultset placeholder");

  UIForm::scoped_connect(_splitter.signal_position_changed(), std::bind(&SqlEditorPanel::splitter_resized, this));
  _tab_action_box.set_spacing(4);
  _tab_action_box.add_end(&_tab_action_info, false, true);
  _tab_action_box.add_end(&_tab_action_icon, false, true);
  _tab_action_box.add_end(&_tab_action_revert, false, true);
  _tab_action_box.add_end(&_tab_action_apply, false, true);
  _tab_action_icon.set_image(mforms::App::get()->get_resource_path("mini_notice.png"));
  _tab_action_icon.show(false);
  _tab_action_info.show(false);
  _tab_action_apply.enable_internal_padding(true);
  _tab_action_apply.set_text("Apply");
  _tab_action_apply.signal_clicked()->connect(std::bind(&SqlEditorPanel::apply_clicked, this));
  _tab_action_revert.enable_internal_padding(true);
  _tab_action_revert.set_text("Revert");
  _tab_action_revert.signal_clicked()->connect(std::bind(&SqlEditorPanel::revert_clicked, this));

#ifdef _MSC_VER
  // 19 is the size of the tabs in the bottom tabview.
  _tab_action_apply.set_size(-1, 19);
  _tab_action_revert.set_size(-1, 19);
  _tab_action_box.set_size(-1, 19);
  _tab_action_box.set_back_color(Color::getApplicationColorAsString(AppColorTabUnselected, false));
#endif

  _lower_tabview.set_aux_view(&_tab_action_box);
  _lower_tabview.set_allows_reordering(true);
  _lower_tabview.signal_tab_reordered()->connect(std::bind(
    &SqlEditorPanel::lower_tab_reordered, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _lower_tabview.signal_tab_changed()->connect(std::bind(&SqlEditorPanel::lower_tab_switched, this));
  _lower_tabview.signal_tab_closing()->connect(
    std::bind(&SqlEditorPanel::lower_tab_closing, this, std::placeholders::_1));
  _lower_tabview.signal_tab_closed()->connect(
    std::bind(&SqlEditorPanel::lower_tab_closed, this, std::placeholders::_1, std::placeholders::_2));
  _lower_tabview.signal_tab_pin_changed()->connect(
    std::bind(&SqlEditorPanel::tab_pinned, this, std::placeholders::_1, std::placeholders::_2));
  _lower_tabview.is_pinned = std::bind(&SqlEditorPanel::is_pinned, this, std::placeholders::_1);
  _lower_tabview.set_tab_menu(&_lower_tab_menu);

  _splitter.set_expanded(false, false);
  set_on_close(std::bind(&SqlEditorPanel::on_close_by_user, this));

  _lower_tab_menu.signal_will_show()->connect(std::bind(&SqlEditorPanel::tab_menu_will_show, this));
  _lower_tab_menu.add_item_with_title("Rename Tab", std::bind(&SqlEditorPanel::rename_tab_clicked, this), "Rename Tab", "rename");
  _lower_tab_menu.add_check_item_with_title("Pin Tab", std::bind(&SqlEditorPanel::pin_tab_clicked, this), "Pin Tab", "pin");
  _lower_tab_menu.add_separator();
  _lower_tab_menu.add_item_with_title("Close Tab", std::bind(&SqlEditorPanel::close_tab_clicked, this), "Close Tab", "close");
  _lower_tab_menu.add_item_with_title("Close Other Tabs", std::bind(&SqlEditorPanel::close_other_tabs_clicked, this),
                                      "Close Other Tabs", "close_others");
}

//--------------------------------------------------------------------------------------------------

SqlEditorPanel::~SqlEditorPanel() {
  _editor->stop_processing();
  _editor->cancel_auto_completion();
}

//--------------------------------------------------------------------------------------------------

db_query_QueryEditorRef SqlEditorPanel::grtobj() {
  return db_query_QueryEditorRef::cast_from(_editor->grtobj());
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::on_close_by_user() {
  // this can also get closed when close_all_view() is called when the connection is closed
  if (_form->is_closing() || can_close()) {
    // do not call close, since that would undock ourselves and the caller will also undock this
    // we just need to be sure that the d-tor is called in all closing methods (close the tab itself from the X and
    // through kbd,
    // close the connection, close WB)
    //    close();
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::can_close() {
  if (_busy)
    return false;

  bool check_editors = true;
  // if Save of workspace on close is enabled, we don't need to check whether there are unsaved scratch
  // SQL editors but other stuff should be checked.
  grt::ValueRef option(bec::GRTManager::get()->get_app_option("workbench:SaveSQLWorkspaceOnClose"));
  if (option.is_valid() && *grt::IntegerRef::cast_from(option))
    check_editors = false;

  // don't need to check for unsaved changes when closing the whole form
  // if save-workspace is enabled, since they'll get autosaved anyway
  // otoh, when closing the file itself, it should check
  if (!_form->is_closing())
    check_editors = true;

  if (!_is_scratch && check_editors) {
    if (is_dirty()) {
      int result = mforms::Utilities::show_warning(_("Close SQL Tab"),
                                                   strfmt(_("SQL script %s has unsaved changes.\n"
                                                            "Would you like to Save these changes before closing?"),
                                                          get_title().c_str()),
                                                   _("Save"), _("Cancel"), _("Don't Save"));

      if (result == mforms::ResultCancel)
        return false;
      else if (result == mforms::ResultOk) {
        if (!save())
          return false;
      } else
        _editor->get_editor_control()->reset_dirty();
    }
  }

  // check if there are unsaved recordset changes
  int edited_recordsets = 0;
  for (int c = _lower_tabview.page_count(), i = 0; i < c; i++) {
    SqlEditorResult *result = dynamic_cast<SqlEditorResult *>(_lower_tabview.get_page(i));
    if (result && result->has_pending_changes())
      edited_recordsets++;
  }

  int r = -999;
  if (edited_recordsets == 1)
    r = mforms::Utilities::show_warning(
      _("Close SQL Tab"), strfmt(_("An edited recordset has unsaved changes in %s.\n"
                                   "Would you like to save these changes, discard them or cancel closing the page?"),
                                 get_title().c_str()),
      _("Save Changes"), _("Cancel"), _("Don't Save"));
  else if (edited_recordsets > 0)
    r = mforms::Utilities::show_warning(
      _("Close SQL Tab"),
      strfmt(_("There are %i recordsets with unsaved changes in %s.\n"
               "Would you like to save these changes, discard them or cancel closing to review them manually?"),
             edited_recordsets, get_title().c_str()),
      _("Save All"), _("Cancel"), _("Don't Save"));

  bool success = true;
  if (r != -999) {
    if (r == mforms::ResultCancel)
      success = false;
    else {
      for (int c = _lower_tabview.page_count(), i = 0; i < c; i++) {
        SqlEditorResult *result = dynamic_cast<SqlEditorResult *>(_lower_tabview.get_page(i));
        if (result && result->has_pending_changes()) {
          try {
            if (r == mforms::ResultOk)
              result->apply_changes();
            else
              result->discard_changes();
          } catch (const std::exception &exc) {
            if (mforms::Utilities::show_error(
                  _("Save Changes"), strfmt(_("An error occurred while saving changes to the recordset %s\n%s"),
                                            result->recordset()->caption().c_str(), exc.what()),
                  _("Ignore"), _("Cancel"), "") == mforms::ResultCancel) {
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

void SqlEditorPanel::apply_clicked() {
  SqlEditorResult *result = active_result_panel();
  if (result)
    result->apply_changes();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::revert_clicked() {
  SqlEditorResult *result = active_result_panel();
  if (result)
    result->discard_changes();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::resultset_edited() {
  SqlEditorResult *result = active_result_panel();
  Recordset::Ref rset;
  if (result && (rset = result->recordset())) {
    bool edited = rset->has_pending_changes();
    _tab_action_apply.set_enabled(edited);
    _tab_action_revert.set_enabled(edited);

    _form->get_menubar()->set_item_enabled("query.save_edits", edited);
    _form->get_menubar()->set_item_enabled("query.discard_edits", edited);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::splitter_resized() {
  if (_lower_tabview.page_count() > 0) {
    bec::GRTManager::get()->set_app_option("DbSqlEditor:ResultSplitterPosition",
                                           grt::IntegerRef(_splitter.get_divider_position()));
  }
}

//--------------------------------------------------------------------------------------------------

static bool check_if_file_too_big_to_restore(const std::string &path, const std::string &file_caption,
                                             bool allow_save_as = false) {
  std::int64_t length;
  if ((length = get_file_size(path.c_str())) > MAX_FILE_SIZE_FOR_AUTO_RESTORE) {
  again:
    std::string size = sizefmt(length, false);
    int rc = mforms::Utilities::show_warning(
      "Restore Workspace", strfmt("The file %s has a size of %s. Are you sure you want to restore this file?",
                                  file_caption.c_str(), size.c_str()),
      "Restore", "Skip", allow_save_as ? "Save As..." : "");
    if (rc == mforms::ResultCancel)
      return false;

    if (rc == mforms::ResultOther) {
      mforms::FileChooser fchooser(mforms::SaveFile);

      fchooser.set_title(_("Save File As..."));
      if (fchooser.run_modal()) {
        if (!base::copyFile(path.c_str(), fchooser.get_path().c_str())) {
          if (mforms::Utilities::show_error("Save File",
                                            strfmt("File %s could not be saved.", fchooser.get_path().c_str()),
                                            _("Retry"), _("Cancel"), "") == mforms::ResultOk)
            goto again;
        }
      } else
        goto again;
      return false;
    }
  }
  return true;
}

#define EDITOR_TEXT_LIMIT 100 * 1024 * 1024

SqlEditorPanel::AutoSaveInfo::AutoSaveInfo(const std::string &info_file) : word_wrap(false), show_special(false) {
  wchar_t buffer[4098] = {0};
  std::wifstream f = openTextInputStream(info_file);
  while (f.getline(buffer, sizeof(buffer))) {
    std::string key, value;
    base::partition(base::wstring_to_string(buffer), "=", key, value);
    if (key == "orig_encoding")
      orig_encoding = value;
    else if (key == "type")
      type = value;
    else if (key == "filename")
      filename = value;
    else if (key == "title")
      title = value;
    else if (key == "word_wrap")
      word_wrap = value == "1";
    else if (key == "show_special")
      show_special = value == "1";
    else if (key == "first_visible_line")
      first_visible_line = base::atoi<int>(value, 0);
    else if (key == "caret_pos")
      caret_pos = base::atoi<int>(value, 0);
  }
}

SqlEditorPanel::AutoSaveInfo SqlEditorPanel::AutoSaveInfo::old_scratch(const std::string &scratch_file) {
  AutoSaveInfo info;
  info.title = base::strip_extension(base::basename(scratch_file));
  if (base::is_number(info.title))
    info.title = base::strfmt("Query %i", 1 + base::atoi<int>(info.title, 0));
  info.type = "scratch";
  return info;
}

SqlEditorPanel::AutoSaveInfo SqlEditorPanel::AutoSaveInfo::old_autosave(const std::string &autosave_file) {
  char buffer[4098];

  AutoSaveInfo info;
  info.title = base::strip_extension(base::basename(autosave_file));
  info.type = "file";
  std::ifstream f(base::strip_extension(autosave_file).c_str());
  if (f.getline(buffer, sizeof(buffer)))
    info.filename = buffer;
  if (f.getline(buffer, sizeof(buffer)))
    info.orig_encoding = buffer;
  return info;
}

bool SqlEditorPanel::load_autosave(const AutoSaveInfo &info, const std::string &text_file) {
  _orig_encoding = info.orig_encoding;
  _file_timestamp = 0;
  _is_scratch = (info.type == "scratch");

  // there's no autosave
  if (text_file.empty() || !base::file_exists(text_file)) {
    if (!info.filename.empty() &&
        !check_if_file_too_big_to_restore(info.filename, strfmt("Saved editor '%s'", info.title.c_str()))) {
      return false;
    }

    // if this was a file, try to load it
    if (!info.filename.empty() && load_from(info.filename, info.orig_encoding, false) != Loaded)
      return false;
  } else {
    // check if autosave too big
    if (!check_if_file_too_big_to_restore(text_file, strfmt("Saved editor '%s'", info.title.c_str()))) {
      return false;
    }

    // load the autosave
    if (load_from(text_file, info.orig_encoding, true) != Loaded)
      return false;
  }
  _filename = info.filename;
  if (!_filename.empty())
    base::file_mtime(_filename, _file_timestamp);

  set_title(info.title);

  mforms::ToolBarItem *item = get_toolbar()->find_item("query.toggleInvisible");
  item->set_checked(info.show_special);
  (*item->signal_activated())(item);

  item = get_toolbar()->find_item("query.toggleWordWrap");
  item->set_checked(info.word_wrap);
  (*item->signal_activated())(item);

  _editor->get_editor_control()->set_caret_pos(info.caret_pos);
  _editor->get_editor_control()->send_editor(SCI_SETFIRSTVISIBLELINE, info.first_visible_line, 0);

  return true;
}

//--------------------------------------------------------------------------------------------------

SqlEditorPanel::LoadResult SqlEditorPanel::load_from(const std::string &file, const std::string &encoding,
                                                     bool keep_dirty) {
  GError *error = NULL;
  gchar *data;
  gsize length;
  gsize file_size = base_get_file_size(file.c_str());

  if (file_size > EDITOR_TEXT_LIMIT) {
    // File is larger than 100 MB. Tell the user we are going to switch off code folding and
    // auto completion.
    int result = mforms::Utilities::show_warning(
      _("Large File"), strfmt(_("The file \"%s\" has a size "
                                "of %.2f MB. Are you sure you want to open this large file?\n\nNote: code folding "
                                "will be disabled for this file.\n\nClick Run SQL Script... to just execute the file."),
                              file.c_str(), file_size / 1024.0 / 1024.0),
      _("Open"), _("Cancel"), _("Run SQL Script..."));
    if (result == mforms::ResultCancel)
      return Cancelled;
    else if (result == mforms::ResultOther)
      return RunInstead;
  }

  _orig_encoding = encoding;

  if (!g_file_get_contents(file.c_str(), &data, &length, &error)) {
    logError("Could not read file %s: %s\n", file.c_str(), error->message);
    std::string what = error->message;
    g_error_free(error);
    throw std::runtime_error(what);
  }

  char *utf8_data;
  std::string original_encoding;
  FileCharsetDialog::Result result =
    FileCharsetDialog::ensure_filedata_utf8(data, length, encoding, file, utf8_data, &original_encoding);
  if (result == FileCharsetDialog::Cancelled) {
    g_free(data);
    return Cancelled;
  } else if (result == FileCharsetDialog::RunInstead) {
    g_free(data);
    return RunInstead;
  }

  // if original data was in utf8, utf8_data comes back NULL
  if (!utf8_data)
    utf8_data = data;
  else
    g_free(data);

  _editor->set_refresh_enabled(true);
  _editor->sql(utf8_data ? utf8_data : "");

  g_free(utf8_data);

  if (!keep_dirty) {
    _editor->get_editor_control()->reset_dirty();

    _filename = file;
    _orig_encoding = original_encoding;

    set_title(strip_extension(basename(file)));
  }

  if (!file_mtime(file, _file_timestamp)) {
    logWarning("Can't get timestamp for %s\n", file.c_str());
    _file_timestamp = 0;
  }
  return Loaded;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::close() {
  _form->remove_sql_editor(this);
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::save_as(const std::string &path) {
  if (path.empty()) {
    mforms::FileChooser dlg(mforms::SaveFile);

    dlg.set_title(_("Save SQL Script"));
    dlg.set_extensions("SQL Files (*.sql)|*.sql", "sql");
    if (!_filename.empty())
      dlg.set_path(_filename);
    if (!dlg.run_modal())
      return false;
    _filename = dlg.get_path();
  }

  if (save()) {
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

bool SqlEditorPanel::save() {
  if (_filename.empty())
    return save_as("");

  GError *error = NULL;

  // File extension check is already done in FileChooser.

  bec::GRTManager::get()->replace_status_text(strfmt(_("Saving SQL script to '%s'..."), _filename.c_str()));

  std::pair<const char *, size_t> text = text_data();
  if (!g_file_set_contents(_filename.c_str(), text.first, text.second, &error)) {
    logError("Could not save script %s: %s\n", _filename.c_str(), error->message);
    bec::GRTManager::get()->replace_status_text(strfmt(_("Error saving SQL script to '%s'."), _filename.c_str()));

    mforms::Utilities::show_error(strfmt(_("Error writing file %s"), _filename.c_str()), error->message, _("OK"));
    g_error_free(error);
    return false;
  }

  // reset dirty marker, but not the undo stack
  _editor->get_editor_control()->reset_dirty();
  _is_scratch = false; // saving a file makes it not a scratch buffer anymore
  file_mtime(_filename, _file_timestamp);

  bec::GRTManager::get()->replace_status_text(strfmt(_("SQL script saved to '%s'"), _filename.c_str()));

  // update autosave state
  _form->auto_save();

  update_title();

  return true;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::revert_to_saved() {
  _editor->sql("");
  if (load_from(_filename, _orig_encoding) == Loaded) {
    {
      NotificationInfo info;
      info["opener"] = "SqlEditorForm";
      info["path"] = _filename;
      NotificationCenter::get()->send("GNDocumentOpened", this, info);
    }
    _form->auto_save();
    bec::GRTManager::get()->replace_status_text(strfmt(_("Reverted to saved '%s'"), _filename.c_str()));
  }
}

//--------------------------------------------------------------------------------------------------

std::string SqlEditorPanel::autosave_file_suffix() {
  return _autosave_file_suffix;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::auto_save(const std::string &path) {
  // save info about the file
  {
    std::wofstream f = openTextOutputStream(base::makePath(path, _autosave_file_suffix + ".info"));
    std::string content;
    if (_is_scratch)
      content += "type=scratch\n";
    else
      content += "type=file\n";

    if (!_is_scratch && !_filename.empty()) {
      content += "filename=" + _filename + "\n";
    }
    content += "orig_encoding=" + _orig_encoding + "\n";

    content += "title=" + _title + "\n";

    if (get_toolbar()->get_item_checked("query.toggleInvisible"))
      content += "show_special=1\n";
    else
      content += "show_special=0\n";
    if (get_toolbar()->get_item_checked("query.toggleWordWrap"))
      content += "word_wrap=1\n";
    else
      content += "word_wrap=0\n";

    size_t caret_pos = _editor->get_editor_control()->get_caret_pos();
    content += "caret_pos=" + std::to_string(caret_pos) + "\n";

    size_t first_line = _editor->get_editor_control()->send_editor(SCI_GETFIRSTVISIBLELINE, 0, 0);
    content += "first_visible_line=" + std::to_string(first_line) + "\n";

    if (f.good())
      f << base::string_to_wstring(content);
    f.close();
  }

  std::string fn = base::makePath(path, _autosave_file_suffix + ".scratch");

  // only save editor contents for scratch areas and unsaved editors
  if (_is_scratch || _filename.empty() || (!_filename.empty() && is_dirty())) {
    // We don't need to lock the editor as we are in the main thread here
    // and directly set the file content without detouring to anything that could change the text.
    GError *error = 0;

    std::pair<const char *, size_t> text = text_data();
    if (!g_file_set_contents(fn.c_str(), text.first, text.second, &error)) {
      logError("Could not save snapshot of editor contents to %s: %s\n", fn.c_str(), error->message);
      std::string msg(strfmt("Could not save snapshot of editor contents to %s: %s", fn.c_str(), error->message));
      g_error_free(error);
      throw std::runtime_error(msg);
    }
  } else {
    // delete the autosave file if the file was saved
    try {
      base::remove(fn);
    } catch (std::exception &e) {
      logWarning("Error deleting autosave file %s: %s\n", fn.c_str(), e.what());
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::delete_auto_save(const std::string &path) {
  // delete the autosave related files
  try {
    base::remove(base::makePath(path, _autosave_file_suffix + ".autosave"));
  } catch (std::exception &exc) {
    logWarning("Could not delete auto-save file: %s\n", exc.what());
  }
  try {
    base::remove(base::makePath(path, _autosave_file_suffix + ".info"));
  } catch (std::exception &exc) {
    logWarning("Could not delete auto-save file: %s\n", exc.what());
  }
}

//--------------------------------------------------------------------------------------------------

// Toolbar handling.

void SqlEditorPanel::show_find_panel(mforms::CodeEditor *editor, bool show) {
  mforms::FindPanel *panel = editor->get_find_panel();
  if (show && !panel->get_parent())
    _editor_box.add(panel, false, true);
  panel->show(show);
}

//--------------------------------------------------------------------------------------------------

static void toggle_continue_on_error(SqlEditorForm *sql_editor_form) {
  sql_editor_form->continue_on_error(!sql_editor_form->continue_on_error());
}

//--------------------------------------------------------------------------------------------------

mforms::ToolBar *SqlEditorPanel::setup_editor_toolbar() {
  mforms::ToolBar *tbar(mforms::manage(new mforms::ToolBar(mforms::OptionsToolBar)));
  tbar->set_name("Editor Toolbar");
#ifdef _MSC_VER
  tbar->set_size(-1, 27);
#endif
  mforms::ToolBarItem *item;

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Open File");
  item->setInternalName("query.openFile");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_open.png"));
  item->set_tooltip(_("Open a script file in this editor"));
  bec::UIForm::scoped_connect(
    item->signal_activated(),
    std::bind((void (SqlEditorForm::*)(const std::string &, bool, bool)) & SqlEditorForm::open_file, _form, "", false,
              true));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Save File");
  item->setInternalName("query.saveFile");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_save.png"));
  item->set_tooltip(_("Save the script to a file."));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorPanel::save, this));
  tbar->add_item(item);

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Execute");
  item->setInternalName("query.execute");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_execute.png"));
  item->set_tooltip(_("Execute the selected portion of the script or everything, if there is no selection"));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::run_editor_contents, _form, false));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Execute Current Statement");
  item->setInternalName("query.execute_current_statement");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_execute-current.png"));
  item->set_tooltip(_("Execute the statement under the keyboard cursor"));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::run_editor_contents, _form, true));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Explain Current Statement");
  item->setInternalName("query.explain_current_statement");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_explain.png"));
  item->set_tooltip(_("Execute the EXPLAIN command on the statement under the cursor"));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::explain_current_statement, _form));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Cancel");
  item->setInternalName("query.cancel");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop.png"));
  item->set_tooltip(
    _("Stop the query being executed (the connection to the DB server will not be restarted and any open transactions "
      "will remain open)"));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::cancel_query, _form));
  tbar->add_item(item);

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("Continue On Error");
  item->setInternalName("query.continueOnError");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop-on-error-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop-on-error-off.png"));
  item->set_tooltip(_("Toggle whether execution of SQL script should continue after failed statements"));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(toggle_continue_on_error, _form));

  tbar->add_item(item);

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Commit");
  item->setInternalName("query.commit");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_commit.png"));
  item->set_tooltip(
    _("Commit the current transaction.\nNOTE: all query tabs in the same connection share the same transaction. To "
      "have independent transactions, you must open a new connection."));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::commit, _form));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Rollback");
  item->setInternalName("query.rollback");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_rollback.png"));
  item->set_tooltip(
    _("Rollback the current transaction.\nNOTE: all query tabs in the same connection share the same transaction. To "
      "have independent transactions, you must open a new connection."));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::rollback, _form));
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("Auto Commit");
  item->setInternalName("query.autocommit");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_autocommit-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_autocommit-off.png"));
  item->set_tooltip(_(
    "Toggle autocommit mode. When enabled, each statement will be committed immediately.\nNOTE: all query tabs in the "
    "same connection share the same transaction. To have independent transactions, you must open a new connection."));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::toggle_autocommit, _form));
  tbar->add_item(item);

  tbar->add_separator_item();

  item = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
  item->set_name("Limit Rows");
  item->setInternalName("limit_rows");
  item->set_tooltip(
    _("Set limit for number of rows returned by queries.\nWorkbench will automatically add the LIMIT clause with the "
      "configured number of rows to SELECT queries."));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorPanel::limit_rows, this, item));
  tbar->add_item(item);

  tbar->add_separator_item();

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Add Snippet");
  item->setInternalName("add_snippet");
  item->set_icon(IconManager::get_instance()->get_icon_path("snippet_add.png"));
  item->set_tooltip(_("Save current statement or selection to the snippet list."));
  bec::UIForm::scoped_connect(item->signal_activated(), std::bind(&SqlEditorForm::save_snippet, _form));
  tbar->add_item(item);

  tbar->add_separator_item();

  // adds generic SQL editor toolbar buttons
  _editor->set_base_toolbar(tbar);

  update_limit_rows();

  return tbar;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::limit_rows(mforms::ToolBarItem *item) {
  _form->limit_rows(item->get_text());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::update_limit_rows() {
  mforms::MenuItem *mitem = _form->get_menubar()->find_item("limit_rows");
  std::string selected;
  std::vector<std::string> items;
  for (int i = 0; i < mitem->item_count(); i++) {
    if (!mitem->get_item(i)->get_title().empty()) {
      items.push_back(mitem->get_item(i)->get_title());
      if (mitem->get_item(i)->get_checked())
        selected = items.back();
    }
  }

  mforms::ToolBarItem *item = get_toolbar()->find_item("limit_rows");
  item->set_selector_items(items);
  item->set_text(selected);
}

//--------------------------------------------------------------------------------------------------

mforms::ToolBar *SqlEditorPanel::get_toolbar() {
  return _editor->get_toolbar();
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::is_dirty() const {
  return _editor->get_editor_control()->is_dirty();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::check_external_file_changes() {
  time_t ts;
  if (!_filename.empty() && file_mtime(_filename, ts)) {
    if (ts > _file_timestamp) {
      // File was changed externally. For now we ignore local changes if the user chooses to reload.
      std::string connection_description =
        _form->connection_descriptor().is_valid()
          ? strfmt("(from connection to %s) ", _form->connection_descriptor()->name().c_str())
          : "";
      if (mforms::Utilities::show_warning("File Changed",
                                          strfmt(_("File %s %swas changed from outside MySQL Workbench.\nWould you "
                                                   "like to discard your changes and reload it?"),
                                                 _filename.c_str(), connection_description.c_str()),
                                          "Reload File", "Ignore", "") == mforms::ResultOk) {
        revert_to_saved();
      } else {
        _file_timestamp = ts;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::pair<const char *, std::size_t> SqlEditorPanel::text_data() const {
  return _editor->text_ptr();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::set_title(const std::string &title) {
  _title = title;
  grtobj()->name(_title);
  mforms::AppView::set_title(title);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::set_filename(const std::string &filename) {
  _filename = filename;
  if (!filename.empty())
    set_title(strip_extension(basename(filename)));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::update_title() {
  if (!_is_scratch)
    mforms::AppView::set_title(_title + (is_dirty() ? "*" : ""));
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts the auto completion list in the currently active editor. The content of this list is
 * determined from various sources + the current query context.
 */
void SqlEditorPanel::list_members() {
  if (owner()->work_parser_context() != NULL)
    editor_be()->show_auto_completion(false);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::jump_to_placeholder() {
  _editor->get_editor_control()->jump_to_next_placeholder();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::query_started(bool retain_old_recordsets) {
  _busy = true;

  logDebug("Preparing UI for query run\n");

  _form->set_busy_tab(_form->sql_editor_panel_index(this));

  // disable tabview reordering because we can get new tabs added at odd times if a query is running
  _lower_tabview.set_allows_reordering(false);

  // if we're already running the query, it's obvious there's no more need to autocomplete what we typed
  _editor->cancel_auto_completion();

  if (!retain_old_recordsets) {
    logDebug("Releasing old recordset(s) (if possible)\n");

    // close recordsets that were opened previously (unless they're pinned or something)
    for (int i = _lower_tabview.page_count() - 1; i >= 0; --i) {
      SqlEditorResult *result = dynamic_cast<SqlEditorResult *>(_lower_tabview.get_page(i));
      if (result) {
        if (result->pinned())
          continue;

        if (result->has_pending_changes())
          continue;

        // make sure that the result is docked here
        int i = _lower_tabview.get_page_index(result);
        if (i >= 0) {
          result->close();
          result_removed();
        }
      }
    }
  } else {
    logDebug("Retaining old recordset(s)\n");
  }

  _was_empty = (_lower_tabview.page_count() == 0);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::query_finished() {
  logDebug2("Query successfully finished in editor %s\n", get_title().c_str());

  _busy = false;

  _form->set_busy_tab(-1);
  _lower_tabview.set_allows_reordering(true);

  _form->post_query_slot();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::query_failed(const std::string &message) {
  logError("Query execution failed in editor: %s. Error during query: %s\n", get_title().c_str(), message.c_str());
  _busy = false;

  _form->set_busy_tab(-1);
  _lower_tabview.set_allows_reordering(true);

  _form->post_query_slot();
}

//--------------------------------------------------------------------------------------------------

// Resultset management.

SqlEditorResult *SqlEditorPanel::active_result_panel() {
  return result_panel(_lower_tabview.get_active_tab());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::lower_tab_switched() {
  _lower_dock.view_switched();

  db_query_QueryEditorRef qeditor(grtobj());
  SqlEditorResult *result = active_result_panel();
  Recordset::Ref rset;
  if (result && (rset = result->recordset())) {
    bool found = false;
    for (size_t c = qeditor->resultPanels().count(), i = 0; i < c; i++) {
      if (mforms_from_grt(qeditor->resultPanels()[i]->dockingPoint()) == result->dock()) {
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
    if (readonly) {
      _tab_action_info.set_tooltip(rset->readonly_reason());
      _tab_action_icon.set_tooltip(rset->readonly_reason());
    }
  } else {
    qeditor->activeResultPanel(db_query_ResultPanelRef());

    _tab_action_apply.show(true);
    _tab_action_revert.show(true);
    _tab_action_icon.show(false);
    _tab_action_info.show(false);

    _tab_action_apply.set_enabled(false);
    _tab_action_revert.set_enabled(false);
  }

#ifdef _MSC_VER
  _editor->focus();
#endif

  mforms::MenuBar *menu;
  if ((menu = _form->get_menubar())) {
    Recordset::Ref rset(result ? result->recordset() : Recordset::Ref());

    menu->set_item_enabled("query.save_edits", rset && rset->has_pending_changes());
    menu->set_item_enabled("query.discard_edits", rset && rset->has_pending_changes());
    menu->set_item_enabled("query.export", (bool)rset);
  }

  // if a lower tab view selection has changed, we make sure it's visible
  if (!_busy && _lower_tabview.page_count() > 0) // if we're running a query, then let dock_result handle this
  {
    int position = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ResultSplitterPosition", 200);
    if (position > _splitter.get_height() - 100)
      position = _splitter.get_height() - 100;
    _splitter.set_divider_position(position);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::on_recordset_context_menu_show(Recordset::Ptr rs_ptr) {
  Recordset::Ref rs(rs_ptr.lock());
  if (rs) {
    grt::DictRef info(true);

    std::vector<int> selection(rs->selected_rows());
    grt::IntegerListRef rows(grt::Initialized);
    for (std::vector<int>::const_iterator i = selection.begin(); i != selection.end(); ++i)
      rows.insert(*i);

    info.set("selected-rows", rows);
    info.gset("selected-column", rs->selected_column());
    info.set("menu", mforms_to_grt(rs->get_context_menu()));

    db_query_QueryBufferRef qbuffer(grtobj());
    if (qbuffer.is_valid() && db_query_QueryEditorRef::can_wrap(qbuffer)) {
      db_query_QueryEditorRef qeditor(db_query_QueryEditorRef::cast_from(qbuffer));
      for (size_t c = qeditor->resultPanels().count(), i = 0; i < c; i++) {
        db_query_ResultsetRef rset(qeditor->resultPanels()[i]->resultset());

        if (rset.is_valid() && dynamic_cast<WBRecordsetResultset *>(rset->get_data())->recordset == rs) {
          grt::GRTNotificationCenter::get()->send_grt("GRNSQLResultsetMenuWillShow", rset, info);
          break;
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 *	Returns the number of all docked result panels in the editor panel.
 */
size_t SqlEditorPanel::result_panel_count() {
  return _lower_tabview.page_count();
}

//--------------------------------------------------------------------------------------------------

/**
*	Returns the number of all docked resultset panels in the editor panel.
*	That excludes all the explain, spatial etc. panels.
*/
size_t SqlEditorPanel::resultset_count() {
  return grtobj()->resultPanels().count();
}

//--------------------------------------------------------------------------------------------------

SqlEditorResult *SqlEditorPanel::result_panel(int i) {
  if (i >= 0 && i < _lower_tabview.page_count())
    return dynamic_cast<SqlEditorResult *>(_lower_tabview.get_page(i));
  return NULL;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::add_panel_for_recordset_from_main(Recordset::Ref rset) {
  if (bec::GRTManager::get()->in_main_thread()) {
    SqlEditorForm::RecordsetData *rdata = dynamic_cast<SqlEditorForm::RecordsetData *>(rset->client_data());

    rdata->result_panel = add_panel_for_recordset(rset);
  } else
    bec::GRTManager::get()->run_once_when_idle(
      dynamic_cast<bec::UIForm *>(this), std::bind(&SqlEditorPanel::add_panel_for_recordset_from_main, this, rset));
}

//--------------------------------------------------------------------------------------------------

SqlEditorResult *SqlEditorPanel::add_panel_for_recordset(Recordset::Ref rset) {
  SqlEditorResult *result = mforms::manage(new SqlEditorResult(this));
  if (rset)
    result->set_recordset(rset);
  dock_result_panel(result);

  return result;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::dock_result_panel(SqlEditorResult *result) {
  result->grtobj()->owner(grtobj());
  grtobj()->resultPanels().insert(result->grtobj());

  if (Recordset::Ref rset = result->recordset())
    result->set_title(rset->caption());

  _lower_dock.dock_view(result);
  _lower_dock.select_view(result);
  _splitter.set_expanded(false, true);
  if (_was_empty) {
    int position = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ResultSplitterPosition", 200);
    if (position > _splitter.get_height() - 100)
      position = _splitter.get_height() - 100;
    _splitter.set_divider_position(position);

    // scroll the editor to make the cursor visible and keep the selection, if available
    std::size_t selection_start = 0;
    std::size_t selection_length = 0;
    _editor->get_editor_control()->get_selection(selection_start, selection_length);
    _editor->get_editor_control()->set_caret_pos(_editor->get_editor_control()->get_caret_pos());
    _editor->get_editor_control()->set_selection(selection_start, selection_length);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::lower_tab_reordered(mforms::View *view, int from, int to) {
  if (from == to || dynamic_cast<SqlEditorResult *>(view) == NULL)
    return;

  // not all tabs will have a SqlEditorResult
  // so the reordering gets more complicated, because actual reordering only happens if the relative
  // position between the result objects changes...
  // relative result object order changes always mean that a tab was reordered, but the other way around is
  // not always true

  size_t from_index = grtobj()->resultPanels().get_index(dynamic_cast<SqlEditorResult *>(view)->grtobj());
  if (from_index == grt::BaseListRef::npos) {
    logFatal("Result panel is not in resultPanels() list\n");
    return;
  }

  // first build an array of result panel objects, in the same order as the tabview
  std::vector<std::pair<db_query_ResultPanelRef, int> > panels;
  for (int result_order = 0, i = 0; i < _lower_tabview.page_count(); i++) {
    SqlEditorResult *p = result_panel(i);
    if (p)
      panels.push_back(std::make_pair(p->grtobj(), result_order++));
    else
      panels.push_back(std::make_pair(db_query_ResultPanelRef(), 0));
  }

  int to_index = -1;
  // now find out where we have to move to
  if (from < to) {
    for (int i = to; i > from; i--) {
      if (panels[i].first.is_valid()) {
        to_index = panels[i].second;
        break;
      }
    }
  } else {
    for (int i = to; i < from; i++) {
      if (panels[i].first.is_valid()) {
        to_index = panels[i].second;
        break;
      }
    }
  }

  if (to_index < 0) {
    to_index = panels.back().second;
  }

  grtobj()->resultPanels()->reorder(from_index, to_index);
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::lower_tab_closing(int tab) {
  mforms::AppView *view = _lower_dock.view_at_index(tab);
  if (view) {
    if (view->on_close()) {
      view->close();
      result_removed();
      return true;
    }
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::lower_tab_closed(mforms::View *page, int tab) {
  SqlEditorResult *rpage = dynamic_cast<SqlEditorResult *>(page);
  if (rpage) {
    db_query_ResultPanelRef closed_panel(rpage->grtobj());
    grtobj()->resultPanels().remove_value(closed_panel);
    if (closed_panel->resultset().is_valid())
      closed_panel->resultset()->reset_references();
    closed_panel->reset_references();
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::result_removed() {
  if (_lower_tabview.page_count() == 0)
    _splitter.set_expanded(false, false);
  lower_tab_switched();
}

//--------------------------------------------------------------------------------------------------

std::list<SqlEditorResult *> SqlEditorPanel::dirty_result_panels() {
  std::list<SqlEditorResult *> results;

  for (int c = _lower_tabview.page_count(), i = 0; i < c; i++) {
    SqlEditorResult *result = result_panel(i);
    if (result && result->has_pending_changes())
      results.push_back(result);
  }
  return results;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::tab_menu_will_show() {
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

void SqlEditorPanel::rename_tab_clicked() {
  int tab = _lower_tabview.get_menu_tab();
  SqlEditorResult *result = result_panel(tab);
  if (result) {
    std::string title;
    if (mforms::Utilities::request_input(_("Rename Result Tab"), "Enter a new name for the result tab:",
                                         result->caption().c_str(), title))
      _lower_tabview.set_tab_title(tab, title);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::pin_tab_clicked() {
  int tab = _lower_tabview.get_menu_tab();
  SqlEditorResult *result = result_panel(tab);
  if (result)
    result->set_pinned(!result->pinned());
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorPanel::is_pinned(int tab) {
  SqlEditorResult *result = result_panel(tab);
  if (result)
    return result->pinned();
  return false;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::tab_pinned(int tab, bool flag) {
  SqlEditorResult *result = result_panel(tab);
  if (result)
    result->set_pinned(flag);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::close_tab_clicked() {
  lower_tab_closing(_lower_tabview.get_menu_tab());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorPanel::close_other_tabs_clicked() {
  int tab = _lower_tabview.get_menu_tab();
  for (int i = _lower_tabview.page_count() - 1; i >= 0; --i) {
    if (i != tab)
      lower_tab_closing(i);
  }
}

//--------------------------------------------------------------------------------------------------
