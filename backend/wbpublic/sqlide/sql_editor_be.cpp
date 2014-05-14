/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <boost/foreach.hpp>

#include "sql_editor_be.h"
#include "grtsqlparser/sql_facade.h"

#include "sqlide.h"
#include "grt/grt_manager.h"

#include "base/boost_smart_ptr_helpers.h"
#include "base/log.h"
#include "base/string_utilities.h"

#include "objimpl/db.query/db_query_QueryBuffer.h"

#include "grtui/file_charset_dialog.h"

#include "mforms/code_editor.h"
#include "mforms/find_panel.h"
#include "mforms/toolbar.h"
#include "mforms/menu.h"
#include "mforms/filechooser.h"

#include "autocomplete_object_name_cache.h"

DEFAULT_LOG_DOMAIN("Sql_editor");

using namespace bec;
using namespace grt;
using namespace base;

//--------------------------------------------------------------------------------------------------

class Sql_editor::Private
{
public:
  // ref to the GRT object representing this object
  // it will be used in Db_sql_editor queryBuffer list and in standalone
  // editors for plugin support
  db_query_QueryBufferRef _grtobj;

  db_mgmt_RdbmsRef _rdbms;
  bec::GRTManager *_grtm;

  mforms::Box* _container;
  mforms::Menu* _editor_context_menu;
  mforms::Menu* _editor_text_submenu;

  mforms::ToolBar* _toolbar;

  int _last_typed_char;

  SqlFacade::Ref _sql_facade;

  double _last_sql_check_progress_msg_timestamp;
  double _sql_check_progress_msg_throttle;

  Sql_semantic_check::Ref _sql_checker;
  GrtThreadedTask::Ref _sql_checker_task;
  base::Mutex _sql_checker_mutex;
  int _sql_checker_tag;
  bool _clearance_pending;
  bec::GRTManager::Timer* _current_timer;
  std::pair<const char*, size_t> _text_info; // Only valid during a parse run.

  struct SqlError
  {
    SqlError(int tok_lineno, int tok_line_pos, int tok_len, const std::string &msg, int tag)
      : tok_lineno(tok_lineno), tok_line_pos(tok_line_pos), tok_len(tok_len), msg(msg), tag(tag) {}
    int tok_lineno;
    int tok_line_pos;
    int tok_len;
    std::string msg;
    int tag;
  };
  typedef std::vector<SqlError> SqlErrors;
  SqlErrors _parse_error_cache; // Temporary storage used during parsing.
  base::Mutex _sql_errors_mutex;
  std::vector<std::pair<std::string, base::Range> > _parse_errors; // Permanent error storage.
  unsigned int _last_error_count;

  bool _splitting_required;
  base::Mutex _sql_statement_borders_mutex;

  // Each entry is a pair of statement position (byte position) and statement length (also bytes).
  std::vector<std::pair<size_t, size_t> > _statement_ranges;

  bool _is_refresh_enabled;   // whether FE control is permitted to replace its contents from BE
  bool _is_sql_check_enabled; // Enables automatic syntax checks.
  bool _large_content;        // True if the current content exceeds the limit we set.
  bool _owns_toolbar;

  boost::signals2::signal<void ()> _text_change_signal;

  Private(db_mgmt_RdbmsRef rdbms)
    : _grtobj(rdbms->get_grt()), _rdbms(rdbms)
  {
    _owns_toolbar = false;
  }

  /**
   * Determines ranges for all statements in the current text.
   * This function must only be called after the statement border mutex was acquired!
   */
  void split_statements_if_required()
  {
    if (_splitting_required)
    {
      log_debug3("Start splitting\n");
      _splitting_required = false;
      _statement_ranges.clear();
      double start = timestamp();
      _sql_facade->splitSqlScript(_text_info.first, _text_info.second, ";", _statement_ranges);
      log_debug3("Splitting ended after %f ticks\n", timestamp() - start);
    }
  }
};

//--------------------------------------------------------------------------------------------------

Sql_editor::Ref Sql_editor::create(db_mgmt_RdbmsRef rdbms, GrtVersionRef version, db_query_QueryBufferRef grtobj)
{
  Ref sql_editor;

  const char *def_module_name= "Sql"; // XXX: rename to SqlSupport
  std::string module_name= rdbms->name().repr() + def_module_name;
  Sql::Ref sql_module= dynamic_cast<Sql::Ref>(rdbms->get_grt()->get_module(module_name));
  if (!sql_module) // fall back to db agnostic sql module
    sql_module= dynamic_cast<Sql::Ref>(rdbms->get_grt()->get_module(def_module_name));
  if (sql_module)
    sql_editor= sql_module->getSqlEditor(rdbms, version); // XXX: remove all the module stuff, just create the editor.

  if (sql_editor)
  {
    // replace the default object with the custom one
    if (grtobj.is_valid())
      sql_editor->set_grtobj(grtobj);

    // setup the GRT object
    db_query_QueryBuffer::ImplData *data= new db_query_QueryBuffer::ImplData(sql_editor->grtobj(), sql_editor);
    sql_editor->grtobj()->set_data(data);
  }
  return sql_editor;
}


