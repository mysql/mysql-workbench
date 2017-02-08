/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/boost_smart_ptr_helpers.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include "base/threaded_timer.h"
#include "base/util_functions.h"

#include "grt/grt_manager.h"
#include "grt/grt_threaded_task.h"

#include "objimpl/db.query/db_query_QueryBuffer.h"

#include "grtui/file_charset_dialog.h"

#include "mforms/code_editor.h"
#include "mforms/find_panel.h"
#include "mforms/toolbar.h"
#include "mforms/menu.h"
#include "mforms/filechooser.h"

#include "grts/structs.db.mysql.h"

#include "mysql-scanner.h"
#include "code-completion/mysql-code-completion.h"
#include "sql_editor_be.h"

DEFAULT_LOG_DOMAIN("MySQL editor");

using namespace bec;
using namespace grt;
using namespace base;

using namespace parser;

//--------------------------------------------------------------------------------------------------

class MySQLEditor::Private {
public:
  // ref to the GRT object representing this object
  // it will be used in Db_sql_editor queryBuffer list and in standalone
  // editors for plugin support
  db_query_QueryBufferRef _grtobj;
  mforms::Box *_container;
  mforms::Menu *_editor_context_menu;
  mforms::Menu *_editor_text_submenu;

  mforms::ToolBar *_toolbar;

  int _last_typed_char;

  MySQLParserContext::Ref _parser_context;
  MySQLParserContext::Ref _autocompletion_context;
  MySQLParserServices::Ref _services;

  double _last_sql_check_progress_msg_timestamp;
  double _sql_check_progress_msg_throttle;

  base::RecMutex _sql_checker_mutex;
  MySQLParseUnit _parse_unit; // The type of query we want to limit our parsing to.

  // We use 2 timers here for delayed work. One is a grt timer to run a task in the main thread after a certain delay.
  // The other one is to run the actual work task in a background thread.
  bec::GRTManager::Timer *_current_delay_timer;
  int _current_work_timer_id;

  std::pair<const char *, size_t> _text_info; // Only valid during a parse run.

  std::vector<ParserErrorEntry> _recognition_errors; // List of errors from the last sql check run.
  std::set<size_t> _error_marker_lines;

  bool _splitting_required;
  bool _updating_statement_markers;
  std::set<size_t> _statement_marker_lines;
  base::RecMutex _sql_statement_borders_mutex;

  // Each entry is a pair of statement position (byte position) and statement length (also bytes).
  std::vector<std::pair<size_t, size_t>> _statement_ranges;

  bool _is_refresh_enabled;   // whether FE control is permitted to replace its contents from BE
  bool _is_sql_check_enabled; // Enables automatic syntax checks.
  bool _stop_processing;      // To stop ongoing syntax checks (because of text changes etc.).
  bool _owns_toolbar;

  boost::signals2::signal<void()> _text_change_signal;

