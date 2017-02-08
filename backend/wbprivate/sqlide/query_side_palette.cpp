/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <boost/signals2/signal.hpp>
#include <pcrecpp.h>

#include "wb_sql_editor_form.h"
#include "wb_sql_editor_panel.h"
#include "query_side_palette.h"

#include "wb_sql_editor_snippets.h"
#include "wb_sql_editor_help.h"
#include "grt/tree_model.h"
#include "snippet_popover.h"
#include "snippet_list.h"
#include "grt/grt_manager.h"

#include "base/geometry.h"
#include "base/notifications.h"
#include "base/drawing.h"
#include "base/log.h"

#include "mforms/toolbar.h"
#include "mforms/hypertext.h"
#include "mforms/scrollpanel.h"
#include "mforms/drawbox.h"
#include "mforms/utilities.h"
#include "mforms/code_editor.h"

DEFAULT_LOG_DOMAIN("QuerySidebar");

using namespace mforms;
using namespace base;

//----------------- SnippetListView --------------------------------------------------------------------

/**
  * Internal class containing a list of snippet entries in a scrollable list.
  */
class SnippetListView : public BaseSnippetList {
private:
  friend class QuerySidePalette;

  wb::SnippetPopover *_snippet_popover;
  bool _user_snippets_active;
  bool _shared_snippets_active;

private:
  DbSqlEditorSnippets *model() {
    return dynamic_cast<DbSqlEditorSnippets *>(_model);
  }

  void popover_closed() {
    if (_snippet_popover->has_changed()) {
      std::string title = _snippet_popover->get_heading();
      model()->set_field(bec::NodeId(_selected_index), DbSqlEditorSnippets::Description, title);
      std::string sub_title = _snippet_popover->get_text();
      model()->set_field(bec::NodeId(_selected_index), DbSqlEditorSnippets::Script, sub_title);
      if (_selected_snippet)
        set_snippet_info(_selected_snippet, title, sub_title);
      model()->save();

      set_needs_repaint();
    }
  }

  //------------------------------------------------------------------------------------------------

  void prepare_context_menu() {
    _context_menu = manage(new Menu());
    _context_menu->set_handler(std::bind(&SnippetListView::on_action, this, std::placeholders::_1));
    _context_menu->signal_will_show()->connect(std::bind(&SnippetListView::menu_will_show, this));

    _context_menu->add_item(_("Insert Snippet at Cursor"), "insert_text");
    _context_menu->add_item(_("Replace Editor Content with Snippet"), "replace_text");
    _context_menu->add_item(_("Execute Snippet"), "exec_snippet");
    _context_menu->add_separator();
    _context_menu->add_item(_("Copy Snippet to Clipboard"), "copy_to_clipboard");
    _context_menu->add_separator();
    _context_menu->add_item(_("Edit Snippet"), "edit_snippet");
    _context_menu->add_item(_("Add Snippet from Editor Content"), "add_snippet");
    _context_menu->add_item(_("Delete Snippet"), "del_snippet");
    _context_menu->add_separator();
    _context_menu->add_item(_("Restore Original Snippet List"), "restore_snippets");
  }

  //------------------------------------------------------------------------------------------------

  void menu_will_show() {
    bool shared_usable = model()->shared_snippets_usable();

    _context_menu->set_item_enabled(0, _selected_index > -1);
    _context_menu->set_item_enabled(1, _selected_index > -1);
    _context_menu->set_item_enabled(2, _selected_index > -1);
    _context_menu->set_item_enabled(4, _selected_index > -1);
    _context_menu->set_item_enabled(6, _selected_index > -1 && (!_shared_snippets_active || shared_usable));
    _context_menu->set_item_enabled(7, (!_shared_snippets_active || shared_usable));
    _context_menu->set_item_enabled(8, _selected_index > -1 && (!_shared_snippets_active || shared_usable));
    _context_menu->set_item_enabled(10, !_user_snippets_active && !_shared_snippets_active);
  }

  //------------------------------------------------------------------------------------------------

  void on_action(const std::string &action) {
    if (action == "edit_snippet") {
      if (_selected_snippet)
        edit_snippet(_selected_snippet);
    } else
      model()->activate_toolbar_item(bec::NodeId(_selected_index), action);

    // Refresh display if we added or removed a snippet.
    if (action == "add_snippet" || action == "del_snippet" || action == "restore_snippets")
      refresh_snippets();
  }

