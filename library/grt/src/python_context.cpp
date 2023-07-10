/*
 * Copyright (c) 2009, 2021, Oracle and/or its affiliates.
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

#include "python_context.h"
#include "grt.h"
#include "grtpp_util.h"
#include "grtpp_notifications.h"
#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/util_functions.h"
#include "base/wb_memory.h"

// python internals
#include <Python.h>
#include <errcode.h>
#include <token.h>
#include <frameobject.h>

#include "python_grtobject.h"
#include "python_grtlist.h"
#include "python_grtdict.h"

#include "base/log.h"

#ifdef _MSC_VER
#include "base/event_log.h"
#endif

DEFAULT_LOG_DOMAIN("python context")

using namespace grt;
using namespace base;

// used to identify a proper GRT context object as a PyCObject
static const char *GRTTypeSignature = "GRTCONTEXT";
// used to identify a GRT value as a PyCObject
static const char *GRTValueSignature = "GRTVALUE";

static std::string flatten_class_name(std::string name) {
  std::string::size_type p;
  while ((p = name.find('.')) != std::string::npos)
    name[p] = '_';
  return name;
}

//--------------------------------------------------------------------------------------------------

PythonContextHelper::PythonContextHelper(const std::string &module_path) {
  std::string wb_pythonpath;
  if (getenv("PYTHON_DEBUG"))
    Py_VerboseFlag = 5;

#ifdef _MSC_VER
  // Hack needed in Windows because Python lib uses C:\Python26 as default pythonhome
  // That will cause conflicts if there is some other Python installed in there (bug #52949)

  // Program.cs also does this, but somehow, they don't seem to get reflected here
  char *path = getenv("PATH");
  if (path) {
    // strip away everything with Python in it
    std::vector<std::string> parts = base::split(path, ";");
    std::string npath;
    for (std::vector<std::string>::const_iterator p = parts.begin(); p != parts.end(); ++p) {
      if (!strstr(base::tolower(*p).c_str(), "python")) {
        if (!npath.empty())
          npath.append(";");
        npath.append(*p);
      }
    }

    putenv(g_strdup_printf("PATH=%s", npath.c_str()));

    // This allows to define additional paths for python loadin
    char *pythonpath = getenv("WB_PYTHONPATH");
    if (pythonpath)
      wb_pythonpath = getenv("WB_PYTHONPATH");
  }

  putenv(g_strdup_printf("PYTHONHOME=%s\\Python", module_path.c_str()));
  putenv(g_strdup_printf("PYTHONPATH=%s\\Python;%s\\Python\\DLLs;%s\\Python\\Lib;%s\\Python\\mysql_libs.zip;%s",
                         module_path.c_str(), module_path.c_str(), module_path.c_str(), module_path.c_str(),
                         wb_pythonpath.c_str()));
#endif
}

//--------------------------------------------------------------------------------------------------

void PythonContextHelper::InitPython() {
#ifndef _MSC_VER
  static const wchar_t *argv[2] = { L"/dev/null", nullptr };
  Py_InitializeEx(0); // skips signal handler init

  PyRun_SimpleString(
      "import importlib.abc\n" \
      "import importlib.machinery\n" \
      "import sys\n" \
      "\n" \
      "\n" \
      "class Finder(importlib.abc.MetaPathFinder):\n" \
      "    def find_spec(self, fullname, path, target=None):\n" \
      "        if fullname in sys.builtin_module_names:\n" \
      "            return importlib.machinery.ModuleSpec(\n" \
      "                fullname,\n" \
      "                importlib.machinery.BuiltinImporter,\n" \
      "            )\n" \
      "\n" \
      "\n" \
      "sys.meta_path.append(Finder())\n" \
  );

  // Stores the main thread state
  _main_thread_state = PyThreadState_Get();

  PySys_SetArgv(1, const_cast<wchar_t **>(argv));

  // PyEval_InitThreads();
  // Changed in version 3.9: The function now does nothing.
  // Changed in version 3.7: This function is now called by Py_Initialize(),
  //                         so you don’t have to call it yourself anymore.
#else
  char const* argv[2] = { "/dev/null", nullptr };
  Py_InitializeEx(0);
  PyStatus status; 
  PyConfig_InitPythonConfig(&_config);
  _config.use_environment = 1;

  do {
    status = PyConfig_SetBytesArgv(&_config, 1, const_cast<char *const *>(argv));
    if (PyStatus_Exception(status)) {
      break;
    }

    status = Py_InitializeFromConfig(&_config);
    if (PyStatus_Exception(status)) {
      break;
    }

    int ret = Py_IsInitialized();
    if (ret == 0) {
      break;
    }

    PyRun_SimpleString(
      "from importlib.abc import MetaPathFinder\n"
      "from importlib.machinery import ModuleSpec, BuiltinImporter\n"
      "import sys\n"
      "\n"
      "\n"
      "class Finder(MetaPathFinder):\n"
      "    def find_spec(self, fullname, path, target=None):\n"
      "        if fullname in sys.builtin_module_names:\n"
      "            return ModuleSpec(\n"
      "                fullname,\n"
      "                BuiltinImporter,\n"
      "            )\n"
      "\n"
      "\n"
      "sys.meta_path.append(Finder())\n");

    _main_thread_state = PyThreadState_Get();
    return;
  } while (1 == 0);

  PyConfig_Clear(&_config);
  if (PyStatus_IsExit(status)) {
    return;
  }
  Py_ExitStatusException(status);

 #endif
}

//--------------------------------------------------------------------------------------------------

PythonContextHelper::~PythonContextHelper() {
  PyEval_RestoreThread(_main_thread_state);
  _main_thread_state = nullptr;
#ifdef _MSC_VER
  PyConfig_Clear(&_config);
#endif
  Py_Finalize();
}

//--------------------------------------------------------------------------------------------------
#include <iostream>
PythonContext::PythonContext(const std::string &module_path) : PythonContextHelper(module_path) {
  _grt_list_class = 0;
  _grt_dict_class = 0;
  _grt_object_class = 0;
  _grt_method_class = 0;

  PyImport_AppendInittab("grt", grt_module_create);   // Add a single module to the existing table of built-in modules
  InitPython();

  PyObject *main = PyImport_AddModule("__main__");  //  Get module if exists
  PyObject *module = PyImport_ImportModule("grt");  //  Import grt (will call grt_module_create)
  PyDict_SetItemString(PyModule_GetDict(main), "grt", module);

  register_grt_module( module );

  PySys_SetObject((char *)"real_stdout", PySys_GetObject((char *)"stdout"));
  PySys_SetObject((char *)"real_stderr", PySys_GetObject((char *)"stderr"));
  PySys_SetObject((char *)"real_stdin", PySys_GetObject((char *)"stdin"));

  // make sys.stdout and sys.stderr send stuff to GRT
  PySys_SetObject((char *)"stdout", get_grt_module());
  PySys_SetObject((char *)"stderr", get_grt_module());

  // set stdin to the GRT shell console
  PySys_SetObject((char *)"stdin", get_grt_module());

  run_post_init_script();

  {
    WillEnterPython lock;
    PyObject *path = from_grt(grt::StringRef(base::Logger::log_filename()));
    PyDict_SetItemString(PyModule_GetDict(PyImport_AddModule("grt")), "logpath", path);
    Py_XDECREF(path);
  }

  PyEval_SaveThread(); // Started in InitPython()

  // listen to all notifications so it can be forwarded to python
  GRTNotificationCenter::get()->add_grt_observer(this);
  NotificationCenter::get()->add_observer(this);
}

PythonContext::~PythonContext() {
  GRTNotificationCenter::get()->remove_grt_observer(this);
  NotificationCenter::get()->remove_observer(this);
}

void PythonContext::add_module_path(const std::string &modpath, bool prepend) {
  WillEnterPython lock;
  // add the path to the search path so that it can be imported
  PyObject *path_list;
  PyObject *path = PyUnicode_FromString(modpath.c_str());

  path_list = PySys_GetObject(
    (char *)"path"); // cast to (char *) required for gcc 4.3 to avoid warning about deprecated conversion
                     // from string constant to 'char*'.
  Py_ssize_t i;

  // check if the path is already in it
  for (i = PyList_Size(path_list) - 1; i >= 0; --i) {
    if (PyObject_RichCompareBool(PyList_GetItem(path_list, i), path, Py_EQ) == 1)
      break;
  }

  if (i < 0) // not found
  {
    if (prepend)
      PyList_Insert(path_list, 0, path);
    else
      PyList_Append(path_list, path);
  }
  Py_DECREF(path);
}

void PythonContext::set_user_interrupted(const grt::user_cancelled &exc) {
  PyErr_SetString(PythonContext::get()->_grt_user_interrupt_error, exc.what());
}

void PythonContext::set_db_access_denied(const grt::db_access_denied &exc) {
  PyErr_SetString(PythonContext::get()->_grt_db_access_denied_error, exc.what());
}

void PythonContext::set_db_login_error(const grt::db_login_error &exc) {
  PyErr_SetString(PythonContext::get()->_grt_db_login_error, exc.what());
}

void PythonContext::set_db_error(const grt::db_error &exc) {
  PyObject *arg = Py_BuildValue("(si)", exc.what(), exc.error());
  PyErr_SetObject(PythonContext::get()->_grt_db_error, arg);
  Py_DECREF(arg);
}

void PythonContext::set_db_not_conected(const grt::db_not_connected &exc) {
  PyErr_SetString(PythonContext::get()->_grt_db_not_connected, exc.what());
}

void PythonContext::set_python_error(const grt::type_error &exc, const std::string &location) {
  PyErr_SetString(PyExc_TypeError, (location.empty() ? exc.what() : location + ": " + exc.what()).c_str());
}

void PythonContext::set_python_error(const grt::bad_item &exc, const std::string &location) {
  PyErr_SetString(PyExc_IndexError, (location.empty() ? exc.what() : location + ": " + exc.what()).c_str());
}

void PythonContext::set_python_error(const std::exception &exc, const std::string &location) {
  PyErr_SetString(PyExc_SystemError, (location.empty() ? exc.what() : location + ": " + exc.what()).c_str());
}

/** Gets the PythonContext from the Python interpreter.
 */