  // autocomplete_context will go after auto completion refactoring.
  Private(MySQLParserContext::Ref syntaxcheck_context, MySQLParserContext::Ref autocomplete_context)
    : _grtobj(grt::Initialized), _last_sql_check_progress_msg_timestamp(0.0), _stop_processing(false) {
    _owns_toolbar = false;
    _parse_unit = MySQLParseUnit::PuGeneric;
    _is_refresh_enabled = true;
    _sql_check_progress_msg_throttle = 500;

    _splitting_required = false;

    _parser_context = syntaxcheck_context;
    _autocompletion_context = autocomplete_context;
    _services = MySQLParserServices::get();

    _current_delay_timer = NULL;
    _current_work_timer_id = -1;

    _is_sql_check_enabled = true;
    _container = NULL;
    _editor_text_submenu = NULL;
    _editor_context_menu = NULL;
    _toolbar = NULL;
    _last_typed_char = 0;
    _updating_statement_markers = false;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Determines ranges for all statements in the current text.
   */
  void split_statements_if_required() {
    // If we have restricted content (e.g. for object editors) then we don't split and handle the entire content
    // as a single statement. This will then show syntax errors for any invalid additional input.
    if (_splitting_required) {
      logDebug3("Start splitting\n");
      _splitting_required = false;

      base::RecMutexLock lock(_sql_statement_borders_mutex);

      _statement_ranges.clear();
      if (_parse_unit == MySQLParseUnit::PuGeneric) {
        double start = timestamp();
        determineStatementRanges(_text_info.first, _text_info.second, ";", _statement_ranges);
        logDebug3("Splitting ended after %f ticks\n", timestamp() - start);
      } else
        _statement_ranges.push_back({0, _text_info.second});
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
   * One or more markers moved to other lines or were deleted. We have to stay in sync with our
   * statement markers list to make the optimized add/remove algorithm working.
   */
  void marker_changed(const mforms::LineMarkupChangeset &changeset, bool deleted) {
    if (_updating_statement_markers)
      return;

    if (changeset.empty() && deleted) { // The document is empty now.
      _statement_marker_lines.clear();
      _error_marker_lines.clear();
      return;
    }

    for (auto &change : changeset) {
      if ((change.markup & mforms::LineMarkupStatement) != 0)
        _statement_marker_lines.erase(change.original_line);
      if ((change.markup & mforms::LineMarkupError) != 0)
        _error_marker_lines.erase(change.original_line);
    }

    if (!deleted) {
      for (auto &change : changeset) {
        if ((change.markup & mforms::LineMarkupStatement) != 0)
          _statement_marker_lines.insert(change.new_line);
        if ((change.markup & mforms::LineMarkupError) != 0)
          _error_marker_lines.insert(change.new_line);
      }
    }
  }

  //------------------------------------------------------------------------------------------------
};

//--------------------------------------------------------------------------------------------------

MySQLEditor::Ref MySQLEditor::create(MySQLParserContext::Ref syntax_check_context,
                                     MySQLParserContext::Ref autocopmlete_context, db_query_QueryBufferRef grtobj) {
  Ref sql_editor = MySQLEditor::Ref(new MySQLEditor(syntax_check_context, autocopmlete_context));
  // replace the default object with the custom one
  if (grtobj.is_valid())
    sql_editor->set_grtobj(grtobj);

  // setup the GRT object
  db_query_QueryBuffer::ImplData *data = new db_query_QueryBuffer::ImplData(sql_editor->grtobj(), sql_editor);
  sql_editor->grtobj()->set_data(data);

  return sql_editor;
}

//--------------------------------------------------------------------------------------------------

MySQLEditor::MySQLEditor(MySQLParserContext::Ref syntax_check_context, MySQLParserContext::Ref autocopmlete_context) {
  d = new Private(syntax_check_context, autocopmlete_context);

  _code_editor = new mforms::CodeEditor(this);
  _code_editor->set_font(bec::GRTManager::get()->get_app_option_string("workbench.general.Editor:Font"));
  _code_editor->set_features(mforms::FeatureUsePopup, false);
  _code_editor->set_features(mforms::FeatureConvertEolOnPaste | mforms::FeatureAutoIndent, true);
  _code_editor->set_name("Code Editor");

  GrtVersionRef version = syntax_check_context->get_server_version();
  _editor_config = NULL;
  create_editor_config_for_version(version);

  _code_editor->send_editor(SCI_SETTABWIDTH, bec::GRTManager::get()->get_app_option_int("Editor:TabWidth", 4), 0);
  _code_editor->send_editor(SCI_SETINDENT, bec::GRTManager::get()->get_app_option_int("Editor:IndentWidth", 4), 0);
  _code_editor->send_editor(SCI_SETUSETABS, !bec::GRTManager::get()->get_app_option_int("Editor:TabIndentSpaces", 0),
                            0);

  scoped_connect(_code_editor->signal_changed(),
                 std::bind(&MySQLEditor::text_changed, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, std::placeholders::_4));
  scoped_connect(_code_editor->signal_char_added(), std::bind(&MySQLEditor::char_added, this, std::placeholders::_1));
  scoped_connect(_code_editor->signal_dwell(),
                 std::bind(&MySQLEditor::dwell_event, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, std::placeholders::_4));
  scoped_connect(_code_editor->signal_marker_changed(),
                 std::bind(&MySQLEditor::Private::marker_changed, d, std::placeholders::_1, std::placeholders::_2));

  setup_auto_completion();

  _auto_completion_cache = NULL;
  _continueOnError = false;

  setup_editor_menu();
}

//--------------------------------------------------------------------------------------------------

MySQLEditor::~MySQLEditor() {
  stop_processing();

  {
    d->_is_sql_check_enabled = false;

    // We lock all mutexes for a moment here to ensure no background thread is still holding them.
    base::RecMutexLock lock1(d->_sql_checker_mutex);
    base::RecMutexLock lock2(d->_sql_statement_borders_mutex);
  }

  if (d->_editor_text_submenu != NULL)
    delete d->_editor_text_submenu;
  delete d->_editor_context_menu;
  if (d->_owns_toolbar && d->_toolbar != NULL)
    d->_toolbar->release();

  delete _editor_config;
  delete _code_editor;

  delete d;
}

//--------------------------------------------------------------------------------------------------

db_query_QueryBufferRef MySQLEditor::grtobj() {
  return d->_grtobj;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_grtobj(db_query_QueryBufferRef grtobj) {
  d->_grtobj = grtobj;
}

//--------------------------------------------------------------------------------------------------

mforms::CodeEditor *MySQLEditor::get_editor_control() {
  return _code_editor;
};

//--------------------------------------------------------------------------------------------------

static void toggle_show_special_chars(mforms::ToolBarItem *item, MySQLEditor *sql_editor) {
  sql_editor->show_special_chars(item->get_checked());
}

//--------------------------------------------------------------------------------------------------

static void toggle_word_wrap(mforms::ToolBarItem *item, MySQLEditor *sql_editor) {
  sql_editor->enable_word_wrap(item->get_checked());
}

//--------------------------------------------------------------------------------------------------

static void show_find_panel_for_active_editor(MySQLEditor *sql_editor) {
  sql_editor->get_editor_control()->show_find_panel(false);
}

//--------------------------------------------------------------------------------------------------

static void beautify_script(MySQLEditor *sql_editor) {
  grt::BaseListRef args(true);
  args.ginsert(sql_editor->grtobj());

  grt::GRT::get()->call_module_function("SQLIDEUtils", "enbeautificate", args);
}

//--------------------------------------------------------------------------------------------------

static void open_file(MySQLEditor *sql_editor) {
  mforms::FileChooser fc(mforms::OpenFile);
  if (fc.run_modal()) {
    std::string file = fc.get_path();

    gchar *contents;
    gsize length;
    GError *error = NULL;

    if (g_file_get_contents(file.c_str(), &contents, &length, &error)) {
      char *converted;

      mforms::CodeEditor *code_editor = sql_editor->get_editor_control();
      if (FileCharsetDialog::ensure_filedata_utf8(contents, length, "", file, converted)) {
        code_editor->set_text_keeping_state(converted ? converted : contents);
        g_free(contents);
        g_free(converted);
      } else {
        g_free(contents);
        code_editor->set_text(_("Data is not UTF8 encoded and cannot be displayed."));
      }
    } else if (error) {
      mforms::Utilities::show_error("Load File",
                                    base::strfmt("Could not load file %s:\n%s", file.c_str(), error->message), "OK");
      g_error_free(error);
    }
  }
}

//--------------------------------------------------------------------------------------------------

static void save_file(MySQLEditor *sql_editor) {
  mforms::FileChooser fc(mforms::SaveFile);
  fc.set_extensions("SQL Scripts (*.sql)|*.sql", "sql");

  if (fc.run_modal()) {
    GError *error = NULL;
    std::string file = fc.get_path();
    mforms::CodeEditor *code_editor = sql_editor->get_editor_control();
    std::pair<const char *, size_t> data = code_editor->get_text_ptr();

    if (!g_file_set_contents(file.c_str(), data.first, (gssize)data.second, &error) && error) {
      mforms::Utilities::show_error("Save File",
                                    base::strfmt("Could not save to file %s:\n%s", file.c_str(), error->message), "OK");
      g_error_free(error);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_base_toolbar(mforms::ToolBar *toolbar) {
  /* TODO: that is a crude implementation as the toolbar is sometimes directly created and set and *then* also set
           here, so deleting it crashs.
  if (d->_toolbar != NULL && d->_owns_toolbar)
    delete d->_toolbar;
  */
  d->_toolbar = toolbar;
  d->_owns_toolbar = false;

  mforms::ToolBarItem *item;

  if (d->_is_sql_check_enabled) {
    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("query.beautify");
    item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_beautifier.png"));
    item->set_tooltip(_("Beautify/reformat the SQL script"));
    scoped_connect(item->signal_activated(), std::bind(beautify_script, this));
    d->_toolbar->add_item(item);
  }
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.search");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_find.png"));
  item->set_tooltip(_("Show the Find panel for the editor"));
  scoped_connect(item->signal_activated(), std::bind(show_find_panel_for_active_editor, this));
  d->_toolbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.toggleInvisible");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_special-chars-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_special-chars-off.png"));
  item->set_tooltip(_("Toggle display of invisible characters (spaces, tabs, newlines)"));
  scoped_connect(item->signal_activated(), std::bind(toggle_show_special_chars, item, this));
  d->_toolbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.toggleWordWrap");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_word-wrap-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_word-wrap-off.png"));
  item->set_tooltip(_("Toggle wrapping of long lines (keep this off for large files)"));
  scoped_connect(item->signal_activated(), std::bind(toggle_word_wrap, item, this));
  d->_toolbar->add_item(item);
}

//--------------------------------------------------------------------------------------------------

static void embed_find_panel(mforms::CodeEditor *editor, bool show, mforms::Box *container) {
  mforms::View *panel = editor->get_find_panel();
  if (show) {
    if (!panel->get_parent())
      container->add(panel, false, true);
  } else {
    container->remove(panel);
    editor->focus();
  }
}

mforms::View *MySQLEditor::get_container() {
  if (!d->_container) {
    d->_container = new mforms::Box(false);

    d->_container->add(get_toolbar(), false, true);
    get_editor_control()->set_show_find_panel_callback(
      std::bind(embed_find_panel, std::placeholders::_1, std::placeholders::_2, d->_container));
    d->_container->add_end(get_editor_control(), true, true);
  }
  return d->_container;
};

//--------------------------------------------------------------------------------------------------

mforms::ToolBar *MySQLEditor::get_toolbar(bool include_file_actions) {
  if (!d->_toolbar) {
    d->_owns_toolbar = true;
    d->_toolbar = mforms::manage(new mforms::ToolBar(mforms::SecondaryToolBar));
#ifdef _WIN32
    d->_toolbar->set_size(-1, 27);
#endif
    if (include_file_actions) {
      mforms::ToolBarItem *item;

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("query.openFile");
      item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_open.png"));
      item->set_tooltip(_("Open a script file in this editor"));
      scoped_connect(item->signal_activated(), std::bind(open_file, this));
      d->_toolbar->add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("query.saveFile");
      item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_save.png"));
      item->set_tooltip(_("Save the script to a file."));
      scoped_connect(item->signal_activated(), std::bind(save_file, this));
      d->_toolbar->add_item(item);

      d->_toolbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));
    }
    set_base_toolbar(d->_toolbar);
  }
  return d->_toolbar;
};

