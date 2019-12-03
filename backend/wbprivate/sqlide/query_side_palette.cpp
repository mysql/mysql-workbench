/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <boost/signals2/signal.hpp>

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

//----------------- SnippetListView ------------------------------------------------------------------------------------

/**
  * Internal class containing a list of snippet entries in a scrollable list.
  */
class SnippetListView : public BaseSnippetList {
private:
  friend class QuerySidePalette;

  wb::SnippetPopover *_snippetPopover;
  bool _user_snippets_active;
  bool _shared_snippets_active;

private:
  DbSqlEditorSnippets *model() {
    return dynamic_cast<DbSqlEditorSnippets *>(_model);
  }

  void popover_closed() {
    if (getPopover()->has_changed()) {
      std::string title = getPopover()->get_heading();
      model()->set_field(bec::NodeId(_selected_index), DbSqlEditorSnippets::Description, title);
      std::string sub_title = getPopover()->get_text();
      model()->set_field(bec::NodeId(_selected_index), DbSqlEditorSnippets::Script, sub_title);
      if (_selected_snippet)
        set_snippet_info(_selected_snippet, title, sub_title);
      model()->save();
      refresh_snippets();

      set_needs_repaint();
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------

public:
  SnippetListView(const std::string &icon_name) : BaseSnippetList(icon_name, DbSqlEditorSnippets::get_instance()), _snippetPopover(nullptr) {
    _user_snippets_active = false;
    _shared_snippets_active = false;

    _defaultSnippetActionCb = [&](int x, int y) {
       Snippet *snippet = snippet_from_point(x, y);
       if (snippet != nullptr) {
         set_selected(snippet);
         edit_snippet(snippet);
       }
    };

    prepare_context_menu();
  }

  //--------------------------------------------------------------------------------------------------------------------

  wb::SnippetPopover* getPopover() {
    if (_snippetPopover != nullptr) {
      return _snippetPopover;
    }

    _snippetPopover = new wb::SnippetPopover(this);
    _snippetPopover->set_size(376, 257);
    _snippetPopover->signal_closed()->connect(std::bind(&SnippetListView::popover_closed, this));

    return _snippetPopover;
  }

  //--------------------------------------------------------------------------------------------------------------------

  ~SnippetListView() {
    delete _snippetPopover;
    _context_menu->release();
  }

  //--------------------------------------------------------------------------------------------------------------------

  void edit_new_snippet() {
    if (!_snippets.empty()) {
      _selected_index = 0;
      _selected_snippet = _snippets.front();
      edit_snippet(_selected_snippet);
      getPopover()->set_read_only(false);
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

  std::string selected_category() {
    return model()->selected_category();
  }

  bool shared_snippets_active() const {
    return _shared_snippets_active;
  }

  //--------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------

  void edit_snippet(Snippet *snippet) {
    base::Rect bounds = snippet_bounds(snippet);

    std::pair<int, int> left_top = client_to_screen((int)bounds.left(), (int)bounds.top());
    std::pair<int, int> bottom = client_to_screen(0, (int)bounds.bottom());
    left_top.second = (left_top.second + bottom.second) / 2;

    std::string title, description;
    get_snippet_info(snippet, title, description);


    getPopover()->set_heading(title);
    getPopover()->set_read_only(false);
    getPopover()->set_text(description);
    getPopover()->set_read_only(true);
    getPopover()->show(left_top.first, left_top.second, mforms::StartLeft);
  }

  //--------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------

  void close_popover() {
    // Check if it's at least created cause maybe we don't need to close it.
    if (_snippetPopover != nullptr) {
      getPopover()->close();
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

  // End of internal class SnippetListView.
};

//----------------- QuerySidePalette -----------------------------------------------------------------------------------

QuerySidePalette::QuerySidePalette(const SqlEditorForm::Ref &owner)
  :
#ifdef _MSC_VER
    TabView(mforms::TabViewPalette),
#else
    TabView(mforms::TabViewSelectorSecondary),
#endif
    _owner(owner) {

  _help_timer = NULL;
  _automatic_help = bec::GRTManager::get()->get_app_option_int("DbSqlEditor:DisableAutomaticContextHelp", 0) == 0;
  _switching_help = false;
  _helpContext = new help::HelpContext(owner->rdbms()->characterSets(), owner->sql_mode(), owner->server_version());
  set_name("Query Side Palette");
  setInternalName("querySidePalette");

  _pending_snippets_refresh = true;

  mforms::Box *help_page = manage(new Box(false));
  _help_toolbar = prepare_help_toolbar();
  _help_text = manage(new mforms::HyperText());

  // Separate box since we need a border around the content, but not the toolbar.
  _contentBorder = manage(new Box(false));

  // No scoped_connect needed since _help_text is a member variable.
  scoped_connect(_help_text->signal_link_click(),
                 std::bind(&QuerySidePalette::click_link, this, std::placeholders::_1));

#if _MSC_VER
  std::string backgroundColor = base::Color::getApplicationColorAsString(AppColorPanelContentArea, false);
  _help_text->set_font("Tahoma 8");
#elif __APPLE__
  std::string backgroundColor = Color::getSystemColor(base::SystemColor::WindowBackgroundColor).to_html();
#else
  std::string backgroundColor = "#ebebeb";
#endif

  _help_text->set_back_color(backgroundColor);
  _contentBorder->set_back_color(backgroundColor);
  _contentBorder->set_padding(3, 3, 3, 3);

  _help_text->set_markup_text("");
  _current_topic_index = -1;

  help_page->add(_help_toolbar, false, true);
  _contentBorder->add(_help_text, true, true);
  help_page->add(_contentBorder, true, true);
  add_page(help_page, _("Context Help"));

  Box *snippet_page = manage(new Box(false));

  Box *content_border = manage(new Box(false));
  _snippet_list = manage(new SnippetListView("snippet_sql.png"));
  _snippet_list->set_name("Snippet List");
  _snippet_list->setInternalName("Snippet list");

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

  updateColors();

  base::NotificationCenter::get()->add_observer(this, "GNTextSelectionChanged");
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//----------------------------------------------------------------------------------------------------------------------

QuerySidePalette::~QuerySidePalette() {
  base::NotificationCenter::get()->remove_observer(this);

  cancel_timer();

  delete _helpContext;
}

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::cancel_timer() {
  if (_help_timer != NULL)
    bec::GRTManager::get()->cancel_timer(_help_timer);
}

//----------------------------------------------------------------------------------------------------------------------

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
      SqlEditorForm::Ref form = _owner.lock();
      cancel_timer();
      _help_timer = bec::GRTManager::get()->run_every(
        std::bind(&QuerySidePalette::find_context_help, this, editor), 0.5);
    }
  } else if (name == "GNColorsChanged") {
    updateColors();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::updateColors() {
#if _MSC_VER
  std::string backgroundColor = base::Color::getApplicationColorAsString(AppColorPanelContentArea, false);
  _help_text->set_font("Tahoma 8");
#elif __APPLE__
  std::string backgroundColor = Color::getSystemColor(base::SystemColor::WindowBackgroundColor).to_html();
#else
  std::string backgroundColor = "#ebebeb";
#endif

  _help_text->set_back_color(backgroundColor);
  _contentBorder->set_back_color(backgroundColor);

#ifdef _MSC_VER 
  _contentBorder->set_padding(3, 3, 3, 3);
  _snippet_list->set_back_color(base::Color::getApplicationColorAsString(AppColorPanelContentArea, false));
#elif __APPLE__
  _snippet_list->set_back_color(backgroundColor);
#else
  _snippet_list->set_back_color("#f2f2f2");
#endif

  // Also reload any help text, to update HTML colors.
  if (!_currentHelpTopic.empty()) {
    std::string text;
    help::DbSqlEditorContextHelp::get()->helpTextForTopic(_helpContext, _currentHelpTopic, text);
    _help_text->set_markup_text(text);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::show_help_text_for_topic(const std::string &topic) {
  if (_currentHelpTopic != topic) {
    _currentHelpTopic = topic;
    if (_currentHelpTopic.empty()) {
      _help_text->set_markup_text(std::string("<hmtl><body style=\"font-family:'") + DEFAULT_FONT_FAMILY + "'; \">"
                                  "<div style='text-align: center;'><b style='color: gray; font-size: 16pt;'>No Context Help</b></div></body></html>");
    } else {
      std::string text;
      help::DbSqlEditorContextHelp::get()->helpTextForTopic(_helpContext, _currentHelpTopic, text);
      _help_text->set_markup_text(text);

      _switching_help = true;
      _quick_jump_item->set_text(_currentHelpTopic);
      _switching_help = false;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::show_help_hint_or_update() {
  if (!_automatic_help) {
    _help_text->set_markup_text(
      std::string("<hmtl><body style=\"font-family:") + DEFAULT_FONT_FAMILY + ";\">"
      "<div style='text-align: center;'><b style='color: gray; font-size: 12pt;'>"
      "Automatic context help is disabled. Use the toolbar to manually get help for the current caret "
      "position or to toggle automatic help.</b></div></body></html>");
  } else {
    show_help_text_for_topic(_current_topic_index > 0 ? _topic_history[_current_topic_index] : "");
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Triggered by timer or manually to find a help topic from the given editor's text + position.
 */
bool QuerySidePalette::find_context_help(MySQLEditor *editor) {
  _help_timer = nullptr;

  // If no editor was given use the currently active one in the SQL editor form.
  if (editor == nullptr) {
    SqlEditorForm::Ref form = _owner.lock();
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel != nullptr)
      editor = panel->editor_be().get();
    else
      return false;
  }

  size_t caretPosition = editor->cursor_pos();

  size_t start, stop;
  editor->get_current_statement_range(start, stop); // To convert the caret position to a local statement position.

  std::string topic = help::DbSqlEditorContextHelp::get()->helpTopicFromPosition(_helpContext,
    editor->current_statement(), caretPosition - start);
  update_help_history(topic);
  show_help_text_for_topic(topic);

  return false; // Don't re-run this task, it's a single-shot.
}

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

ToolBar *QuerySidePalette::prepare_snippet_toolbar() {
  ToolBar *toolbar = manage(new ToolBar(mforms::SecondaryToolBar));
  toolbar->set_name("Snippet Toolbar");
  toolbar->setInternalName("snippet_toolbar");
#ifndef __APPLE__
  toolbar->set_padding(0, 0, 0, 0);
  toolbar->set_size(-1, 27);
#endif
  ToolBarItem *item;

  item = mforms::manage(new ToolBarItem(mforms::SelectorItem));
  item->set_name("Select Category");
  item->setInternalName("select_category");

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
  item->set_name("Replace Text");
  item->setInternalName("replace_text");
  item->set_icon(App::get()->get_resource_path("snippet_use.png"));
  item->set_tooltip(_("Replace the current text by this snippet"));
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::snippet_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  item = mforms::manage(new ToolBarItem(mforms::ActionItem));
  item->set_name("Insert Text");
  item->setInternalName("insert_text");
  item->set_icon(App::get()->get_resource_path("snippet_insert.png"));
  item->set_tooltip(_("Insert the snippet text at the current caret position replacing selected text if there is any"));
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::snippet_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  item = manage(new ToolBarItem(mforms::ActionItem));
  item->set_name("Copy To Clipboard");
  item->setInternalName("copy_to_clipboard");
  item->set_icon(App::get()->get_resource_path("snippet_clipboard.png"));
  item->set_tooltip(_("Copy the snippet text to the clipboard"));
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::snippet_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  return toolbar;
}

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::snippet_toolbar_item_activated(ToolBarItem *item) {
  std::string action = item->getInternalName();
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

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::snippet_selection_changed() {
  bool has_selection = _snippet_list->selected_index() > -1;
  _snippet_toolbar->set_item_enabled("copy_to_clipboard", has_selection);
  _snippet_toolbar->set_item_enabled("replace_text", has_selection);
  _snippet_toolbar->set_item_enabled("insert_text", has_selection);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called by the owning form if the main form changes to remove the popover.
 */
void QuerySidePalette::close_popover() {
  _snippet_list->close_popover();
}

//----------------------------------------------------------------------------------------------------------------------

ToolBar *QuerySidePalette::prepare_help_toolbar() {
  ToolBar *toolbar = manage(new ToolBar(mforms::SecondaryToolBar));
  toolbar->set_name("Help Toolbar");
  toolbar->setInternalName("help_toolbar");
#ifndef __APPLE__
  toolbar->set_padding(0, 0, 0, 0);
  toolbar->set_size(-1, 27);
#endif

  _back_item = manage(new ToolBarItem(mforms::ActionItem));
  _back_item->set_name("Back");
  _back_item->setInternalName("back");
  _back_item->set_icon(App::get()->get_resource_path("wb-toolbar_nav-back.png"));
  _back_item->set_tooltip(_("One topic back"));
  _back_item->set_enabled(false);
  scoped_connect(_back_item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(_back_item);

  _forward_item = manage(new ToolBarItem(mforms::ActionItem));
  _forward_item->set_name("Forward");
  _forward_item->setInternalName("forward");
  _forward_item->set_icon(App::get()->get_resource_path("wb-toolbar_nav-forward.png"));
  _forward_item->set_tooltip(_("One topic forward"));
  _forward_item->set_enabled(false);
  scoped_connect(_forward_item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(_forward_item);

  toolbar->add_item(manage(new ToolBarItem(mforms::SeparatorItem)));

  ToolBarItem *item = manage(new ToolBarItem(mforms::ToggleItem));
  item->set_name("Toggle Auto Context Help");
  item->setInternalName("toggle-auto-context-help");
  item->set_icon(App::get()->get_resource_path("wb-toolbar_automatic-help-off.png"));
  item->set_alt_icon(App::get()->get_resource_path("wb-toolbar_automatic-help-on.png"));
  item->set_tooltip(_("Toggle automatic context help"));
  item->set_checked(_automatic_help);
  scoped_connect(item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(item);

  _manual_help_item = manage(new ToolBarItem(mforms::ActionItem));
  _manual_help_item->set_name("Manual Context Help");
  _manual_help_item->setInternalName("manual-context-help");
  _manual_help_item->set_icon(App::get()->get_resource_path("wb-toolbar_manual-help.png"));
  _manual_help_item->set_tooltip(_("Get context help for the item at the current caret position"));
  _manual_help_item->set_enabled(!_automatic_help);
  scoped_connect(_manual_help_item->signal_activated(),
                 std::bind(&QuerySidePalette::help_toolbar_item_activated, this, std::placeholders::_1));
  toolbar->add_item(_manual_help_item);

  toolbar->add_item(manage(new ToolBarItem(mforms::SeparatorItem)));

  _quick_jump_item = manage(new ToolBarItem(mforms::SelectorItem));
  _quick_jump_item->set_name("Quick Jump");
  _quick_jump_item->setInternalName("quick_jump");

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

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::help_toolbar_item_activated(ToolBarItem *item) {
  if (_switching_help)
    return;

  std::string action = item->getInternalName();
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

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

void QuerySidePalette::edit_last_snippet() {
  _snippet_list->edit_new_snippet();
}

//----------------------------------------------------------------------------------------------------------------------
