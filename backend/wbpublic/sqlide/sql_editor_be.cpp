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

#include "SymbolTable.h"

#include "sql_editor_be.h"
#include <mutex>

DEFAULT_LOG_DOMAIN("MySQL editor");

using namespace bec;
using namespace grt;
using namespace base;

using namespace parsers;

//----------------------------------------------------------------------------------------------------------------------

class MySQLEditor::Private {
public:
  // ref to the GRT object representing this object
  // it will be used in Db_sql_editor queryBuffer list and in standalone
  // editors for plugin support
  db_query_QueryBufferRef grtobj;
  mforms::Box *container;
  mforms::Menu *editorContextMenu;
  mforms::Menu *editorTextSubmenu;

  mforms::ToolBar *toolbar;

  int lastTypedChar;

  MySQLParserContext::Ref parserContext;
  MySQLParserContext::Ref autocompletionContext;
  MySQLParserServices::Ref services;
  SymbolTable symbolTable;

  // Entries determined the last time we started auto completion. The actually shown list
  // is derived from these entries filtered by the current input.
  std::vector<std::pair<int, std::string>> codeCompletionCandidates;

  base::RecMutex sqlCheckerMutex;
  MySQLParseUnit parseUnit; // The type of query we want to limit our parsing to.

  // We use 2 timers here for delayed work. One is a grt timer to run a task in
  // the main thread after a certain delay.
  // The other one is to run the actual work task in a background thread.
  bec::GRTManager::Timer *currentDelayTimer;
  int currentWorkTimerID;

  std::pair<const char *, size_t> textInfo; // Only valid during a parse run.

  std::vector<ParserErrorInfo> recognitionErrors; // List of errors from the last sql check run.
  std::set<size_t> errorMarkerLines;

  bool splittingRequired;
  bool updatingStatementMarkers;
  std::set<size_t> statementMarkerLines;
  base::RecMutex sqlStatementBordersMutex;

  std::vector<StatementRange> statementRanges;

  bool isRefreshEnabled;  // Whether the FE control is permitted to replace its contents from the BE.
  bool isSQLCheckEnabled; // Enables automatic syntax checks.
  bool stopProcessing;    // To stop ongoing syntax checks (because of text changes).
  bool ownsToolbar;

  boost::signals2::signal<void()> textChangeSignal;

  mforms::CodeEditor *codeEditor = nullptr;
  std::string currentSchema;
  std::string sqlMode;