//--------------------------------------------------------------------------------------------------

mforms::CodeEditorConfig *MySQLEditor::get_editor_settings() {
  return _editor_config;
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::is_refresh_enabled() const {
  return d->_is_refresh_enabled;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_refresh_enabled(bool val) {
  d->_is_refresh_enabled = val;
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::is_sql_check_enabled() const {
  return d->_is_sql_check_enabled;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the text of the editor. Usage of this function is discouraged because it copies the
 * (potentially) large editor content. Use text_ptr() instead.
 */
std::string MySQLEditor::sql() {
  return _code_editor->get_text(false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a direct pointer to the editor content, which is only valid until the next change.
 * So if you want to keep it for longer copy the text.
 * Note: since the text can be large don't do this unless absolutely necessary.
 */
std::pair<const char *, size_t> MySQLEditor::text_ptr() {
  return _code_editor->get_text_ptr();
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_current_schema(const std::string &schema) {
  _current_schema = schema;
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::empty() {
  return _code_editor->text_length() == 0;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::append_text(const std::string &text) {
  _code_editor->append_text(text.data(), text.size());
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Used to the set the content of the editor from outside (e.g. when loading a file or for tests).
 */
void MySQLEditor::sql(const char *sql) {
  _code_editor->set_text(sql);
  d->_splitting_required = true;
  d->_statement_marker_lines.clear();
  _code_editor->set_eol_mode(mforms::EolLF, true);
}

//----------------------------------------------------------------------------------------------------------------------

std::size_t MySQLEditor::cursor_pos() {
  return _code_editor->get_caret_pos();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the caret position as column/row pair. The returned column (char index) is utf-8 save and computes
 * the actual character index as displayed in the editor, not the byte index in a std::string.
 * If @local is true then the line position is relative to the statement, otherwise that in the entire editor.
 */
std::pair<std::size_t, std::size_t> MySQLEditor::cursor_pos_row_column(bool local) {
  size_t position = _code_editor->get_caret_pos();
  ssize_t line = _code_editor->line_from_position(position);
  ssize_t line_start, line_end;
  _code_editor->get_range_of_line(line, line_start, line_end);

  ssize_t offset = position - line_start; // This is a byte offset.
  std::string line_text = _code_editor->get_text_in_range(line_start, line_end);
  offset = g_utf8_pointer_to_offset(line_text.c_str(), line_text.c_str() + offset);

  if (local) {
    size_t min, max;
    if (get_current_statement_range(min, max))
      line -= _code_editor->line_from_position(min);
  }

  return {offset, line};
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_cursor_pos(std::size_t position) {
  _code_editor->set_caret_pos(position);
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::selected_range(std::size_t &start, std::size_t &end) {
  size_t length;
  _code_editor->get_selection(start, length);
  end = start + length;
  return length > 0;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_selected_range(std::size_t start, std::size_t end) {
  _code_editor->set_selection(start, end - start);
}

//--------------------------------------------------------------------------------------------------

boost::signals2::signal<void()> *MySQLEditor::text_change_signal() {
  return &d->_text_change_signal;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_sql_mode(const std::string &value) {
  _sql_mode = value;
  d->_parser_context->use_sql_mode(value);
}

//--------------------------------------------------------------------------------------------------

/**
 * Update the parser's server version in case of external changes (e.g. model settings).
 */
void MySQLEditor::set_server_version(GrtVersionRef version) {
  d->_parser_context->use_server_version(version);
  create_editor_config_for_version(version);
  start_sql_processing();
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::restrict_content_to(ContentType type) {
  switch (type) {
    case ContentTypeTrigger:
      d->_parse_unit = MySQLParseUnit::PuCreateTrigger;
      break;
    case ContentTypeView:
      d->_parse_unit = MySQLParseUnit::PuCreateView;
      break;
    case ContentTypeRoutine:
      d->_parse_unit = MySQLParseUnit::PuCreateRoutine;
      break;
    case ContentTypeEvent:
      d->_parse_unit = MySQLParseUnit::PuCreateEvent;
      break;

    default:
      d->_parse_unit = MySQLParseUnit::PuGeneric;
      break;
  }
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::has_sql_errors() const {
  return d->_recognition_errors.size() > 0;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::text_changed(int position, int length, int lines_changed, bool added) {
  stop_processing();
  if (_code_editor->auto_completion_active() && !added) {
    // Update auto completion list if a char was removed, but not added.
    // When adding a char the caret is not yet updated leading to strange behavior.
    // So we use a different notification for adding chars.
    std::string text = getWrittenPart(position);
    update_auto_completion(text);
  }

  d->_splitting_required = true;
  d->_text_info = _code_editor->get_text_ptr();
  if (d->_is_sql_check_enabled)
    d->_current_delay_timer =
      bec::GRTManager::get()->run_every(std::bind(&MySQLEditor::start_sql_processing, this), 0.001);
  else
    d->_text_change_signal(); // If there is no timer set up then trigger change signals directly.
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::char_added(int char_code) {
  if (!_code_editor->auto_completion_active())
    d->_last_typed_char = char_code; // UTF32 encoded char.
  else {
    std::string text = getWrittenPart(_code_editor->get_caret_pos());
    update_auto_completion(text);
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::dwell_event(bool started, size_t position, int x, int y) {
  if (started) {
    if (_code_editor->indicator_at(position) == mforms::RangeIndicatorError) {
      // TODO: sort by position and do a binary search.
      for (size_t i = 0; i < d->_recognition_errors.size(); ++i) {
        ParserErrorEntry entry = d->_recognition_errors[i];
        if (entry.position <= position && position <= entry.position + entry.length) {
          _code_editor->show_calltip(true, position, entry.message);
          break;
        }
      }
    }
  } else
    _code_editor->show_calltip(false, 0, "");
}

//--------------------------------------------------------------------------------------------------

/**
 * Prepares and triggers an sql check run. Runs in the context of the main thread.
 */
bool MySQLEditor::start_sql_processing() {
  // Here we trigger our text change signal, to avoid frequent signals for each key press.
  // Consumers are expected to use this signal for UI updates, so we need to coalesce messages.
  d->_text_change_signal();

  d->_current_delay_timer = NULL; // The timer will be deleted by the grt manager.

  {
    RecMutexLock sql_errors_mutex(d->_sql_checker_mutex);
    d->_recognition_errors.clear();
  }

  d->_stop_processing = false;

  _code_editor->set_status_text("");
  if (d->_text_info.first != NULL && d->_text_info.second > 0)
    d->_current_work_timer_id = ThreadedTimer::get()->add_task(
      TimerTimeSpan, 0.05, true, std::bind(&MySQLEditor::do_statement_split_and_check, this, std::placeholders::_1));
  return false; // Don't re-run this task, it's a single-shot.
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::do_statement_split_and_check(int id) {
  // TODO: there's no need always split and error-check all text in the editor.
  //       Only split and scan from the current caret position.
  d->split_statements_if_required();

  // Start tasks that depend on the statement ranges (markers + auto completion).
  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&MySQLEditor::splitting_done, this));

  if (d->_stop_processing)
    return false;

  base::RecMutexLock lock(d->_sql_checker_mutex);

  // Now do error checking for each of the statements, collecting error positions for later markup.
  d->_last_sql_check_progress_msg_timestamp = timestamp();
  for (std::vector<std::pair<size_t, size_t>>::const_iterator range_iterator = d->_statement_ranges.begin();
       range_iterator != d->_statement_ranges.end(); ++range_iterator) {
    if (d->_stop_processing)
      return false;

    if (d->_services->checkSqlSyntax(d->_parser_context, d->_text_info.first + range_iterator->first,
                                     range_iterator->second, d->_parse_unit) > 0) {
      std::vector<ParserErrorEntry> errors = d->_parser_context->get_errors_with_offset(range_iterator->first, true);
      d->_recognition_errors.insert(d->_recognition_errors.end(), errors.begin(), errors.end());
    }
  }

  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&MySQLEditor::update_error_markers, this));

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Updates the statement markup and starts auto completion if enabled. This is called in the
 * context of the main thread.
 */
void *MySQLEditor::splitting_done() {
  // Trigger auto completion for certain keys (if enabled).
  // This has to be done after our statement  splitter has completed (which is the case when we appear here).
  if (auto_start_code_completion() && !_code_editor->auto_completion_active() &&
      (g_unichar_isalnum(d->_last_typed_char) || d->_last_typed_char == '.' || d->_last_typed_char == '@')) {
    d->_last_typed_char = 0;
    show_auto_completion(false, d->_autocompletion_context);
  }

  std::set<size_t> removal_candidates;
  std::set<size_t> insert_candidates;

  std::set<size_t> lines;
  for (std::vector<std::pair<size_t, size_t>>::const_iterator iterator = d->_statement_ranges.begin();
       iterator != d->_statement_ranges.end(); ++iterator)
    lines.insert(_code_editor->line_from_position(iterator->first));

  std::set_difference(lines.begin(), lines.end(), d->_statement_marker_lines.begin(), d->_statement_marker_lines.end(),
                      inserter(insert_candidates, insert_candidates.begin()));

  std::set_difference(d->_statement_marker_lines.begin(), d->_statement_marker_lines.end(), lines.begin(), lines.end(),
                      inserter(removal_candidates, removal_candidates.begin()));

  d->_statement_marker_lines.swap(lines);

  d->_updating_statement_markers = true;
  for (std::set<size_t>::const_iterator iterator = removal_candidates.begin(); iterator != removal_candidates.end();
       ++iterator)
    _code_editor->remove_markup(mforms::LineMarkupStatement, *iterator);

  for (std::set<size_t>::const_iterator iterator = insert_candidates.begin(); iterator != insert_candidates.end();
       ++iterator)
    _code_editor->show_markup(mforms::LineMarkupStatement, *iterator);
  d->_updating_statement_markers = false;

  return NULL;
}

//--------------------------------------------------------------------------------------------------

void *MySQLEditor::update_error_markers() {
  std::set<size_t> removalCandidates;
  std::set<size_t> insertCandidates;

  std::set<size_t> lines;

  _code_editor->remove_indicator(mforms::RangeIndicatorError, 0, _code_editor->text_length());
  if (d->_recognition_errors.size() > 0) {
    if (d->_recognition_errors.size() == 1)
      _code_editor->set_status_text(_("1 error found"));
    else
      _code_editor->set_status_text(base::strfmt(_("%lu errors found"), (unsigned long)d->_recognition_errors.size()));

    for (size_t i = 0; i < d->_recognition_errors.size(); ++i) {
      _code_editor->show_indicator(mforms::RangeIndicatorError, d->_recognition_errors[i].position,
                                   d->_recognition_errors[i].length);
      lines.insert(_code_editor->line_from_position(d->_recognition_errors[i].position));
    }
  } else {
    _code_editor->set_status_text("");
  }

  std::set_difference(lines.begin(), lines.end(), d->_error_marker_lines.begin(), d->_error_marker_lines.end(),
                      inserter(insertCandidates, insertCandidates.begin()));

  std::set_difference(d->_error_marker_lines.begin(), d->_error_marker_lines.end(), lines.begin(), lines.end(),
                      inserter(removalCandidates, removalCandidates.begin()));

  d->_error_marker_lines.swap(lines);

  // TODO: continue-on-error shouldn't change error markers.
  mforms::LineMarkup unmark = /*_continueOnError ? mforms::LineMarkupErrorContinue :*/ mforms::LineMarkupError;
  mforms::LineMarkup mark = /*_continueOnError ? mforms::LineMarkupErrorContinue :*/ mforms::LineMarkupError;

  for (auto candidate : removalCandidates)
    _code_editor->remove_markup(unmark, candidate);

  for (auto candidate : insertCandidates)
    _code_editor->show_markup(mark, candidate);

  return NULL;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLEditor::selected_text() {
  return _code_editor->get_text(true);
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_selected_text(const std::string &new_text) {
  _code_editor->replace_selected_text(new_text);
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::insert_text(const std::string &new_text) {
  _code_editor->clear_selection();
  _code_editor->replace_selected_text(new_text);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the statement at the current caret position.
 */
std::string MySQLEditor::current_statement() {
  size_t min, max;
  if (get_current_statement_range(min, max))
    return _code_editor->get_text_in_range(min, max);
  return "";
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::sql_check_progress_msg_throttle(double val) {
  d->_sql_check_progress_msg_throttle = val;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::setup_editor_menu() {
  d->_editor_context_menu = new mforms::Menu();
  scoped_connect(d->_editor_context_menu->signal_will_show(), std::bind(&MySQLEditor::editor_menu_opening, this));

  d->_editor_context_menu->add_item(_("Undo"), "undo");
  d->_editor_context_menu->add_item(_("Redo"), "redo");
  d->_editor_context_menu->add_separator();
  d->_editor_context_menu->add_item(_("Cut"), "cut");
  d->_editor_context_menu->add_item(_("Copy"), "copy");
  d->_editor_context_menu->add_item(_("Paste"), "paste");
  d->_editor_context_menu->add_item(_("Delete"), "delete");
  d->_editor_context_menu->add_separator();
  d->_editor_context_menu->add_item(_("Select All"), "select_all");

  std::list<std::string> groups;
  groups.push_back("Menu/Text");

  {
    bec::ArgumentPool argpool;
    argpool.add_entries_for_object("activeQueryBuffer", grtobj());
    argpool.add_entries_for_object("", grtobj());

    bec::MenuItemList plugin_items = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argpool);

    if (!plugin_items.empty()) {
      d->_editor_context_menu->add_separator();
      d->_editor_context_menu->add_items_from_list(plugin_items);
    }
  }

  bec::MenuItemList plugin_items;
  bec::ArgumentPool argpool;
  argpool.add_simple_value("selectedText", grt::StringRef(""));
  argpool.add_simple_value("document", grt::StringRef(""));

  groups.clear();
  groups.push_back("Filter");
  plugin_items = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argpool);
  if (!plugin_items.empty()) {
    d->_editor_context_menu->add_separator();

    d->_editor_text_submenu = new mforms::Menu();
    d->_editor_text_submenu->add_items_from_list(plugin_items);
    d->_editor_context_menu->add_submenu(_("Text"), d->_editor_text_submenu);
  }
  _code_editor->set_context_menu(d->_editor_context_menu);
  scoped_connect(d->_editor_context_menu->signal_on_action(),
                 std::bind(&MySQLEditor::activate_context_menu_item, this, std::placeholders::_1));
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::editor_menu_opening() {
  int index = d->_editor_context_menu->get_item_index("undo");
  d->_editor_context_menu->set_item_enabled(index, _code_editor->can_undo());
  index = d->_editor_context_menu->get_item_index("redo");
  d->_editor_context_menu->set_item_enabled(index, _code_editor->can_redo());
  index = d->_editor_context_menu->get_item_index("cut");
  d->_editor_context_menu->set_item_enabled(index, _code_editor->can_cut());
  index = d->_editor_context_menu->get_item_index("copy");
  d->_editor_context_menu->set_item_enabled(index, _code_editor->can_copy());
  index = d->_editor_context_menu->get_item_index("paste");
  d->_editor_context_menu->set_item_enabled(index, _code_editor->can_paste());
  index = d->_editor_context_menu->get_item_index("delete");
  d->_editor_context_menu->set_item_enabled(index, _code_editor->can_delete());
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::activate_context_menu_item(const std::string &name) {
  // Standard commands first.
  if (name == "undo")
    _code_editor->undo();
  else if (name == "redo")
    _code_editor->redo();
  else if (name == "cut")
    _code_editor->cut();
  else if (name == "copy")
    _code_editor->copy();
  else if (name == "paste")
    _code_editor->paste();
  else if (name == "delete")
    _code_editor->replace_selected_text("");
  else if (name == "select_all")
    _code_editor->set_selection(0, _code_editor->text_length());
  else {
    std::vector<std::string> parts = base::split(name, ":", 1);
    if (parts.size() == 2 && parts[0] == "plugin") {
      app_PluginRef plugin(bec::GRTManager::get()->get_plugin_manager()->get_plugin(parts[1]));

      if (!plugin.is_valid())
        throw std::runtime_error("Invalid plugin " + name);

      bec::ArgumentPool argpool;
      argpool.add_entries_for_object("activeQueryBuffer", grtobj());
      argpool.add_entries_for_object("", grtobj());

      bool input_was_selection = false;
      if (bec::ArgumentPool::needs_simple_input(plugin, "selectedText")) {
        argpool.add_simple_value("selectedText", grt::StringRef(selected_text()));
        input_was_selection = true;
      }

      if (bec::ArgumentPool::needs_simple_input(plugin, "document"))
        argpool.add_simple_value("document", grt::StringRef(sql()));

      bool is_filter = false;
      if (plugin->groups().get_index("Filter") != grt::BaseListRef::npos)
        is_filter = true;

      grt::BaseListRef fargs(argpool.build_argument_list(plugin));

      grt::ValueRef result = bec::GRTManager::get()->get_plugin_manager()->execute_plugin_function(plugin, fargs);

      if (is_filter) {
        if (!result.is_valid() || !grt::StringRef::can_wrap(result))
          throw std::runtime_error(base::strfmt("plugin %s returned unexpected value", plugin->name().c_str()));

        grt::StringRef str(grt::StringRef::cast_from(result));
        if (input_was_selection)
          _code_editor->replace_selected_text(str.c_str());
        else
          _code_editor->set_text(str.c_str());
      }
    } else {
      logWarning("Unhandled context menu item %s", name.c_str());
    }
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::create_editor_config_for_version(GrtVersionRef version) {
  delete _editor_config;

  mforms::SyntaxHighlighterLanguage lang = mforms::LanguageMySQL;
  if (version.is_valid() && version->majorNumber() == 5) {
    switch (version->minorNumber()) {
      case 0:
        lang = mforms::LanguageMySQL50;
        break;
      case 1:
        lang = mforms::LanguageMySQL51;
        break;
      case 5:
        lang = mforms::LanguageMySQL55;
        break;
      case 6:
        lang = mforms::LanguageMySQL56;
        break;
      case 7:
        lang = mforms::LanguageMySQL57;
        break;
    }
  }
  _editor_config = new mforms::CodeEditorConfig(lang);
  _code_editor->set_language(lang);
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::show_special_chars(bool flag) {
  _code_editor->set_features(mforms::FeatureShowSpecial, flag);
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::enable_word_wrap(bool flag) {
  _code_editor->set_features(mforms::FeatureWrapText, flag);
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_sql_check_enabled(bool flag) {
  if (d->_is_sql_check_enabled != flag) {
    d->_is_sql_check_enabled = flag;
    if (flag) {
      ThreadedTimer::get()->remove_task(d->_current_work_timer_id); // Does nothing if the id is -1.
      if (d->_current_delay_timer == NULL)
        d->_current_delay_timer =
          bec::GRTManager::get()->run_every(std::bind(&MySQLEditor::start_sql_processing, this), 0.01);
    } else
      stop_processing();
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::setup_auto_completion() {
  _code_editor->auto_completion_max_size(80, 15);

  static std::vector<std::pair<int, std::string>> ccImages = {{AC_KEYWORD_IMAGE, "ac_keyword.png"},
                                                              {AC_SCHEMA_IMAGE, "ac_schema.png"},
                                                              {AC_TABLE_IMAGE, "ac_table.png"},
                                                              {AC_ROUTINE_IMAGE, "ac_routine.png"},
                                                              {AC_FUNCTION_IMAGE, "ac_function.png"},
                                                              {AC_VIEW_IMAGE, "ac_view.png"},
                                                              {AC_COLUMN_IMAGE, "ac_column.png"},
                                                              {AC_OPERATOR_IMAGE, "ac_operator.png"},
                                                              {AC_ENGINE_IMAGE, "ac_engine.png"},
                                                              {AC_TRIGGER_IMAGE, "ac_trigger.png"},
                                                              {AC_LOGFILE_GROUP_IMAGE, "ac_logfilegroup.png"},
                                                              {AC_USER_VAR_IMAGE, "ac_uservar.png"},
                                                              {AC_SYSTEM_VAR_IMAGE, "ac_sysvar.png"},
                                                              {AC_TABLESPACE_IMAGE, "ac_tablespace.png"},
                                                              {AC_EVENT_IMAGE, "ac_event.png"},
                                                              {AC_INDEX_IMAGE, "ac_index.png"},
                                                              {AC_USER_IMAGE, "ac_user.png"},
                                                              {AC_CHARSET_IMAGE, "ac_charset.png"},
                                                              {AC_COLLATION_IMAGE, "ac_collation.png"}};

  _code_editor->auto_completion_register_images(ccImages);
  _code_editor->auto_completion_stops("\t,.*;) "); // Will close ac even if we are in an identifier.
  _code_editor->auto_completion_fillups("");

  std::string grammarPath = base::makePath(bec::GRTManager::get()->get_basedir(), "data/MySQL.g");
  initializeMySQLCodeCompletionIfNeeded(grammarPath);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the text in the editor starting at the given position backwards until the line start.
 * If there's a back tick or double quote char then text until this quote char is returned. If there's
 * no quoting char but a space or dot char then everything up to (but not including) this is returned.
 */
std::string MySQLEditor::getWrittenPart(size_t position) {
  ssize_t line = _code_editor->line_from_position(position);
  ssize_t start, stop;
  _code_editor->get_range_of_line(line, start, stop);
  std::string text = _code_editor->get_text_in_range(start, position);
  if (text.empty())
    return "";

  const char *head = text.c_str();
  const char *run = head;

  while (*run != '\0') {
    if (*run == '\'' || *run == '"' || *run == '`') {
      // Entering a quoted text.
      head = run + 1;
      char quote_char = *run;
      while (true) {
        run = g_utf8_next_char(run);
        if (*run == quote_char || *run == '\0')
          break;

        // If there's an escape char skip it and the next char too (if we didn't reach the end).
        if (*run == '\\') {
          run++;
          if (*run != '\0')
            run = g_utf8_next_char(run);
        }
      }
      if (*run == '\0') // Unfinished quoted text. Return everything.
        return head;
      head = run + 1; // Skip over this quoted text and start over.
    }
    run++;
  }

  // If we come here then we are outside any quoted text. Scan back for anything we consider
  // to be a word stopper (for now anything below '0', char code wise).
  while (head < run--) {
    if (*run < '0')
      return run + 1;
  }
  return head;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::show_auto_completion(bool auto_choose_single, parser::MySQLParserContext::Ref parser_context) {
  if (!code_completion_enabled())
    return;

  _code_editor->auto_completion_options(true, auto_choose_single, false, true, false);

  // Get the statement and its absolute position.
  size_t caretPosition = _code_editor->get_caret_pos();
  size_t caretLine = _code_editor->line_from_position(caretPosition);

  ssize_t lineStart, lineEnd;
  _code_editor->get_range_of_line(caretLine, lineStart, lineEnd);
  size_t caretOffset = caretPosition - lineStart; // This is a byte offset.

  size_t min, max;
  std::string statement;
  bool fixedCaretPos = false;
  if (get_current_statement_range(min, max, true)) {
    // If the caret is in the whitespaces before the query we would get a wrong line number
    // (because the statement splitter doesn't include these whitespaces in the determined ranges).
    // We set the caret pos to the first position in the query, which has the same effect for
    // code completion (we don't generate error line numbers).
    uint32_t codeStartLine = (uint32_t)_code_editor->line_from_position(min);
    if (codeStartLine > caretLine) {
      caretLine = 0;
      caretOffset = 0;
      fixedCaretPos = true;
    } else
      caretLine -= codeStartLine;

    statement = _code_editor->get_text_in_range(min, max);
  } else {
    // No query, means we have nothing typed yet in the current query (at least nothing valuable).
    caretLine = 0;
    caretOffset = 0;
    fixedCaretPos = true;
  }

  // Convert current caret position into a position of the single statement.
  // The byte-based offset in the line must be converted to a character offset.
  if (!fixedCaretPos) {
    std::string line_text = _code_editor->get_text_in_range(lineStart, lineEnd);
    caretOffset = g_utf8_pointer_to_offset(line_text.c_str(), line_text.c_str() + caretOffset);
  }

  std::string writtenPart = getWrittenPart(caretPosition);
  _auto_completion_entries = getCodeCompletionList(caretLine, caretOffset, writtenPart, _current_schema,
                                                   make_keywords_uppercase(), parser_context->createScanner(statement),
                                                   _editor_config->get_keywords()["Functions"], _auto_completion_cache);
  update_auto_completion(writtenPart);
}

//--------------------------------------------------------------------------------------------------

/**
 * Updates the auto completion list by filtering the determined entries by the text the user
 * already typed. If auto completion is not yet active it becomes active here.
 * Returns the list sent to the editor for unit tests to validate them.
 */
std::vector<std::pair<int, std::string>> MySQLEditor::update_auto_completion(const std::string &typed_part) {
  logDebug2("Updating auto completion popup in editor\n");

  // Remove all entries that don't start with the typed text before showing the list.
  if (!typed_part.empty()) {
    gchar *prefix = g_utf8_casefold(typed_part.c_str(), -1);

    std::vector<std::pair<int, std::string>> filteredEntries;
    for (auto &entry : _auto_completion_entries) {
      gchar *folded = g_utf8_casefold(entry.second.c_str(), -1);
      if (g_str_has_prefix(folded, prefix))
        filteredEntries.push_back(entry);
      g_free(folded);
    }

    switch (filteredEntries.size()) {
      case 0:
        logDebug2("Nothing to autocomplete - hiding popup if it was active\n");
        _code_editor->auto_completion_cancel();
        break;
      case 1:
        // See if that single entry matches the typed part. If so we don't need to show ac either.
        if (base::same_string(filteredEntries[0].second, prefix,
                              false)) // Exact (but case insensitive) match, not just string parts.
        {
          logDebug2("The only match is the same as the written input - hiding popup if it was active\n");
          _code_editor->auto_completion_cancel();
          break;
        }
      // Fall through.
      default:
        logDebug2("Showing auto completion popup\n");
        _code_editor->auto_completion_show(typed_part.size(), filteredEntries);
        break;
    }

    g_free(prefix);

    return filteredEntries;
  } else {
    if (!_auto_completion_entries.empty()) {
      logDebug2("Showing auto completion popup\n");
      _code_editor->auto_completion_show(0, _auto_completion_entries);
    } else {
      logDebug2("Nothing to autocomplete - hiding popup if it was active\n");
      _code_editor->auto_completion_cancel();
    }
  }

  return _auto_completion_entries;
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::cancel_auto_completion() {
  // Make sure a pending timed autocompletion won't kick in after we cancel it.
  d->_last_typed_char = 0;
  _code_editor->auto_completion_cancel();
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_auto_completion_cache(MySQLObjectNamesCache *cache) {
  logDebug2("Auto completion cache set to: %p\n", cache);

  _auto_completion_cache = cache;
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::code_completion_enabled() {
  return bec::GRTManager::get()->get_app_option_int("DbSqlEditor:CodeCompletionEnabled") == 1;
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::auto_start_code_completion() {
  return (bec::GRTManager::get()->get_app_option_int("DbSqlEditor:AutoStartCodeCompletion") == 1) &&
         (d->_autocompletion_context != NULL);
}

//--------------------------------------------------------------------------------------------------

bool MySQLEditor::make_keywords_uppercase() {
  return bec::GRTManager::get()->get_app_option_int("DbSqlEditor:CodeCompletionUpperCaseKeywords") == 1;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines the start and end position of the current statement, that is, the statement
 * where the caret is in. For effective search in a large set binary search is used.
 *
 * Note: search can be done in two modes:
 *       - strict: whitespaces before a statement belong to that statement.
 *       - loose: such whitespaces belong to the previous statement (and are ignored).
 *       Loose mode allows to have the caret in the whitespaces after a statement and execute that,
 *       while strict mode is needed for code completion (should be done for the following statement then).
 *
 * @returns true if a statement could be found at the caret position, otherwise false.
 */
bool MySQLEditor::get_current_statement_range(size_t &start, size_t &end, bool strict) {
  // In case the splitter is right now processing the text we wait here until its done.
  // If the splitter wasn't triggered yet (e.g. when typing fast and then immediately running a statement)
  // then we do the splitting here instead.
  RecMutexLock sql_statement_borders_mutex(d->_sql_statement_borders_mutex);
  d->split_statements_if_required();

  if (d->_statement_ranges.empty())
    return false;

  typedef std::vector<std::pair<size_t, size_t>>::iterator RangeIterator;

  size_t caret_position = _code_editor->get_caret_pos();
  RangeIterator low = d->_statement_ranges.begin();
  RangeIterator high = d->_statement_ranges.end() - 1;
  while (low < high) {
    RangeIterator middle = low + (high - low + 1) / 2;
    if (middle->first > caret_position)
      high = middle - 1;
    else {
      size_t end = low->first + low->second;
      if (end >= caret_position)
        break;
      low = middle;
    }
  }

  if (low == d->_statement_ranges.end())
    return false;

  // If we are between two statements (in white spaces) then the algorithm above returns the lower one.
  if (strict) {
    if (low->first + low->second < caret_position)
      ++low;
    if (low == d->_statement_ranges.end())
      return false;
  }

  start = low->first;
  end = low->first + low->second;
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Stops any ongoing processing like splitting, syntax checking etc.
 */
void MySQLEditor::stop_processing() {
  d->_stop_processing = true;

  ThreadedTimer::get()->remove_task(d->_current_work_timer_id);
  d->_current_work_timer_id = -1;

  if (d->_current_delay_timer != NULL) {
    bec::GRTManager::get()->cancel_timer(d->_current_delay_timer);
    d->_current_delay_timer = NULL;
  }

  d->_services->stopProcessing();
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::focus() {
  _code_editor->focus();
}

//--------------------------------------------------------------------------------------------------

/**
 * Register a target for file drop operations which will handle these cases.
 */
void MySQLEditor::register_file_drop_for(mforms::DropDelegate *target) {
  std::vector<std::string> formats;
  formats.push_back(mforms::DragFormatFileName);
  _code_editor->register_drop_formats(target, formats);
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::set_continue_on_error(bool value) {
  _continueOnError = value;

  std::vector<size_t> lines;

  // TODO: continue-on-error shouldn't change error markers.
  mforms::LineMarkup unmark = /*_continueOnError ? mforms::LineMarkupErrorContinue :*/ mforms::LineMarkupError;
  mforms::LineMarkup mark = /*_continueOnError ? mforms::LineMarkupErrorContinue :*/ mforms::LineMarkupError;

  for (size_t i = 0; i < d->_recognition_errors.size(); ++i) {
    _code_editor->show_indicator(mforms::RangeIndicatorError, d->_recognition_errors[i].position,
                                 d->_recognition_errors[i].length);
    lines.push_back(_code_editor->line_from_position(d->_recognition_errors[i].position));
  }

  for (std::vector<size_t>::iterator iter = lines.begin(); iter != lines.end(); ++iter) {
    _code_editor->remove_markup(unmark, *iter);
    _code_editor->show_markup(mark, *iter);
  }
}

//--------------------------------------------------------------------------------------------------