Sql_editor::Sql_editor(db_mgmt_RdbmsRef rdbms, GrtVersionRef version)
{
  d = new Private(rdbms);
  
  d->_is_refresh_enabled = true;

  d->_sql_check_progress_msg_throttle = 500;

  d->_splitting_required = false;

  d->_grtm= GRTManager::get_instance_for(rdbms->get_grt());

  d->_sql_facade = SqlFacade::instance_for_rdbms(rdbms);
  d->_sql_checker = d->_sql_facade->sqlSemanticCheck();
  d->_sql_checker->is_ast_generation_enabled(false);

  d->_sql_checker_task = GrtThreadedTask::create(d->_grtm);
  d->_sql_checker_tag = 0;
  d->_last_error_count = 0;
  d->_large_content = false;

  d->_current_timer = NULL;
  d->_is_sql_check_enabled = true;

  _code_editor = new mforms::CodeEditor(this);
  _code_editor->set_font(d->_grtm->get_app_option_string("workbench.general.Editor:Font"));
  _code_editor->set_features(mforms::FeatureUsePopup, false);
  _code_editor->set_features(mforms::FeatureConvertEolOnPaste, true);
  scoped_connect(_code_editor->signal_changed(), boost::bind(&Sql_editor::text_changed, this, _1, _2, _3, _4));
  scoped_connect(_code_editor->signal_char_added(), boost::bind(&Sql_editor::char_added, this, _1));
  scoped_connect(_code_editor->signal_dwell(), boost::bind(&Sql_editor::dwell_event, this, _1, _2, _3, _4));

  setup_auto_completion();

  d->_container = NULL;
  d->_editor_text_submenu = NULL;
  d->_editor_context_menu = NULL;
  d->_toolbar = NULL;

  _editor_config = NULL;
  _auto_completion_cache = NULL;
  d->_last_typed_char = 0;

  // Create a server version of the form "Mmmrr" as long int for quick comparisons.
  if (version.is_valid())
    _server_version = (unsigned)(version->majorNumber() * 10000 + version->minorNumber() * 100 + version->releaseNumber());
  else
    _server_version = 50501; // Assume some reasonable default (5.5.1).

  // Prepare the set of charset names for the parsers.
  grt::ListRef<db_CharacterSet> list= rdbms->characterSets();
  for (size_t i = 0; i < list->count(); i++)
    _charsets.insert(base::tolower(*list[i]->name()));

  // 3 character sets were added in version 5.5.3. Remove them from the list if the current version
  // is lower than that.
  if (_server_version < 50503)
  {
    _charsets.erase("utf8mb4");
    _charsets.erase("utf16");
    _charsets.erase("utf32");
  }
  _sql_mode = "";

  setup_editor_menu();
}

//--------------------------------------------------------------------------------------------------

Sql_editor::~Sql_editor()
{
  stop_processing();

  {
    // We lock all mutexes for a moment here to ensure no background thread is still holding them.
    MutexLock sql_checker_mutex(d->_sql_checker_mutex);
    MutexLock sql_errors_mutex(d->_sql_errors_mutex);
    MutexLock sql_statement_borders_mutex(d->_sql_statement_borders_mutex);

     d->_is_sql_check_enabled = false;
  }

  if (d->_editor_text_submenu != NULL)
    delete d->_editor_text_submenu;
  delete d->_editor_context_menu;
  if (d->_owns_toolbar)
    delete d->_toolbar;
  delete _code_editor;

  // Descendants which set _editor_config are responsible to free it.
  
  delete d;
}

//--------------------------------------------------------------------------------------------------