  Private(MySQLParserContext::Ref syntaxcheck_context, MySQLParserContext::Ref autocompleteContext)
    : grtobj(grt::Initialized), stopProcessing(false) {
    ownsToolbar = false;
    parseUnit = MySQLParseUnit::PuGeneric;
    isRefreshEnabled = true;
    splittingRequired = false;

    parserContext = syntaxcheck_context;
    autocompletionContext = autocompleteContext;
    services = MySQLParserServices::get();

    currentDelayTimer = nullptr;
    currentWorkTimerID = -1;

    isSQLCheckEnabled = true;
    container = nullptr;
    editorTextSubmenu = nullptr;
    editorContextMenu = nullptr;
    toolbar = nullptr;
    lastTypedChar = 0;
    updatingStatementMarkers = false;
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Determines ranges for all statements in the current text.
   */
  void splitStatementsIfRequired() {
    // If we have restricted content (e.g. for object editors) then we don't split and handle the entire content
    // as a single statement. This will then show syntax errors for any invalid additional input.
    if (splittingRequired) {
      logDebug3("Start splitting\n");
      splittingRequired = false;

      base::RecMutexLock lock(sqlStatementBordersMutex);

      statementRanges.clear();
      if (parseUnit == MySQLParseUnit::PuGeneric) {
        double start = timestamp();
        services->determineStatementRanges(textInfo.first, textInfo.second, ";", statementRanges);
        logDebug3("Splitting ended after %f ticks\n", timestamp() - start);
      } else
        statementRanges.push_back({ 0, 0, textInfo.second });
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * One or more markers on that line where changed. We have to stay in sync with our statement markers list
   * to make the optimized add/remove algorithm working.
   */
  void markerChanged(const mforms::LineMarkupChangeset &changeset, bool deleted) {
    if (updatingStatementMarkers || changeset.size() == 0)
      return;

    if (deleted) {
      for (mforms::LineMarkupChangeset::const_iterator iterator = changeset.begin(); iterator != changeset.end();
           ++iterator) {
        if ((iterator->markup & mforms::LineMarkupStatement) != 0)
          statementMarkerLines.erase(iterator->original_line);
        if ((iterator->markup & mforms::LineMarkupError) != 0)
          errorMarkerLines.erase(iterator->original_line);
      }
    } else {
      for (mforms::LineMarkupChangeset::const_iterator iterator = changeset.begin(); iterator != changeset.end();
           ++iterator) {
        if ((iterator->markup & mforms::LineMarkupStatement) != 0)
          statementMarkerLines.erase(iterator->original_line);
        if ((iterator->markup & mforms::LineMarkupError) != 0)
          errorMarkerLines.erase(iterator->original_line);
      }
      for (mforms::LineMarkupChangeset::const_iterator iterator = changeset.begin(); iterator != changeset.end();
           ++iterator) {
        if ((iterator->markup & mforms::LineMarkupStatement) != 0)
          statementMarkerLines.insert(iterator->new_line);
        if ((iterator->markup & mforms::LineMarkupError) != 0)
          errorMarkerLines.insert(iterator->new_line);
      }
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

};

//----------------------------------------------------------------------------------------------------------------------

MySQLEditor::Ref MySQLEditor::create(MySQLParserContext::Ref syntax_check_context,
                                     MySQLParserContext::Ref autocompleteContext,
                                     std::vector<SymbolTable *> const &globalSymbols,
                                     db_query_QueryBufferRef grtobj) {
  Ref editor = MySQLEditor::Ref(new MySQLEditor(syntax_check_context, autocompleteContext));

  editor->d->symbolTable.addDependencies(globalSymbols);

  // Replace the default object with the custom one.
  if (grtobj.is_valid())
    editor->set_grtobj(grtobj);

  // setup the GRT object
  db_query_QueryBuffer::ImplData *data = new db_query_QueryBuffer::ImplData(editor->grtobj(), editor);
  editor->grtobj()->set_data(data);

  return editor;
}

//----------------------------------------------------------------------------------------------------------------------

MySQLEditor::MySQLEditor(MySQLParserContext::Ref syntax_check_context, MySQLParserContext::Ref autocompleteContext) {
  d = new Private(syntax_check_context, autocompleteContext);

  d->codeEditor = new mforms::CodeEditor(this);
  d->codeEditor->set_font(bec::GRTManager::get()->get_app_option_string("workbench.general.Editor:Font"));
  d->codeEditor->set_features(mforms::FeatureUsePopup, false);
  d->codeEditor->set_features(mforms::FeatureConvertEolOnPaste | mforms::FeatureAutoIndent, true);
  d->codeEditor->set_name("Code Editor");

  setServerVersion(syntax_check_context->serverVersion());

  d->codeEditor->send_editor(SCI_SETTABWIDTH, bec::GRTManager::get()->get_app_option_int("Editor:TabWidth", 4), 0);
  d->codeEditor->send_editor(SCI_SETINDENT, bec::GRTManager::get()->get_app_option_int("Editor:IndentWidth", 4), 0);
  d->codeEditor->send_editor(SCI_SETUSETABS, !bec::GRTManager::get()->get_app_option_int("Editor:TabIndentSpaces", 0),
                            0);

  scoped_connect(d->codeEditor->signal_changed(),
                 std::bind(&MySQLEditor::text_changed, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, std::placeholders::_4));
  scoped_connect(d->codeEditor->signal_char_added(), std::bind(&MySQLEditor::char_added, this, std::placeholders::_1));
  scoped_connect(d->codeEditor->signal_dwell(),
                 std::bind(&MySQLEditor::dwell_event, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, std::placeholders::_4));
  scoped_connect(d->codeEditor->signal_marker_changed(),
                 std::bind(&MySQLEditor::Private::markerChanged, d, std::placeholders::_1, std::placeholders::_2));

  setup_auto_completion();
  setup_editor_menu();
}

//----------------------------------------------------------------------------------------------------------------------

MySQLEditor::~MySQLEditor() {
  stop_processing();

  {
    d->isSQLCheckEnabled = false;

    // We lock all mutexes for a moment here to ensure no background thread is
    // still holding them.
    base::RecMutexLock lock1(d->sqlCheckerMutex);
    base::RecMutexLock lock2(d->sqlStatementBordersMutex);
  }

  if (d->editorTextSubmenu != nullptr)
    delete d->editorTextSubmenu;
  delete d->editorContextMenu;
  if (d->ownsToolbar && d->toolbar != nullptr)
    d->toolbar->release();

  delete d->codeEditor;

  delete d;
}

//----------------------------------------------------------------------------------------------------------------------

db_query_QueryBufferRef MySQLEditor::grtobj() {
  return d->grtobj;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_grtobj(db_query_QueryBufferRef grtobj) {
  d->grtobj = grtobj;
}

//----------------------------------------------------------------------------------------------------------------------

mforms::CodeEditor *MySQLEditor::get_editor_control() {
  return d->codeEditor;
};

//----------------------------------------------------------------------------------------------------------------------

static void toggle_show_special_chars(mforms::ToolBarItem *item, MySQLEditor *sql_editor) {
  sql_editor->show_special_chars(item->get_checked());
}

//----------------------------------------------------------------------------------------------------------------------

static void toggle_word_wrap(mforms::ToolBarItem *item, MySQLEditor *sql_editor) {
  sql_editor->enable_word_wrap(item->get_checked());
}

//----------------------------------------------------------------------------------------------------------------------

static void show_find_panel_for_active_editor(MySQLEditor *sql_editor) {
  sql_editor->get_editor_control()->show_find_panel(false);
}

//----------------------------------------------------------------------------------------------------------------------

static void beautify_script(MySQLEditor *sql_editor) {
  grt::BaseListRef args(true);
  args.ginsert(sql_editor->grtobj());

  grt::GRT::get()->call_module_function("SQLIDEUtils", "enbeautificate", args);
}

//----------------------------------------------------------------------------------------------------------------------

static void open_file(MySQLEditor *sql_editor) {
  mforms::FileChooser fc(mforms::OpenFile);
  if (fc.run_modal()) {
    std::string file = fc.get_path();

    gchar *contents;
    gsize length;
    GError *error = nullptr;

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

//----------------------------------------------------------------------------------------------------------------------

static void save_file(MySQLEditor *sql_editor) {
  mforms::FileChooser fc(mforms::SaveFile);
  fc.set_extensions("SQL Scripts (*.sql)|*.sql", "sql");

  if (fc.run_modal()) {
    GError *error = nullptr;
    std::string file = fc.get_path();
    mforms::CodeEditor *code_editor = sql_editor->get_editor_control();
    std::pair<const char *, size_t> data = code_editor->get_text_ptr();

    if (!g_file_set_contents(file.c_str(), data.first, (gssize)data.second, &error) && error) {
      mforms::Utilities::show_error("Save File", base::strfmt("Could not save to file %s:\n%s", file.c_str(),
        error->message), "OK");
      g_error_free(error);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_base_toolbar(mforms::ToolBar *toolbar) {
  /* TODO: that is a crude implementation as the toolbar is sometimes directly
  created and set and *then* also set
           here, so deleting it crashs.
  if (d->toolbar != nullptr && d->owns_toolbar)
    delete d->toolbar;
  */
  d->toolbar = toolbar;
  d->ownsToolbar = false;

  mforms::ToolBarItem *item;

  if (d->isSQLCheckEnabled) {
    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("Beautify");
    item->setInternalName("query.beautify");
    item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_beautifier.png"));
    item->set_tooltip(_("Beautify/reformat the SQL script"));
    scoped_connect(item->signal_activated(), std::bind(beautify_script, this));
    d->toolbar->add_item(item);
  }
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Search");
  item->setInternalName("query.search");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_find.png"));
  item->set_tooltip(_("Show the Find panel for the editor"));
  scoped_connect(item->signal_activated(), std::bind(show_find_panel_for_active_editor, this));
  d->toolbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("Toggle Invisible");
  item->setInternalName("query.toggleInvisible");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_special-chars-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_special-chars-off.png"));
  item->set_tooltip(_("Toggle display of invisible characters (spaces, tabs, newlines)"));
  scoped_connect(item->signal_activated(), std::bind(toggle_show_special_chars, item, this));
  d->toolbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("Toggle Word Wrap");
  item->setInternalName("query.toggleWordWrap");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_word-wrap-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_word-wrap-off.png"));
  item->set_tooltip(_("Toggle wrapping of long lines (keep this off for large files)"));
  scoped_connect(item->signal_activated(), std::bind(toggle_word_wrap, item, this));
  d->toolbar->add_item(item);
}

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

mforms::View *MySQLEditor::get_container() {
  if (d->container == nullptr) {
    d->container = new mforms::Box(false);

    d->container->add(get_toolbar(), false, true);
    get_editor_control()->set_show_find_panel_callback(
      std::bind(embed_find_panel, std::placeholders::_1, std::placeholders::_2, d->container));
    d->container->add_end(get_editor_control(), true, true);
  }
  return d->container;
};

//----------------------------------------------------------------------------------------------------------------------

mforms::ToolBar *MySQLEditor::get_toolbar(bool include_file_actions) {
  if (!d->toolbar) {
    d->ownsToolbar = true;
    d->toolbar = mforms::manage(new mforms::ToolBar(mforms::SecondaryToolBar));
#ifdef _MSC_VER
    d->toolbar->set_size(-1, 27);
#endif
    if (include_file_actions) {
      mforms::ToolBarItem *item;

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("Open File");
      item->setInternalName("query.openFile");
      item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_open.png"));
      item->set_tooltip(_("Open a script file in this editor"));
      scoped_connect(item->signal_activated(), std::bind(open_file, this));
      d->toolbar->add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("Save File");
      item->setInternalName("query.saveFile");
      item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_save.png"));
      item->set_tooltip(_("Save the script to a file."));
      scoped_connect(item->signal_activated(), std::bind(save_file, this));
      d->toolbar->add_item(item);

      d->toolbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));
    }
    set_base_toolbar(d->toolbar);
  }
  return d->toolbar;
};

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::is_refresh_enabled() const {
  return d->isRefreshEnabled;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_refresh_enabled(bool val) {
  d->isRefreshEnabled = val;
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::is_sql_check_enabled() const {
  return d->isSQLCheckEnabled;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the text of the editor. Usage of this function is discouraged because
 * it copies the
 * (potentially) large editor content. Use text_ptr() instead.
 */
std::string MySQLEditor::sql() {
  return d->codeEditor->get_text(false);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a direct pointer to the editor content, which is only valid until the
 * next change.
 * So if you want to keep it for longer copy the text.
 * Note: since the text can be large don't do this unless absolutely necessary.
 */
std::pair<const char *, size_t> MySQLEditor::text_ptr() {
  return d->codeEditor->get_text_ptr();
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_current_schema(const std::string &schema) {
  d->currentSchema = schema;
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::empty() {
  return d->codeEditor->text_length() == 0;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::append_text(const std::string &text) {
  d->codeEditor->append_text(text.data(), text.size());
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Used to the set the content of the editor from outside (e.g. when loading a
 * file or for tests).
 */
void MySQLEditor::sql(const char *sql) {
  d->codeEditor->set_text(sql);
  d->splittingRequired = true;
  d->statementMarkerLines.clear();
  d->codeEditor->set_eol_mode(mforms::EolLF, true);
}

//----------------------------------------------------------------------------------------------------------------------

std::size_t MySQLEditor::cursor_pos() {
  return d->codeEditor->get_caret_pos();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the caret position as column/row pair. The returned column (char
 * index) is utf-8 safe and computes the actual character index as displayed in the editor, not the byte index in
 * a std::string. If @local is true then the line position is relative to the statement,
 * otherwise that in the entire editor.
 */
std::pair<std::size_t, std::size_t> MySQLEditor::cursor_pos_row_column(bool local) {
  size_t position = d->codeEditor->get_caret_pos();
  ssize_t line = d->codeEditor->line_from_position(position);
  ssize_t line_start, line_end;
  d->codeEditor->get_range_of_line(line, line_start, line_end);

  ssize_t offset = position - line_start; // This is a byte offset.
  std::string line_text = d->codeEditor->get_text_in_range(line_start, line_end);
  offset = g_utf8_pointer_to_offset(line_text.c_str(), line_text.c_str() + offset);

  if (local) {
    size_t min, max;
    if (get_current_statement_range(min, max))
      line -= d->codeEditor->line_from_position(min);
  }

  return { offset, line };
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_cursor_pos(std::size_t position) {
  d->codeEditor->set_caret_pos(position);
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::selected_range(std::size_t &start, std::size_t &end) {
  size_t length;
  d->codeEditor->get_selection(start, length);
  end = start + length;
  return length > 0;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_selected_range(std::size_t start, std::size_t end) {
  d->codeEditor->set_selection(start, end - start);
}

///----------------------------------------------------------------------------------------------------------------------

boost::signals2::signal<void()> *MySQLEditor::text_change_signal() {
  return &d->textChangeSignal;
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLEditor::sql_mode() {
  return d->sqlMode;
};

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_sql_mode(const std::string &value) {
  d->sqlMode = value;
  d->parserContext->updateSqlMode(value);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Update the parser's server version in case of external changes (e.g. model
 * settings).
 */
void MySQLEditor::setServerVersion(GrtVersionRef version) {
  mforms::SyntaxHighlighterLanguage lang = mforms::LanguageMySQL;
  if (version.is_valid()) {
    switch (version->majorNumber()) {
      case 5:
        switch (version->minorNumber()) {
          case 6:
            lang = mforms::LanguageMySQL56;
            break;
          case 7:
            lang = mforms::LanguageMySQL57;
            break;
        }
        break;

      case 8:
        switch (version->minorNumber()) {
          case 0:
            lang = mforms::LanguageMySQL80;
            break;
        }
        break;
    }
  }
  d->codeEditor->set_language(lang);

  d->parserContext->updateServerVersion(version);
  start_sql_processing();
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::restrict_content_to(ContentType type) {
  switch (type) {
    case ContentTypeTrigger:
      d->parseUnit = MySQLParseUnit::PuCreateTrigger;
      break;
    case ContentTypeView:
      d->parseUnit = MySQLParseUnit::PuCreateView;
      break;
    case ContentTypeFunction:
      d->parseUnit = MySQLParseUnit::PuCreateFunction;
      break;
    case ContentTypeProcedure:
      d->parseUnit = MySQLParseUnit::PuCreateProcedure;
      break;
    case ContentTypeUdf:
      d->parseUnit = MySQLParseUnit::PuCreateUdf;
      break;
    case ContentTypeRoutine:
      d->parseUnit = MySQLParseUnit::PuCreateRoutine;
      break;
    case ContentTypeEvent:
      d->parseUnit = MySQLParseUnit::PuCreateEvent;
      break;

    default:
      d->parseUnit = MySQLParseUnit::PuGeneric;
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::has_sql_errors() const {
  return d->recognitionErrors.size() > 0;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::text_changed(Sci_Position position, Sci_Position length, Sci_Position lines_changed, bool added) {
  stop_processing();
  if (d->codeEditor->auto_completion_active() && !added) {
    // Update auto completion list if a char was removed, but not added.
    // When adding a char the caret is not yet updated leading to strange
    // behavior.
    // So we use a different notification for adding chars.
    std::string text = getWrittenPart(position);
    update_auto_completion(text);
  }

  d->splittingRequired = true;
  d->textInfo = d->codeEditor->get_text_ptr();
  if (d->isSQLCheckEnabled)
    d->currentDelayTimer =
      bec::GRTManager::get()->run_every(std::bind(&MySQLEditor::start_sql_processing, this), 0.001);
  else
    d->textChangeSignal(); // If there is no timer set up then trigger
                              // change signals directly.
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::char_added(int char_code) {
  if (!d->codeEditor->auto_completion_active())
    d->lastTypedChar = char_code; // UTF32 encoded char.
  else {
    std::string text = getWrittenPart(d->codeEditor->get_caret_pos());
    update_auto_completion(text);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::dwell_event(bool started, size_t position, int x, int y) {
  if (started) {
    if (d->codeEditor->indicator_at(position) == mforms::RangeIndicatorError) {
      // TODO: sort by position and do a binary search.
      for (size_t i = 0; i < d->recognitionErrors.size(); ++i) {
        ParserErrorInfo entry = d->recognitionErrors[i];
        if (entry.charOffset <= position && position <= entry.charOffset + entry.length) {
          d->codeEditor->show_calltip(true, position, entry.message);
          break;
        }
      }
    }
  } else
    d->codeEditor->show_calltip(false, 0, "");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Prepares and triggers an sql check run. Runs in the context of the main
 * thread.
 */
bool MySQLEditor::start_sql_processing() {
  // Here we trigger our text change signal, to avoid frequent signals for each
  // key press.
  // Consumers are expected to use this signal for UI updates, so we need to
  // coalesce messages.
  d->textChangeSignal();

  d->currentDelayTimer = nullptr; // The timer will be deleted by the grt manager.

  {
    RecMutexLock sql_errors_mutex(d->sqlCheckerMutex);
    d->recognitionErrors.clear();
  }

  d->stopProcessing = false;

  d->codeEditor->set_status_text("");
  if (d->textInfo.first != nullptr && d->textInfo.second > 0)
    d->currentWorkTimerID = ThreadedTimer::get()->add_task(
      TimerTimeSpan, 0.05, true, std::bind(&MySQLEditor::do_statement_split_and_check, this, std::placeholders::_1));
  return false; // Don't re-run this task, it's a single-shot.
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::do_statement_split_and_check(int id) {
  d->splitStatementsIfRequired();

  // Start tasks that depend on the statement ranges (markers + auto completion).
  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&MySQLEditor::splitting_done, this));

  if (d->stopProcessing)
    return false;

  base::RecMutexLock lock(d->sqlCheckerMutex);

  // Now do error checking for each of the statements, collecting error
  // positions for later markup.
  for (auto &range : d->statementRanges) {
    if (d->stopProcessing)
      return false;

    if (d->services->checkSqlSyntax(d->parserContext, d->textInfo.first + range.start, range.length, d->parseUnit) > 0) {
      std::vector<ParserErrorInfo> errors = d->parserContext->errorsWithOffset(range.start);
      d->recognitionErrors.insert(d->recognitionErrors.end(), errors.begin(), errors.end());
    }
  }

  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&MySQLEditor::update_error_markers, this));

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Updates the statement markup and starts auto completion if enabled. This is called in the context of the main thread.
 */
void *MySQLEditor::splitting_done() {
  // Trigger auto completion for certain keys (if enabled).
  // This has to be done after our statement  splitter has completed (which is
  // the case when we appear here).
  if (auto_start_code_completion() && !d->codeEditor->auto_completion_active() &&
      (g_unichar_isalnum(d->lastTypedChar) || d->lastTypedChar == '.' || d->lastTypedChar == '@')) {
    d->lastTypedChar = 0;
    show_auto_completion(false);
  }

  std::set<size_t> removal_candidates;
  std::set<size_t> insert_candidates;

  std::set<size_t> lines;
  for (auto &range : d->statementRanges)
    lines.insert(d->codeEditor->line_from_position(range.start));

  std::set_difference(lines.begin(), lines.end(), d->statementMarkerLines.begin(), d->statementMarkerLines.end(),
                      inserter(insert_candidates, insert_candidates.begin()));

  std::set_difference(d->statementMarkerLines.begin(), d->statementMarkerLines.end(), lines.begin(), lines.end(),
                      inserter(removal_candidates, removal_candidates.begin()));

  d->statementMarkerLines.swap(lines);

  d->updatingStatementMarkers = true;
  for (std::set<size_t>::const_iterator iterator = removal_candidates.begin(); iterator != removal_candidates.end();
       ++iterator)
    d->codeEditor->remove_markup(mforms::LineMarkupStatement, *iterator);

  for (std::set<size_t>::const_iterator iterator = insert_candidates.begin(); iterator != insert_candidates.end();
       ++iterator)
    d->codeEditor->show_markup(mforms::LineMarkupStatement, *iterator);
  d->updatingStatementMarkers = false;

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void *MySQLEditor::update_error_markers() {
  std::set<size_t> removal_candidates;
  std::set<size_t> insert_candidates;

  std::set<size_t> lines;

  d->codeEditor->remove_indicator(mforms::RangeIndicatorError, 0, d->codeEditor->text_length());
  if (d->recognitionErrors.size() > 0) {
    if (d->recognitionErrors.size() == 1)
      d->codeEditor->set_status_text(_("1 error found"));
    else
      d->codeEditor->set_status_text(base::strfmt(_("%lu errors found"),
        static_cast<unsigned long>(d->recognitionErrors.size())));

    for (size_t i = 0; i < d->recognitionErrors.size(); ++i) {
      d->codeEditor->show_indicator(mforms::RangeIndicatorError, d->recognitionErrors[i].charOffset,
                                   d->recognitionErrors[i].length);
      lines.insert(d->codeEditor->line_from_position(d->recognitionErrors[i].charOffset));
    }
  } else
    d->codeEditor->set_status_text("");

  std::set_difference(lines.begin(), lines.end(), d->errorMarkerLines.begin(), d->errorMarkerLines.end(),
                      inserter(insert_candidates, insert_candidates.begin()));

  std::set_difference(d->errorMarkerLines.begin(), d->errorMarkerLines.end(), lines.begin(), lines.end(),
                      inserter(removal_candidates, removal_candidates.begin()));

  d->errorMarkerLines.swap(lines);

  for (std::set<size_t>::const_iterator iterator = removal_candidates.begin(); iterator != removal_candidates.end();
       ++iterator)
    d->codeEditor->remove_markup(mforms::LineMarkupError, *iterator);

  for (std::set<size_t>::const_iterator iterator = insert_candidates.begin(); iterator != insert_candidates.end();
       ++iterator)
    d->codeEditor->show_markup(mforms::LineMarkupError, *iterator);

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLEditor::selected_text() {
  return d->codeEditor->get_text(true);
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_selected_text(const std::string &new_text) {
  d->codeEditor->replace_selected_text(new_text);
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::insert_text(const std::string &new_text) {
  d->codeEditor->clear_selection();
  d->codeEditor->replace_selected_text(new_text);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the statement at the current caret position.
 */
std::string MySQLEditor::current_statement() {
  size_t min, max;
  if (get_current_statement_range(min, max))
    return d->codeEditor->get_text_in_range(min, max);
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::setup_editor_menu() {
  d->editorContextMenu = new mforms::Menu();
  scoped_connect(d->editorContextMenu->signal_will_show(), std::bind(&MySQLEditor::editor_menu_opening, this));

  d->editorContextMenu->add_item(_("Undo"), "undo");
  d->editorContextMenu->add_item(_("Redo"), "redo");
  d->editorContextMenu->add_separator();
  d->editorContextMenu->add_item(_("Cut"), "cut");
  d->editorContextMenu->add_item(_("Copy"), "copy");
  d->editorContextMenu->add_item(_("Paste"), "paste");
  d->editorContextMenu->add_item(_("Delete"), "delete");
  d->editorContextMenu->add_separator();
  d->editorContextMenu->add_item(_("Select All"), "select_all");

  std::list<std::string> groups;
  groups.push_back("Menu/Text");

  {
    bec::ArgumentPool argpool;
    argpool.add_entries_for_object("activeQueryBuffer", grtobj());
    argpool.add_entries_for_object("", grtobj());

    bec::MenuItemList plugin_items = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argpool);

    if (!plugin_items.empty()) {
      d->editorContextMenu->add_separator();
      d->editorContextMenu->add_items_from_list(plugin_items);
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
    d->editorContextMenu->add_separator();

    d->editorTextSubmenu = new mforms::Menu();
    d->editorTextSubmenu->add_items_from_list(plugin_items);
    d->editorContextMenu->add_submenu(_("Text"), d->editorTextSubmenu);
  }
  d->codeEditor->set_context_menu(d->editorContextMenu);
  scoped_connect(d->editorContextMenu->signal_on_action(),
                 std::bind(&MySQLEditor::activate_context_menu_item, this, std::placeholders::_1));
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::editor_menu_opening() {
  int index = d->editorContextMenu->get_item_index("undo");
  d->editorContextMenu->set_item_enabled(index, d->codeEditor->can_undo());
  index = d->editorContextMenu->get_item_index("redo");
  d->editorContextMenu->set_item_enabled(index, d->codeEditor->can_redo());
  index = d->editorContextMenu->get_item_index("cut");
  d->editorContextMenu->set_item_enabled(index, d->codeEditor->can_cut());
  index = d->editorContextMenu->get_item_index("copy");
  d->editorContextMenu->set_item_enabled(index, d->codeEditor->can_copy());
  index = d->editorContextMenu->get_item_index("paste");
  d->editorContextMenu->set_item_enabled(index, d->codeEditor->can_paste());
  index = d->editorContextMenu->get_item_index("delete");
  d->editorContextMenu->set_item_enabled(index, d->codeEditor->can_delete());
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::activate_context_menu_item(const std::string &name) {
  // Standard commands first.
  if (name == "undo")
    d->codeEditor->undo();
  else if (name == "redo")
    d->codeEditor->redo();
  else if (name == "cut")
    d->codeEditor->cut();
  else if (name == "copy")
    d->codeEditor->copy();
  else if (name == "paste")
    d->codeEditor->paste();
  else if (name == "delete")
    d->codeEditor->replace_selected_text("");
  else if (name == "select_all")
    d->codeEditor->set_selection(0, d->codeEditor->text_length());
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
          d->codeEditor->replace_selected_text(str.c_str());
        else
          d->codeEditor->set_text(str.c_str());
      }
    } else {
      logWarning("Unhandled context menu item %s", name.c_str());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::show_special_chars(bool flag) {
  d->codeEditor->set_features(mforms::FeatureShowSpecial, flag);
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::enable_word_wrap(bool flag) {
  d->codeEditor->set_features(mforms::FeatureWrapText, flag);
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::set_sql_check_enabled(bool flag) {
  if (d->isSQLCheckEnabled != flag) {
    d->isSQLCheckEnabled = flag;
    if (flag) {
      ThreadedTimer::get()->remove_task(d->currentWorkTimerID); // Does nothing if the id is -1.
      if (d->currentDelayTimer == nullptr)
        d->currentDelayTimer =
          bec::GRTManager::get()->run_every(std::bind(&MySQLEditor::start_sql_processing, this), 0.01);
    } else
      stop_processing();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::setup_auto_completion() {
  d->codeEditor->auto_completion_max_size(80, 15);

  static std::vector<std::pair<int, std::string>> ccImages = {
    {AC_KEYWORD_IMAGE, "ac_keyword.png"},
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

  d->codeEditor->auto_completion_register_images(ccImages);
  d->codeEditor->auto_completion_stops("\t,.*;) "); // Will close ac even if we are in an identifier.
  d->codeEditor->auto_completion_fillups("");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the text in the editor starting at the given position backwards until
 * the line start or the first non alphanumeric char is found.
 */
std::string MySQLEditor::getWrittenPart(size_t position) {
  ssize_t line = d->codeEditor->line_from_position(position);
  ssize_t start, stop;
  d->codeEditor->get_range_of_line(line, start, stop);
  std::string text = d->codeEditor->get_text_in_range(start, position);
  if (text.empty())
    return "";

  const char *head = text.c_str();
  const char *run = head;
  std::string lastQuotedText;

  while (*run != '\0') {
    if (*run == '\'' || *run == '"' || *run == '`') {
      // Entering a quoted text.
      head = run + 1;
      char quote_char = *run;
      while (true) {
        run = g_utf8_next_char(run);
        if (*run == quote_char || *run == '\0')
          break;

        // If there's an escape char skip it and the next char too (if we didn't
        // reach the end).
        if (*run == '\\') {
          run++;
          if (*run != '\0')
            run = g_utf8_next_char(run);
        }
      }
      if (*run == '\0') // Unfinished quoted text. Return everything.
        return head;

      lastQuotedText = std::string(head - 1, run - head); // Include the quotes or scintilla will mess up
      head = run + 1;                                     // Skip over this quoted text and start over.
    }
    run++;
  }

  // If we come here then we are outside any quoted text. Scan back for anything we consider to be a word stopper.
  // There is a special case however: if we are directly after a quoted part, this part is used as typed text
  // (treating it so as if it wasn't quoted).
  if (head == run && (*(head - 1) == '\'' || *(head - 1) == '\'' || *(head - 1) == '\''))
    return lastQuotedText;

  while (head < run--) {
    if (!std::isalnum(*run) && *run != '_' && *run != '$' && *run != '@') // Allowed parts in an unquoted identifier.
      return run + 1;
  }
  return head;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::show_auto_completion(bool auto_choose_single) {
  if (!code_completion_enabled())
    return;

  d->codeEditor->auto_completion_options(true, auto_choose_single, false, true, false);

  // Get the statement and its absolute position.
  size_t caretPosition = d->codeEditor->get_caret_pos();
  size_t caretLine = d->codeEditor->line_from_position(caretPosition);

  ssize_t lineStart, lineEnd;
  d->codeEditor->get_range_of_line(caretLine, lineStart, lineEnd);
  size_t caretOffset = caretPosition - lineStart; // This is a byte offset.

  size_t min, max;
  std::string statement;
  bool fixedCaretPos = false;
  if (get_current_statement_range(min, max, true)) {
    // If the caret is in the whitespaces before the query we would get a wrong line number
    // (because the statement splitter doesn't include these whitespaces in the determined ranges).
    // We set the caret pos to the first position in the query, which has the same effect for
    // code completion (we don't generate error line numbers).
    uint32_t codeStartLine = (uint32_t)d->codeEditor->line_from_position(min);
    if (codeStartLine > caretLine) {
      caretLine = 0;
      caretOffset = 0;
      fixedCaretPos = true;
    } else
      caretLine -= codeStartLine;

    statement = d->codeEditor->get_text_in_range(min, max);
  } else {
    // No query, means we have nothing typed yet in the current query (except whitespaces/comments).
    caretLine = 0;
    caretOffset = 0;
    fixedCaretPos = true;
  }

  // Convert current caret position into a position of the single statement.
  // The byte-based offset in the line must be converted to a character offset.
  if (!fixedCaretPos) {
    std::string line_text = d->codeEditor->get_text_in_range(lineStart, lineEnd);
    caretOffset = g_utf8_pointer_to_offset(line_text.c_str(), line_text.c_str() + caretOffset);
  }

  d->codeCompletionCandidates = d->services->getCodeCompletionCandidates(
    d->autocompletionContext, { caretOffset, caretLine }, statement, d->currentSchema, make_keywords_uppercase(),
    d->symbolTable);

  update_auto_completion(getWrittenPart(caretPosition));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Updates the auto completion list by filtering the determined entries by the
 * text the user
 * already typed. If auto completion is not yet active it becomes active here.
 * Returns the list sent to the editor for unit tests to validate them.
 */
std::vector<std::pair<int, std::string>> MySQLEditor::update_auto_completion(const std::string &typed_part) {
  logDebug2("Updating auto completion popup in editor\n");

  // Remove all entries that don't start with the typed text before showing the
  // list.
  if (!typed_part.empty()) {
    gchar *prefix = g_utf8_casefold(typed_part.c_str(), -1);

    std::vector<std::pair<int, std::string>> filteredEntries;
    for (auto &entry : d->codeCompletionCandidates) {
      gchar *folded = g_utf8_casefold(entry.second.c_str(), -1);
      if (g_str_has_prefix(folded, prefix))
        filteredEntries.push_back(entry);
      g_free(folded);
    }

    switch (filteredEntries.size()) {
      case 0:
        logDebug2("Nothing to autocomplete - hiding popup if it was active\n");
        d->codeEditor->auto_completion_cancel();
        break;
      case 1:
        // See if that single entry matches the typed part. If so we don't need
        // to show ac either.
        if (base::same_string(filteredEntries[0].second, prefix, false)) {
          logDebug2(
            "The only match is the same as the written input - hiding popup "
            "if it was active\n");
          d->codeEditor->auto_completion_cancel();
          break;
        }
      // Fall through.
      default:
        logDebug2("Showing auto completion popup\n");
        d->codeEditor->auto_completion_show(typed_part.size(), filteredEntries);
        break;
    }

    g_free(prefix);

    return filteredEntries;
  } else {
    if (!d->codeCompletionCandidates.empty()) {
      logDebug2("Showing auto completion popup\n");
      d->codeEditor->auto_completion_show(0, d->codeCompletionCandidates);
    } else {
      logDebug2("Nothing to autocomplete - hiding popup if it was active\n");
      d->codeEditor->auto_completion_cancel();
    }
  }

  return d->codeCompletionCandidates;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::cancel_auto_completion() {
  // Make sure a pending timed autocompletion won't kick in after we cancel it.
  d->lastTypedChar = 0;
  d->codeEditor->auto_completion_cancel();
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::code_completion_enabled() {
  return bec::GRTManager::get()->get_app_option_int("DbSqlEditor:CodeCompletionEnabled") == 1;
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::auto_start_code_completion() {
  return (bec::GRTManager::get()->get_app_option_int("DbSqlEditor:AutoStartCodeCompletion") == 1) &&
         (d->autocompletionContext != nullptr);
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLEditor::make_keywords_uppercase() {
  return bec::GRTManager::get()->get_app_option_int("DbSqlEditor:CodeCompletionUpperCaseKeywords") == 1;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines the start and end position of the current statement, that is, the statement where the caret is in.
 * For effective search in a large set binary search is used.
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
  RecMutexLock sql_statement_borders_mutex(d->sqlStatementBordersMutex);
  d->splitStatementsIfRequired();

  if (d->statementRanges.empty())
    return false;

  typedef std::vector<StatementRange>::iterator RangeIterator;

  size_t caret_position = d->codeEditor->get_caret_pos();
  RangeIterator low = d->statementRanges.begin();
  RangeIterator high = d->statementRanges.end() - 1;
  while (low < high) {
    RangeIterator middle = low + (high - low + 1) / 2;
    if (middle->start > caret_position)
      high = middle - 1;
    else {
      size_t end = low->start + low->length;
      if (end >= caret_position)
        break;
      low = middle;
    }
  }

  if (low == d->statementRanges.end())
    return false;

  // If we are between two statements (in white spaces) then the algorithm above
  // returns the lower one.
  if (strict) {
    if (low->start + low->length < caret_position)
      ++low;
    if (low == d->statementRanges.end())
      return false;
  }

  start = low->start;
  end = low->start + low->length;
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Stops any ongoing processing like splitting, syntax checking etc.
 */
void MySQLEditor::stop_processing() {
  d->stopProcessing = true;

  ThreadedTimer::get()->remove_task(d->currentWorkTimerID);
  d->currentWorkTimerID = -1;

  if (d->currentDelayTimer != nullptr) {
    bec::GRTManager::get()->cancel_timer(d->currentDelayTimer);
    d->currentDelayTimer = nullptr;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLEditor::focus() {
  d->codeEditor->focus();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Register a target for file drop operations which will handle these cases.
 */
void MySQLEditor::register_file_drop_for(mforms::DropDelegate *target) {
  std::vector<std::string> formats;
  formats.push_back(mforms::DragFormatFileName);
  d->codeEditor->register_drop_formats(target, formats);
}

//----------------------------------------------------------------------------------------------------------------------
