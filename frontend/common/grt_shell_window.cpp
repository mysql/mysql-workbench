/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt_python_debugger.h"
#include "grt_shell_window.h"
#include "base/file_functions.h"
#include "base/string_utilities.h"
#include "base/wb_iterators.h"
#include "base/notifications.h"
#include "base/log.h"
#include <errno.h>

DEFAULT_LOG_DOMAIN("grtshell")

#include "grt/common.h"
#include "grt_code_editor.h"
#include "grt_plugin_wizard.h"

#include "workbench/wb_context.h"
#include "mforms/app.h"
#include "mforms/imagebox.h"

#include <glib/gstdio.h>

using namespace base;
using namespace bec;
using namespace mforms;

#define EDITOR_TAB_OFFSET 2

//--------------------------------------------------------------------------------------------------

GRTShellWindow::GRTShellWindow(wb::WBContext *context)
  : mforms::Form(mforms::Form::main_form(),
                 (mforms::FormFlag)(mforms::FormResizable | mforms::FormMinimizable | mforms::FormHideOnClose)),
    _context(context),
    _toolbar(true),
    _content(false),
    _padding_box(false),
    _hsplitter(true),
    _side_tab(mforms::TabViewPalette),
    _main_tab(mforms::TabViewDocumentClosable),
    _global_box1(false),
    _global_box2(false),
    _global_splitter(false),
    _global_combo(mforms::SelectorCombobox),
    _global_tree(mforms::TreeDefault),
    _global_list(mforms::TreeFlatList),
    _classes_box(false),
    _classes_splitter(false),
    _classes_tree(mforms::TreeNoBorder),
    _classes_text(mforms::VerticalScrollBar),
    _modules_splitter(false),
    _modules_tree(mforms::TreeNoBorder),
    _modules_text(mforms::VerticalScrollBar),
    _notifs_splitter(false),
    _notifs_tree(mforms::TreeNoBorder),
    _notifs_text(mforms::VerticalScrollBar),
    _right_splitter(false),
    _shell_box(false),
    _shell_text(mforms::VerticalScrollBar),
    _shell_hbox(true),
#ifdef _MSC_VER
    _side_header_panel(mforms::FilledHeaderPanel),
    _lower_tab(mforms::TabViewPalette),
    _lower_header_panel(mforms::FilledHeaderPanel),
#else
    _lower_tab(mforms::TabViewDocument),
