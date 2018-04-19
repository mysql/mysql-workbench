/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __GRT_SHELL_WINDOW_H__
#define __GRT_SHELL_WINDOW_H__
namespace wb {
  class WBContext;
};

#include "mforms/treeview.h"
#include "mforms/code_editor.h"
#include "mforms/menubar.h"
#include "mforms/splitter.h"
#include "mforms/tabview.h"
#include "mforms/panel.h"
#include "mforms/textentry.h"
#include "mforms/textbox.h"
#include "mforms/form.h"
#include "mforms/label.h"
#include "mforms/button.h"
#include "mforms/selector.h"
#include "mforms/box.h"

class GRTCodeEditor;
class PythonDebugger;

class GRTShellWindow : public mforms::Form {
  friend class PythonDebugger;

public:
  GRTShellWindow(wb::WBContext *context);

  virtual void show(bool flag = true);

  bool execute_script(const std::string &script, const std::string &language);

  GRTCodeEditor *add_editor(bool is_script, const std::string &language);
  void add_new_script();
  void open_script_file();

  GRTCodeEditor *show_file_at_line(const std::string &path, int line);

  void open_file_in_editor(const std::string &path, bool is_script);

  void close_editor(GRTCodeEditor *editor);
  void refresh_files();

  void set_editor_title(GRTCodeEditor *editor, const std::string &title);

  bool request_quit();

  bool add_output(const std::string &text);
  void activate_output_tab();

  GRTCodeEditor *get_editor_for(const std::string &path, bool select_tab);
  GRTCodeEditor *get_active_editor();

  void on_file_save(const std::string &file);

  bool can_close();

protected:
  wb::WBContext *_context;

  mforms::MenuBar _menu;
  mforms::Box _toolbar;
  mforms::Button *_save_button;
  mforms::Button *_save_as_button;
  mforms::Button *_clear_script_output_button;
  mforms::Button *_close_script_tab_button;

  mforms::Button *_run_button;
  mforms::Button *_step_button;
  mforms::Button *_step_into_button;
  mforms::Button *_step_out_button;
  mforms::Button *_continue_button;
  mforms::Button *_stop_button;
  mforms::Button *_pause_button;

  mforms::Button *_show_find_button;

  mforms::Box _content;
  mforms::Box _padding_box;
  mforms::Splitter _hsplitter;
#ifdef _MSC_VER
  mforms::Panel _side_header_panel;
#endif
  mforms::TabView _side_tab;
  mforms::TabView _main_tab;

  mforms::Button _file_add;
  mforms::Button _file_delete;
  mforms::TreeView *_files_tree;
  mforms::ContextMenu _files_menu;

  mforms::Box _global_box1;
  mforms::Box _global_box2;
  mforms::Splitter _global_splitter;
  mforms::Selector _global_combo;
  mforms::TreeView _global_tree;
  mforms::TextEntry _global_entry;
  mforms::TreeView _global_list;
  mforms::ContextMenu _global_menu;
  bec::ValueInspectorBE *_inspector;

  mforms::Box _classes_box;
  mforms::Splitter _classes_splitter;
  mforms::Selector _classes_sorting;
  mforms::TreeView _classes_tree;
  mforms::TextBox _classes_text;

  mforms::Splitter _modules_splitter;
  mforms::TreeView _modules_tree;
  mforms::TextBox _modules_text;

  mforms::Splitter _notifs_splitter;
  mforms::TreeView _notifs_tree;
  mforms::TextBox _notifs_text;

  mforms::Splitter _right_splitter;

  mforms::Box _shell_box;
  mforms::TextBox _shell_text;
  mforms::Box _shell_hbox;
  mforms::Label _shell_prompt;
  mforms::TextEntry _shell_entry;

  mforms::TabView _lower_tab;
#ifdef _MSC_VER
  mforms::Panel _lower_header_panel;
#endif
  mforms::TextBox _output_text;
  int _lower_tab_height;

  mforms::Splitter _snippet_splitter;
  mforms::TreeView *_snippet_list;
  mforms::Button *_snippet_delete_button;
  mforms::Button *_snippet_copy_button;
  mforms::CodeEditor _snippet_text;

  mforms::ContextMenu _snippet_menu;
  int _global_snippet_count;
  std::string _comment_prefix;
  std::string _script_extension;

  PythonDebugger *_debugger;

  std::vector<GRTCodeEditor *> _editors;
  
  bool _userSnippetsLoaded;
  bool _snippetClicked;

  void refresh_all();
  void side_tab_changed();

  void set_splitter_positions();

  bool capture_output(const grt::Message &msg, void *sender, bool send_to_output);

  void shell_action(mforms::TextEntryAction action);

  void handle_prompt(const std::string &text);
  void handle_output(const std::string &text);
  void handle_error(const std::string &text, const std::string &detail);

  void handle_global_menu(const std::string &action);

  void global_selected();
  void class_selected();
  void module_selected();
  void notif_selected();

  void delete_selected_file();

  void file_list_activated(mforms::TreeNodeRef node, int column);
  void add_files_from_dir(mforms::TreeNodeRef parent, const std::string &dir, bool is_script);

  void load_snippets_from(const std::string &path);
  void save_snippets();
  void refresh_snippets();
  void add_snippet();
  void del_snippet();
  void copy_snippet();
  void run_snippet();
  void scriptize_snippet();
  void snippet_selected();
  void snippet_changed(int line, int linesAdded);

  void snippet_menu_activate(const std::string &action);
  void file_menu_activate(const std::string &action);

  void shell_closed();

  void load_state();
  void save_state();

  void on_tab_changed();
  bool on_tab_closing(int index);

  virtual mforms::MenuBar *get_menubar() {
    return &_menu;
  }

private:
  void cut();
  void copy();
  void paste();
  void select_all();

  mforms::Button *add_tool_button(const std::string &image, const std::function<void()> &action,
                                  const std::string &tooltip, bool left = true);
  void add_tool_separator();

  void execute_file();
  void save_file(bool save_as);
  void close_tab();
  void show_find_panel();
  void show_replace_panel();

  void refresh_modules_tree();
  std::string get_module_node_description(const mforms::TreeNodeRef &node);

  void refresh_classes_tree();
  void refresh_classes_tree_by_name();
  void refresh_classes_tree_by_hierarchy();
  void refresh_classes_tree_by_package();
  std::string get_class_node_description(const mforms::TreeNodeRef &node);

  void globals_expand_toggle(const mforms::TreeNodeRef &node, bool expanded);
  std::string get_global_path_at_node(const mforms::TreeNodeRef &node);
  grt::ValueRef get_global_at_node(const mforms::TreeNodeRef &node);
  void refresh_globals_tree();

  void refresh_global_list();

  void refresh_notifs_list();

private:
  void debug_step();
  void debug_step_into();
  void debug_step_out();
  void debug_continue();
  void debug_stop();
  void debug_pause();
};

#endif