db_query_QueryBufferRef Sql_editor::grtobj()
{
  return d->_grtobj;
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::set_grtobj(db_query_QueryBufferRef grtobj)
{
  d->_grtobj = grtobj;
}

//--------------------------------------------------------------------------------------------------

mforms::CodeEditor* Sql_editor::get_editor_control()
{
  return _code_editor;
};

//--------------------------------------------------------------------------------------------------

static void toggle_show_special_chars(mforms::ToolBarItem *item, Sql_editor *sql_editor)
{
  sql_editor->show_special_chars(item->get_checked());
}

//--------------------------------------------------------------------------------------------------

static void toggle_word_wrap(mforms::ToolBarItem *item, Sql_editor *sql_editor)
{
  sql_editor->enable_word_wrap(item->get_checked());
}

//--------------------------------------------------------------------------------------------------

static void show_find_panel_for_active_editor(Sql_editor *sql_editor)
{
  sql_editor->get_editor_control()->show_find_panel(false);
}

//--------------------------------------------------------------------------------------------------

static void beautify_script(Sql_editor *sql_editor)
{
  grt::GRT *grt = sql_editor->grtobj()->get_grt();
  grt::BaseListRef args(grt);
  args.ginsert(sql_editor->grtobj());

  grt->call_module_function("SQLIDEUtils", "enbeautificate", args);
}

//--------------------------------------------------------------------------------------------------

static void open_file(Sql_editor *sql_editor)
{
  mforms::FileChooser fc(mforms::OpenFile);
  if (fc.run_modal())
  {
    std::string file = fc.get_path();

    gchar *contents;
    gsize length;
    GError *error = NULL;

    if (g_file_get_contents(file.c_str(), &contents, &length, &error))
    {
      char *converted;

      mforms::CodeEditor* code_editor = sql_editor->get_editor_control();
      if (FileCharsetDialog::ensure_filedata_utf8(contents, length, "", file, converted))
      {
        code_editor->set_text_keeping_state(converted ? converted : contents);
        g_free(contents);
        g_free(converted);
      }
      else
      {
        g_free(contents);
        code_editor->set_text(_("Data is not UTF8 encoded and cannot be displayed."));
      }
    }
    else if (error)
    {
      mforms::Utilities::show_error("Load File", base::strfmt("Could not load file %s:\n%s", file.c_str(), error->message),
                                    "OK");
      g_error_free(error);
    }
  }
}

//--------------------------------------------------------------------------------------------------

static void save_file(Sql_editor *sql_editor)
{
  mforms::FileChooser fc(mforms::SaveFile);
  if (fc.run_modal())
  {
    GError *error = NULL;
    std::string file = fc.get_path();
    mforms::CodeEditor* code_editor = sql_editor->get_editor_control();
    std::pair<const char*, size_t> data = code_editor->get_text_ptr();

    if (!g_file_set_contents(file.c_str(), data.first, (gssize)data.second, &error) && error)
    {
      mforms::Utilities::show_error("Save File", base::strfmt("Could not save to file %s:\n%s", file.c_str(), error->message),
                                    "OK");
      g_error_free(error);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::set_base_toolbar(mforms::ToolBar *toolbar)
{
  d->_toolbar = toolbar;
  d->_owns_toolbar = false;

  mforms::ToolBarItem *item;

  if (d->_is_sql_check_enabled)
  {
    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("query.beautify");
    item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_beautifier.png"));
    item->set_tooltip(_("Beautify/reformat the SQL script"));
    scoped_connect(item->signal_activated(), boost::bind(beautify_script, this));
    d->_toolbar->add_item(item);
  }
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.search");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_find.png"));
  item->set_tooltip(_("Show the Find panel for the editor"));
  scoped_connect(item->signal_activated(), boost::bind(show_find_panel_for_active_editor, this));
  d->_toolbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.toggleInvisible");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_special-chars-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_special-chars-off.png"));
  item->set_tooltip(_("Toggle display of invisible characters (spaces, tabs, newlines)"));
  scoped_connect(item->signal_activated(),boost::bind(toggle_show_special_chars, item, this));
  d->_toolbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.toggleWordWrap");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_word-wrap-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_word-wrap-off.png"));
  item->set_tooltip(_("Toggle wrapping of long lines (keep this off for large files)"));
  scoped_connect(item->signal_activated(),boost::bind(toggle_word_wrap, item, this));
  d->_toolbar->add_item(item);
}

//--------------------------------------------------------------------------------------------------

static void embed_find_panel(mforms::CodeEditor *editor, bool show, mforms::Box *container)
{
  mforms::View *panel = editor->get_find_panel();
  if (show)
  {
    if (!panel->get_parent())
      container->add(panel, false, true);
  }
  else
  {
    container->remove(panel);
    editor->focus();
  }
}


mforms::View* Sql_editor::get_container()
{
  if (!d->_container)
  {
    d->_container = new mforms::Box(false);

    d->_container->add(get_toolbar(), false, true);
    get_editor_control()->set_show_find_panel_callback(boost::bind(embed_find_panel, _1, _2, d->_container));
    d->_container->add_end(get_editor_control(), true, true);
  }
  return d->_container;
};


//--------------------------------------------------------------------------------------------------