  //------------------------------------------------------------------------------------------------

public:
  SnippetListView(const std::string &icon_name) : BaseSnippetList(icon_name, DbSqlEditorSnippets::get_instance()) {
    _user_snippets_active = false;
    _shared_snippets_active = false;

    _snippet_popover = new wb::SnippetPopover();
    _snippet_popover->set_size(376, 257);
    _snippet_popover->signal_closed()->connect(std::bind(&SnippetListView::popover_closed, this));

    prepare_context_menu();
  }

  //------------------------------------------------------------------------------------------------

  ~SnippetListView() {
    delete _snippet_popover;
    _context_menu->release();
  }

  //------------------------------------------------------------------------------------------------

  void edit_new_snippet() {
    if (!_snippets.empty()) {
      _selected_index = (int)_snippets.size() - 1;
      _selected_snippet = _snippets.back();
      edit_snippet(_selected_snippet);
      _snippet_popover->set_read_only(false);
    }
  }

  //------------------------------------------------------------------------------------------------

  std::string selected_category() {
    return model()->selected_category();
  }

  bool shared_snippets_active() const {
    return _shared_snippets_active;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Updates the content depending on the selected snippet group.
   */
  void show_category(std::string category) {
    _user_snippets_active = (category == USER_SNIPPETS);
    _shared_snippets_active = (category == SHARED_SNIPPETS);
    try {
      model()->select_category(category);
    } catch (std::exception &exc) {
      logWarning("Error switching snippet category: %s\n", exc.what());
    }
    refresh_snippets();
  }

  //------------------------------------------------------------------------------------------------

  void edit_snippet(Snippet *snippet) {
    base::Rect bounds = snippet_bounds(snippet);

    std::pair<int, int> left_top = client_to_screen((int)bounds.left(), (int)bounds.top());
    std::pair<int, int> bottom = client_to_screen(0, (int)bounds.bottom());
    left_top.second = (left_top.second + bottom.second) / 2;

    std::string title, description;
    get_snippet_info(snippet, title, description);

    _snippet_popover->set_heading(title);
    _snippet_popover->set_read_only(false);
    _snippet_popover->set_text(description);
    _snippet_popover->set_read_only(true);
    _snippet_popover->show(left_top.first, left_top.second, mforms::StartLeft);
  }

  //------------------------------------------------------------------------------------------------

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y) {
    bool result = BaseSnippetList::mouse_double_click(button, x, y);

    if (!result) {
      if (button == MouseButtonLeft) {
        Snippet *snippet = snippet_from_point(x, y);
        if (snippet != NULL && snippet == _selected_snippet) {
          edit_snippet(snippet);
          result = true;
        }
      }
    }
    return result;
  }

  //------------------------------------------------------------------------------------------------

  void close_popover() {
    _snippet_popover->close();
  }

  //------------------------------------------------------------------------------------------------

  // End of internal class SnippetListView.
};

//----------------- QuerySidePalette ---------------------------------------------------------------

QuerySidePalette::QuerySidePalette(const SqlEditorForm::Ref &owner)
  :
#ifdef _WIN32
    TabView(mforms::TabViewPalette),
#else
    TabView(mforms::TabViewSelectorSecondary),