#endif
    _output_text(mforms::VerticalScrollBar),
    _snippet_splitter(false),
    _snippet_text(),
    _userSnippetsLoaded(false),
    _snippetClicked(false) {
  set_title(("Workbench Scripting Shell"));
  set_name("Shell Window");
  setInternalName("shell_window");

  // Minimum size for the entire window.
  set_size(800, 600);

  set_content(&_content);
  set_menubar(&_menu);
  scoped_connect(signal_closed(), std::bind(&GRTShellWindow::shell_closed, this));
  set_on_close(std::bind(&GRTShellWindow::can_close, this));

  _content.add(&_toolbar, false, true);

  // setup the menubar
  {
    mforms::MenuItem *menu = mforms::manage(new mforms::MenuItem("File"));
    mforms::MenuItem *item;
    _menu.add_submenu(menu);
    item = menu->add_item_with_title("New...", std::bind(&GRTShellWindow::add_new_script, this), "New", "");
    item->set_shortcut("Modifier+N");
    menu->add_item_with_title("New Script", std::bind(&GRTShellWindow::add_editor, this, true, "python"), "New Script", "");
    item = menu->add_item_with_title("Open...", std::bind(&GRTShellWindow::open_script_file, this), "Open", "");
    item->set_shortcut("Modifier+O");
    menu->add_separator();
    item = menu->add_item_with_title("Save", std::bind(&GRTShellWindow::save_file, this, false), "Save", "");
    item->set_shortcut("Modifier+S");
    item = menu->add_item_with_title("Save As...", std::bind(&GRTShellWindow::save_file, this, true), "Save As", "");
    item->set_shortcut("Modifier+Shift+S");
    menu->add_separator();
    item = menu->add_item_with_title("Close Script", std::bind(&GRTShellWindow::close_tab, this), "Close Script", "");
    item->set_shortcut("Modifier+W");
    item = menu->add_item_with_title("Close Window", std::bind(&GRTShellWindow::close, this), "Close Window", "");
#ifdef _MSC_VER
    item->set_shortcut("Control+F4");
#else
    item->set_shortcut("Modifier+Shift+W");
#endif
    menu = mforms::manage(new mforms::MenuItem("Edit"));
    _menu.add_submenu(menu);

    item = menu->add_item_with_title("Cut", std::bind(&GRTShellWindow::cut, this), "Cut", "");
    item->set_shortcut("Modifier+X");
    item = menu->add_item_with_title("Copy", std::bind(&GRTShellWindow::copy, this), "Copy", "");
    item->set_shortcut("Modifier+C");
    item = menu->add_item_with_title("Paste", std::bind(&GRTShellWindow::paste, this), "Paste", "");
    item->set_shortcut("Modifier+V");
    item = menu->add_item_with_title("Select All", std::bind(&GRTShellWindow::select_all, this), "Select All", "");
    item->set_shortcut("Modifier+A");
    menu->add_separator();
    item = menu->add_item_with_title("Find...", std::bind(&GRTShellWindow::show_find_panel, this), "Find", "");
    item->set_shortcut("Modifier+F");
    item = menu->add_item_with_title("Replace...", std::bind(&GRTShellWindow::show_replace_panel, this), "Replace", "");
#if defined(__APPLE__)
    item->set_shortcut("Command+Option+F");
#else
    item->set_shortcut("Modifier+H");
#endif

    menu = mforms::manage(new mforms::MenuItem("Script"));
    _menu.add_submenu(menu);

    item = menu->add_item_with_title("Run", std::bind(&GRTShellWindow::execute_file, this), "Run", "");
    item->set_shortcut("Modifier+R");
    item->set_name("Run");
    item->setInternalName("run");
  }

#ifdef _MSC_VER
  _content.add(&_padding_box, true, true);
  _padding_box.add(&_hsplitter, true, true);

  _padding_box.set_padding(6);
  set_back_color("#283752");
  _hsplitter.set_back_color("#283752");
  _side_header_panel.add(&_side_tab);
  _side_header_panel.set_back_color("#283752");
  _hsplitter.add(&_side_header_panel);
#else
  _content.add(&_hsplitter, true, true);
  _hsplitter.add(&_side_tab);
#endif

  scoped_connect(_side_tab.signal_tab_changed(), std::bind(&GRTShellWindow::side_tab_changed, this));

  _hsplitter.add(&_right_splitter);

// Side bar consists of 4 pages: files, globals tree, classes tree and modules tree.

// Setup toolbar.
#ifdef _MSC_VER
  _toolbar.set_size(-1, 24);
  _toolbar.set_back_color("#BCC7D8");
#endif

  _toolbar.set_padding(2);
  _toolbar.set_spacing(4);

  add_tool_button("tiny_new_doc.png", std::bind(&GRTShellWindow::add_new_script, this),
                  _("Create a new file from a template"));
  add_tool_button("tiny_load.png", std::bind(&GRTShellWindow::open_script_file, this), _("Open a script file"));

  add_tool_separator();
  _save_button = add_tool_button("tiny_save.png", std::bind(&GRTShellWindow::save_file, this, false), _("Save file"));
  _save_as_button = add_tool_button("tiny_saveas.png", std::bind(&GRTShellWindow::save_file, this, true),
                                    _("Save file with a new name"));

  add_tool_separator();
  //  if (_editing_module)
  {
    //   add_tool_button("tiny_refresh.png", std::bind(&GRTCodeEditor::execute, this),
    //                   "Refresh the module");
  } //  else
  {
    _run_button = add_tool_button("qe_sql-editor-tb-icon_execute.png", std::bind(&GRTShellWindow::execute_file, this),
                                  _("Execute script"));
    _continue_button = add_tool_button("debug_continue.png", std::bind(&GRTShellWindow::debug_continue, this),
                                       _("Continue execution until next breakpoint"));
    _pause_button =
      add_tool_button("debug_pause.png", std::bind(&GRTShellWindow::debug_pause, this), "Pause script execution");
    _step_button =
      add_tool_button("debug_step.png", std::bind(&GRTShellWindow::debug_step, this), _("Step to next statement"));
    _step_into_button = add_tool_button("debug_step_into.png", std::bind(&GRTShellWindow::debug_step_into, this),
                                        _("Step into next function"));
    _step_out_button = add_tool_button("debug_step_out.png", std::bind(&GRTShellWindow::debug_step_out, this),
                                       _("Finish execution of current function"));
    _stop_button =
      add_tool_button("debug_stop.png", std::bind(&GRTShellWindow::debug_stop, this), _("Stop executing script"));

    _continue_button->show(false);
    _step_button->set_enabled(false);
    _step_into_button->set_enabled(false);
    _step_out_button->set_enabled(false);
    _continue_button->set_enabled(false);
    _stop_button->set_enabled(false);
    _pause_button->set_enabled(false);
  }

#if !defined(_MSC_VER) && !defined(__APPLE__)
  // TODO: remove as soon as all platforms support closable tabs.
  _close_script_tab_button =
    add_tool_button("Discard.png", std::bind(&GRTShellWindow::close_tab, this), _("Close this script tab"), false);
#else
  _close_script_tab_button = NULL;
#endif
  add_tool_separator();

  _clear_script_output_button = add_tool_button(
    "clear_output.png", std::bind(&mforms::TextBox::set_value, &_output_text, ""), _("Clear script output"), false);

  add_tool_button("snippet_add.png", std::bind(&GRTShellWindow::add_snippet, this), _("Add a new snippet"));
  _snippet_delete_button =
    add_tool_button("snippet_del.png", std::bind(&GRTShellWindow::del_snippet, this), _("Delete the selected snippet"));
  _snippet_copy_button = add_tool_button("snippet_clipboard.png", std::bind(&GRTShellWindow::copy_snippet, this),
                                         _("Copy snippet text to the clipboard"));
  add_tool_separator();

  _show_find_button =
    add_tool_button("qe_sql-editor-tb-icon_find.png", std::bind(&GRTShellWindow::show_find_panel, this),
                    _("Show the Find panel for the editor"));
  _show_find_button->set_enabled(false);

  // Files
  mforms::Box *files_box = mforms::manage(new mforms::Box(false));

#ifdef _MSC_VER
  _files_tree = mforms::manage(new mforms::TreeView(mforms::TreeNoBorder | mforms::TreeNoHeader));
#else
  _files_tree = mforms::manage(new mforms::TreeView(mforms::TreeDefault | mforms::TreeNoHeader));
#endif

  _files_menu.add_item_with_title("Add New File",
                                  std::bind(&GRTShellWindow::file_menu_activate, this, "file-from-template"), "Add New File", "");
  _files_menu.add_item_with_title("Open Script File",
                                  std::bind(&GRTShellWindow::file_menu_activate, this, "open-script"), "Open Script File", "");
  _files_menu.add_separator();
  _files_menu.add_item_with_title("Delete Script",
                                  std::bind(&GRTShellWindow::file_menu_activate, this, "delete-script"), "Delete Script", "");

  _files_tree->set_context_menu(&_files_menu);

  _files_tree->add_column(IconStringColumnType, "", 400, false);
  _files_tree->end_columns();

  scoped_connect(_files_tree->signal_node_activated(),
                 std::bind(&GRTShellWindow::file_list_activated, this, std::placeholders::_1, std::placeholders::_2));

  files_box->add(_files_tree, true, true);
#ifdef _MSC_VER
  files_box->set_back_color("#FFFFFF");
  files_box->set_padding(0, 0, 0, 2);
#endif
  _side_tab.add_page(files_box, "Files");

// 1) Globals tree
#ifdef _MSC_VER
  _global_splitter.set_back_color("#FFFFFF");
  _hsplitter.set_back_color("#283752");
//_global_splitter.set_padding(0, 0, 0, 2); TODO: might require work around since we removed padding from View.
#endif
  _side_tab.add_page(&_global_splitter, "Globals");
  _global_splitter.add(&_global_box1, 0);
  _global_splitter.add(&_global_box2, 0);
#ifndef _MSC_VER
  _global_box1.set_spacing(4);
  _global_box2.set_spacing(4);
#endif
  _global_box1.add(&_global_combo, false, false);
  _global_box1.add(&_global_tree, true, true);

  _global_box2.add(&_global_entry, false, true);
  _global_entry.set_read_only(true);
#if defined(_MSC_VER) | defined(__APPLE__)
  _global_entry.set_back_color("#FFFFFF");
#endif

  _global_box2.add(&_global_list, true, true);
  _global_list.add_column(mforms::IconStringColumnType, "Name", 100);
  _global_list.add_column(mforms::StringColumnType, "Value", 100);
  _global_list.end_columns();
  _global_tree.add_column(mforms::IconStringColumnType, "GRT Globals", 200, false);
  _global_tree.add_column(mforms::StringColumnType, "Type", 100, false);
  _global_tree.end_columns();
  scoped_connect(_global_tree.signal_expand_toggle(),
                 std::bind(&GRTShellWindow::globals_expand_toggle, this, std::placeholders::_1, std::placeholders::_2));
  scoped_connect(_global_tree.signal_changed(), std::bind(&GRTShellWindow::global_selected, this));
  scoped_connect(_global_combo.signal_changed(), std::bind(&GRTShellWindow::refresh_globals_tree, this));
  _inspector = 0;

  _global_menu.add_item_with_title(_("Copy Value"), std::bind(&GRTShellWindow::handle_global_menu, this, "copy_value"), "Copy Value", "");
  _global_menu.add_item_with_title(_("Copy Path"), std::bind(&GRTShellWindow::handle_global_menu, this, "copy_path"), "Copy Path", "");
  _global_menu.add_item_with_title(_("Copy Path for Python"),
                                   std::bind(&GRTShellWindow::handle_global_menu, this, "copy_path_py"), "Copy Path for Python", "");

  _global_tree.set_context_menu(&_global_menu);
// 2) Classes tree
#ifdef _MSC_VER
  _classes_splitter.set_back_color("#FFFFFF");
//_classes_splitter.set_padding(0, 0, 0, 2); TODO: might require workaround.
#endif
  _side_tab.add_page(&_classes_splitter, "Classes");
  _classes_splitter.add(&_classes_box, 0);
  _classes_box.set_spacing(4);
  _classes_box.add(&_classes_sorting, false, true);
  _classes_box.add(&_classes_tree, true, true);

  _classes_splitter.add(&_classes_text, 0);
  _classes_text.set_read_only(true);
  _classes_text.set_back_color("#FFFFFF");
  _classes_tree.add_column(mforms::IconStringColumnType, "Name", 150, false);
  _classes_tree.add_column(mforms::StringColumnType, "Type", 100, false);
  _classes_tree.add_column(mforms::StringColumnType, "Caption", 100, false);
  _classes_tree.end_columns();
  scoped_connect(_classes_tree.signal_changed(), std::bind(&GRTShellWindow::class_selected, this));
  _classes_sorting.add_item("Group by Name");
  _classes_sorting.add_item("Group by Hierarchy");
  _classes_sorting.add_item("Group by Package");
  scoped_connect(_classes_sorting.signal_changed(), std::bind(&GRTShellWindow::refresh_classes_tree, this));

// 3) Modules tree
#ifdef _MSC_VER
  _modules_splitter.set_back_color("#FFFFFF");
//_modules_splitter.set_padding(0, 0, 0, 2); TODO: might require workaround.
#endif
  _side_tab.add_page(&_modules_splitter, "Modules");
  _modules_splitter.add(&_modules_tree, 0);
  _modules_splitter.add(&_modules_text, 0);
  _modules_text.set_read_only(true);
  _modules_text.set_back_color("#FFFFFF");
  _modules_tree.add_column(mforms::IconStringColumnType, "GRT Modules", 220, false);
  _modules_tree.end_columns();
  scoped_connect(_modules_tree.signal_changed(), std::bind(&GRTShellWindow::module_selected, this));

  _right_splitter.add(&_main_tab);

// 4) Notifications tree
#ifdef _MSC_VER
  _notifs_splitter.set_back_color("#FFFFFF");
//_modules_splitter.set_padding(0, 0, 0, 2); TODO: might require workaround.
#endif
  _side_tab.add_page(&_notifs_splitter, "Notifications");
  _notifs_splitter.add(&_notifs_tree, 0);
  _notifs_splitter.add(&_notifs_text, 0);
  _notifs_text.set_read_only(true);
  _notifs_text.set_back_color("#FFFFFF");
  _notifs_tree.add_column(mforms::IconStringColumnType, "Notifications", 220, false);
  _notifs_tree.end_columns();
  scoped_connect(_notifs_tree.signal_changed(), std::bind(&GRTShellWindow::notif_selected, this));

#ifdef _MSC_VER
  _right_splitter.set_back_color("#283752");
  _lower_header_panel.add(&_lower_tab);
  _right_splitter.add(&_lower_header_panel);
  _lower_header_panel.set_title(_("Debugging"));
  _lower_header_panel.set_back_color("#283752");
#else
  _right_splitter.add(&_lower_tab);
#endif

// setup shell
#ifdef _MSC_VER
  _snippet_list = mforms::manage(new TreeView(mforms::TreeNoBorder | mforms::TreeFlatList));
#else
  _snippet_list = mforms::manage(new TreeView(mforms::TreeDefault | mforms::TreeFlatList));
#endif

  _shell_box.add(&_shell_text, true, true);
  _shell_text.set_monospaced(true);
  _shell_text.set_read_only(true);
#if defined(_MSC_VER) | defined(__APPLE__)
  _shell_text.set_front_color("#FFFFFF");
  _shell_text.set_back_color("#000000");
#endif
  _shell_text.set_padding(2);
  _shell_box.add(&_shell_hbox, false, true);
  _shell_hbox.add(&_shell_prompt, false, true);
  _shell_hbox.add(&_shell_entry, true, true);
  _main_tab.add_page(&_shell_box, "Shell", false);

  scoped_connect(_shell_entry.signal_action(), std::bind(&GRTShellWindow::shell_action, this, std::placeholders::_1));

// snippets tab
#ifdef _MSC_VER
  _snippet_splitter.set_back_color("#283752");
#else
  _snippet_splitter.set_padding(8);
#endif

  //_snippet_box.set_spacing(8);
  _snippet_splitter.add(_snippet_list, 50, true);
  _snippet_splitter.add(&_snippet_text, 50, false);

  _snippet_menu.add_item_with_title("Execute Snippet",
                                    std::bind(&GRTShellWindow::snippet_menu_activate, this, "execute"), "Execute Snippet", "");
  _snippet_menu.add_item_with_title("Send to Script Editor",
                                    std::bind(&GRTShellWindow::snippet_menu_activate, this, "new_with_snippet"), "Send to Script Editor", "");
  _snippet_menu.add_separator();
  _snippet_menu.add_item_with_title("Copy to Clipboard",
                                    std::bind(&GRTShellWindow::snippet_menu_activate, this, "copy_clipboard"), "Copy to Clipboard", "");
  _snippet_menu.add_separator();
  _snippet_menu.add_item_with_title("Delete Snippet",
                                    std::bind(&GRTShellWindow::snippet_menu_activate, this, "delete"), "Delete Snippet", "");

  _snippet_list->set_context_menu(&_snippet_menu);

  scoped_connect(_snippet_list->signal_changed(), std::bind(&GRTShellWindow::snippet_selected, this));
  _snippet_text.set_language(LanguagePython);

  scoped_connect(_snippet_text.signal_changed(),
                 std::bind(&GRTShellWindow::snippet_changed, this, std::placeholders::_1, std::placeholders::_2));

  _snippet_list->add_column(mforms::StringColumnType, "Snippet", 500, false);
  _snippet_list->end_columns();
  _main_tab.add_page(&_snippet_splitter, "Snippets", false);

  scoped_connect(_main_tab.signal_tab_closing(),
                 std::bind(&GRTShellWindow::on_tab_closing, this, std::placeholders::_1));
  scoped_connect(_main_tab.signal_tab_changed(), std::bind(&GRTShellWindow::on_tab_changed, this));

  //

  _output_text.set_read_only(true);
  _output_text.set_monospaced(true);
  _lower_tab.add_page(&_output_text, "Output");

  _debugger = new PythonDebugger(this, &_lower_tab);

  try {
    _debugger->init_pdb();
  } catch (const std::exception &exc) {
    logError("Could not initialize the debugger: %s\n", exc.what());
    add_output("Could not initialize the debugger\n");
    delete _debugger;
    _debugger = 0;
  }

  bec::GRTManager::get()->run_once_when_idle(std::bind(&GRTShellWindow::set_splitter_positions, this));

  bec::GRTManager::get()->get_shell()->set_ready_handler(
    std::bind(&GRTShellWindow::handle_prompt, this, std::placeholders::_1));
  bec::GRTManager::get()->get_shell()->set_output_handler(
    std::bind(&GRTShellWindow::handle_output, this, std::placeholders::_1));

  on_tab_changed();
  snippet_selected();
  side_tab_changed();
}

