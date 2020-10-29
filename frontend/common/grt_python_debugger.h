/*
 * Copyright (c) 2010, 2020, Oracle and/or its affiliates.
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

#ifndef __GRT_PYTHON_DEBUGGER_H_
#define __GRT_PYTHON_DEBUGGER_H_

#include "python_context.h"
#include "grt/grt_manager.h"
#include "base/trackable.h"

#include "mforms/tabview.h"
#include "mforms/treeview.h"
#include "mforms/utilities.h"
#include "mforms/panel.h"
#include "mforms/splitter.h"
#include "mforms/textentry.h"
#include "mforms/textbox.h"

class GRTCodeEditor;
class GRTShellWindow;

class PythonDebugger : public base::trackable {
  GRTShellWindow *_shell;
  mforms::TabView *_lower_tabs;

  mforms::TreeView *_stack_list;
  mforms::TreeView *_breakpoint_list;
  mforms::TreeView *_variable_list;

  GRTCodeEditor *_stack_position_editor;
  int _stack_position_line;

  std::string _tmpfile_name;

  grt::AutoPyObject _pdb;
  std::string _pdb_varname;

  bec::GRTManager::Timer *_heartbeat_timeout_timer;

  bool _pause_clicked;
  bool _program_stopped;

  void show_stack();

  bool ensure_code_saved();

  void edit_breakpoint(mforms::TreeNodeRef node, int column, std::string value);
  void line_gutter_clicked(int margin, int line, mforms::ModifierKey mods, GRTCodeEditor *editor);
  void editor_text_changed(int line, int linesAdded, GRTCodeEditor *editor);
  void stack_selected();

  bool heartbeat_timeout();

private:
  bool toggle_breakpoint(const char *file, int line);

public:
  static PythonDebugger *from_cobject(PyObject *cobj);
  PyObject *as_cobject();

  void debug_print(const std::string &s);
  void ui_clear_breakpoints();
  void ui_add_breakpoint(const char *file, int line, const char *condition);
  const char *ui_program_stopped(const char *file, int line, int reason);
  void ui_clear_stack();
  void ui_add_stack(const char *location, const char *file, int line);

  void ui_clear_variables();
  void ui_add_variable(const char *varname, const char *value);

public:
  PythonDebugger(GRTShellWindow *shell, mforms::TabView *tabview);
  void init_pdb();

  bool program_stopped() {
    return _program_stopped;
  }

  void editor_added(GRTCodeEditor *editor);
  void editor_closed(GRTCodeEditor *editor);

  void refresh_file(const std::string &file);

  void run(GRTCodeEditor *editor, bool stepping = false);
  void stop();
  void pause();
  void step_into();
  void step();
  void step_out();
  void continue_();
};

#endif
