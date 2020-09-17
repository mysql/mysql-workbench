/*
 * Copyright (c) 2011, 2020, Oracle and/or its affiliates.
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
#include "base/log.h"
#include "base/string_utilities.h"
#include "grt_shell_window.h"
#include "grt_code_editor.h"

#include "mforms/app.h"

DEFAULT_LOG_DOMAIN("pydebugger");

/* The PythonDebugger object is meant to be used as an editor tab for editing
 Python scripts. Executing the script in the debugger tab will save the script
 to a temporary file and run that file through the pdb instance for the
 PythonDebugger.
 */

using namespace mforms;

#define BP_ACTION_STOP 0
#define BP_ACTION_CONTINUE 1
#define BP_ACTION_STEP 2
#define BP_ACTION_STEP_INTO 3
#define BP_ACTION_STEP_OUT 4
#define BP_ACTION_PAUSE 5

static PyObject *ui_print(PyObject *unused, PyObject *args) {
  PyObject *self;
  char *s = nullptr;

  if (!PyArg_ParseTuple(args, "Os:ui_print", &self, &s)) {
    PyErr_Print();
    return nullptr;
  }

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  d->debug_print(s);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *ui_clear_breakpoints(PyObject *unused, PyObject *args) {
  PyObject *self;

  if (!PyArg_ParseTuple(args, "O:ui_clear_breakpoints", &self))
    return nullptr;

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  d->ui_clear_breakpoints();

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *ui_add_breakpoint(PyObject *unused, PyObject *args) {
  PyObject *self;
  const char *file = "";
  int active;
  int line = 0;
  const char *condition = "";

  if (!PyArg_ParseTuple(args, "Oisiz:ui_add_breakpoint", &self, &active, &file, &line, &condition))
    return nullptr;

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  d->ui_add_breakpoint(file, line, condition);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *ui_program_stopped(PyObject *unused, PyObject *args) {
  PyObject *self;
  const char *file = "";
  int line = 0;
  int reason = 0;

  if (!PyArg_ParseTuple(args, "Osii:ui_breakpoint_hit", &self, &file, &line, &reason))
    return nullptr;

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  const char *next_command = d->ui_program_stopped(file, line, reason);

  return Py_BuildValue("s", next_command);
}

static PyObject *ui_clear_stack(PyObject *unused, PyObject *args) {
  PyObject *self;

  if (!PyArg_ParseTuple(args, "O:ui_clear_stack", &self))
    return nullptr;

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  d->ui_clear_stack();

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *ui_add_stack(PyObject *unused, PyObject *args) {
  PyObject *self;
  const char *file = "";
  const char *location = "";
  int line = 0;

  if (!PyArg_ParseTuple(args, "Ossi:ui_add_stack", &self, &location, &file, &line))
    return nullptr;

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  d->ui_add_stack(location, file, line);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *ui_clear_variables(PyObject *unused, PyObject *args) {
  PyObject *self;

  if (!PyArg_ParseTuple(args, "O:ui_clear_variables", &self))
    return nullptr;

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  d->ui_clear_variables();

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *ui_add_variable(PyObject *unused, PyObject *args) {
  PyObject *self;
  const char *variable = "";
  const char *value = "";

  if (!PyArg_ParseTuple(args, "Oss:ui_add_variable", &self, &variable, &value))
    return nullptr;

  PythonDebugger *d = PythonDebugger::from_cobject(self);
  if (!d)
    return nullptr;

  d->ui_add_variable(variable, value);

  Py_INCREF(Py_None);
  return Py_None;
}

static int pdb_desc = 0;

static void init_pdb_python() {
  static PyObject *module = nullptr;

  if(module)
    return;
  
  static PyMethodDef ui_methods[] = {
    {"ui_print", ui_print, METH_VARARGS, nullptr},
    {"ui_clear_breakpoints", ui_clear_breakpoints, METH_VARARGS, nullptr},
    {"ui_add_breakpoint", ui_add_breakpoint, METH_VARARGS, nullptr},
    {"ui_program_stopped", ui_program_stopped, METH_VARARGS, nullptr},
    {"ui_clear_stack", ui_clear_stack, METH_VARARGS, nullptr},
    {"ui_add_stack", ui_add_stack, METH_VARARGS, nullptr},
    {"ui_clear_variables", ui_clear_variables, METH_VARARGS, nullptr},
    {"ui_add_variable", ui_add_variable, METH_VARARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}};

  static struct PyModuleDef wbpdbModuleDef = {
    PyModuleDef_HEAD_INIT,
    "wbpdb",      // name
    nullptr,         // documentation
    -1,              // size -1 meand it can not be re-initialized
    ui_methods,      //
    nullptr,         // reload method
    nullptr,         // traverse method
    nullptr,         // clear method
    nullptr          //  free method
  };

  module = PyModule_Create(&wbpdbModuleDef);
  
  PyObject *main = PyImport_AddModule("__main__");  //  Get module if exists
  PyDict_SetItemString(PyModule_GetDict(main), "wbpdb", module);
  
  grt::PythonContext *pyc = grt::PythonContext::get();

  PyObject *debugger = pyc->import_module("grt_python_debugger");

  if (debugger == nullptr)
    throw std::runtime_error("Could not import Python debugger");
  
  PyDict_SetItemString(PyModule_GetDict(debugger), "wbpdb", module);
  
}

PyObject *PythonDebugger::as_cobject() {
  PyObject* ret = PyCapsule_New(this, "PythonDebugger", nullptr);
  PyCapsule_SetContext(ret, &pdb_desc);
  return ret;
}

PythonDebugger *PythonDebugger::from_cobject(PyObject *cobj) {
  if (!PyCapsule_CheckExact(cobj))
    return nullptr;

  if (PyCapsule_GetContext(cobj) != &pdb_desc)
    return nullptr;

  return reinterpret_cast<PythonDebugger *>(PyCapsule_GetPointer(cobj, "PythonDebugger"));
}

PythonDebugger::PythonDebugger(GRTShellWindow *shell, mforms::TabView *tabview)
  : _shell(shell), _lower_tabs(tabview), _stack_position_editor(0), _stack_position_line(0) {
  _breakpoint_list = manage(new TreeView(TreeDefault | TreeFlatList));
  //  _breakpoint_list->add_column(CheckColumnType, " ", 30, true);
  _breakpoint_list->add_column(StringColumnType, "Breakpoint", 200, false);
  _breakpoint_list->add_column(StringColumnType, "Location", 200, false);
  _breakpoint_list->add_column(StringColumnType, "Condition", 200, true);
  _breakpoint_list->end_columns();
  _breakpoint_list->set_cell_edit_handler(std::bind(&PythonDebugger::edit_breakpoint, this, std::placeholders::_1,
                                                    std::placeholders::_2, std::placeholders::_3));
  _lower_tabs->add_page(_breakpoint_list, _("Breakpoints"));

  mforms::Splitter *spl = manage(new Splitter(true));

  _lower_tabs->add_page(spl, _("Debug Info"));

  _stack_list = manage(new TreeView(TreeDefault | TreeFlatList));
  _stack_list->add_column(StringColumnType, "#", 30, false);
  _stack_list->add_column(StringColumnType, "Stack Location", 300, false);
  _stack_list->add_column(StringColumnType, "File", 300, false);
  _stack_list->end_columns();
  spl->add(_stack_list);
  scoped_connect(_stack_list->signal_changed(), std::bind(&PythonDebugger::stack_selected, this));
  //_stack_list->signal_row_activated().connect();

  _variable_list = manage(new TreeView(TreeDefault | TreeFlatList));
  _variable_list->add_column(StringColumnType, "Variable", 200, false);
  _variable_list->add_column(StringColumnType, "Value", 400, false);
  _variable_list->end_columns();
  spl->add(_variable_list);

  spl->set_divider_position(500);
  _program_stopped = false;
}

void PythonDebugger::init_pdb() {
  WillEnterPython lock;

  init_pdb_python();

  grt::PythonContext *pyc = grt::PythonContext::get();

  PyObject *debugger_class = pyc->eval_string("grt_python_debugger.PyDebugger");
  if (!debugger_class)
    throw std::runtime_error("Could not initialize Python debugger");

  PyObject *ui = as_cobject();

  // PyDebugger(this) ctor
  PyObject *ctor_result = PyObject_Call(debugger_class, Py_BuildValue("(O)", ui), nullptr);
  Py_DECREF(ui);
  Py_DECREF(debugger_class);

  if (!ctor_result)
    throw std::runtime_error("Error instantiating Python debugger object");

  _pdb = ctor_result;
  Py_DECREF(ctor_result);

  // come up with a global variable name so that we have a reference to the debugger
  // object from Python itself
  _pdb_varname = base::strfmt("wbpdb_instance_%p", this);

  pyc->set_global(_pdb_varname, _pdb);
}

void PythonDebugger::editor_added(GRTCodeEditor *editor) {
  scoped_connect(editor->get_editor()->signal_gutter_clicked(),
                 std::bind(&PythonDebugger::line_gutter_clicked, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, editor));
  scoped_connect(
    editor->get_editor()->signal_changed(),
    std::bind(&PythonDebugger::editor_text_changed, this, std::placeholders::_1, std::placeholders::_2, editor));
}

void PythonDebugger::editor_closed(GRTCodeEditor *editor) {
  if (editor == _stack_position_editor)
    _stack_position_editor = 0;
}

bool PythonDebugger::ensure_code_saved() {
  GRTCodeEditor *editor = _shell->get_active_editor();

  if (editor->is_dirty()) {
    if (Utilities::show_warning("Debug Script", "Script must be saved to be debugged. Would you like to save it now?",
                                "Save", "Cancel", "") == ResultOk) {
      if (!editor->save(false))
        return false;
    } else
      return false;
  }
  return true;
}

void PythonDebugger::line_gutter_clicked(int margin, int line, mforms::ModifierKey mods, GRTCodeEditor *editor) {
  if (margin == 1 || margin == 0) // click line numbers or on the markers
  {
    WillEnterPython lock;

    if (toggle_breakpoint(editor->get_path().c_str(), line + 1)) // line numbers from editor are 0 based
      editor->get_editor()->show_markup(LineMarkupBreakpoint, line);
    else
      editor->get_editor()->remove_markup(LineMarkupBreakpoint | LineMarkupBreakpointHit, line);
  }
}

void PythonDebugger::editor_text_changed(int line, int linesAdded, GRTCodeEditor *editor) {
  if (linesAdded != 0) {
    WillEnterPython lock;

    std::string path = editor->get_path();

    grt::AutoPyObject r(
      PyObject_CallMethod(_pdb, "wdb_update_breakpoint", "(sii)", path.c_str(), line + 1, linesAdded),
      false);
    if (!r) {
      PyErr_Print();
      PyErr_Clear();
    }
  }
}

void PythonDebugger::edit_breakpoint(mforms::TreeNodeRef node, int column, std::string value) {
  int row = _breakpoint_list->row_for_node(node);
  if (column == 2 && row >= 0) // edit bp condition
  {
    WillEnterPython lock;
    grt::AutoPyObject r(PyObject_CallMethod(_pdb, "wdb_set_bp_condition", "(is)", row, value.c_str()),
                        false);
    if (!r) {
      // exception while running, dump the exception to the debugger console
      // exceptions in the script being debugged will be caught and reported by wdb_run
      debug_print("There was an unhandled internal exception setting a bp condition.\n");
      PyErr_Print();
      PyErr_Clear();
    }

    if (PyBool_Check(r) && r == Py_True) {
      node->set_string(column, value);
    }
  }
}

void PythonDebugger::refresh_file(const std::string &file) {
  WillEnterPython lock;
  grt::AutoPyObject r(PyObject_CallMethod(_pdb, "wdb_reload_module_for_file", "(s)", file.c_str()),
                      false);
}

void PythonDebugger::run(GRTCodeEditor *editor, bool stepping) {
  if (editor->is_dirty() && !ensure_code_saved())
    return;

  WillEnterPython lock;

  //  debug_print(base::strfmt("Running script %s...\n", _shell->get_path().c_str()));

  _pause_clicked = false;
  // run the script
  try {
    grt::AutoPyObject r(
      PyObject_CallMethod(_pdb, "wdb_run", "(si)", editor->get_path().c_str(), stepping ? 1 : 0),
      false);
    if (!r) {
      // exception while running, dump the exception to the debugger console
      // exceptions in the script being debugged will be caught and reported by wdb_run
      debug_print("There was an unhandled internal exception executing the script.\n");
      PyErr_Print();
      PyErr_Clear();
    }
  } catch (std::exception &e) {
    debug_print("There was an unhandled internal exception executing the script.\n");
    debug_print(e.what());
  }

  _variable_list->clear();
  _stack_list->clear();

  debug_print("Execution finished\n");
}

void PythonDebugger::step_into() {
  mforms::App::get()->exit_event_loop(BP_ACTION_STEP_INTO);
}

void PythonDebugger::step() {
  mforms::App::get()->exit_event_loop(BP_ACTION_STEP);
}

void PythonDebugger::continue_() {
  mforms::App::get()->exit_event_loop(BP_ACTION_CONTINUE);
}

void PythonDebugger::stop() {
  mforms::App::get()->exit_event_loop(BP_ACTION_STOP);
}

void PythonDebugger::pause() {
  _pause_clicked = true;
  mforms::App::get()->exit_event_loop(BP_ACTION_PAUSE);
}

void PythonDebugger::step_out() {
  mforms::App::get()->exit_event_loop(BP_ACTION_STEP_OUT);
}

void PythonDebugger::debug_print(const std::string &s) {
  _shell->add_output(s);
}

void PythonDebugger::ui_clear_breakpoints() {
  _breakpoint_list->clear();
}

void PythonDebugger::ui_add_breakpoint(const char *file, int line, const char *condition) {
  if (!file)
    file = "";

  if (!condition)
    condition = "";

  mforms::TreeNodeRef node = _breakpoint_list->add_node();
  //_breakpoint_list->set(row, 0, true);
  node->set_string(0, base::strfmt("%s:%i", base::basename(file).c_str(), line));
  node->set_string(1, "");
  node->set_string(2, condition);
}

const char *PythonDebugger::ui_program_stopped(const char *file, int line, int reason) {
  GRTCodeEditor *editor = 0;
  mforms::CodeEditor *code_editor = nullptr;
  bool continue_possible = true;

  if (_pause_clicked && reason == 5) {
    _pause_clicked = false;
    return "pause";
  }

  if (reason != 5) {
    editor = _shell->get_editor_for(file, true);
    if (!editor) {
      if (strcmp(file, "<string>") != 0)
        editor = _shell->show_file_at_line(file, line);
      if (!editor && reason != 6) {
        _shell->add_output(base::strfmt("continuing from %s:%i...\n", file, line));
        return "continue";
      }
    }
  }

  if (editor)
    code_editor = editor->get_editor();

  if (reason == 6)
    reason = 0; // treat like we're stepping

  _program_stopped = true;
  float timeout = 0.0;

  switch (reason) {
    case 0: // step
      break;

    case 1: // breakpoint
      // show the cursor position in the editor
      if (code_editor)
        code_editor->show_markup(LineMarkupBreakpointHit, line - 1);
      break;

    case 2: // exception
      if (code_editor)
        code_editor->show_markup(LineMarkupError, line - 1);
      continue_possible = false;

      _shell->activate_output_tab();
      break;

    case 5: // heartbeat
      // schedule for the event loop to exit in 0.01s
      timeout = 0.01f;
      continue_possible = false;
      break;
  }
  if (code_editor)
    code_editor->show_markup(LineMarkupCurrent, line - 1);
  _stack_position_editor = editor;
  _stack_position_line = line - 1;

  _shell->_run_button->show(false);
  _shell->_continue_button->show(true);

  _shell->_continue_button->set_enabled(continue_possible);
  _shell->_step_button->set_enabled(continue_possible);
  _shell->_step_into_button->set_enabled(continue_possible);
  _shell->_step_out_button->set_enabled(continue_possible);
  _shell->_stop_button->set_enabled(true);
  _shell->_pause_button->set_enabled(reason == 5);

  if (reason != 5) {
    _stack_list->select_node(_stack_list->node_at_row(0));
    stack_selected();
  }

  // enter event loop and wait for some action from user
  int action = mforms::App::get()->enter_event_loop(timeout);

  // check if editor is still there
  editor = _shell->get_editor_for(file, true);
  if (!editor) {
    logWarning("editor gone\n");
    return "abort";
  }

  switch (reason) {
    case 0: // step
      break;

    case 1: // breakpoint
      // show the cursor position in the editor
      if (code_editor)
        code_editor->remove_markup(LineMarkupBreakpointHit, line - 1);
      break;

    case 2: // exception
      if (code_editor)
        code_editor->remove_markup(LineMarkupError, line - 1);
      break;

    case 5:           // heartbeat
      if (action < 0) // timeout
        action = BP_ACTION_CONTINUE;
      break;
  }
  if (code_editor)
    code_editor->remove_markup(LineMarkupCurrent, line - 1);

  if (_stack_position_editor) {
    _stack_position_editor->get_editor()->remove_markup(LineMarkupCurrent, _stack_position_line);
    _stack_position_editor = 0;
    _stack_position_line = 0;
  }

  _shell->_step_button->set_enabled(false);
  _shell->_step_into_button->set_enabled(false);
  _shell->_step_out_button->set_enabled(false);
  _shell->_stop_button->set_enabled(true);
  _shell->_continue_button->set_enabled(false);
  _shell->_pause_button->set_enabled(true);

  _pause_clicked = false;
  // determine command
  const char *command = "abort";
  switch (action) {
    case BP_ACTION_CONTINUE:
      command = "continue";
      break;

    case BP_ACTION_STEP:
      command = "step";
      break;

    case BP_ACTION_STEP_INTO:
      command = "step_into";
      break;

    case BP_ACTION_STOP:
      command = "stop";
      break;

    case BP_ACTION_STEP_OUT:
      command = "step_out";
      break;

    case BP_ACTION_PAUSE:
      _pause_clicked = true;
      command = "pause";
      break;

    default: // error or unknown, abort everything
      command = "abort";
      break;
  }
  _program_stopped = false;
  return command;
}

bool PythonDebugger::heartbeat_timeout() {
  _heartbeat_timeout_timer = 0;
  mforms::App::get()->exit_event_loop(BP_ACTION_CONTINUE);
  return false;
}

void PythonDebugger::ui_clear_stack() {
  _stack_list->clear();
}

void PythonDebugger::ui_add_stack(const char *location, const char *file, int line) {
  if (!file)
    file = "";

  mforms::TreeNodeRef node = _stack_list->add_node();
  node->set_tag(file);
  node->set_int(0, _stack_list->row_for_node(node));
  node->set_string(1, location);
  node->set_string(2, base::strfmt("%s:%i", base::basename(file).c_str(), line));
}

void PythonDebugger::ui_clear_variables() {
  _variable_list->clear();
}

void PythonDebugger::ui_add_variable(const char *varname, const char *value) {
  mforms::TreeNodeRef node = _variable_list->add_node();
  node->set_string(0, varname);
  node->set_string(1, value);
}

void PythonDebugger::stack_selected() {
  mforms::TreeNodeRef node(_stack_list->get_selected_node());
  int show_frame = 0;

  if (_stack_position_editor) {
    _stack_position_editor->get_editor()->remove_markup(LineMarkupCurrent, _stack_position_line);
    _stack_position_editor = 0;
    _stack_position_line = 0;
    _variable_list->clear();
  }

  if (node) {
    std::string location = node->get_string(2);
    std::string::size_type pos = location.rfind(':');
    std::string file = node->get_tag();
    int line = base::atoi<int>(location.substr(pos + 1), 0);

    if (!file.empty() && line > 0) {
      GRTCodeEditor *editor = _shell->show_file_at_line(file, line - 1);
      editor->get_editor()->show_markup(LineMarkupCurrent, line - 1);

      _stack_position_editor = editor;
      _stack_position_line = line - 1;
    }
    show_frame = -1 * (_stack_list->row_for_node(node) + 1);
  }

  WillEnterPython lock;

  grt::AutoPyObject r(PyObject_CallMethod(_pdb, "wdb_refresh_variables", "(i)", show_frame), false);
  if (!r) {
    debug_print("Internal error showing variables\n");
    PyErr_Print();
    PyErr_Clear();
  }
}

bool PythonDebugger::toggle_breakpoint(const char *file, int line) {
  WillEnterPython lock;

  grt::AutoPyObject r(PyObject_CallMethod(_pdb, "wdb_toggle_breakpoint", "(si)", file, line), false);
  if (!r) {
    debug_print("Internal error toggling debugger breakpoint\n");
    PyErr_Print();
    PyErr_Clear();
    return false;
  }

  if (PyBool_Check(r) && r == Py_True) {
    debug_print(base::strfmt("Added breakpoint to line %i\n", line));
    return true;
  }
  debug_print(base::strfmt("Removed breakpoint from line %i\n", line));
  return false;
}