mforms::ToolBar* Sql_editor::get_toolbar(bool include_file_actions)
{
  if (!d->_toolbar)
  {
    d->_owns_toolbar = true;
    d->_toolbar = new mforms::ToolBar(mforms::SecondaryToolBar);
#ifdef _WIN32
    d->_toolbar->set_size(-1, 27);
#endif
    if (include_file_actions)
    {
      mforms::ToolBarItem *item;

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("query.openFile");
      item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_open.png"));
      item->set_tooltip(_("Open a script file in this editor"));
      scoped_connect(item->signal_activated(),boost::bind(open_file, this));
      d->_toolbar->add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("query.saveFile");
      item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_save.png"));
      item->set_tooltip(_("Save the script to a file."));
      scoped_connect(item->signal_activated(),boost::bind(save_file, this));
      d->_toolbar->add_item(item);

      d->_toolbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));
      
    }
    set_base_toolbar(d->_toolbar);
  }
  return d->_toolbar;
};

//--------------------------------------------------------------------------------------------------

mforms::CodeEditorConfig* Sql_editor::get_editor_settings()
{
  return _editor_config;
}

//--------------------------------------------------------------------------------------------------

db_mgmt_RdbmsRef Sql_editor::rdbms()
{
  return d->_rdbms;
}

//--------------------------------------------------------------------------------------------------

bec::GRTManager *Sql_editor::grtm()
{
  return d->_grtm;
}

//--------------------------------------------------------------------------------------------------