#endif
    _owner(owner) {
  _help_timer = NULL;
  _no_help = true;
  _automatic_help = bec::GRTManager::get()->get_app_option_int("DbSqlEditor:DisableAutomaticContextHelp", 1) == 0;
  _switching_help = false;
  _help_task = GrtThreadedTask::create();
  _help_task->desc("Context Help Task");

  _pending_snippets_refresh = true;

  mforms::Box *help_page = manage(new Box(false));
  _help_toolbar = prepare_help_toolbar();
  _help_text = manage(new mforms::HyperText());

  // Separate box since we need a border around the content, but not the toolbar.
  Box *content_border = manage(new Box(false));

  // No scoped_connect needed since _help_text is a member variable.
  scoped_connect(_help_text->signal_link_click(),
                 std::bind(&QuerySidePalette::click_link, this, std::placeholders::_1));

#if _WIN32
  _help_text->set_back_color(base::Color::get_application_color_as_string(AppColorPanelContentArea, false));
  _help_text->set_font("Tahoma 8");
  content_border->set_padding(3, 3, 3, 3);
#else
  _help_text->set_back_color("#ebebeb");
#endif

  _help_text->set_markup_text("");
  _current_topic_index = -1;

  help_page->add(_help_toolbar, false, true);
  content_border->add(_help_text, true, true);
  help_page->add(content_border, true, true);
  add_page(help_page, _("Context Help"));

  Box *snippet_page = manage(new Box(false));

  content_border = manage(new Box(false));
  _snippet_list = manage(new SnippetListView("snippet_sql.png"));
#ifdef _WIN32
  content_border->set_padding(3, 3, 3, 3);
  _snippet_list->set_back_color(base::Color::get_application_color_as_string(AppColorPanelContentArea, false));
#else
  _snippet_list->set_back_color("#f2f2f2");
#endif
  _snippet_box = manage(new ScrollPanel());
  _snippet_box->add(_snippet_list);

  DbSqlEditorSnippets *snippets_model = DbSqlEditorSnippets::get_instance();
  std::vector<std::string> snippet_categories = snippets_model->get_category_list();
  if (snippet_categories.size() > 0)
    _snippet_list->show_category(snippet_categories[0]);
  else
    _snippet_list->show_category(USER_SNIPPETS);

  _snippet_toolbar = prepare_snippet_toolbar();

  snippet_page->add(_snippet_toolbar, false, true);
  content_border->add(_snippet_box, true, true);
  snippet_page->add(content_border, true, true);
  add_page(snippet_page, "Snippets");

  scoped_connect(_snippet_list->signal_selection_changed(),
                 std::bind(&QuerySidePalette::snippet_selection_changed, this));

  std::string old_category = bec::GRTManager::get()->get_app_option_string("DbSqlEditor:SelectedSnippetCategory");
  if (!old_category.empty()) {
    mforms::ToolBarItem *item = _snippet_toolbar->find_item("select_category");
    item->set_text(old_category);
    snippet_toolbar_item_activated(item);
  }

  snippet_selection_changed();
  show_help_hint_or_update();

  base::NotificationCenter::get()->add_observer(this, "GNTextSelectionChanged");
}

//--------------------------------------------------------------------------------------------------