bool GRTShellWindow::can_close() {
  // Because there's no other way to know if the window is about to close, we'll use this event to stop debugger.
  if (_stop_button->is_enabled() && _debugger)
    _debugger->stop();

  // GRTShellWindow is about to close so we ask each editor if it's fine to quit.
  return request_quit();
}

void GRTShellWindow::set_splitter_positions() {
  _hsplitter.set_divider_position(300);
  _global_splitter.set_divider_position(400);
  _modules_splitter.set_divider_position(400);
  _classes_splitter.set_divider_position(400);
  _notifs_splitter.set_divider_position(400);
  _snippet_splitter.set_divider_position(200);
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::shell_action(mforms::TextEntryAction action) {
  switch (action) {
    case mforms::EntryActivate: {
      std::string command = _shell_entry.get_string_value();
      _shell_entry.set_value("");
      //  _completion->add_completion_text(command);
      command += '\n';
      bec::GRTManager::get()->get_shell()->write(grt::GRT::get()->get_shell()->get_prompt() + " " + command);
      bec::GRTManager::get()->get_shell()->process_line_async(command);
      break;
    }
    case mforms::EntryKeyUp: {
      std::string line;
      if (bec::GRTManager::get()->get_shell()->previous_history_line(_shell_entry.get_string_value(), line))
        _shell_entry.set_value(line);
      break;
    }
    case mforms::EntryCKeyUp:
      break;
    case mforms::EntryKeyDown: {
      std::string line;
      if (bec::GRTManager::get()->get_shell()->next_history_line(line))
        _shell_entry.set_value(line);
      break;
    }
    case mforms::EntryCKeyDown:
      break;
    case mforms::EntryEscape:
      break;
  }
}

void GRTShellWindow::show(bool flag) {
  if (flag)
    refresh_all();

  load_state();
  mforms::Form::show(flag);
}

void GRTShellWindow::refresh_all() {
  refresh_files();

  int idx = 0;
  std::string root = _global_tree.root_node()->get_tag();
  std::vector<std::string> l = bec::GRTManager::get()->get_shell()->get_grt_tree_bookmarks();
  _global_combo.clear();
  for (std::vector<std::string>::const_iterator i = l.begin(); i != l.end(); ++i, ++idx) {
    _global_combo.add_item(*i);
    if (root == *i)
      _global_combo.set_selected(idx);
  }

  // refresh values
  refresh_globals_tree();
  global_selected();

  // refresh _struct
  refresh_classes_tree();

  // refresh modules
  refresh_modules_tree();

  refresh_notifs_list();

  _script_extension = ".py";
  _comment_prefix = "# ";

  refresh_snippets();
}

//--------------------------------------------------------------------------------------------------
bool GRTShellWindow::capture_output(const grt::Message &msg, void *sender, bool send_to_output) {
  if (msg.type == grt::OutputMsg) {
    if (bec::GRTManager::get()->in_main_thread()) {
      if (send_to_output)
        add_output(msg.text);
      else
        handle_output(msg.text); // sends to shell window
    } else {
      if (send_to_output)
        bec::GRTManager::get()->run_once_when_idle(std::bind(&GRTShellWindow::add_output, this, msg.text));
      else
        bec::GRTManager::get()->run_once_when_idle(std::bind(&GRTShellWindow::handle_output, this, msg.text));
    }
    return true;
  }
  return false;
}

void GRTShellWindow::execute_file() {
  GRTCodeEditor *editor = get_active_editor();
  if (!editor)
    return;

  grt::GRT::get()->pushMessageHandler(
    new grt::SlotHolder(std::bind(&GRTShellWindow::capture_output, this, std::placeholders::_1, std::placeholders::_2, true)));

  if (_debugger && g_str_has_suffix(editor->get_path().c_str(), ".py")) {
    _run_button->show(false);
    _continue_button->show(true);
    _pause_button->set_enabled(true);

    _debugger->run(editor);

    _run_button->show(true);
    _continue_button->show(false);

    _step_button->set_enabled(false);
    _step_into_button->set_enabled(false);
    _step_out_button->set_enabled(false);
    _continue_button->set_enabled(false);
    _stop_button->set_enabled(false);
    _pause_button->set_enabled(false);
  } else
    try {
      editor->execute();
    } catch (const std::exception &exc) {
      logError("Error during execution of script: %s\n", exc.what());
      add_output("There were errors during execution. Please review log messages.\n");
    }

  grt::GRT::get()->popMessageHandler();
}

void GRTShellWindow::debug_step() {
  GRTCodeEditor *editor = get_active_editor();

  if (editor && _debugger && g_str_has_suffix(editor->get_path().c_str(), ".py")) {
    if (_debugger->program_stopped())
      _debugger->step();
    else {
      // start the program stopping at the 1st line
      grt::GRT::get()->pushMessageHandler(
        new grt::SlotHolder(std::bind(&GRTShellWindow::capture_output, this, std::placeholders::_1, std::placeholders::_2, true)));

      _run_button->show(false);
      _continue_button->show(true);
      _pause_button->set_enabled(true);

      _debugger->run(editor, true);

      _run_button->show(true);
      _continue_button->show(false);

      _step_button->set_enabled(true);
      _step_into_button->set_enabled(false);
      _step_out_button->set_enabled(false);
      _continue_button->set_enabled(false);
      _stop_button->set_enabled(false);
      _pause_button->set_enabled(false);

      grt::GRT::get()->popMessageHandler();
    }
  }
}

void GRTShellWindow::debug_step_into() {
  if (_debugger)
    _debugger->step_into();
}

void GRTShellWindow::debug_step_out() {
  if (_debugger)
    _debugger->step_out();
}

void GRTShellWindow::debug_continue() {
  if (_debugger)
    _debugger->continue_();
}

void GRTShellWindow::debug_stop() {
  if (_debugger)
    _debugger->stop();
}

void GRTShellWindow::debug_pause() {
  if (_debugger)
    _debugger->pause();
}

void GRTShellWindow::save_file(bool save_as) {
  GRTCodeEditor *editor = get_active_editor();
  if (editor)
    editor->save(save_as);
}

void GRTShellWindow::close_tab() {
  GRTCodeEditor *editor = get_active_editor();
  if (editor) {
    if (editor->can_close())
      close_editor(editor);
  }
}

void GRTShellWindow::show_find_panel() {
  GRTCodeEditor *editor = get_active_editor();
  if (editor) {
    editor->get_editor()->show_find_panel(false);
  }
}

void GRTShellWindow::show_replace_panel() {
  GRTCodeEditor *editor = get_active_editor();
  if (editor) {
    editor->get_editor()->show_find_panel(true);
  }
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::side_tab_changed() {
#ifdef _MSC_VER
  static std::string side_bar_titles[] = {_("File Browser"), _("Globals Tree"), _("Classes List"), _("Modules List"),
                                          _("Notifications")};

  _side_header_panel.set_title(side_bar_titles[_side_tab.get_active_tab()]);
#endif
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::handle_output(const std::string &text) {
  _shell_text.append_text(text, true);
}

void GRTShellWindow::handle_error(const std::string &text, const std::string &detail) {
  _shell_text.append_text(text);

  _shell_text.append_text(detail);
}

void GRTShellWindow::handle_prompt(const std::string &text) {
  _shell_prompt.set_text(text);
}

void GRTShellWindow::global_selected() {
  if (_inspector) {
    delete _inspector;
    _inspector = 0;
  }

  mforms::TreeNodeRef selected;

  try {
    if ((selected = _global_tree.get_selected_node())) {
      grt::ValueRef value(get_global_at_node(selected));

      if (value.is_valid()) {
        _inspector = ValueInspectorBE::create(value, false, false);
        refresh_global_list();
      }

      _global_entry.set_value(get_global_path_at_node(selected));
    } else
      _global_entry.set_value("");
  } catch (std::exception &exc) {
    logError("Exception when selecting item in globals tree: %s\n", exc.what());
  }
}

void GRTShellWindow::class_selected() {
  mforms::TreeNodeRef selected;

  if ((selected = _classes_tree.get_selected_node()))
    _classes_text.set_value(get_class_node_description(selected));
  else
    _classes_text.set_value("");
}

void GRTShellWindow::module_selected() {
  mforms::TreeNodeRef selected;

  if ((selected = _modules_tree.get_selected_node())) {
    std::string text(get_module_node_description(selected));
    _modules_text.set_value(text);
  } else
    _modules_text.set_value("");
}

void GRTShellWindow::notif_selected() {
  mforms::TreeNodeRef selected;

  if ((selected = _notifs_tree.get_selected_node()) && selected->get_parent() != _notifs_tree.root_node()) {
    std::string text;
    std::string name = selected->get_string(0);
    base::NotificationCenter::NotificationHelp info =
      base::NotificationCenter::get()->get_registered_notification(name);

    text = base::strfmt(
      "%s (%s)\n"
      "%s\n\n"
      "Sender: %s\n\n"
      "Extra Info Dictionary:\n%s",
      name.c_str(), info.context.c_str(), info.summary.c_str(), info.sender.empty() ? "NULL" : info.sender.c_str(),
      info.info.empty() ? "No additional info is sent" : info.info.c_str());

    _notifs_text.set_value(text);
  } else
    _notifs_text.set_value("");
}

void GRTShellWindow::handle_global_menu(const std::string &action) {
  mforms::TreeNodeRef selected;

  if ((selected = _global_tree.get_selected_node())) {
    if (action == "copy_value") {
      grt::ValueRef value(get_global_at_node(selected));
      mforms::Utilities::set_clipboard_text(value.debugDescription());
    } else if (action == "copy_path") {
      mforms::Utilities::set_clipboard_text(get_global_path_at_node(selected));
    } else if (action == "copy_path_py") {
      std::string path = "grt.root";
      std::vector<std::string> parts;

      parts = base::split(get_global_path_at_node(selected), "/");
      for (base::const_range<std::vector<std::string> > p(parts); p; ++p) {
        if (p->empty())
          continue;

        if (isdigit(p->at(0)))
          path.append("[").append(*p).append("]");
        else
          path.append(".").append(*p);
      }

      mforms::Utilities::set_clipboard_text(path);
    }
  }
}

void GRTShellWindow::save_snippets() {
  //  If the user snippets were not loaded yet, a save is invalid
  if (!_userSnippetsLoaded || _snippetClicked)
    return;

  std::string path = base::makePath(bec::GRTManager::get()->get_user_datadir(), "shell_snippets" + _script_extension);
  std::fstream file(path, std::fstream::out | std::fstream::trunc);

  if (!file.is_open()) {
    _shell_text.append_text(base::strfmt("Cannot save snippets to %s: %s", path.c_str(), g_strerror(errno)));
    return;
  }

  int c = _snippet_list->root_node()->count();
  for (int i = _global_snippet_count; i < c; i++) {
    std::string snippet = _snippet_list->root_node()->get_child(i)->get_tag();

    if (i > _global_snippet_count)
      file << std::endl;

    file << " " << base::replaceString(snippet, "\n", "\n ") << std::endl;
  }
}

void GRTShellWindow::load_snippets_from(const std::string &path) {
  FILE *f = base_fopen(path.c_str(), "r");
  if (f) {
    char line[4096];

    while (fgets(line, sizeof(line), f)) {
      std::string script = line + 1;
      char *ptr = strchr(line, '\n');
      if (ptr)
        *ptr = 0;
      std::string name = line + 1;

      while (fgets(line, sizeof(line) - 1, f) && line[0] == ' ') {
        script.append(line + 1);
      }

      // Remove the last line break, we added that, not the user.
      if (script.size() > 0)
        script.erase(script.size() - 1, 1);
      mforms::TreeNodeRef node = _snippet_list->add_node();
      node->set_string(0, name);
      node->set_tag(script);
    }
    fclose(f);
  }
}

void GRTShellWindow::refresh_snippets() {
  _snippet_list->clear();

  load_snippets_from(bec::GRTManager::get()->get_data_file_path("shell_snippets" + _script_extension + ".txt"));
  _global_snippet_count = _snippet_list->root_node()->count();
  load_snippets_from(base::makePath(bec::GRTManager::get()->get_user_datadir(), "shell_snippets" + _script_extension));
  _userSnippetsLoaded = true;

  snippet_selected();
}

void GRTShellWindow::open_script_file() {
  mforms::FileChooser chooser(mforms::OpenFile);
  chooser.set_title(_("Open GRT Script"));
  if (chooser.run_modal()) {
    open_file_in_editor(chooser.get_path(), true);
  }
}

bool GRTShellWindow::execute_script(const std::string &script, const std::string &language) {
  bool result = bec::GRTManager::get()->get_shell()->run_script(script, language);
  save_state();

  return result;
}

void GRTShellWindow::add_snippet() {
  std::string snippet = _comment_prefix + " new snippet\n";

  mforms::TreeNodeRef node = _snippet_list->add_node();
  node->set_tag(snippet);
  _snippet_list->select_node(node);

  snippet_selected();    // force snippet to be displayed
  save_snippets();

  save_state();
}

void GRTShellWindow::del_snippet() {
  mforms::TreeNodeRef node = _snippet_list->get_selected_node();
  if (node) {
    node->remove_from_parent();
    snippet_selected();
    save_snippets();
  }
}

void GRTShellWindow::copy_snippet() {
  mforms::TreeNodeRef node = _snippet_list->get_selected_node();
  if (node)
    mforms::Utilities::set_clipboard_text(node->get_tag());
}

void GRTShellWindow::scriptize_snippet() {
  mforms::TreeNodeRef node = _snippet_list->get_selected_node();
  if (node) {
    std::string snippet = node->get_tag();
    std::string language = "python";

    GRTCodeEditor *editor = add_editor(true, language);
    editor->set_text(snippet);
  }
}

bool run_return_true(std::function<void(const std::string &)> f, const std::string &param) {
  f(param);
  return true;
}

void GRTShellWindow::run_snippet() {
  mforms::TreeNodeRef node = _snippet_list->get_selected_node();
  if (node) {
    std::string script = node->get_tag();

    // auto-select the tab where output goes
    _main_tab.set_active_tab(0);

    handle_output("Running snippet...\n");
    // redirect snippet output to the shell
    grt::GRT::get()->pushMessageHandler(
      new grt::SlotHolder(std::bind(&GRTShellWindow::capture_output, this, std::placeholders::_1, std::placeholders::_2, false)));

    try {
      std::string language = "python";

      bool ret = execute_script(script, language);
      grt::GRT::get()->popMessageHandler();
      if (!ret) {
        handle_output("Snippet execution finished with an error\n");
      } else {
        handle_output("...execution finished\n");
      }
    } catch (const std::exception &exc) {
      grt::GRT::get()->popMessageHandler();

      handle_output("Exception caught while executing snippet:\n");
      handle_output(std::string(exc.what()).append("\n"));
    }
  }

  save_state();
}

void GRTShellWindow::snippet_selected() {
  bool read_only = false;
  _snippetClicked = true;

  _snippet_text.set_features(mforms::FeatureReadOnly, false); // Necessary to be able to change the text.
  int sel = _snippet_list->get_selected_row();
  if (sel < 0) {
    _snippet_delete_button->set_enabled(false);
    _snippet_copy_button->set_enabled(false);
    _snippet_text.set_value("");
    read_only = true;

    for (int i = 0; i < 6; i++)
      _snippet_menu.get_item(i)->set_enabled(false);
  } else {
    if (sel < _global_snippet_count) {
      read_only = true;
      _snippet_delete_button->set_enabled(false);

      for (int i = 0; i < 6; i++) {
        if (i != 5)
          _snippet_menu.get_item(i)->set_enabled(true);
        else
          _snippet_menu.get_item(i)->set_enabled(false); // 5 is delete
      }
    } else {
      _snippet_delete_button->set_enabled(true);

      for (int i = 0; i < 6; i++)
        _snippet_menu.get_item(i)->set_enabled(true);
    }
    mforms::TreeNodeRef node(_snippet_list->get_selected_node());
    if (node)
      _snippet_text.set_value(node->get_tag());
    _snippet_copy_button->set_enabled(true);
  }

  _snippet_text.set_features(mforms::FeatureReadOnly, read_only);
  _snippetClicked = false;
}

void GRTShellWindow::snippet_changed(int line, int linesAdded) {
  std::string snippet = _snippet_text.get_string_value();
  mforms::TreeNodeRef node = _snippet_list->get_selected_node();

  if (node) {
    node->set_tag(snippet);

    std::string::size_type p = snippet.find('\n');
    if (p != std::string::npos)
      snippet = snippet.substr(0, p);
    node->set_string(0, snippet);

    save_snippets();
  }
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::snippet_menu_activate(const std::string &action) {
  if (action == "execute")
    run_snippet();
  else if (action == "new_with_snippet")
    scriptize_snippet();
  else if (action == "copy_clipboard")
    copy_snippet();
  else if (action == "delete")
    del_snippet();
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::file_menu_activate(const std::string &action) {
  if (action == "file-from-template")
    add_new_script();
  else if (action == "open-script")
    open_script_file();
  else if (action == "delete-script")
    delete_selected_file();
}

//--------------------------------------------------------------------------------------------------

GRTCodeEditor *GRTShellWindow::add_editor(bool is_script, const std::string &language) {
  GRTCodeEditor *editor = manage(new GRTCodeEditor(this, !is_script, language));

  _editors.push_back(editor);

  int page = _main_tab.add_page(editor, editor->get_title());
  _main_tab.set_active_tab(page);

  save_state();

  if (language == "python" && _debugger)
    _debugger->editor_added(editor);

  return editor;
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::close_editor(GRTCodeEditor *editor) {
  for (std::vector<GRTCodeEditor *>::iterator iter = _editors.begin(); iter != _editors.end(); ++iter) {
    if ((*iter) == editor) {
      _editors.erase(iter);
      break;
    }
  }

  if (_debugger)
    _debugger->editor_closed(editor);

  _main_tab.remove_page(editor);

  save_state();
}

void GRTShellWindow::open_file_in_editor(const std::string &path, bool is_script) {
  if (get_editor_for(path, true) != NULL)
    return;

  std::string language = ""; // No syntax highlighting if file extension is unknown.
  if (g_str_has_suffix(path.c_str(), ".py"))
    language = "python"; // Python script
  else if (g_str_has_suffix(path.c_str(), ".sql") || g_str_has_suffix(path.c_str(), ".qbquery"))
    language = "sql";

  // Show warning messages if applicable...
  if (language == "") {
    std::string text =
      base::strfmt(_("The file %s has an unsupported extension for this script editor."), path.c_str());
    if (Utilities::show_message_and_remember(_("Unsupported File Format"), text, _("OK"), _("Cancel"), "",
                                             "ShellWindowUnknownLanguageFile", "") == mforms::ResultCancel)
      return;
  } else if (language == "sql") {
    if (Utilities::show_message_and_remember(
          _("Unsupported Execution"), _("This script editor is meant for developing Workbench plugins and scripts. SQL "
                                        "scripts should be opened and executed in the SQL Editor."),
          _("OK"), _("Cancel"), "", "ShellWindowSqlLanguageFile", "") == mforms::ResultCancel)
      return;
  }

  GRTCodeEditor *editor = add_editor(is_script, language);
  if (!editor->load(path)) {
    close_editor(editor);
    return;
  }
#ifdef _DEBUG
  editor->test_markup();
#endif
}

GRTCodeEditor *GRTShellWindow::show_file_at_line(const std::string &path, int line) {
  open_file_in_editor(path, true);
  GRTCodeEditor *editor = get_editor_for(path, true);
  if (!editor)
    add_output(base::strfmt("Cannot open file %s", path.c_str()));
  else {
    ssize_t start, length;
    editor->get_editor()->get_range_of_line(line, start, length);
    editor->get_editor()->set_selection(start, 0);
  }
  return editor;
}

void GRTShellWindow::add_new_script() {
  NewPluginDialog wizard(this, bec::GRTManager::get()->get_data_file_path("script_templates"));
  std::string path;
  std::string code;
  bool is_script;
  std::string language;

  if (wizard.run(path, code, is_script, language)) {
    GRTCodeEditor *editor = add_editor(is_script, language);
    if (!path.empty() && base::basename(path) == path)
      path = base::makePath(bec::GRTManager::get()->get_user_script_path(), path);
    editor->set_path(path);
    editor->set_text(code);
  }

  save_state();
}

bool GRTShellWindow::add_output(const std::string &text) {
  _output_text.append_text(text, true);
  return true;
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::set_editor_title(GRTCodeEditor *editor, const std::string &title) {
  int index = _main_tab.get_page_index(editor);
  if (index >= 0)
    _main_tab.set_tab_title(index, editor->get_title());
}

//--------------------------------------------------------------------------------------------------

/**
 * Called from the UI context when WB is about to quit. Check if we have pending changes.
 * Return true if we are clear, false otherwise.
 */
bool GRTShellWindow::request_quit() {
  std::vector<GRTCodeEditor *>::reverse_iterator editor;
  while ((editor = _editors.rbegin()) != _editors.rend()) {
    if (!(*editor)->can_close())
      return false;
    else
      close_editor(*editor);
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::add_files_from_dir(mforms::TreeNodeRef parent, const std::string &dirname, bool is_script) {
  GDir *dir = g_dir_open(dirname.c_str(), 0, NULL);
  if (!dir)
    return;

  while (const gchar *name = g_dir_read_name(dir)) {
    if (g_str_has_suffix(name, ".py")) {
      mforms::TreeNodeRef node = parent->add_child();
      node->set_string(0, name);
      if (is_script)
        node->set_tag(std::string("s").append(dirname).append(G_DIR_SEPARATOR_S).append(name));
      else
        node->set_tag(std::string("m").append(dirname).append(G_DIR_SEPARATOR_S).append(name));
    }
  }

  g_dir_close(dir);
}

void GRTShellWindow::refresh_files() {
  mforms::TreeNodeRef node;

  _files_tree->clear();

  node = _files_tree->root_node()->add_child();
  node->set_string(0, "User Scripts");
  node->set_icon_path(0, "folder");
  add_files_from_dir(node, bec::GRTManager::get()->get_user_script_path(), true);
  node->expand();

  node = _files_tree->root_node()->add_child();
  node->set_string(0, "User Modules");
  node->set_icon_path(0, "folder");
  add_files_from_dir(node, bec::GRTManager::get()->get_user_module_path(), false);
  node->expand();

  node = _files_tree->root_node()->add_child();
  node->set_string(0, "User Libraries");
  node->set_icon_path(0, "folder");
  add_files_from_dir(node, bec::GRTManager::get()->get_user_library_path(), true);
  node->expand();
}

void GRTShellWindow::file_list_activated(mforms::TreeNodeRef node, int column) {
  if (node) {
    std::string path = node->get_tag();
    if (!path.empty()) {
      open_file_in_editor(path.substr(1), path[0] == 's');
    }
  }
}

void GRTShellWindow::on_file_save(const std::string &file) {
  refresh_files();
  if (_debugger)
    _debugger->refresh_file(file);
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::delete_selected_file() {
  mforms::TreeNodeRef node(_files_tree->get_selected_node());
  if (node) {
    std::string path = node->get_tag();
    if (!path.empty()) {
      std::string fn = path.substr(1);
      if (mforms::Utilities::show_message(
            _("Delete File"),
            base::strfmt(_("Really delete '%s' from disk? This operation cannot be undone."), fn.c_str()), _("Delete"),
            _("Cancel")) == mforms::ResultOk) {
        ::g_remove(fn.c_str());
        ::g_remove((fn + 'c').c_str());
        refresh_files();
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

mforms::Button *GRTShellWindow::add_tool_button(const std::string &image, const std::function<void()> &action,
                                                const std::string &tooltip, bool left) {
  App *app = App::get();
  Button *b = manage(new Button(ToolButton));
  b->set_icon(app->get_resource_path(image));
  b->set_tooltip(tooltip);
#ifdef __APPLE__
  b->set_size(-1, 24);
#endif
  scoped_connect(b->signal_clicked(), action);
  if (left)
    _toolbar.add(b, false, true);
  else
    _toolbar.add_end(b, false, true);
  return b;
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::add_tool_separator() {
  App *app = App::get();
  ImageBox *image = manage(new ImageBox());
  image->set_image(app->get_resource_path("statusbar_separator.png"));
  image->set_image_align(MiddleCenter);
  _toolbar.add(image, false, true);
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::load_state() {
  int x = _context->read_state("left", "scripting-shell", 100);
  int y = _context->read_state("top", "scripting-shell", 100);
  int width = _context->read_state("width", "scripting-shell", 800);
  int height = _context->read_state("height", "scripting-shell", 600);

  set_size(width, height);
  set_position(x, y);

  // Restore divider positions.
  _hsplitter.set_divider_position(_context->read_state("main-splitter", "scripting-shell", 250));
  _global_splitter.set_divider_position(_context->read_state("global-splitter", "scripting-shell", 400));
  _modules_splitter.set_divider_position(_context->read_state("modules-splitter", "scripting-shell", 400));
  _classes_splitter.set_divider_position(_context->read_state("classes-splitter", "scripting-shell", 400));
  _snippet_splitter.set_divider_position(_context->read_state("snippets-splitter", "scripting-shell", 400));

  _shell_text.set_font(bec::GRTManager::get()->get_app_option_string("workbench.scripting.ScriptingShell:Font"));
  _snippet_text.set_font(bec::GRTManager::get()->get_app_option_string("workbench.scripting.ScriptingEditor:Font"));
  for (std::vector<GRTCodeEditor *>::iterator editor = _editors.begin(); editor != _editors.end(); editor++)
    (*editor)->set_font(bec::GRTManager::get()->get_app_option_string("workbench.scripting.ScriptingEditor:Font"));

  _lower_tab_height = _context->read_state("editor-splitter", "scripting-shell", 400);
  on_tab_changed();
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::save_state() {
  // Store form's size and position.
  _context->save_state("left", "scripting-shell", get_x());
  _context->save_state("top", "scripting-shell", get_y());
  _context->save_state("width", "scripting-shell", get_width());
  _context->save_state("height", "scripting-shell", get_height());

  // Store all divider positions.
  _context->save_state("main-splitter", "scripting-shell", _hsplitter.get_divider_position());
  _context->save_state("global-splitter", "scripting-shell", _global_splitter.get_divider_position());
  _context->save_state("modules-splitter", "scripting-shell", _modules_splitter.get_divider_position());
  _context->save_state("classes-splitter", "scripting-shell", _classes_splitter.get_divider_position());
  _context->save_state("snippet-splitter", "scripting-shell", _snippet_splitter.get_divider_position());
}

//--------------------------------------------------------------------------------------------------

/**
 *  Triggered when the shell window was closed by the user. We can use this event to store our state.
 */
void GRTShellWindow::shell_closed() {
  save_state();
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered when a tab is about to close. Don't allow shell and snippets to close and check if
 * editors are dirty.
 */
bool GRTShellWindow::on_tab_closing(int index) {
  if (index == 0 || index == 1)
    return false;

  GRTCodeEditor *editor = _editors[index - EDITOR_TAB_OFFSET];
  if (editor->can_close()) {
    close_editor(editor);
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::on_tab_changed() {
  GRTCodeEditor *editor = get_active_editor();
  mforms::MenuItem *_run = _menu.find_item("run");
  if (editor) {
    bool exec_enabled = (editor->get_language() == "python");

    _save_button->set_enabled(true);
    _save_as_button->set_enabled(true);
    _run_button->set_enabled(exec_enabled);
    if (_run)
      _run->set_enabled(exec_enabled);
    _step_button->set_enabled(exec_enabled);
    _clear_script_output_button->set_enabled(true);

    if (_close_script_tab_button)
      _close_script_tab_button->set_enabled(true);

    _show_find_button->set_enabled(true);
    _right_splitter.set_expanded(false, true);
  } else {
    _save_button->set_enabled(false);
    _save_as_button->set_enabled(false);
    _run_button->set_enabled(false);
    if (_run)
      _run->set_enabled(false);
    _step_button->set_enabled(false);
    _clear_script_output_button->set_enabled(false);

    if (_close_script_tab_button)
      _close_script_tab_button->set_enabled(false);

    _show_find_button->set_enabled(false);
    _right_splitter.set_expanded(false, false);
  }
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::activate_output_tab() {
  _lower_tab.set_active_tab(0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the editor which is currently editing the given file.
 */
GRTCodeEditor *GRTShellWindow::get_editor_for(const std::string &path, bool select_tab) {
#ifdef _MSC_VER
  // We probably would need g_utf8_normalize too if we want it really good, but since this is
  // supposed to be a temporary solution...
  gchar *path1 = g_utf8_strdown(path.c_str(), -1);
  for (std::vector<GRTCodeEditor *>::iterator editor = _editors.begin(); editor != _editors.end(); editor++) {
    gchar *path2 = g_utf8_strdown((*editor)->get_path().c_str(), -1);
    if (g_utf8_collate(path1, path2) == 0) {
      if (select_tab)
        _main_tab.set_active_tab((int)(editor - _editors.begin() + EDITOR_TAB_OFFSET));

      g_free(path2);
      return *editor;
    }
    g_free(path2);
  }

  g_free(path1);
#else
  for (std::vector<GRTCodeEditor *>::iterator editor = _editors.begin(); editor != _editors.end(); editor++) {
    if ((*editor)->get_path() == path) {
      if (select_tab)
        _main_tab.set_active_tab(int(editor - _editors.begin() + EDITOR_TAB_OFFSET));

      return *editor;
    }
  }
#endif
  return NULL;
}

GRTCodeEditor *GRTShellWindow::get_active_editor() {
  int index = _main_tab.get_active_tab() - EDITOR_TAB_OFFSET;
  if (index >= 0 && index < (int)_editors.size())
    return _editors[index];
  return 0;
}

//--------------------------------------------------------------------------------------------------

template <class C>
struct CompareNamedObject {
  bool operator()(C *a, C *b) {
    return a->name() < b->name();
  }
};

void GRTShellWindow::refresh_modules_tree() {
  IconManager *im = IconManager::get_instance();
  std::string mod_icon = im->get_icon_path("grt_module.png");
  ;
  std::string fun_icon = im->get_icon_path("grt_function.png");
  ;
  _modules_tree.clear();

  std::vector<grt::Module *> modules(grt::GRT::get()->get_modules());
  std::sort(modules.begin(), modules.end(), CompareNamedObject<grt::Module>());

  for (std::vector<grt::Module *>::const_iterator m = modules.begin(); m != modules.end(); ++m) {
    mforms::TreeNodeRef mod_node = _modules_tree.add_node();
    const std::vector<grt::Module::Function> functions((*m)->get_functions());

    if ((*m)->description().empty())
      mod_node->set_string(0, (*m)->name());
    else
      mod_node->set_string(0, (*m)->name() + " *");
    mod_node->set_icon_path(0, mod_icon);
    mod_node->set_tag("m");

    for (std::vector<grt::Module::Function>::const_iterator f = functions.begin(); f != functions.end(); ++f) {
      mforms::TreeNodeRef fun_node = mod_node->add_child();

      fun_node->set_string(0, f->name);
      fun_node->set_icon_path(0, fun_icon);
    }
  }
}

std::string GRTShellWindow::get_module_node_description(const mforms::TreeNodeRef &node) {
  std::string value;
  if (node->get_parent() == _modules_tree.root_node()) {
    std::string name = node->get_string(0);
    if (!name.empty() && name[name.size() - 1] == '*')
      name = name.substr(0, name.size() - 2);
    grt::Module *module = grt::GRT::get()->get_module(name);
    if (module) {
      std::string descr;

      descr.append("Module: " + module->name() + "\n");
      descr.append("Path: " + module->path() + "\n");
      descr.append("Language: " + module->get_loader()->get_loader_name() + "\n");
      descr.append("Extends: " + module->extends() + "\n");
      descr.append("Implements: ");
      for (std::vector<std::string>::const_iterator iter = module->get_interfaces().begin();
           iter != module->get_interfaces().end(); ++iter) {
        descr.append(*iter).append("\n");
      }
      descr.append("\n\n").append(module->description());
      value = descr;
    }
  } else {
    std::string name = node->get_parent()->get_string(0);
    if (!name.empty() && name[name.size() - 1] == '*')
      name = name.substr(0, name.size() - 2);
    grt::Module *module = grt::GRT::get()->get_module(name);
    if (module) {
      const grt::Module::Function *func = module->get_function(node->get_string(0));

      value = base::strfmt("Function:\n    %s %s(%s)\n\n", fmt_type_spec(func->ret_type).c_str(), func->name.c_str(),
                           fmt_arg_spec_list(func->arg_types).c_str());
      value.append("Arguments:\n");
      std::string args;

      for (grt::ArgSpecList::const_iterator arg = func->arg_types.begin(); arg != func->arg_types.end(); ++arg) {
        if (!arg->name.empty())
          args.append("    - ").append(arg->name).append(": ").append(arg->doc).append("\n");
        else
          args.append("    - ").append(fmt_type_spec(arg->type)).append("\n");
      }
      value.append(args);

      value.append("\n").append(func->description);
    }
  }
  return value;
}

//--------------------------------------------------------------------------------------------------

void GRTShellWindow::refresh_classes_tree() {
  _classes_tree.clear();
  switch (_classes_sorting.get_selected_index()) {
    case 0:
      refresh_classes_tree_by_name();
      break;
    case 1:
      refresh_classes_tree_by_hierarchy();
      break;
    case 2:
      refresh_classes_tree_by_package();
      break;
  }
}

static std::string struct_member_icon(grt::TypeSpec type) {
  IconManager *im = IconManager::get_instance();
  switch (type.base.type) {
    case grt::ListType:
      return im->get_icon_path("grt_list.png");
    case grt::DictType:
      return im->get_icon_path("grt_dict.png");
    case grt::ObjectType:
      return im->get_icon_path("grt_object.png");
    default:
      return im->get_icon_path("grt_simple_type.png");
  }
  return im->get_icon_path("grt_simple_type.png");
}

struct SortableClassMember {
  std::string name;
  std::string caption;
  std::string type;
  std::string icon;
  std::string tag;

  bool operator<(const SortableClassMember &o) const {
    return name < o.name;
  }
};

static void scan_class_members(mforms::TreeNodeRef node, grt::MetaClass *gstruct) {
  IconManager *im = IconManager::get_instance();
  std::vector<SortableClassMember> members;
  for (grt::MetaClass::MethodList::const_iterator mem = gstruct->get_methods_partial().begin();
       mem != gstruct->get_methods_partial().end(); ++mem) {
    SortableClassMember m;
    m.name = mem->second.name;
    m.caption = gstruct->get_member_attribute(mem->second.name, "caption");
    m.type = mem->second.ret_type.base.type == grt::AnyType ? "void" : grt::fmt_type_spec(mem->second.ret_type);
    m.icon = im->get_icon_path("grt_function.png");

    std::string value;
    value = base::strfmt("Function:\n    %s %s(%s)\n", m.type.c_str(), mem->second.name.c_str(),
                         fmt_arg_spec_list(mem->second.arg_types).c_str());
    value.append(gstruct->get_member_attribute(mem->second.name, "caption")).append("\n");
    value.append("Arguments:\n");
    std::string args;

    for (grt::ArgSpecList::const_iterator arg = mem->second.arg_types.begin(); arg != mem->second.arg_types.end();
         ++arg) {
      if (!arg->name.empty())
        args.append("    - ").append(arg->name).append(": ").append(arg->doc).append("\n");
      else
        args.append("    - ").append(fmt_type_spec(arg->type)).append("\n");
    }
    value.append(args);

    value.append("\n").append(gstruct->get_member_attribute(mem->second.name, "desc"));

    m.tag = value;
    members.push_back(m);
  }

  for (grt::MetaClass::MemberList::const_iterator mem = gstruct->get_members_partial().begin();
       mem != gstruct->get_members_partial().end(); ++mem) {
    SortableClassMember m;
    m.name = mem->second.name;
    m.caption = gstruct->get_member_attribute(mem->second.name, "caption");
    m.type = grt::fmt_type_spec(mem->second.type);
    m.icon = struct_member_icon(mem->second.type);
    m.tag = base::strfmt("Member:\n    %s %s\n%s\n\n", m.type.c_str(), m.name.c_str(),
                         gstruct->get_member_attribute(mem->second.name, "desc").c_str());
    members.push_back(m);
  }
  std::sort(members.begin(), members.end());

  for (std::vector<SortableClassMember>::const_iterator i = members.begin(); i != members.end(); ++i) {
    mforms::TreeNodeRef mnode = node->add_child();

    mnode->set_string(0, i->name);
    mnode->set_string(1, i->type);
    mnode->set_string(2, i->caption);
    mnode->set_icon_path(0, i->icon);
    mnode->set_tag(i->tag);
  }
}

void GRTShellWindow::refresh_classes_tree_by_name() {
  std::list<grt::MetaClass *> metaclasses(grt::GRT::get()->get_metaclasses());

  std::string struct_icon = IconManager::get_instance()->get_icon_path("grt_struct.png");

  metaclasses.sort(CompareNamedObject<grt::MetaClass>());

  for (std::list<grt::MetaClass *>::const_iterator iter = metaclasses.begin(); iter != metaclasses.end(); ++iter) {
    grt::MetaClass *gstruct = *iter;
    mforms::TreeNodeRef node;

    node = _classes_tree.add_node();
    node->set_tag(base::strfmt("Class:\n    %s %s\n\n%s", gstruct->name().c_str(),
                               gstruct->parent() ? base::strfmt("(%s)", gstruct->parent()->name().c_str()).c_str() : "",
                               (*iter)->get_attribute("desc").c_str()));
    node->set_string(0, gstruct->name());
    node->set_string(2, gstruct->get_attribute("caption"));
    node->set_icon_path(0, struct_icon);

    scan_class_members(node, gstruct);
  }
}

static void scan_subclasses(const std::list<grt::MetaClass *> &metaclasses, mforms::TreeNodeRef parnode,
                            grt::MetaClass *parent) {
  std::string struct_icon = IconManager::get_instance()->get_icon_path("grt_struct.png");

  for (std::list<grt::MetaClass *>::const_iterator iter = metaclasses.begin(); iter != metaclasses.end(); ++iter) {
    mforms::TreeNodeRef node;
    if ((*iter)->parent() != parent)
      continue;

    node = parnode->add_child();
    node->set_tag((*iter)->get_attribute("desc"));
    node->set_string(0, (*iter)->name());
    node->set_string(2, (*iter)->get_attribute("caption"));
    node->set_icon_path(0, struct_icon);

    scan_class_members(node, *iter);

    // add child structs
    scan_subclasses(metaclasses, node, *iter);
  }
}

void GRTShellWindow::refresh_classes_tree_by_hierarchy() {
  std::list<grt::MetaClass *> metaclasses(grt::GRT::get()->get_metaclasses());
  metaclasses.sort(CompareNamedObject<grt::MetaClass>());

  scan_subclasses(metaclasses, _classes_tree.root_node(),
                  grt::GRT::get()->get_metaclass(grt::internal::Object::static_class_name()));
}

void GRTShellWindow::refresh_classes_tree_by_package() {
  IconManager *im = IconManager::get_instance();
  std::map<std::string, mforms::TreeNodeRef> package_nodes;
  std::list<grt::MetaClass *> metaclasses(grt::GRT::get()->get_metaclasses());
  metaclasses.sort(CompareNamedObject<grt::MetaClass>());

  std::string struct_icon = im->get_icon_path("grt_struct.png");

  for (std::list<grt::MetaClass *>::const_iterator iter = metaclasses.begin(); iter != metaclasses.end(); ++iter) {
    std::string pkgname = (*iter)->name();

    std::string::size_type p = pkgname.rfind('.');
    if (p != std::string::npos)
      pkgname = pkgname.substr(0, p);
    else
      pkgname = "";

    mforms::TreeNodeRef pkgnode = package_nodes[pkgname];
    if (!pkgnode) {
      pkgnode = _classes_tree.add_node();
      pkgnode->set_string(0, pkgname);
      pkgnode->set_icon_path(0, "folder");

      package_nodes[pkgname] = pkgnode;
    }

    mforms::TreeNodeRef node = pkgnode->add_child();
    node->set_tag((*iter)->get_attribute("desc"));
    node->set_string(0, (*iter)->name());
    node->set_string(2, (*iter)->get_attribute("caption"));
    node->set_icon_path(0, struct_icon);

    scan_class_members(node, *iter);
  }
}

std::string GRTShellWindow::get_class_node_description(const mforms::TreeNodeRef &selected) {
  return selected->get_tag();
}

//--------------------------------------------------------------------------------------------------

static bool find_expandable_member(const grt::MetaClass::Member *member, bool *expandable) {
  if (!grt::is_simple_type(member->type.base.type))
    *expandable = true;
  return !*expandable;
}

static void globals_get_node_info(const grt::ValueRef &value, std::string &type, std::string &icon, bool &expandable) {
  IconManager *im = IconManager::get_instance();
  type = grt::type_to_str(value.type());
  expandable = false;

  switch (value.type()) {
    case grt::ListType: {
      grt::BaseListRef l(grt::BaseListRef::cast_from(value));
      std::string struct_name;
      if (l.content_type() != grt::AnyType) {
        type += " [";
        if (l.content_type() == grt::ObjectType) {
          if (l.content_class_name().empty()) {
            type += "object";
            struct_name = "";
          } else {
            type += "object:" + l.content_class_name();
            struct_name = l.content_class_name();
          }
        } else {
          if (l.content_type() == grt::AnyType)
            type += "*";
          else
            type += grt::type_to_str(l.content_type());
        }
        type += "]";
      }
      if (!struct_name.empty())
        icon = im->get_icon_path(im->get_icon_id(grt::GRT::get()->get_metaclass(struct_name), Icon16, "many_$"));
      if (icon.empty())
        icon = im->get_icon_path("grt_list.png");

      for (size_t c = l.count(), i = 0; i < c; i++) {
        if (!grt::is_simple_type(l[i].type())) {
          expandable = true;
          break;
        }
      }
    } break;

    case grt::DictType: {
      grt::DictRef d(grt::DictRef::cast_from(value));
      if (d.content_type() != grt::AnyType) {
        type += " [";
        if (d.content_type() == grt::ObjectType) {
          type += "object:" + d.content_class_name();
          icon = im->get_icon_path(im->get_icon_id(grt::GRT::get()->get_metaclass(d.content_class_name()), Icon16));
        } else
          type += grt::type_to_str(d.content_type());
        type += "]";
      }
      if (icon.empty())
        icon = im->get_icon_path("grt_dict.png");

      for (grt::DictRef::const_iterator iter = d.begin(); iter != d.end(); ++iter) {
        if (!grt::is_simple_type(iter->second.type())) {
          expandable = true;
          break;
        }
      }
    } break;

    case grt::ObjectType: {
      grt::ObjectRef o(grt::ObjectRef::cast_from(value));
      type += ":" + std::string(o.class_name());

      icon = im->get_icon_path(im->get_icon_id(o, Icon16));
      if (icon.empty())
        icon = im->get_icon_path("grt_object.png");

      grt::MetaClass *meta = o.get_metaclass();

      meta->foreach_member(std::bind(&find_expandable_member, std::placeholders::_1, &expandable));
    } break;

    default:
      icon = im->get_icon_path("grt_simple_type.png");
      break;
  }
}

static void globals_rescan_list(mforms::TreeNodeRef &node, const std::string &path, const grt::BaseListRef &value) {
  char buffer[30];

  node->remove_children();
  for (size_t i = 0; i < value.count(); i++) {
    grt::ValueRef v = value.get(i);
    std::string label;

    sprintf(buffer, "%lu", (long unsigned)i);

    if (v.is_valid() && !grt::is_simple_type(v.type())) {
      mforms::TreeNodeRef child = node->add_child();
      std::string type;
      std::string icon;
      bool expandable;

      globals_get_node_info(v, type, icon, expandable);
      child->set_tag(buffer);
      child->set_string(0, label.empty() ? buffer : label);
      child->set_string(1, type);
      child->set_icon_path(0, icon);
      if (v.type() == grt::ObjectType && label.empty()) {
        grt::ObjectRef o(grt::ObjectRef::cast_from(v));
        std::string s = std::string("[") + buffer + "]";
        try {
          if (o.has_member("name") && o.get_string_member("name") != "")
            s.append(" ").append(o.get_string_member("name"));
        } catch (grt::type_error &) {
          s.append(" ").append("?");
        }
        child->set_string(0, s);
      }

      if (expandable)
        child->add_child();
    }
  }
}

static void globals_rescan_dict(mforms::TreeNodeRef &node, const std::string &path, const grt::DictRef &value) {
  node->remove_children();
  for (grt::DictRef::const_iterator item = value.begin(); item != value.end(); ++item) {
    std::string key(item->first);
    grt::ValueRef v(item->second);
    std::string label;

    if (v.is_valid() && !grt::is_simple_type(v.type())) {
      mforms::TreeNodeRef child = node->add_child();
      std::string type;
      std::string icon;
      bool expandable;

      globals_get_node_info(v, type, icon, expandable);
      child->set_tag(key);
      child->set_string(0, label.empty() ? key : label);
      child->set_string(1, type);
      child->set_icon_path(0, icon);
      if (v.type() == grt::ObjectType && label.empty()) {
        grt::ObjectRef o(grt::ObjectRef::cast_from(v));
        if (o.has_member("name") && o.get_string_member("name") != "")
          child->set_string(0, o.get_string_member("name"));
        else
          child->set_string(0, "[" + child->get_tag() + "]");
      }

      if (expandable)
        child->add_child();
    }
  }
}

static bool globals_rescan_member(const grt::MetaClass::Member *mem, mforms::TreeNodeRef &node,
                                  const grt::ObjectRef &value) {
  std::string name(mem->name);
  grt::ValueRef v(value.get_member(name));
  std::string label;

  if (v.is_valid() && !is_simple_type(v.type())) {
    mforms::TreeNodeRef child = node->add_child();
    std::string type;
    std::string icon;
    bool expandable;

    globals_get_node_info(v, type, icon, expandable);
    child->set_tag(name);
    child->set_string(0, label.empty() ? name : label);
    child->set_string(1, type);
    child->set_icon_path(0, icon);
    if (expandable)
      child->add_child();
  }
  return true;
}

static void globals_rescan_object(mforms::TreeNodeRef &node, const std::string &path, const grt::ObjectRef &value) {
  grt::MetaClass *meta = value.get_metaclass();

  node->remove_children();
  meta->foreach_member(std::bind(&globals_rescan_member, std::placeholders::_1, node, value));
}

static void globals_rescan_value(mforms::TreeNodeRef &node, const std::string &path, const grt::ValueRef &value) {
  switch (value.type()) {
    case grt::ListType:
      globals_rescan_list(node, path, grt::BaseListRef::cast_from(value));
      break;
    case grt::DictType:
      globals_rescan_dict(node, path, grt::DictRef::cast_from(value));
      break;
    case grt::ObjectType:
      globals_rescan_object(node, path, grt::ObjectRef::cast_from(value));
      break;
    default:
      break;
  }
}

void GRTShellWindow::refresh_globals_tree() {
  std::string path = _global_combo.get_string_value();

  if (path.empty())
    path = "/";

  try {
    grt::ValueRef value = grt::GRT::get()->get(path);
    if (value.is_valid()) {
      _global_tree.clear();

      mforms::TreeNodeRef root = _global_tree.add_node();
      std::string type;
      std::string icon;
      bool expandable;

      globals_get_node_info(value, type, icon, expandable);
      root->set_string(0, path);
      root->set_string(1, type);
      root->set_icon_path(0, icon);
      root->set_tag(path);
      globals_rescan_value(root, path, value);
      //      root->expand();
    }
  } catch (const grt::bad_item &) {
    // ignore
  }
}

void GRTShellWindow::globals_expand_toggle(const mforms::TreeNodeRef &node, bool expanded) {
  if (expanded) {
    grt::ValueRef value = get_global_at_node(node);
    if (value.is_valid()) {
      mforms::TreeNodeRef mnode = node;
      globals_rescan_value(mnode, mnode->get_tag(), value);
    }
  }
}

grt::ValueRef GRTShellWindow::get_global_at_node(const mforms::TreeNodeRef &node) {
  return grt::GRT::get()->get(get_global_path_at_node(node));
}

std::string GRTShellWindow::get_global_path_at_node(const mforms::TreeNodeRef &node) {
  std::string path;
  mforms::TreeNodeRef parent = node;

  while (parent != _global_tree.root_node()) {
    if (parent->get_tag() == "/")
      path = "/" + path;
    else {
      if (path.empty())
        path = parent->get_tag();
      else
        path = parent->get_tag() + "/" + path;
    }
    parent = parent->get_parent();
  }
  return path;
}

void GRTShellWindow::refresh_global_list() {
  _global_list.clear();
  if (_inspector) {
    for (size_t c = _inspector->count(), i = 0; i < c; i++) {
      mforms::TreeNodeRef node = _global_list.add_node();
      std::string value;
      _inspector->get_field(i, 0, value);
      node->set_string(0, value);
      _inspector->get_field(i, 1, value);
      node->set_string(1, value);
      value = IconManager::get_instance()->get_icon_path(_inspector->get_field_icon(i, 0, Icon16));
      node->set_icon_path(0, value);
    }
  }
}

void GRTShellWindow::refresh_notifs_list() {
  const std::map<std::string, base::NotificationCenter::NotificationHelp> &info =
    base::NotificationCenter::get()->get_registered_notifications();
  std::map<std::string, std::vector<std::string> > contexts;

  _notifs_tree.clear();
  for (std::map<std::string, base::NotificationCenter::NotificationHelp>::const_iterator i = info.begin();
       i != info.end(); ++i)
    contexts[i->second.context].push_back(i->first);

  for (std::map<std::string, std::vector<std::string> >::const_iterator iter = contexts.begin(); iter != contexts.end();
       ++iter) {
    mforms::TreeNodeRef node = _notifs_tree.add_node();
    node->set_string(0, iter->first);
    node->set_icon_path(0, "folder");

    for (std::vector<std::string>::const_iterator n = iter->second.begin(); n != iter->second.end(); ++n) {
      mforms::TreeNodeRef nnode = node->add_child();
      nnode->set_string(0, *n);
    }
    node->expand();
  }
}

void GRTShellWindow::cut() {
  GRTCodeEditor *editor = get_active_editor();
  if (editor)
    editor->get_editor()->cut();
  else if (_shell_entry.has_focus())
    _shell_entry.cut();
}

void GRTShellWindow::copy() {
  GRTCodeEditor *editor = get_active_editor();
  if (editor)
    editor->get_editor()->copy();
  else if (_shell_entry.has_focus())
    _shell_entry.copy();
}

void GRTShellWindow::paste() {
  GRTCodeEditor *editor = get_active_editor();
  if (editor)
    editor->get_editor()->paste();
  else if (_shell_entry.has_focus())
    _shell_entry.paste();
}

void GRTShellWindow::select_all() {
  GRTCodeEditor *editor = get_active_editor();
  if (editor)
    editor->get_editor()->select_all();
  else if (_shell_entry.has_focus())
    _shell_entry.select(base::Range(0, (size_t)-1));
}