bool Sql_editor::is_refresh_enabled() const
{
  return d->_is_refresh_enabled;
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::set_refresh_enabled(bool val)
{
  d->_is_refresh_enabled = val;
}

//--------------------------------------------------------------------------------------------------

bool Sql_editor::is_sql_check_enabled() const
{
  return d->_is_sql_check_enabled;
}

//--------------------------------------------------------------------------------------------------

Sql_semantic_check::Ref Sql_editor::sql_checker()
{ 
  return d->_sql_checker;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the text of the editor. Usage of this function is discouraged because it copies the
 * (potentially) large editor content. Use text_ptr() instead.
 */
std::string Sql_editor::sql()
{
  return _code_editor->get_text(false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a direct pointer to the editor content, which is only valid until the next change.
 * So if you want to keep it for longer copy the text.
 * Note: since the text can be large don't do this unless absolutely necessary.
 */
std::pair<const char*, size_t> Sql_editor::text_ptr()
{
  return _code_editor->get_text_ptr();
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::set_current_schema(const std::string &schema)
{
  _current_schema = schema;
}

//--------------------------------------------------------------------------------------------------

bool Sql_editor::empty()
{
  return _code_editor->text_length() == 0;
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::append_text(const std::string &text)
{
  _code_editor->append_text(text.data(), text.size());
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Used to the set the content of the editor from outside (usually when loading a file).
 */
void Sql_editor::sql(const char *sql)
{
  _code_editor->set_text(sql);
  _code_editor->set_eol_mode(mforms::EolLF, true);
}

//----------------------------------------------------------------------------------------------------------------------

size_t Sql_editor::cursor_pos()
{
  return _code_editor->get_caret_pos();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the caret position as column/row pair. The returned column (char index) is utf-8 save and computes
 * the actual character index as displayed in the editor, not the byte index in a std::string.
 * If @local is true then the line position is relative to the statement, otherwise that in the entire editor.
 */
std::pair<size_t, size_t> Sql_editor::cursor_pos_row_column(bool local)
{
  size_t position = _code_editor->get_caret_pos();
  ssize_t line = _code_editor->line_from_position(position);
  ssize_t line_start, line_end;
  _code_editor->get_range_of_line(line, line_start, line_end);

  ssize_t offset = position - line_start; // This is a byte offset.
  std::string line_text = _code_editor->get_text_in_range(line_start, line_end);
  offset = g_utf8_pointer_to_offset(line_text.c_str(), line_text.c_str() + offset);

  if (local)
  {
    size_t min, max;
    if (get_current_statement_range(min, max))
      line -= _code_editor->line_from_position(min);
  }
  
  return std::make_pair(offset, line);
}

//----------------------------------------------------------------------------------------------------------------------

void Sql_editor::set_cursor_pos(size_t position)
{
  _code_editor->set_caret_pos(position);
}

//----------------------------------------------------------------------------------------------------------------------

bool Sql_editor::selected_range(size_t &start, size_t &end)
{
  size_t length;
  _code_editor->get_selection(start, length);
  end = start + length;
  return length > 0;
}

void Sql_editor::set_selected_range(size_t start, size_t end)
{
  _code_editor->set_selection(start, end - start);
}

boost::signals2::signal<void ()>* Sql_editor::text_change_signal()
{
  return &d->_text_change_signal;
}

void Sql_editor::sql_mode(const std::string &value)
{
  _sql_mode = value; 
  d->_sql_checker->sql_mode(value);
}

bool Sql_editor::has_sql_errors() const
{
  return d->_last_error_count > 0;
}

//--------------------------------------------------------------------------------------------------

#define LARGE_CONTENT_SIZE 100 * 1024 * 1024

void Sql_editor::text_changed(int position, int length, int lines_changed, bool added)
{
  if (d->_current_timer != NULL)
    d->_grtm->cancel_timer(d->_current_timer);

  // Stop the parser as early as possible. It is using now an invalid char* pointer.
  d->_sql_checker->stop();
  d->_sql_facade->stop_processing();

  size_t size = _code_editor->text_length();
  if ((size > LARGE_CONTENT_SIZE) != d->_large_content)
  {
    // Size just changed from large to not large or vice versa.
    d->_large_content = size > LARGE_CONTENT_SIZE;
    _code_editor->set_features(mforms::FeatureFolding, !d->_large_content);
  }

  if (_code_editor->auto_completion_active() && !added)
  {
    // Update auto completion list if a char was removed, but not added.
    // When adding a char the caret is not yet updated leading to strange behavior.
    // So we use a different notification for adding chars.
    std::string text = get_written_part(position);
    update_auto_completion(text);
  }
  
  d->_splitting_required = true;
  d->_text_info = _code_editor->get_text_ptr();
  if (d->_is_sql_check_enabled)
    d->_current_timer = d->_grtm->run_every(boost::bind(&Sql_editor::check_sql, this, false), 0.5);
  else
    d->_text_change_signal(); // If there is no timer set up then trigger change signals directly.
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::char_added(int char_code)
{
  if (!_code_editor->auto_completion_active())
    d->_last_typed_char = char_code; // UTF32 encoded char.
  else
  {
    std::string text = get_written_part(_code_editor->get_caret_pos());
    update_auto_completion(text);
  }
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::cancel_auto_completion()
{
  // make sure a pending timed autocompletion won't kick in after we cancel it
  d->_last_typed_char = 0;
  _code_editor->auto_completion_cancel();
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::dwell_event(bool started, int position, int x, int y)
{
  if (started)
  {
    if (_code_editor->indicator_at(position) == mforms::RangeIndicatorError)
    {
      for (size_t n = 0; n < d->_parse_errors.size(); ++n)
      {
        if (d->_parse_errors[n].second.contains_point(position))
        {
          _code_editor->show_calltip(true, position, d->_parse_errors[n].first);
          break;
        }
      }
    }
  }
  else
    _code_editor->show_calltip(false, 0, "");
}

//--------------------------------------------------------------------------------------------------

/** Runs in the context of the main thread. */
bool Sql_editor::check_sql(bool sync)
{
  // Here we trigger our text change signal, to avoid frequent signals for each key press.
  // Consumers are expected to use this signal for UI updates, so we need to coalesce messages.
  d->_text_change_signal();

  d->_current_timer = NULL; // The timer will be deleted by the grt manager.

  d->_sql_checker_tag++;

  {
    MutexLock sql_errors_mutex(d->_sql_errors_mutex);
    d->_parse_error_cache.clear();
    d->_parse_errors.clear();
  }

  d->_clearance_pending = true;
  d->_last_error_count = 0;

  _code_editor->set_status_text("");
  if (d->_text_info.first != NULL && d->_text_info.second > 0)
    d->_sql_checker_task->exec(sync, boost::bind(&Sql_editor::do_check_sql, this, _1, weak_ptr_from(this)));

  return false; // Don't re-run this task, it's a single-shot.
}

//--------------------------------------------------------------------------------------------------

grt::StringRef Sql_editor::do_check_sql(grt::GRT *grt, Ptr self_ptr)
{
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR (Sql_editor, self_ptr, self, grt::StringRef(""))

  // Because the server parser returns only partial statements if there was a syntax error we do a
  // dedicated statement splitting here. It's also way faster (7-10 times) compared to the
  // server parser based splitter.
  // TODO: instead doing another splitting let the parser use the result of this dedicated run.
  {
    MutexLock sql_statement_borders_mutex(d->_sql_statement_borders_mutex);
    d->split_statements_if_required();
  }
  
  MutexLock sql_checker_mutex(d->_sql_checker_mutex);

  // Serves as flag that we should go out here immediately (when closing the editor).
  if (!d->_is_sql_check_enabled)
    return grt::StringRef("");

  // Callback binding done here to have the current checker tag included, but that doesn't seem right as there can only be one
  // callback for each task anyway, so we can never have several active with different checker tags.
  // TODO: review this callback binding.

  // Since there are no progress messages via the grt task we use the statement border reporting as such
  // instead. Should this ever change (e.g. when using the split info from above) then this needs 
  // to be reviewed.
  d->_sql_checker->report_sql_statement_border = boost::bind(&Sql_editor::on_report_sql_statement_border, this, _1, _2, _3, _4, d->_sql_checker_tag);
  d->_sql_checker->parse_error_cb(boost::bind(&Sql_editor::on_sql_error, this, _1, _2, _3, _4, d->_sql_checker_tag));

  d->_sql_checker_task->progress_cb(boost::bind(&Sql_editor::on_sql_check_progress, this, _1, _2, d->_sql_checker_tag));
  d->_sql_checker_task->finish_cb(boost::bind(&Sql_editor::on_sql_check_finished, this));

  d->_last_sql_check_progress_msg_timestamp= timestamp();

  d->_sql_checker->check_sql(d->_text_info.first);

  // to ensure there is no pending items (sql errors, statement borders) to process, send final progress message
  d->_sql_checker_task->send_progress(1, "", "");

  return grt::StringRef("");
}

//--------------------------------------------------------------------------------------------------

int Sql_editor::on_sql_error(int tok_lineno, int tok_line_pos, int tok_len, const std::string &msg, int tag)
{
  if (tag != d->_sql_checker_tag)
    return 0;
  d->_last_error_count++;
  {
    MutexLock sql_errors_mutex(d->_sql_errors_mutex);
    d->_parse_error_cache.push_back(Private::SqlError(tok_lineno, tok_line_pos, tok_len, msg, tag));
  }
  request_sql_check_results_refresh();

  return 0;
}

//--------------------------------------------------------------------------------------------------

int Sql_editor::on_report_sql_statement_border(int begin_lineno, int begin_line_pos, int end_lineno, int end_line_pos, int tag)
{
  // Our pseudo progress events...
  if (tag != d->_sql_checker_tag)
    return 0;
  request_sql_check_results_refresh();
  return 0;
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::request_sql_check_results_refresh()
{
  if (d->_last_sql_check_progress_msg_timestamp + d->_sql_check_progress_msg_throttle < timestamp())
  {
    d->_sql_checker_task->send_progress(0.f, "", "");
    d->_last_sql_check_progress_msg_timestamp = timestamp();
  }
}

//--------------------------------------------------------------------------------------------------

int Sql_editor::on_sql_check_progress(float progress, const std::string &msg, int tag)
{
  // We are now in the context of the main thread.

  if (tag != d->_sql_checker_tag)
    return 0;

  if (d->_clearance_pending)
  {
    // Remove old error markers if that wasn't done yet (don't do it earlier in the process or
    // there will be notable flashing.
    d->_clearance_pending = false;
    _code_editor->remove_indicator(mforms::RangeIndicatorError, 0, _code_editor->text_length());
    _code_editor->remove_markup(mforms::LineMarkupAll, -1);

    // Add all statement markers here too.
    {
      MutexLock sql_statement_borders_mutex(d->_sql_statement_borders_mutex);
      for (std::vector<std::pair<size_t, size_t> >::const_iterator iterator = d->_statement_ranges.begin();
        iterator != d->_statement_ranges.end(); ++iterator)
      {
        size_t line = _code_editor->line_from_position(iterator->first);
        _code_editor->show_markup(mforms::LineMarkupStatement, line);
      }
    }
  }
  
  // Trigger auto completion for alphanumeric chars, space or dot (if enabled).
  // This has to be done after our statement  splitter has completed (which is the case when we appear here).
  if (auto_start_code_completion() && !_code_editor->auto_completion_active() &&
    (g_unichar_isalnum(d->_last_typed_char)
     || d->_last_typed_char == '.'
     /* do not try to autocomplete after a space. The filtering is too weak, so we always get a keyword list that's
      too long to be useful for anything
      || d->_last_typed_char == ' '*/))
  {
    d->_last_typed_char = 0;
    show_auto_completion(false);
  }
  
  // errors
  {
    Private::SqlErrors sql_errors;
    {
      MutexLock sql_errors_mutex(d->_sql_errors_mutex);
      d->_parse_error_cache.swap(sql_errors);
    }
    
    BOOST_FOREACH (const Private::SqlError &sql_error, sql_errors)
      if (sql_error.tag == d->_sql_checker_tag)
      {
        size_t line_position = sql_error.tok_line_pos + _code_editor->position_from_line(sql_error.tok_lineno - 1);

        _code_editor->show_indicator(mforms::RangeIndicatorError, line_position, sql_error.tok_len);
        _code_editor->show_markup(mforms::LineMarkupError, sql_error.tok_lineno - 1);

        // Move errors to permanent storage (for call tips).
        base::Range range(line_position, sql_error.tok_len);
        d->_parse_errors.push_back(std::pair<std::string, base::Range>(sql_error.msg, range));
      }
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

int Sql_editor::on_sql_check_finished()
{
  if (d->_last_error_count > 0)
  {
    if (d->_last_error_count == 1)
      _code_editor->set_status_text(_("1 error found"));
    else
      _code_editor->set_status_text(base::strfmt(_("%d errors found"), d->_last_error_count));
  }
  else
    _code_editor->set_status_text("");

  return 0;
}

//--------------------------------------------------------------------------------------------------

std::string Sql_editor::selected_text()
{
  return _code_editor->get_text(true);
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::set_selected_text(const std::string &new_text)
{
  _code_editor->replace_selected_text(new_text);
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::insert_text(const std::string &new_text)
{
  _code_editor->clear_selection();
  _code_editor->replace_selected_text(new_text);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the statement at the current caret position.
 */
std::string Sql_editor::current_statement()
{
  size_t min, max;
  if (get_current_statement_range(min, max))
    return _code_editor->get_text_in_range(min, max);
  return "";
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::sql_check_progress_msg_throttle(double val)
{
  d->_sql_check_progress_msg_throttle = val;
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::setup_editor_menu()
{
  d->_editor_context_menu = new mforms::Menu();
  scoped_connect(d->_editor_context_menu->signal_will_show(), boost::bind(&Sql_editor::editor_menu_opening, this));
  
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
    
    bec::MenuItemList plugin_items= grtm()->get_plugin_context_menu_items(groups, argpool);
    
    if (!plugin_items.empty())
    {
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
  plugin_items = grtm()->get_plugin_context_menu_items(groups, argpool);  
  if (!plugin_items.empty())
  {
    d->_editor_context_menu->add_separator();
    
    d->_editor_text_submenu = new mforms::Menu();
    d->_editor_text_submenu->add_items_from_list(plugin_items);
    d->_editor_context_menu->add_submenu(_("Text"), d->_editor_text_submenu);
  }
  _code_editor->set_context_menu(d->_editor_context_menu);
  scoped_connect(d->_editor_context_menu->signal_on_action(), boost::bind(&Sql_editor::activate_context_menu_item, this, _1));
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::editor_menu_opening()
{
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

void Sql_editor::activate_context_menu_item(const std::string &name)
{
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
  else
  {
    std::vector<std::string> parts= base::split(name, ":", 1);
    if (parts.size() == 2 && parts[0] == "plugin")
    {
      app_PluginRef plugin(grtm()->get_plugin_manager()->get_plugin(parts[1]));

      if (!plugin.is_valid())
        throw std::runtime_error("Invalid plugin "+name);

      bec::ArgumentPool argpool;
      argpool.add_entries_for_object("activeQueryBuffer", grtobj());
      argpool.add_entries_for_object("", grtobj());

      bool input_was_selection= false;
      if (bec::ArgumentPool::needs_simple_input(plugin, "selectedText"))
      {
        argpool.add_simple_value("selectedText", grt::StringRef(selected_text()));
        input_was_selection= true;
      }

      if (bec::ArgumentPool::needs_simple_input(plugin, "document"))
        argpool.add_simple_value("document", grt::StringRef(sql()));

      bool is_filter= false;
      if (plugin->groups().get_index("Filter") != grt::BaseListRef::npos)
        is_filter= true;

      grt::BaseListRef fargs(argpool.build_argument_list(plugin));

      grt::ValueRef result= grtm()->get_plugin_manager()->execute_plugin_function(plugin, fargs);

      if (is_filter)
      {
        if (!result.is_valid() || !grt::StringRef::can_wrap(result))
          throw std::runtime_error(base::strfmt("plugin %s returned unexpected value", plugin->name().c_str()));

        grt::StringRef str(grt::StringRef::cast_from(result));
        if (input_was_selection)
          _code_editor->replace_selected_text(str.c_str());
        else
          _code_editor->set_text(str.c_str());
      }
    }
    else {
      log_warning("Unhandled context menu item %s", name.c_str());
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::show_special_chars(bool flag)
{
  _code_editor->set_features(mforms::FeatureShowSpecial, flag);
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::enable_word_wrap(bool flag)
{
  _code_editor->set_features(mforms::FeatureWrapText, flag);
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::set_sql_check_enabled(bool flag)
{
  if (d->_is_sql_check_enabled != flag)
  {
    d->_is_sql_check_enabled = flag;
    if (flag)
    {
      if (d->_current_timer == NULL)
        d->_current_timer = d->_grtm->run_every(boost::bind(&Sql_editor::check_sql, this, false), 0.5);
    }
    else
    {
      if (d->_current_timer != NULL)
      {
        d->_grtm->cancel_timer(d->_current_timer);
        d->_current_timer = NULL;
      }
      d->_sql_checker->stop();
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool Sql_editor::code_completion_enabled()
{
  return d->_grtm->get_app_option_int("DbSqlEditor:CodeCompletionEnabled") == 1;
}

//--------------------------------------------------------------------------------------------------

bool Sql_editor::auto_start_code_completion()
{
  return d->_grtm->get_app_option_int("DbSqlEditor:AutoStartCodeCompletion") == 1;
}

//--------------------------------------------------------------------------------------------------

bool Sql_editor::make_keywords_uppercase()
{
  return d->_grtm->get_app_option_int("DbSqlEditor:CodeCompletionUpperCaseKeywords") == 1;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the start and end position of the current statement, that is, the statement
 * where the caret is in. For effective search in a large set binary search is used.
 * Returns true if a statement could be found at the caret position, otherwise false.
 */
bool Sql_editor::get_current_statement_range(size_t &start, size_t &end)
{
  // In case the splitter is right now processing the text we wait here until its done.
  // If the splitter wasn't triggered yet (e.g. when typing fast and then immediately running a statement)
  // then we do the splitting here instead.
  MutexLock sql_statement_borders_mutex(d->_sql_statement_borders_mutex);
  d->split_statements_if_required();

  if (d->_statement_ranges.empty())
    return false;

  typedef std::vector<std::pair<size_t, size_t> >::iterator RangeIterator;

  size_t caret_position = _code_editor->get_caret_pos();
  RangeIterator low = d->_statement_ranges.begin();
  RangeIterator high = d->_statement_ranges.end() - 1;
  while (low < high)
  {
    RangeIterator middle = low + (high - low + 1) / 2;
    if (middle->first > caret_position)
      high = middle - 1;
    else
    {
      size_t end = low->first + low->second;
      if (end >= caret_position)
        break;
      low = middle;
    }
  }

  if (low == d->_statement_ranges.end())
    return false;

  // If we are between two statements (in white spaces) then the algorithm above returns the lower one.
  // For this special case however we want to pretend this belongs to the following query -
  // with one exception: if this is already the last statement then use it anyway.

  // TODO: some users prefer to get the previous statement in this case.
  // So for now we return this. We might later make this switchable (loose vs strict mode).
  /*
  bool is_last_statement = (low + 1) == d->_statement_ranges.end();
  if (!is_last_statement)
  {
    if (low->first + low->second < caret_position)
      ++low;
    if (low == d->_statement_ranges.end())
      return false;
  }
  */

  // Similar for the very first entry. If we are in the white space before this then we still get
  // the first entry (which is then after our caret position).
  /*
   TODO: this code is only needed if the out-commented code above is reactivated.
  if (is_last_statement || low->first > caret_position
    || (low->first <= caret_position && caret_position <= low->first + low->second))
  */
  {
    start = low->first;
    end = low->first + low->second;
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Stops any ongoing processing like splitting, syntax checking etc.
 */
void Sql_editor::stop_processing()
{
  d->_sql_checker_tag++; // Makes all event handlers ignore any dangling calls from the worker task.

  if (d->_current_timer != NULL)
  {
    d->_grtm->cancel_timer(d->_current_timer);
    d->_current_timer = NULL;
  }
  
  d->_sql_checker->stop();
  d->_sql_facade->stop_processing();

  // Remove any callback binding or our reference won't be released and hence the editor
  // instance is not freed in time.
  d->_sql_checker->report_sql_statement_border = Sql_parser_base::Report_sql_statement_border();
  d->_sql_checker->parse_error_cb(Sql_parser_base::Parse_error_cb());

  d->_sql_checker_task->progress_cb(GrtThreadedTask::Progress_cb());
  d->_sql_checker_task->finish_cb(GrtThreadedTask::Finish_cb());
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::focus()
{
  _code_editor->focus();
}

//--------------------------------------------------------------------------------------------------

/**
 * Register a target for file drop operations which will handle these cases.
 */
void Sql_editor::register_file_drop_for(mforms::DropDelegate *target)
{
  std::vector<std::string> formats;
  formats.push_back(mforms::DragFormatFileName);
  _code_editor->register_drop_formats(target, formats);
}

//--------------------------------------------------------------------------------------------------