PythonContext *PythonContext::get() {
  PyObject *ctx;
  PyObject *module;
  PyObject *dict;

  module = PyDict_GetItemString(PyImport_GetModuleDict(), "grt");
  if (!module)
    throw std::runtime_error("GRT module not found in Python runtime");

  dict = PyModule_GetDict(module);
  if (!dict)
    throw std::runtime_error("GRT module is invalid in Python runtime");

  ctx = PyDict_GetItemString(dict, "__GRT__");
  if (!ctx)
    throw std::runtime_error("GRT context not found in Python runtime");

  if (PyCapsule_GetContext(ctx) == &GRTTypeSignature)
    return static_cast<PythonContext *>(PyCapsule_GetPointer(ctx, "contextObject"));

  throw std::runtime_error("Invalid GRT context in Python runtime");
}

PythonContext *PythonContext::get_and_check() {
  try {
    return PythonContext::get();
  } catch (std::exception &exc) {
    PyErr_SetString(PyExc_SystemError, strfmt("Could not get GRT context: %s", exc.what()).c_str());
  }
  return nullptr;
}

void PythonContext::set_grt_observer_callable(PyObject *obj) {
  _grt_notification_observer = obj;
}

void PythonContext::handle_grt_notification(const std::string &name, ObjectRef sender, DictRef info) {
  if (_grt_notification_observer) {
    WillEnterPython lock;

    PyObject *psender = from_grt(sender);
    PyObject *pinfo = from_grt(info);
    PyObject *args = Py_BuildValue("sOO", name.c_str(), psender, pinfo);
    PyObject *res;

    if (!(res = PyObject_CallObject(_grt_notification_observer, args))) {
      log_python_error("Error forwarding GRT notification to Python");
    }
    if (res) {
      Py_DECREF(res);
    }
    Py_XDECREF(psender);
    Py_XDECREF(pinfo);
    Py_DECREF(args);
  }
}

void PythonContext::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (_grt_notification_observer) {
    WillEnterPython lock;

    PyObject *infodict = PyDict_New();
    PyObject *res;

    for (base::NotificationInfo::iterator i = info.begin(); i != info.end(); ++i) {
      PyObject *str = PyUnicode_FromString(i->second.c_str());
      PyDict_SetItemString(infodict, i->first.c_str(), str);
      Py_DECREF(str);
    }
    PyObject *args = Py_BuildValue("sOO", name.c_str(), Py_None, infodict);

    if (!(res = PyObject_CallObject(_grt_notification_observer, args))) {
      log_python_error("Error forwarding notification to Python");
    }
    Py_XDECREF(res);
    Py_DECREF(args);

    // apply back changes made to the info dict from python code
    {
      PyObject *key, *value;
      Py_ssize_t pos = 0;

      while (PyDict_Next(infodict, &pos, &key, &value)) {
        std::string s;
        grt::ValueRef v;
        v = from_pyobject(value);
        if (!pystring_to_string(key, s) || !v.is_valid()) {
          logError("Error in Python notification handler: info dictionary contains an invalid item");
          continue;
        }
        info[s] = v.toString();
      }
    }
    Py_DECREF(infodict);
  }
}

static PyObject *grt_print(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  std::string text;

  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *o;
  if (!PyArg_ParseTuple(args, "O", &o)) {
    if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
      PyErr_Clear();
      text = "None";
    } else
      return nullptr;
  } else if (!ctx->pystring_to_string(o, text, true))
    return nullptr;

#ifdef _MSC_VER
  OutputDebugStringA(text.c_str());
#else
  g_print("%s", text.c_str()); // g_print is not routed to g_log