QuerySidePalette::~QuerySidePalette() {
  base::NotificationCenter::get()->remove_observer(this);

  cancel_timer();
  if (_help_task->is_busy() && _help_task->task())
    _help_task->task()->cancel();
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::cancel_timer() {
  if (_help_timer != NULL)
    bec::GRTManager::get()->cancel_timer(_help_timer);
}

//--------------------------------------------------------------------------------------------------

static bool contains_editor(SqlEditorForm::Ref form, MySQLEditor *ed) {
  for (int c = form->sql_editor_count(), i = 0; i < c; i++) {
    SqlEditorPanel *panel = form->sql_editor_panel(i);
    if (panel && panel->editor_be().get() == ed)
      return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  // Selection and caret changes notification.
  // Only act if this side palette is actually visible.
  if ((name == "GNTextSelectionChanged") && _automatic_help && (get_active_tab() == 0) && is_fully_visible()) {
    mforms::Object *object = (mforms::Object *)sender;
    mforms::CodeEditor *code_editor = dynamic_cast<mforms::CodeEditor *>(object);
    if (code_editor == NULL)
      return;

    MySQLEditor *editor = static_cast<MySQLEditor *>(code_editor->get_host());
    if (editor != NULL && editor->grtobj().is_valid()) {
      // See if this editor instance is actually from the IDE this palette sits in.
      SqlEditorForm::Ref form = _owner.lock();
      if (form && contains_editor(form, editor)) {
        check_format_structures(editor);
        cancel_timer();
        _help_timer =
          bec::GRTManager::get()->run_every(std::bind(&QuerySidePalette::find_context_help, this, editor), 0.7);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::show_help_text_for_topic(const std::string &topic) {
  std::string title, text, html_text;

  if (topic.empty()) {
    _last_topic = "";
    update_help_ui();
    return;
  }

  std::string topic_upper = base::toupper(topic);
  if (_topic_cache.find(topic_upper) != _topic_cache.end()) {
    std::pair<std::string, std::string> entry = _topic_cache[topic_upper];
    html_text = entry.second;
    _help_text->set_markup_text(html_text);
    _no_help = false;

    return;
  }

  // Add a new task to retrieve the actual help text in the background, but only
  // if the help task wasn't canceled, which indicates we are probably going down right now.
  if (_help_task->task() && _help_task->task()->is_cancelled())
    return;

  _last_topic = topic_upper;
  _help_task->exec(false, std::bind(&QuerySidePalette::get_help_text_threaded, this));
}

//--------------------------------------------------------------------------------------------------

/**
 * Runs in the background to find the text for the last stored topic. Never called if there's no topic.
 */
grt::StringRef QuerySidePalette::get_help_text_threaded() {
  SqlEditorForm::Ref form = _owner.lock();
  if (!form)
    return "";

  std::string title, text, html_text;

  if (DbSqlEditorContextHelp::get_help_text(form, _last_topic, title, text)) {
#ifdef _WIN32
    std::string additional_space = "<div style=\"font-size:4pt\"> </div>";
#else
    std::string additional_space;
#endif
    html_text = std::string("<html><body style=\"font-family:") + DEFAULT_FONT_FAMILY +
                "; font-size: 8pt\">"
                "<b style=\"font-size: 11pt\">Topic: <span style=\"color:#688b5e\">" +
                title + "</span></b><br>" + additional_space + format_help_as_html(text) + "</body></html>";
    _no_help = false;
    _topic_cache[_last_topic] = std::make_pair(text, html_text);
  } else {
    _no_help = true;
    _last_topic = "";
  }

  // At this point the previous task has already finished (it's the one that triggered this function).
  _help_task->execute_in_main_thread(std::bind(&QuerySidePalette::update_help_ui, this), false, false);

  return "";
}

//--------------------------------------------------------------------------------------------------

/**
 * Runs on the main thread for updating UI elements. Called by the help background task.
 */
void QuerySidePalette::update_help_ui() {
  if (_last_topic.empty()) {
    _help_text->set_markup_text(std::string("<hmtl><body style=\"font-family:") + DEFAULT_FONT_FAMILY +
                                "; font-size: 8pt\">"
                                "<div style=\"width: 100%\"><b style=\"font-size: 10pt; color:#B0B0B0\">"
                                "No Context Help<b><br><br><hr></div></body></html>");
  } else {
    _help_text->set_markup_text(_topic_cache[_last_topic].second);

    _switching_help = true;
    _quick_jump_item->set_text(_last_topic);
    _switching_help = false;
  }
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::show_help_hint_or_update() {
  if (!_automatic_help) {
    _help_text->set_markup_text(
      std::string("<hmtl><body style=\"font-family:") + DEFAULT_FONT_FAMILY +
      "; font-size: 8pt\">"
      "<div style=\"width: 100%\"><b style=\"font-size: 10pt; color:#B0B0B0\">"
      "Automatic context help is disabled. Use the toolbar to manually get help for the current caret "
      "position or to toggle automatic help.<b><br><br><hr></div></body></html>");
  } else {
    if (_current_topic_index > 0)
      _last_topic = _topic_history[_current_topic_index]; // Restore the last displayed topic.
    update_help_ui();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered by timer or manually to find a help topic from the given editor's text + position.
 */
bool QuerySidePalette::find_context_help(MySQLEditor *editor) {
  _help_timer = NULL;

  if (_help_task->is_busy())
    return false;

  // If no editor was given use the currently active one in the SQL editor form.
  if (editor == NULL) {
    SqlEditorForm::Ref form = _owner.lock();
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
      editor = panel->editor_be().get();
    else
      return false;
  }

  // Caret position as <column, row>.
  std::pair<size_t, size_t> caret = editor->cursor_pos_row_column(true);

  _help_task->exec(false,
                   std::bind(&QuerySidePalette::get_help_topic_threaded, this, editor->current_statement(), caret));

  return false; // Don't re-run this task, it's a single-shot.
}

//--------------------------------------------------------------------------------------------------

/**
 * Get a help topic from the current editor position. This might involve server access already
 * to resolve ambiguities, hence it is a threaded task. Once we got the topic we run another task
 * to get the actual help text from it.
 */
grt::StringRef QuerySidePalette::get_help_topic_threaded(const std::string &query, std::pair<ssize_t, ssize_t> caret) {
  SqlEditorForm::Ref form = _owner.lock();
  if (!form)
    return "";

  std::string topic = DbSqlEditorContextHelp::find_help_topic_from_position(form, query, caret);

  if (!_help_task->task()->is_cancelled())
    _help_task->execute_in_main_thread(std::bind(&QuerySidePalette::process_help_topic, this, topic), false, false);

  return "";
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::process_help_topic(const std::string &topic) {
  update_help_history(topic);
  show_help_text_for_topic(topic);
}

//--------------------------------------------------------------------------------------------------

/**
 * Adds the given topic to the topic history if the current topic is not the same and updates
 * the forward/backward buttons.
 */
void QuerySidePalette::update_help_history(const std::string &topic) {
  std::string topic_upper = base::toupper(topic);
  if (_current_topic_index > 0 && _topic_history[_current_topic_index] == topic_upper)
    return;

  if (!topic.empty()) {
    _current_topic_index++;

    // Remove everything after the new topic position.
    _topic_history.erase(_topic_history.begin() + _current_topic_index, _topic_history.end());
    _topic_history.push_back(topic_upper);
    _back_item->set_enabled(_current_topic_index > 0);
    _forward_item->set_enabled(false);
  }
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::click_link(const std::string &link) {
  if (link.find("local:") == 0) {
    // Internal link.
    std::string topic = base::trim(link.substr(6, link.size() - 6));
    base::replaceStringInplace(topic, "%20", " ");
    while (topic.find("  ") != std::string::npos)
      base::replaceStringInplace(topic, "  ", " ");

    update_help_history(topic);
    show_help_text_for_topic(topic);
  } else
    mforms::Utilities::open_url(link);
}

//--------------------------------------------------------------------------------------------------

ToolBar *QuerySidePalette::prepare_snippet_toolbar() {
  ToolBar *toolbar = manage(new ToolBar(mforms::SecondaryToolBar));
  toolbar->set_name("snippet_toolbar");
#ifndef __APPLE__
  toolbar->set_padding(0, 0, 0, 0);
  toolbar->set_size(-1, 27);
#endif
  ToolBarItem *item;

  item = mforms::manage(new ToolBarItem(mforms::SelectorItem));
  item->set_name("select_category");

  DbSqlEditorSnippets *snippets_model = DbSqlEditorSnippets::get_instance();
  item->set_selector_items(snippets_model->get_category_list());
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::snippet_toolbar_item_activated, this, std::placeholders::_1));
  item->set_text(USER_SNIPPETS);
  item->set_tooltip(_("Select a snippet category for display"));
  toolbar->add_item(item);

  item = mforms::manage(new ToolBarItem(mforms::SeparatorItem));
  toolbar->add_item(item);

  item = mforms::manage(new ToolBarItem(mforms::ActionItem));
  item->set_name("replace_text");
  item->set_icon(App::get()->get_resource_path("snippet_use.png"));
  item->set_tooltip(_("Replace the current text by this snippet"));
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::snippet_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  item = mforms::manage(new ToolBarItem(mforms::ActionItem));
  item->set_name("insert_text");
  item->set_icon(App::get()->get_resource_path("snippet_insert.png"));
  item->set_tooltip(_("Insert the snippet text at the current caret position replacing selected text if there is any"));
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::snippet_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  item = manage(new ToolBarItem(mforms::ActionItem));
  item->set_name("copy_to_clipboard");
  item->set_icon(App::get()->get_resource_path("snippet_clipboard.png"));
  item->set_tooltip(_("Copy the snippet text to the clipboard"));
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::snippet_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  return toolbar;
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::snippet_toolbar_item_activated(ToolBarItem *item) {
  std::string action = item->get_name();
  if (action == "select_category") {
    _snippet_list->show_category(item->get_text());
    bec::GRTManager::get()->set_app_option("DbSqlEditor:SelectedSnippetCategory", grt::StringRef(item->get_text()));
  } else {
    DbSqlEditorSnippets *snippets_model = DbSqlEditorSnippets::get_instance();
    snippets_model->activate_toolbar_item(bec::NodeId(_snippet_list->selected_index()), action);

    // Refresh display if we added or removed a snippet.
    if (action == "add_snippet" || action == "del_snippet")
      _snippet_list->refresh_snippets();
  }
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::snippet_selection_changed() {
  bool has_selection = _snippet_list->selected_index() > -1;
  //  bool user_snippets_active = _snippet_list->selected_category() == USER_SNIPPETS;
  // _snippet_toolbar->set_item_enabled("add_snippet", true);
  _snippet_toolbar->set_item_enabled("del_snippet", has_selection);
  _snippet_toolbar->set_item_enabled("copy_to_clipboard", has_selection);
  _snippet_toolbar->set_item_enabled("replace_text", has_selection);
  _snippet_toolbar->set_item_enabled("insert_text", has_selection);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the owning form if the main form changes to remove the popover.
 */
void QuerySidePalette::close_popover() {
  _snippet_list->close_popover();
}

//--------------------------------------------------------------------------------------------------

ToolBar *QuerySidePalette::prepare_help_toolbar() {
  ToolBar *toolbar = manage(new ToolBar(mforms::SecondaryToolBar));
  toolbar->set_name("help_toolbar");
#ifndef __APPLE__
  toolbar->set_padding(0, 0, 0, 0);
  toolbar->set_size(-1, 27);
#endif

  _back_item = manage(new ToolBarItem(mforms::ActionItem));
  _back_item->set_name("back");
  _back_item->set_icon(App::get()->get_resource_path("wb-toolbar_nav-back.png"));
  _back_item->set_tooltip(_("One topic back"));
  _back_item->set_enabled(false);
  scoped_connect(_back_item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(_back_item);

  _forward_item = manage(new ToolBarItem(mforms::ActionItem));
  _forward_item->set_name("forward");
  _forward_item->set_icon(App::get()->get_resource_path("wb-toolbar_nav-forward.png"));
  _forward_item->set_tooltip(_("One topic forward"));
  _forward_item->set_enabled(false);
  scoped_connect(_forward_item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(_forward_item);

  toolbar->add_item(manage(new ToolBarItem(mforms::SeparatorItem)));

  ToolBarItem *item = manage(new ToolBarItem(mforms::ToggleItem));
  item->set_name("toggle-auto-context-help");
  item->set_icon(App::get()->get_resource_path("wb-toolbar_automatic-help-off.png"));
  item->set_alt_icon(App::get()->get_resource_path("wb-toolbar_automatic-help-on.png"));
  item->set_tooltip(_("Toggle automatic context help"));
  item->set_checked(_automatic_help);
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  _manual_help_item = manage(new ToolBarItem(mforms::ActionItem));
  _manual_help_item->set_name("manual-context-help");
  _manual_help_item->set_icon(App::get()->get_resource_path("wb-toolbar_manual-help.png"));
  _manual_help_item->set_tooltip(_("Get context help for the item at the current caret position"));
  _manual_help_item->set_enabled(!_automatic_help);
  scoped_connect(_manual_help_item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(_manual_help_item);

  toolbar->add_item(manage(new ToolBarItem(mforms::SeparatorItem)));

  _quick_jump_item = manage(new ToolBarItem(mforms::SelectorItem));
  _quick_jump_item->set_name("quick_jump");

  std::vector<std::string> topic_entries;
  topic_entries.push_back(_("Jump to"));
  topic_entries.push_back("SELECT");
  topic_entries.push_back("UPDATE");
  topic_entries.push_back("INSERT");
  topic_entries.push_back("DELETE");
  topic_entries.push_back("CREATE TABLE");
  topic_entries.push_back("CREATE VIEW");
  topic_entries.push_back("CREATE PROCEDURE");
  topic_entries.push_back("CREATE FUNCTION");
  topic_entries.push_back("ALTER TABLE");
  _quick_jump_item->set_selector_items(topic_entries);
  _quick_jump_item->set_text(_("Jump To"));
  scoped_connect(_quick_jump_item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(_quick_jump_item);

  return toolbar;
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::help_toolbar_item_activated(ToolBarItem *item) {
  if (_switching_help)
    return;

  std::string action = item->get_name();
  if (action == "back" && _current_topic_index > 0) {
    std::string topic = _topic_history[--_current_topic_index];
    _back_item->set_enabled(_current_topic_index > 0);
    _forward_item->set_enabled(true);
    show_help_text_for_topic(topic);
    return;
  }

  if (action == "forward" && _current_topic_index < (int)_topic_history.size() - 1) {
    std::string topic = _topic_history[++_current_topic_index];
    _back_item->set_enabled(true);
    _forward_item->set_enabled(_current_topic_index < (int)_topic_history.size() - 1);
    show_help_text_for_topic(topic);
    return;
  }

  if (action == "quick_jump") {
    std::string topic = _quick_jump_item->get_text();
    update_help_history(topic);
    show_help_text_for_topic(topic);
    return;
  }

  if (action == "toggle-auto-context-help") {
    _automatic_help = item->get_checked();
    _manual_help_item->set_enabled(!_automatic_help);
    bec::GRTManager::get()->set_app_option("DbSqlEditor:DisableAutomaticContextHelp",
                                           grt::IntegerRef(_automatic_help ? 0 : 1));

    show_help_hint_or_update();

    return;
  }

  if (action == "manual-context-help")
    find_context_help(NULL);

  if (_current_topic_index >= 0) {
    if (action == "copy_to_clipboard") {
      std::pair<std::string, std::string> entry = _topic_cache[_topic_history[_current_topic_index]];
      mforms::Utilities::set_clipboard_text(entry.first);
    }
    if (action == "copy_html_to_clipboard") {
      std::pair<std::string, std::string> entry = _topic_cache[_topic_history[_current_topic_index]];
      mforms::Utilities::set_clipboard_text(entry.second);
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool compare_lengths(const std::pair<std::string, std::string> &l, const std::pair<std::string, std::string> &r) {
  return l.first.size() > r.first.size();
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::check_format_structures(MySQLEditor *editor) {
  mforms::CodeEditorConfig *config = editor->get_editor_settings();

  if (_keyword_color.empty()) {
    // We use the major keyword color for all keywords in the help. Just to simplify things.
    std::map<std::string, std::string> major_style = config->get_styles()[7]; // Major keyword.
    _keyword_color = major_style["fore-color"];
    if (_keyword_color.empty())
      _keyword_color = "#000000";
  }

  // Initialize pattern -> color associations if not yet done.
  // We don't consider background colors atm.
  if (_pattern_and_colors.empty()) {
    std::map<int, std::map<std::string, std::string> > styles = config->get_styles();
    _pattern_and_colors.push_back(
      std::make_pair("([^A-Za-z])([0-9]+(\\.[0-9]+)?((e|E)[0-9]+)?)", styles[6]["fore-color"]));
    _pattern_and_colors.push_back(std::make_pair("/\\*.*?\\*/", styles[1]["fore-color"]));
    _pattern_and_colors.push_back(std::make_pair("--\\s.*", styles[2]["fore-color"]));
    _pattern_and_colors.push_back(
      std::make_pair("@@?([A-Za-z]+|'.*?')", styles[3]["fore-color"])); // user and system variable
    _pattern_and_colors.push_back(std::make_pair("'.*?'", styles[12]["fore-color"]));
    _pattern_and_colors.push_back(std::make_pair("`.*?`", styles[17]["fore-color"]));
    // _pattern_and_colors.push_back(std::make_pair("/\\*!.*\\*/", styles[21]["fore-color"])); don't convert hidden
    // command.
    //_pattern_and_colors.push_back(std::make_pair("\".*?\"", styles[13]["fore-color"])); collides with already done
    //markup
  }
}

//--------------------------------------------------------------------------------------------------

std::string QuerySidePalette::format_help_as_html(const std::string &text) {
  // Start by extracting the syntax part which is always at the top. This will be embedded into
  // <pre> tags to stay as formatted.
  std::string result, line, block;
  std::istringstream stream(text);

  // Line was read already for the syntax block check (if there's any input remaining yet).
  while (std::getline(stream, line)) {
    block = "";

    if (line.empty()) {
      result += "<br><br>";
      continue;
    }

    // First look for list entries es they are similarly formatted like code sections, so we catch lists first.
    if (line.find("o ") == 0) {
      // A list starts. End is reached if we find two linebreaks followed by anything not indented
      // and which is not a list starter.
      block = "<ul>";
      std::string pending = line.substr(2, line.size());
      while (std::getline(stream, line)) {
        if (line.empty()) {
          // List item might be done (or just has an empty line in it).
          // We might even be done with the entire list.
          if (!std::getline(stream, line))
            break;

          if (line.find("  ") == 0) {
            // Just another item part.
            pending += "<br><br>" + line;
            continue;
          }

          if (line.find("o ") == 0) {
            // Found a new list item. Add the pending one and start over.
            block += "<li>" + pending + "</li>";
            pending = line.substr(2, line.size());
          } else
            break; // We are done.
        } else
          pending += line; // Another part of the current list item.
      }

      // Last pending list element.
      result += block + "<li>" + pending + "</li></ul><br>" + line;

      continue;
    }

    // Syntax lines consist usually of a lead line followed by indented trail lines.
    // A code block ends on an empty line.
    if (line.find("mysql>") == 0)
      block = "mysql&gt;" + line.substr(6, line.size()) + '\n';
    else if (line[line.size() - 1] == ';')
      block = line;
    {
      std::istream::pos_type position = stream.tellg();
      std::string pending;
      if (std::getline(stream, pending) && pending.find("  ") == 0)
        block = line + '\n' + pending + '\n';
      else {
        // Track back and start over with the next line.
        stream.seekg(position);
      }
    }

    if (!block.empty()) {
      while (std::getline(stream, line)) {
        block += line + '\n';
        if (line.empty())
          break;
      }

      // Done with the code block. Colorize elements.
      // Note: this has some limitations because certain text can be catched by several patterns.
      for (size_t i = 0; i < _pattern_and_colors.size(); i++) {
        pcrecpp::RE pattern(_pattern_and_colors[i].first);
        if (i == 0)
          pattern.GlobalReplace("\\1<span style=\"color:" + _pattern_and_colors[i].second + "\">\\2</span>", &block);
        else
          pattern.GlobalReplace("<span style=\"color:" + _pattern_and_colors[i].second + "\">\\0</span>", &block);
      }

      result += "<pre style=\"font-family: " + std::string(DEFAULT_MONOSPACE_FONT_FAMILY) + "; font-size: 8pt;\">" +
                block + "</pre>";

      continue;
    }

    // Wrap emphasized text by <b>.
    pcrecpp::RE pattern = "\\*([A-Za-z]+)\\*";
    pattern.GlobalReplace("<b>\\1</b>", &line);

    // Replace triple hyphen by typographic dash.
    pattern = "---";
    pattern.GlobalReplace("&mdash;", &line);

    if (!result.empty())
      result += " ";
    result += line;
  }

  // Replace "URL: <link>" by "See Also: <link>"
  pcrecpp::RE pattern = "URL:";
  pattern.Replace("<b>See also:</b>", &result);

  // Color all upper case words with at least 2 chars (which should mostly cover keywords).
  // This step will mess up internal links, so we have to correct them.
  pattern = "[A-Z]{2,}";
  pattern.GlobalReplace("<span style=\"color:" + _keyword_color + "\">\\0</span>", &result);

  // Internal help links. Original format: [HELP THE HELP TOPIC]. But due to the previous step each word
  // is now surrounded by markup. Because of the variable number of keywords in a link we need several runs.
  pattern = "\\[<span style=\"color:[^\"]+\">HELP</span> +<span style=\"color:[^\"]+\">([A-Z]+)</span>\\]";
  pattern.GlobalReplace("<a href=\"local:\\1\">\\1</a>", &result);
  pattern =
    "\\[<span style=\"color:[^\"]+\">HELP</span> +<span style=\"color:[^\"]+\">([A-Z]+)</span> +"
    "<span style=\"color:[^\"]+\">([A-Z]+)</span>\\]";
  pattern.GlobalReplace("<a href=\"local:\\1 \\2\">\\1 \\2</a>", &result);
  pattern =
    "\\[<span style=\"color:[^\"]+\">HELP</span> +<span style=\"color:[^\"]+\">([A-Z]+)</span> +"
    "<span style=\"color:[^\"]+\">([A-Z]+)</span> +<span style=\"color:[^\"]+\">([A-Z]+)</span>\\]";
  pattern.GlobalReplace("<a href=\"local:\\1 \\2 \\3\">\\1 \\2 \\3</a>", &result);

  // Change references to become real links. A problem here is that due to hard line breaks
  // in the original text we can now have space chars in links. We cannot fix any possible combination
  // but we do so for certain cases we know.
  pattern = ". ?h ?t ?m ?l ?";
  pattern.GlobalReplace(".html", &result);

  // Not a general URL pattern, but one that fits exactly the help urls.
  pattern = "https?://[A-Za-z]+(?:\\.[A-Za-z]+)+(?:/[A-Za-z0-9-.]+)+/([A-Za-z0-9-]+)\\.html";
  pattern.GlobalReplace(" <a href=\"\\0\">Online help \\1</a> ", &result);

  return result;
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::refresh_snippets() {
  if (_pending_snippets_refresh && _snippet_list->shared_snippets_active()) {
    SqlEditorForm::Ref owner(_owner.lock());

    try {
      _snippet_list->model()->load_from_db(owner.get());
      _pending_snippets_refresh = false;
    } catch (std::exception &exc) {
      logError("Error loading DB snippets: %s\n", exc.what());
    }
  }
  _snippet_list->refresh_snippets();
}

//--------------------------------------------------------------------------------------------------

void QuerySidePalette::edit_last_snippet() {
  _snippet_list->edit_new_snippet();
}