#endif
  grt::GRT::get()->send_output(text);
  logDebug3("%s\n", text.c_str());

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *pylog(base::Logger::LogLevel level, PyObject *args) {
  PythonContext *ctx;
  std::string text;

  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  char *domain;
  PyObject *o;
  if (!PyArg_ParseTuple(args, "sO", &domain, &o))
    return nullptr;

  if (!ctx->pystring_to_string(o, text, true))
    return nullptr;

  base::Logger::log(level, domain, "%s", text.c_str());

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_log_error(PyObject *self, PyObject *args) {
  return pylog(base::Logger::LogLevel::Error, args);
}

static PyObject *grt_log_warning(PyObject *self, PyObject *args) {
  return pylog(base::Logger::LogLevel::Warning, args);
}

static PyObject *grt_log_info(PyObject *self, PyObject *args) {
  return pylog(base::Logger::LogLevel::Info, args);
}

static PyObject *grt_log_debug(PyObject *self, PyObject *args) {
  return pylog(base::Logger::LogLevel::Debug, args);
}

static PyObject *grt_log_debug2(PyObject *self, PyObject *args) {
  return pylog(base::Logger::LogLevel::Debug2, args);
}

static PyObject *grt_log_debug3(PyObject *self, PyObject *args) {
  return pylog(base::Logger::LogLevel::Debug3, args);
}

static PyObject *grt_send_output(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  std::string text;

  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *o;
  if (!PyArg_ParseTuple(args, "O", &o)) {
    if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
      PyErr_Clear();
      text = "None";
    }
    else
      return nullptr;
  }
  else if (!ctx->pystring_to_string(o, text, true))
    return nullptr;

  grt::GRT::get()->send_output(text);

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_send_warning(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  std::string text;
  std::string detail;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *o, *d = nullptr;
  if (!PyArg_ParseTuple(args, "O|O", &o, &d)) {
    if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
      PyErr_Clear();
      text = "None";
    } else
      return nullptr;
  } else {
    if (!ctx->pystring_to_string(o, text, true))
      return nullptr;
    if (d && !ctx->pystring_to_string(d, detail, true))
      return nullptr;
  }

  grt::GRT::get()->send_warning(text, detail);

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_send_info(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  std::string text;
  std::string detail;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *o, *d = nullptr;
  if (!PyArg_ParseTuple(args, "O|O", &o, &d)) {
    if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
      PyErr_Clear();
      text = "None";
    } else
      return nullptr;
  } else {
    if (!ctx->pystring_to_string(o, text, true))
      return nullptr;
    if (d && !ctx->pystring_to_string(d, detail, true))
      return nullptr;
  }

  grt::GRT::get()->send_info(text, detail);
  //  logDebug2("grt.python", "%s: (%s)", text.c_str(), detail.c_str());

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_send_error(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  std::string text;
  std::string detail;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *o, *d = nullptr;
  if (!PyArg_ParseTuple(args, "O|O", &o, &d)) {
    if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
      PyErr_Clear();
      text = "None";
    } else
      return nullptr;
  } else {
    if (!ctx->pystring_to_string(o, text, true))
      return nullptr;
    if (d && !ctx->pystring_to_string(d, detail, true))
      return nullptr;
  }

  grt::GRT::get()->send_error(text, detail);

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_flush(PyObject *self, PyObject *args) {
  return Py_None;
}

static PyObject *grt_send_progress(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  float pct;
  PyObject *texto, *detailo = nullptr;
  if (!PyArg_ParseTuple(args, "fO|O", &pct, &texto, &detailo))
    return nullptr;

  std::string text, detail;
  if (!ctx->pystring_to_string(texto, text, true))
    return nullptr;
  if (detailo && !ctx->pystring_to_string(detailo, detail, true))
    return nullptr;
  grt::GRT::get()->send_progress(pct, text, detail, nullptr);
  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_begin_progress_step(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  float pct1, pct2;
  if (!PyArg_ParseTuple(args, "ff", &pct1, &pct2))
    return nullptr;

  grt::GRT::get()->begin_progress_step(pct1, pct2);

  Py_RETURN_NONE;
}

static PyObject *grt_end_progress_step(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  if (!PyArg_ParseTuple(args, ""))
    return nullptr;

  grt::GRT::get()->end_progress_step();

  Py_RETURN_NONE;
}

static PyObject *grt_reset_progress_steps(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  if (!PyArg_ParseTuple(args, ""))
    return nullptr;

  grt::GRT::get()->reset_progress_steps();

  Py_RETURN_NONE;
}

static PyObject *grt_get_by_path(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  const char *path = "";
  PyObject *object;
  grt::ValueRef value;

  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  if (!PyArg_ParseTuple(args, "s|O", &path, &object))
    return nullptr;

  if (object) {
    try {
      value = ctx->from_pyobject(object);
    } catch (grt::type_error &exc) {
      PyErr_SetString(PyExc_TypeError, exc.what());
      return nullptr;
    } catch (std::exception &exc) {
      PythonContext::set_python_error(exc);
      return nullptr;
    }
  } else
    value = grt::GRT::get()->root();

  if (!path)
    path = "";

  try {
    value = get_value_by_path(value, path);
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  }

  return ctx->from_grt(value);
}

void PythonContext::setEventlogCallback(PyObject *obj) {
  _grtEventLogNotification = obj;
}

void PythonContext::printResult(std::map<std::string, std::string> &output) {
  if (_grtEventLogNotification) {
    WillEnterPython lock;
    PyObject *dict = PyDict_New();
    auto end = output.end();
    for (auto it = output.begin(); it != end; ++it) {
      PyObject *str = PyUnicode_FromString(it->second.c_str());
      PyDict_SetItemString(dict, it->first.c_str(), str);
      Py_DECREF(str);
    }
    PyObject *args = Py_BuildValue("sO", "", dict);
    PyObject *res;
    if (!(res = PyObject_CallObject(_grtEventLogNotification, args))) {
      log_python_error("Error forwarding GRT notification to Python");
    }
    Py_XDECREF(res);
    Py_DECREF(args);
    Py_DECREF(dict);
  }
}

#ifdef _MSC_VER
static void printResultCallback(std::map<std::string, std::string> &output) {
  PythonContext *ctx;

  if (!(ctx = PythonContext::get_and_check()))
    return;

  ctx->printResult(output);
}
#endif

static PyObject *getEventLogEntry(PyObject *self, PyObject *args) {
  PythonContext *ctx;

  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  char *query = nullptr;
  long seek = 0;
  if (!PyArg_ParseTuple(args, "ls", &seek, &query))
    return nullptr;

#ifdef _MSC_VER
  EventLogReader reader(query, printResultCallback);
  reader.SetPosition(seek);
  reader.ReadEvents();
#endif

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *setEventlogCallback(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *object;

  if (!PyArg_ParseTuple(args, "O", &object))
    return nullptr;

  if (!PyCallable_Check(object)) {
    PyErr_SetString(PyExc_ValueError, "notification observer argument must be a callable");
    return nullptr;
  }

  ctx->setEventlogCallback(object);

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_readline(PyObject *self, PyObject *args) {
  PythonContext *ctx;

  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  if (!PyArg_ParseTuple(args, ""))
    return nullptr;

  if (ctx->stdin_readline_slot) {
    std::string line = ctx->stdin_readline_slot();
    return Py_BuildValue("s", line.c_str());
  }
  PyErr_SetString(PyExc_NotImplementedError, "input()/stdin reading not available");
  return nullptr;
}

void decrement_int(int *i) {
  (*i)--;
}

static bool call_handle_message(const grt::Message &msg, void *sender, AutoPyObject callable) {
  WillEnterPython lock;
  PyObject *ret;
  const char *type = nullptr;
  static int handling_message = 0;

  if (handling_message > 10) {
    logWarning("Force-breaking apparent recursion of GRT message handlers\n");
    return false;
  }
  handling_message++;
  base::scope_ptr<int, decrement_int> guard(&handling_message);

  std::string text = msg.text;
  switch (msg.type) {
    case grt::ErrorMsg:
      type = "ERROR";
      break;
    case grt::WarningMsg:
      type = "WARNING";
      break;
    case grt::InfoMsg:
      type = "INFO";
      break;
    case grt::OutputMsg:
      type = "OUTPUT";
      break;
    case grt::VerboseMsg:
      type = "VERBOSE";
      break;
    case grt::ProgressMsg:
      type = "PROGRESS";
      text = strfmt("%f:%s", msg.progress, msg.text.c_str());
      break;
    default:
      type = "???";
      break;
  }

  PyObject *args = Py_BuildValue("(sss)", type, text.c_str(), msg.detail.c_str());
  if (!(ret = PyObject_Call(callable, args, nullptr))) {
    Py_DECREF(args);
    PythonContext::log_python_error("Error calling Python output handler:");
    PyErr_Clear();
    return false;
  }
  Py_DECREF(args);

  if (ret == Py_None || ret == Py_False || PyLong_AsLong(ret) == 0) {
    Py_DECREF(ret);
    return false;
  }
  Py_DECREF(ret);
  return true;
}

static PyObject *grt_push_message_handler(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *o;
  if (!PyArg_ParseTuple(args, "O", &o))
    return nullptr;

  if (!PyCallable_Check(o))
    return nullptr;

  grt::GRT::get()->pushMessageHandler(
    new grt::SlotHolder(std::bind(&call_handle_message, std::placeholders::_1, std::placeholders::_2, AutoPyObject(o))));

  return Py_BuildValue("i", grt::GRT::get()->messageHandlerCount());
}

static PyObject *grt_pop_message_handler(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  grt::GRT::get()->popMessageHandler();

  return Py_BuildValue("i", grt::GRT::get()->messageHandlerCount());
}

static bool call_status_query(AutoPyObject callable) {
  WillEnterPython lock;

  PyObject *ret;
  PyObject *args = Py_BuildValue("()");
  if (!(ret = PyObject_Call(callable, args, nullptr))) {
    Py_DECREF(args);
    PythonContext::log_python_error("Error calling Python status handler:");
    PyErr_Clear();
    return false;
  }
  Py_DECREF(args);

  if (ret == Py_None || ret == Py_False || PyLong_AsLong(ret) == 0) {
    Py_DECREF(ret);
    return false;
  }
  Py_DECREF(ret);
  return true;
}

static PyObject *grt_push_status_query_handler(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *o;
  if (!PyArg_ParseTuple(args, "O", &o))
    return nullptr;

  if (!PyCallable_Check(o))
    return nullptr;

  grt::GRT::get()->push_status_query_handler(std::bind(&call_status_query, AutoPyObject(o)));

  Py_RETURN_NONE;
}

static PyObject *grt_pop_status_query_handler(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  grt::GRT::get()->pop_status_query_handler();

  Py_RETURN_NONE;
}

static PyObject *grt_query_status(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  if (args && !PyArg_ParseTuple(args, ""))
    return nullptr;

  if (grt::GRT::get()->query_status())
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

static PyObject *grt_serialize(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *object;
  char *path = nullptr;

  if (!PyArg_ParseTuple(args, "Os", &object, &path))
    return nullptr;

  grt::ValueRef value(ctx->from_pyobject(object));
  if (!value.is_valid()) {
    PyErr_SetString(PyExc_TypeError, "First argument must be a GRT object");
    return nullptr;
  }

  if (!path) {
    PyErr_SetString(PyExc_ValueError, "File path expected for argument #2");
    return nullptr;
  }

  try {
    grt::GRT::get()->serialize(value, path);
  } catch (const std::exception &exc) {
    PythonContext::set_python_error(exc, "serializing object");
    return nullptr;
  }

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_unserialize(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  char *path = nullptr;
  if (!PyArg_ParseTuple(args, "s", &path))
    return nullptr;
  if (!path) {
    PyErr_SetString(PyExc_ValueError, "File path expected");
    return nullptr;
  }

  try {
    grt::ValueRef value = grt::GRT::get()->unserialize(path);
    return ctx->from_grt(value);
  } catch (const std::exception &exc) {
    PythonContext::set_python_error(exc, base::strfmt("unserializing file %s", path));
    return nullptr;
  }

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_set_notification_observer(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  PyObject *object;

  if (!PyArg_ParseTuple(args, "O", &object))
    return nullptr;

  if (!PyCallable_Check(object)) {
    PyErr_SetString(PyExc_ValueError, "notification observer argument must be a callable");
    return nullptr;
  }

  ctx->set_grt_observer_callable(object);

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *grt_send_notification(PyObject *self, PyObject *args) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  char *name;
  PyObject *object;
  PyObject *info;

  if (!PyArg_ParseTuple(args, "sOO", &name, &object, &info))
    return nullptr;

  ValueRef infodict = ctx->from_pyobject(info);
  if (infodict.is_valid() && !DictRef::can_wrap(infodict)) {
    PyErr_SetString(PyExc_ValueError, "notification info must be a dict or None");
    return nullptr;
  }
  ValueRef sender = ctx->from_pyobject(object);
  if (sender.is_valid() && !ObjectRef::can_wrap(sender)) {
    PyErr_SetString(PyExc_ValueError, "notification sender info must be a GRT object");
    return nullptr;
  }

  try {
    GRTNotificationCenter::get()->send_grt(name, ObjectRef::cast_from(sender), DictRef::cast_from(infodict));
  } catch (std::exception &exc) {
    ctx->set_python_error(exc, "Error sending notification");
    return nullptr;
  }

  Py_XINCREF(Py_None);
  return Py_None;
}

static PyObject *(*wrap_pyobject_func)(PyObject *, PyObject *) = nullptr;
static PyObject *(*unwrap_pyobject_func)(PyObject *, PyObject *) = nullptr;

static PyObject *grt_wrap_pyobject(PyObject *self, PyObject *args) {
  // since the GRT objects are defined in an outer level, we forward this
  // to a callback which must be registered beforehand with PythonContext::set_wrap_callback
  if (wrap_pyobject_func)
    return wrap_pyobject_func(self, args);

  PyErr_SetString(PyExc_SystemError, "GRT python support not initialized correctly");
  return nullptr;
}

static PyObject *grt_unwrap_pyobject(PyObject *self, PyObject *args) {
  if (unwrap_pyobject_func)
    return unwrap_pyobject_func(self, args);

  PyErr_SetString(PyExc_SystemError, "GRT python support not initialized correctly");
  return nullptr;
}

void PythonContext::set_wrap_pyobject_func(PyObject *(*func)(PyObject *, PyObject *)) {
  wrap_pyobject_func = func;
}

void PythonContext::set_unwrap_pyobject_func(PyObject *(*func)(PyObject *, PyObject *)) {
  unwrap_pyobject_func = func;
}

/** Register grt related functionality as a module

 Stuff made available in the module include:
 <li>__GRT__ variable holding a CObject pointer back to PythonContext
 <li>GRTList, GRTDict and GRTObject types
 <li>send_output, send_error, send_info etc
 <li>etc
 */
static PyMethodDef GrtModuleMethods[] = {
  {"send_output", grt_send_output, METH_VARARGS, "Write a string in the GRT shell." },
  {"write", grt_print, METH_VARARGS, "Write a string in the GRT shell (alias to send_output)."},
  {"send_error", grt_send_error, METH_VARARGS, "Write an error message to the GRT shell."},
  {"flush", grt_flush, METH_VARARGS, "Flush output."},
  {"send_warning", grt_send_warning, METH_VARARGS, "Write a warning message to the GRT shell."},
  {"send_info", grt_send_info, METH_VARARGS, "Write a info message to the GRT shell."},
  {"send_progress", grt_send_progress, METH_VARARGS, "Write a progress message."},
  {"begin_progress_step", grt_begin_progress_step, METH_VARARGS,
   "Starts a new step in a larger task where progress is being tracked."},
  {"end_progress_step", grt_end_progress_step, METH_VARARGS, "Ends a step."},
  {"reset_progress_steps", grt_reset_progress_steps, METH_VARARGS, "Clear the progress steps stack."},

  {"log_error", grt_log_error, METH_VARARGS,
   "Logs an error to the log file, in the specified context ex: log_error('myplugin', 'cannot open file')"},
  {"log_warning", grt_log_warning, METH_VARARGS,
   "Logs a warning to the log file, in the specified context ex: log_warning('myplugin', 'cannot open file very "
   "well')"},
  {"log_debug", grt_log_debug, METH_VARARGS,
   "Logs a debug message to the log file, in the specified context ex: log_debug('myplugin', 'trying to open file with "
   "')"},
  {"log_debug2", grt_log_debug2, METH_VARARGS,
   "Logs a verbose debug message to the log file, in the specified context ex: log_debug2('myplugin', 'reading from "
   "file')"},
  {"log_debug3", grt_log_debug3, METH_VARARGS,
   "Logs a very verbose debug message to the log file, in the specified context ex: log_debug3('myplugin', 'processing "
   "file')"},
  {"log_info", grt_log_info, METH_VARARGS,
   "Logs an informational message to the log file, in the specified context ex: log_info('myplugin', 'file opened')"},

  {"push_message_handler", grt_push_message_handler, METH_VARARGS,
   "Pushes a callback of the form function((type, text, detail)) to be called when a plugin outputs text. Return value "
   "must be True if the message was handled, False if it should be handled by a previously installed handler."},
  {"pop_message_handler", grt_pop_message_handler, METH_NOARGS, "Pops previously pushed handler."},

  {"push_status_query_handler", grt_push_status_query_handler, METH_VARARGS,
   "Pushes a callback that returns a boolean value. True should be returned for aborting running tasks that call "
   "grt.query_status()"},
  {"pop_status_query_handler", grt_pop_status_query_handler, METH_NOARGS, "Pops status_query callback from the stack."},
  {"query_status", grt_query_status, METH_NOARGS,
   "Queries the current status query handler whether currently running task should be cancelled. If True is returned, "
   "the task should be cancelled."},

  {"readline", grt_readline, METH_VARARGS, "Waits for a line of text to be input to the scripting shell prompt."},

  {"get", grt_get_by_path, METH_VARARGS, "Gets a value from a GRT dict or object (or from the global tree) by path."},

  {"serialize", grt_serialize, METH_VARARGS, "Serializes a GRT object into a XML file. serialize(object, path)"},
  {"unserialize", grt_unserialize, METH_VARARGS,
   "Unserializes a GRT object from a XML file created by serialize. unserialize(path) -> object"},

  {"_set_grt_notification_observer", grt_set_notification_observer, METH_VARARGS,
   "Sets the GRT notification observer. Internal use."},

  {"send_grt_notification", grt_send_notification, METH_VARARGS,
   "Sends a GRT notification to the application. ex: send_grt_notification('GNMyPluginStarted', sender, infodict)"},

  {"fromgrt", grt_unwrap_pyobject, METH_VARARGS,
   "Extracts the Python object wrapped in a grt_PyObject GRT object (suitable for use with GRT object values)"},

  {"togrt", grt_wrap_pyobject, METH_VARARGS,
   "Wraps a Python object in a grt_PyObject GRT object, which you can then use to reference from GRT objects (such as "
   "GRT dicts and lists)"},

  {"getEventLogEntry", getEventLogEntry, METH_VARARGS, "Read logentry from windows event log."},
  {"setEventlogCallback", setEventlogCallback, METH_VARARGS,
   "Pushes a callback that print log entry form a EventViewer."},

  {nullptr, nullptr, 0, nullptr} /* Sentinel */
};

static struct PyModuleDef grtModuleDef = {
  PyModuleDef_HEAD_INIT,
  "grt",    // name
  nullptr,  // documentation
  -1,       // size -1 meand it can not be re-initialized
  GrtModuleMethods,
  nullptr,  // reload method
  nullptr,  // traverse method
  nullptr,  // clear method
  nullptr   //  free method
};

static struct PyModuleDef grtModulesModuleDef = {
  PyModuleDef_HEAD_INIT,
  "grt.modules",  // name
  "GRT.modules module documentation",  // documentation
  -1,       // size -1 meand it can not be re-initialized
  nullptr,  // methods
  nullptr,  // reload method
  nullptr,  // traverse method
  nullptr,  // clear method
  nullptr   // free method
};

static struct PyModuleDef grtClassesModuleDef = {
  PyModuleDef_HEAD_INIT,
  "grt.classes",  // name
  "GRT.classes module documentation",  // documentation
  -1,       // size -1 meand it can not be re-initialized
  nullptr,  // methods
  nullptr,  // reload method
  nullptr,  // traverse method
  nullptr,  // clear method
  nullptr   // free method
};

void PythonContext::register_grt_module(PyObject *module) {
  _grt_module = module;

  // add the context ptr
  PyObject *context_object = PyCapsule_New(this, "contextObject", nullptr);
  PyCapsule_SetContext(context_object, &GRTTypeSignature);
  if (context_object != nullptr)
    PyModule_AddObject(module, "__GRT__", context_object);

  PyModule_AddStringConstant(module, "INT", (char *)type_to_str(IntegerType).c_str());
  PyModule_AddStringConstant(module, "DOUBLE", (char *)type_to_str(DoubleType).c_str());
  PyModule_AddStringConstant(module, "STRING", (char *)type_to_str(StringType).c_str());
  PyModule_AddStringConstant(module, "LIST", (char *)type_to_str(ListType).c_str());
  PyModule_AddStringConstant(module, "DICT", (char *)type_to_str(DictType).c_str());
  PyModule_AddStringConstant(module, "OBJECT", (char *)type_to_str(ObjectType).c_str());

  init_grt_module_type();
  init_grt_list_type();
  init_grt_dict_type();
  init_grt_object_type();

  {
    _grt_user_interrupt_error = PyErr_NewException((char *)"grt.UserInterrupt", nullptr, nullptr);
    PyModule_AddObject(_grt_module, "UserInterrupt", _grt_user_interrupt_error);
  }
  {
    _grt_db_error = PyErr_NewException((char *)"grt.DBError", nullptr, nullptr);
    PyModule_AddObject(_grt_module, "DBError", _grt_db_error);
  }
  {
    _grt_db_access_denied_error = PyErr_NewException((char *)"grt.DBAccessDenied", nullptr, nullptr);
    PyModule_AddObject(_grt_module, "DBAccessDenied", _grt_db_access_denied_error);
  }
  {
    _grt_db_login_error = PyErr_NewException((char *)"grt.DBLoginError", nullptr, nullptr);
    PyModule_AddObject(_grt_module, "DBLoginError", _grt_db_login_error);
  }
  {
    _grt_db_not_connected = PyErr_NewException((char *)"grt.DBNotConnected", nullptr, nullptr);
    PyModule_AddObject(_grt_module, "DBNotConnected", _grt_db_not_connected);
  }

  _grt_modules_module = PyModule_Create(&grtModulesModuleDef);
  if (!_grt_modules_module)
    throw std::runtime_error("Error initializing grt.modules module in Python support");

  if(PyModule_AddObject(_grt_modules_module, "__path__", Py_BuildValue("[s]", "grt/modules")) < 0) {
    PyErr_Print();
    throw std::runtime_error("Error initializing grt.modules module in Python support");
  }

  if(PyModule_AddObject(_grt_modules_module, "__package__", Py_BuildValue("s", "grt.modules")) < 0) {
    PyErr_Print();
    throw std::runtime_error("Error initializing grt.modules module in Python support");
  }

  // AutoPyObject need to keep a reference but PyModule_AddObject steals it
  // so it is needed to increase it to avoid problems on destruction
  Py_XINCREF(_grt_modules_module);
  if(PyModule_AddObject(_grt_module, "modules", _grt_modules_module) < 0)
    throw std::runtime_error("Error initializing grt.modules module in Python support");

  _grt_classes_module = PyModule_Create(&grtClassesModuleDef);

  if (!_grt_classes_module)
    throw std::runtime_error("Error initializing grt.classes module in Python support");

//   PyDict_SetItemString(PyModule_GetDict(module), "classes", (PyObject *)_grt_classes_module);

  Py_XINCREF(_grt_classes_module);
  if (PyModule_AddObject(_grt_module, "classes", _grt_classes_module) < 0)
    throw std::runtime_error("Error initializing grt.classes module in Python support");

  if(PyModule_AddObject(_grt_classes_module, "grt", _grt_module) < 0)
    throw std::runtime_error("Error initializing grt module in Python support");

  Py_XINCREF(_grt_classes_module);
  if(PyModule_AddObject(_grt_module, "classes", _grt_classes_module) < 0)
    throw std::runtime_error("Error initializing grt.classes module in Python support");

}

PyObject *PythonContext::grt_module_create(){
  PyObject *module = PyModule_Create(&grtModuleDef);
  if (module == nullptr)
    throw std::runtime_error("Error initializing GRT module in Python support");

  PyModule_AddObject(module, "__path__", Py_BuildValue("[]"));

  return module;
}

PyObject *PythonContext::get_grt_module() {
  return _grt_module;
}

PyObject *PythonContext::import_module(const std::string &name) {
  PyObject *main = PyImport_AddModule("__main__");
  PyObject *module = PyImport_ImportModule((char *)name.c_str());
  if (!main || !module) {
    PyErr_Print();
    PythonContext::log_python_error(base::strfmt("Error importing %s", name.c_str()).c_str());
    return nullptr;
  }
  PyDict_SetItemString(PyModule_GetDict(main), name.c_str(), module);

  return module;
}

PyObject *PythonContext::eval_string(const std::string &expression) {
  //  LockPython lock(this);

  PyObject *mainmod = PyImport_AddModule("__main__");
  if (!mainmod) {
    PyErr_Clear();
    return nullptr;
  }
  PyObject *globals = PyModule_GetDict(mainmod);
  if (globals) {
    PyObject *result = PyRun_String(expression.c_str(), Py_eval_input, globals, globals);
    if (!result) {
      PyErr_Print();
      PythonContext::log_python_error(base::strfmt("Error running expr: %s", expression.c_str()).c_str());
    }
    return result;
  }
  PyErr_Clear();
  return nullptr;
}

PyObject *PythonContext::get_global(const std::string &value) {
  return eval_string(value);
}

bool PythonContext::set_global(const std::string &name, PyObject *value) {
  PyObject *mainmod = PyImport_AddModule("__main__");
  if (!mainmod) {
    PythonContext::log_python_error("Error getting __main__");
    PyErr_Clear();
    return false;
  }
  PyObject *globals = PyModule_GetDict(mainmod);
  if (!globals) {
    PythonContext::log_python_error("Error getting __main__ dict");
    PyErr_Clear();
    return false;
  }

  PyDict_SetItemString(globals, name.c_str(), value);
  return true;
}

/** Wraps a grt value in a PyCObject.

 PyCObjects are used internally to initialize a grt.List/Dict or Object from an existing grt value.
 */
PyObject *PythonContext::internal_cobject_from_value(const ValueRef &value) {
  internal::Value *v = value.valueptr();
  v->retain();
  PyObject *ret = PyCapsule_New(v, "contextObject", nullptr);
  PyCapsule_SetContext(ret, &GRTValueSignature);
  return ret;
}

ValueRef PythonContext::value_from_internal_cobject(PyObject *value) {
  if (PyCapsule_GetContext(value) == &GRTValueSignature)
    return ValueRef(reinterpret_cast<internal::Value *>(PyCapsule_GetPointer(value, "contextObject")));

  throw std::runtime_error("attempt to extract GRT value from invalid Python object");
}

/** Convert a GRT value to a Python object/value.
 *
 * For objects, it will also wrap in the appropriate object subclass from grt.classes
 */
PyObject *PythonContext::from_grt(const ValueRef &value) {
  if (value.is_valid()) {
    switch (value.type()) {
      case IntegerType: {
        return PyLong_FromSsize_t(*IntegerRef::cast_from(value));
      }

      case DoubleType:
        return PyFloat_FromDouble(*DoubleRef::cast_from(value));

      case StringType: {
        // maybe this should start returning unicode data, but before that all python code
        // should be tested if it can handle the unicode type. For now we just return utf8 strings.
        std::string data = *StringRef::cast_from(value);
        return PyUnicode_FromStringAndSize(data.data(), data.size());
      }
      case ListType: {
        PyObject *content = PythonContext::internal_cobject_from_value(value);
        PyObject *args = Py_BuildValue("(ssO)", "", "", content);
        PyObject *r = PyObject_Call(_grt_list_class, args, nullptr);
        Py_XDECREF(args);
        Py_XDECREF(content);
        return r;
      }
      case DictType: {
        PyObject *content = PythonContext::internal_cobject_from_value(value);
        PyObject *args = Py_BuildValue("(ssO)", "", "", content);
        PyObject *r = PyObject_Call(_grt_dict_class, args, nullptr);
        Py_XDECREF(args);
        Py_XDECREF(content);
        return r;
      }
      case ObjectType: {
        std::string class_name = grt::ObjectRef::cast_from(value).class_name();
        PyObject *content = PythonContext::internal_cobject_from_value(value);
        PyObject *theclass = _grt_class_wrappers[class_name];
        PyObject *args = Py_BuildValue("(sO)", "", content);
        PyObject *r = PyObject_Call(theclass ? theclass : (PyObject *)_grt_object_class, args, nullptr);
        Py_XDECREF(args);
        Py_XDECREF(content);

        return r;
      }
      default:
        return nullptr;
    }
  }
  Py_XINCREF(Py_None);
  return Py_None;
}

bool PythonContext::pystring_to_string(PyObject *strobject, std::string &ret_string, bool convert) {
  PyObject *ref = strobject;
  ret_string = "";

  if (!PyUnicode_Check(strobject)) {
    if (convert)
      ref = PyObject_Str(strobject);
    else
      ref = PyUnicode_AsUTF8String(strobject);
  }

  if (ref == nullptr)
    return false;

  const char *str;
  Py_ssize_t len;
  str = PyUnicode_AsUTF8AndSize(ref, &len);
  if (str)
    ret_string = std::string(str, len);

  return str != nullptr;
}

ValueRef PythonContext::from_pyobject(PyObject *object) {
  if (!object || object == Py_None)
    return ValueRef();

  if (PyLong_Check(object))
    return IntegerRef(PyLong_AsLong(object));

  if (PyLong_Check(object))
    return IntegerRef(PyLong_AsLong(object));

  if (PyFloat_Check(object))
    return DoubleRef(PyFloat_AsDouble(object));

  if (PyUnicode_Check(object) || PyUnicode_Check(object)) {
    std::string tmp;
    if (pystring_to_string(object, tmp))
      return StringRef(tmp);
    return ValueRef();
  }

  if (PyTuple_Check(object)) {
    grt::BaseListRef list(true);

    for (Py_ssize_t c = PyTuple_Size(object), i = 0; i < c; i++) {
      PyObject *item = PyTuple_GetItem(object, i);
      list.ginsert(from_pyobject(item));
    }
    return list;
  }

  if (PyList_Check(object)) {
    grt::BaseListRef list(true);

    for (Py_ssize_t c = PyList_Size(object), i = 0; i < c; i++) {
      PyObject *item = PyList_GetItem(object, i);
      list.ginsert(from_pyobject(item));
    }
    return list;
  } else if (PyObject_IsInstance(object, _grt_list_class))
    return *((PyGRTListObject *)object)->list;

  if (PyDict_Check(object)) {
    grt::DictRef dict(true);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(object, &pos, &key, &value)) {
      dict.set(PyUnicode_AsUTF8(key), from_pyobject(value));
    }

    return dict;
  } else if (PyObject_IsInstance(object, _grt_dict_class))
    return *((PyGRTDictObject *)object)->dict;

  if (PyObject_IsInstance(object, _grt_object_class))
    return *((PyGRTObjectObject *)object)->object;

  return ValueRef();
}

ValueRef PythonContext::simple_type_from_pyobject(PyObject *object, const grt::SimpleTypeSpec &type) {
  switch (type.type) {
    case IntegerType: {
      if (PyFloat_Check(object))
        return IntegerRef((long)PyFloat_AsDouble(object));
      else
        PyErr_Clear();

      if (PyLong_Check(object))
        return IntegerRef(PyLong_AsLong(object));
      else
        PyErr_Clear();

      if (!PyLong_Check(object))
        return IntegerRef(PyLong_AsLong(object));
      else
        PyErr_Clear();

      throw grt::type_error("expected integer type x");
    }
    case DoubleType: {
      if (PyLong_Check(object))
        return DoubleRef(PyLong_AsLong(object));
      else
        PyErr_Clear();
      if (!PyFloat_Check(object))
        throw grt::type_error("expected double type");
      return DoubleRef(PyFloat_AsDouble(object));
    }
    case StringType: {
      std::string tmp;
      if (pystring_to_string(object, tmp))
        return StringRef(tmp);
      else
        throw grt::type_error("expected string type");
    }
    case ObjectType: {
      if (!PyObject_IsInstance(object, _grt_object_class))
        throw grt::type_error("expected GRT object");

      grt::ObjectRef grtobject(*((PyGRTObjectObject *)object)->object);

      if (!type.object_class.empty() && !grtobject->is_instance(type.object_class))
        throw grt::type_error(strfmt("expected GRT object of class %s", type.object_class.c_str()));

      return grtobject;
    }
    default:
      return ValueRef();
  }
}

ValueRef PythonContext::from_pyobject(PyObject *object, const grt::TypeSpec &expected_type) {
  if (object == Py_None)
    return ValueRef();

  switch (expected_type.base.type) {
    case ObjectType:
    case IntegerType:
    case DoubleType:
    case StringType:
      return simple_type_from_pyobject(object, expected_type.base);

    case ListType: {
      if (PyList_Check(object)) {
        grt::BaseListRef list(expected_type.content.type);

        for (Py_ssize_t c = PyList_Size(object), i = 0; i < c; i++) {
          PyObject *item = PyList_GetItem(object, i);
          switch (expected_type.content.type) {
            case ObjectType:
            case IntegerType:
            case DoubleType:
            case StringType:
              list.ginsert(simple_type_from_pyobject(item, expected_type.content));
              break;
            case AnyType:
              list.ginsert(from_pyobject(item));
              break;
            default:
              logWarning("invalid type spec requested\n");
              break;
          }
        }
        return list;
      } else if (PyObject_IsInstance(object, _grt_list_class)) {
        return *((PyGRTListObject *)object)->list;
      } else
        throw grt::type_error("expected list");
    }
    case DictType: {
      if (PyDict_Check(object)) {
        grt::DictRef dict(true);
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(object, &pos, &key, &value)) {
          switch (expected_type.content.type) {
            case ObjectType:
            case IntegerType:
            case DoubleType:
            case StringType:
              dict.set(PyUnicode_AsUTF8(key), simple_type_from_pyobject(value, expected_type.content));
              break;
            case AnyType:
              dict.set(PyUnicode_AsUTF8(key), from_pyobject(value));
              break;
            default:
              logWarning("invalid type spec requested\n");
              break;
          }
        }

        return dict;
      } else if (PyObject_IsInstance(object, _grt_dict_class)) {
        return *((PyGRTDictObject *)object)->dict;
      } else
        throw grt::type_error("expected dict");
    }
    default:
      return from_pyobject(object);
  }

  return ValueRef();
}

int PythonContext::run_file(const std::string &file, bool interactive) {
  FILE* f = base_fopen(file.c_str(), "r");
  if (!f) {
    PythonContext::log_python_error(base::strfmt("Could not open file %s\n", file.c_str()).c_str());
    return -1;
  }

  logDebug2("About to pyrun '%s'\n", file.c_str());
  if (PyRun_SimpleFile(f, file.c_str()) != 0) {
    fclose(f);
    PythonContext::log_python_error(base::strfmt("Error running file %s\n", file.c_str()).c_str());
    return -1;
  }
  fclose(f);

  return 0;
}

/** Execute a string as Python code.

 If line_buffer is not null, it will be used as a buffer for multiple-line statements.
 It is used by interactive shell, to construct these from single-line components.

 If line_buffer is null, the passed buffer will be expected to contain complete code.
 */
int PythonContext::run_buffer(const std::string &buffer, std::string *line_buffer) {
  if (line_buffer) {
    // if previous buff is empty and new command is plain enter key, do nothing
    if (line_buffer->empty() && buffer[0] == '\n')
      return 0;
    line_buffer->append(buffer);
  }
  
  WillEnterPython lock;
  PyCompilerFlags flags;
  flags.cf_flags = line_buffer ? Py_single_input : Py_file_input;
#ifndef _MSC_VER
  flags.cf_feature_version = PY_MINOR_VERSION;
#endif
  
  //  This command will compile and run the code in line_buffer or buffer
  if(PyRun_SimpleStringFlags(line_buffer ? line_buffer->c_str() : buffer.c_str(), &flags) == -1){
    PythonContext::log_python_error("Error running buffer");
    
    if (line_buffer)
      line_buffer->clear();
    
    return -1;
  }
  
  return 0;
}

// template of a python function to create a wrapper class for a GRT class
static const char *create_class_template =
  "class %s(grt.Object):\n"
  "  __grtclassname__ = \"%s\"\n"
  "  def __init__(self, classname = None, wrapobj = None):\n"
  "    grt.Object.__init__(self, classname, wrapobj)";

static const char *create_class_template_sub =
  "class %s(%s):\n"
  "  __grtclassname__ = \"%s\"\n"
  "  def __init__(self, classname = '%s', wrapobj = None):\n"
  "    %s.__init__(self, classname, wrapobj)";

/** Create a Python subclass of our grt.Object class, that will wrap around the given GRT class
 */
static void create_class_wrapper(grt::MetaClass *meta, PyObject *locals) {
  std::string script;
  grt::MetaClass *parent;

  if ((parent = meta->parent()) && parent->parent() != 0) {
    std::string parname = flatten_class_name(parent->name());
    script = strfmt(create_class_template_sub, flatten_class_name(meta->name()).c_str(), parname.c_str(),
                    meta->name().c_str(), meta->name().c_str(), parname.c_str());
  } else {
    script = strfmt(create_class_template, flatten_class_name(meta->name()).c_str(), meta->name().c_str());
  }

  PyDict_SetItemString(locals, "__builtins__", PyEval_GetBuiltins());

  if (!PyRun_String(script.c_str(), Py_single_input, locals, locals))
    PythonContext::log_python_error((std::string("Error creating class wrapper:\n") + script).c_str());
}

/** Refresh Python environment with GRT information.
 */
int PythonContext::refresh() {
  WillEnterPython lock;

  _grt_class_wrappers.clear();

  PyModule_AddObject(get_grt_module(), "root", from_grt(grt::GRT::get()->root()));

  PyObject *classes_dict = PyModule_GetDict(_grt_classes_module);

  Py_XINCREF(classes_dict);

  // Generate Python class hierarchy to wrap GRT classes
  const std::list<grt::MetaClass *> &classes(grt::GRT::get()->get_metaclasses());
  for (std::list<grt::MetaClass *>::const_iterator iter = classes.begin(); iter != classes.end(); ++iter) {
    create_class_wrapper(*iter, classes_dict);

    _grt_class_wrappers[(*iter)->name()] =
      PyDict_GetItemString(classes_dict, flatten_class_name((*iter)->name()).c_str());
  }

  Py_DECREF(classes_dict);

  // Generate module wrappers
  const std::vector<grt::Module *> &modules(grt::GRT::get()->get_modules());
  for (std::vector<grt::Module *>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
    std::string name = (*iter)->name();
    PyObject *arg = Py_BuildValue("(s)", name.c_str());
    PyObject *r = PyObject_Call(_grt_module_class, arg, nullptr);
    Py_DECREF(arg);

    if (!r)
      PythonContext::log_python_error("Error refreshing grt modules");
    else if (PyModule_AddObject(_grt_modules_module, (char *)(*iter)->name().c_str(), r) < 0)
      PythonContext::log_python_error("Error refreshing grt modules");
  }

  return 0;
}

void PythonContext::log_python_error(const char *message) {
  PythonContext *ctx = PythonContext::get();
  if (!ctx)
    return;
  std::string reason = message, stack = "Traceback:\n  No stack information.\n";
  PyObject *exc, *val, *tb;

  PyErr_Fetch(&exc, &val, &tb);
  PyErr_NormalizeException(&exc, &val, &tb);

  if (val) {
    PyObject *tmp = PyObject_Str(val);
    if (tmp) {
      reason = PyUnicode_AsUTF8(tmp);
      Py_DECREF(tmp);
    }
  }

  if (tb) {
    /*PyTracebackObject *trace = (PyTracebackObject *)tb;

    stack = "Traceback:\n";
    while (trace && trace->tb_frame) {
      PyFrameObject *frame = (PyFrameObject *)trace->tb_frame;
      if (frame) {
#if defined(__APPLE__) || defined(_MSC_VER)
        auto *codeObject = frame->f_code;
#else
        PyCodeObject *codeObject = PyFrame_GetCode(frame);
#endif
        if (codeObject) {
          stack += base::strfmt("  File \"%s\", line %i, in %s\n", PyUnicode_AsUTF8(codeObject->co_filename),
                                trace->tb_lineno, PyUnicode_AsUTF8(codeObject->co_name));
          PyObject *code = PyErr_ProgramText(PyUnicode_AsUTF8(codeObject->co_filename), trace->tb_lineno);
          if (code) {
            stack += base::strfmt("    %s", PyUnicode_AsUTF8(code));
            Py_DECREF(code);
          }
#ifndef _MSC_VER
          Py_DECREF(codeObject);
#endif
        }
      }
      trace = trace->tb_next;
    }*/
  }

  base::Logger::log(base::Logger::LogLevel::Error, "python", "%s\n%sNameError: %s\n", message, stack.c_str(), reason.c_str());
}

// script to be executed once GRT is initialized
// executed in the grt module namespace
static const char *post_init_script =
  "import grt\n"
  "class _grtFileRedirector:\n"
  "    def __init__(self, logger):\n"
  "        self.logger = logger\n"
  "    def write(self, text):\n"
  "        if type(text) not in (str, unicode):\n"
  "            text = str(text)\n"
  "        grt.send_output(text)\n"
  "        self.logger(grt._log_domain, text)\n"
  "grt._log_domain = 'python'\n"
  "grt._log_stdout = _grtFileRedirector(grt.log_info)\n"
  "grt._log_stderr = _grtFileRedirector(grt.log_error)\n";

void PythonContext::run_post_init_script() {
  WillEnterPython lock;

  if (PyRun_SimpleString((char *)post_init_script) < 0)
    PythonContext::log_python_error("Error running post-init script:");
}
