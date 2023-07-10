/*
 * Copyright (c) 2009, 2020, Oracle and/or its affiliates.
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

#include "grtpp_module_python.h"
#include "grtpp_util.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/log.h"

#include <iostream>

DEFAULT_LOG_DOMAIN("ModulePython");

using namespace grt;
using namespace base;

PythonModule::PythonModule(PythonModuleLoader *loader, PyObject *module) : Module(loader), _module(module) {
}

PythonModule::~PythonModule() {
  Py_XDECREF(_module);
}

static TypeSpec parse_type(PyObject *type) {
  if (PyUnicode_Check(type)) {
    TypeSpec s;
    s.base.type = str_to_type(PyUnicode_AsUTF8(type));
    return s;
  }
  PyErr_Clear();

  if (PyTuple_Check(type)) {
    TypeSpec s;
    PyObject *b, *c;

    if (PyTuple_Size(type) != 2) {
      PythonContext::log_python_error(
        "Invalid type specification for Python module function. Tuple must be in form (<container type>, <content "
        "type>)");
      throw std::runtime_error("Invalid type specification. Tuple must be in form (<container type>, <content type>)");
    }

    b = PyTuple_GetItem(type, 0);
    if (!b) {
      PythonContext::log_python_error("Invalid type specification for Python module function");
      throw std::runtime_error("Invalid type specification 0");
    }
    c = PyTuple_GetItem(type, 1);
    if (!c) {
      PythonContext::log_python_error("Invalid type specification for Python module function");
      throw std::runtime_error("Invalid type specification 1");
    }

    s.base.type = str_to_type(PyUnicode_AsUTF8(b));
    if (s.base.type == grt::ObjectType) {
      if (PyUnicode_Check(c))
        s.base.object_class = PyUnicode_AsUTF8(c);
      else
        throw std::runtime_error("Invalid object type specification");
    } else {
      if (PyUnicode_Check(c))
        s.content.type = str_to_type(PyUnicode_AsUTF8(c));
      else if (PyTuple_Check(c) && PyTuple_Size(c) == 2) {
        s.content.type = grt::ObjectType;
        s.content.object_class = PyUnicode_AsUTF8(PyTuple_GetItem(c, 1));
      } else
        throw std::runtime_error("Invalid type specification");
    }
    return s;
  }
  PyErr_Clear();

  throw std::runtime_error("Invalid type specification");
}

void PythonModule::add_parse_function(const std::string &name, PyObject *return_type, PyObject *arguments,
                                      PyObject *callable) {
  Function func;

  func.name = name;
  try {
    func.ret_type = parse_type(return_type);
  } catch (std::exception &exc) {
    logError("Invalid return type specification in %s.%s: %s\n", _name.c_str(), name.c_str(), exc.what());
    throw std::runtime_error(
      strfmt("Invalid return type specification in %s.%s: %s", _name.c_str(), name.c_str(), exc.what()));
  }

  for (Py_ssize_t c = PyList_Size(arguments), i = 0; i < c; i++) {
    PyObject *spec = PyList_GetItem(arguments, i);
    ArgSpec arg;
    PyObject *tmp;

    if (!PyTuple_Check(spec)) {
      PythonContext::log_python_error("Invalid argument specification for Python module function (not a tuple)");
      throw std::runtime_error("Invalid argument specification (argument spec must be list of tuples)");
    }

    tmp = PyTuple_GetItem(spec, 0);
    if (!tmp || !PyUnicode_Check(tmp)) {
      PythonContext::log_python_error("Invalid argument name specification for Python module function");
      throw std::runtime_error("Invalid argument name specification");
    }
    arg.name = PyUnicode_AsUTF8(tmp);

    tmp = PyTuple_GetItem(spec, 1);
    if (!tmp) {
      PythonContext::log_python_error("Invalid argument type specification for Python module function");
      throw std::runtime_error("Invalid argument type specification");
    }
    try {
      arg.type = parse_type(tmp);
    } catch (std::exception &exc) {
      logError("Invalid argument type specification in %s.%s: %s\n", _name.c_str(), name.c_str(), exc.what());
      throw std::runtime_error(strfmt("Invalid argument type specification in %s.%s", _name.c_str(), name.c_str()));
    }
    func.arg_types.push_back(arg);
  }

  PyObject *doc = PyObject_GetAttrString(callable, "__doc__");
  if (doc && doc != Py_None)
    func.description = PyUnicode_AsUTF8(doc);

  func.call = std::bind(&PythonModule::call_python_function, this, std::placeholders::_1, callable, func);

  add_function(func);
}

static std::string exception_detail() {
  PyObject *exc_class = NULL, *exc = NULL, *exc_tb = NULL;
  PyErr_Fetch(&exc_class, &exc, &exc_tb);
  if (exc) {
    PyObject *str = PyObject_Str(exc);
    if (str) {
      const char *s = PyUnicode_AsUTF8(str);
      if (s)
        return s;
      Py_DECREF(str);
    }
    Py_DECREF(exc);
  }
  Py_XDECREF(exc_class);
  Py_XDECREF(exc_tb);
  return "";
}

ValueRef PythonModule::call_python_function(const BaseListRef &args, PyObject *function, const Function &funcdef) {
  WillEnterPython lock;

  PythonContext *ctx = ((PythonModuleLoader *)get_loader())->get_python_context();
  PyObject *argtuple;

  if (args.is_valid()) {
    argtuple = PyTuple_New(args.count());
    int i = 0;

    // convert arguments to a tuple that can be passed to the function
    for (BaseListRef::raw_const_iterator iter = args.begin(); iter != args.end(); ++iter)
      PyTuple_SetItem(argtuple, i++, ctx->from_grt(*iter));
  } else {
    argtuple = PyTuple_New(0);
  }

  // call the function
  PyObject *ret = PyObject_Call(function, argtuple, NULL);
  Py_DECREF(argtuple);

  if (!ret || PyErr_Occurred()) {
    if (PyErr_ExceptionMatches(ctx->user_interrupted_error())) {
      std::string what = exception_detail();
      if (what.empty())
        what = "Interrupted by user";

      throw grt::user_cancelled(what);
    }
    if (PyErr_ExceptionMatches(ctx->db_access_denied_error())) {
      std::string what = exception_detail();
      if (what.empty())
        what = "Access denied";
      throw grt::db_access_denied(what);
    }
    if (PyErr_ExceptionMatches(ctx->db_login_error())) {
      std::string what = exception_detail();
      if (what.empty())
        what = "Login error, check username and password";
      throw grt::db_login_error(what);
    }
    PyObject *etype;
    PyObject *evalue;
    PyObject *etrace;
    PyErr_Fetch(&etype, &evalue, &etrace);

    PyObject *s = PyObject_GetAttrString(etype, "__name__");
    std::string ename, exc;
    if (!ctx->pystring_to_string(s, ename))
      ename = "???";
    s = PyObject_Str(evalue);
    if (!ctx->pystring_to_string(s, exc))
      exc = "???";
    if (s) {
      // Explicit braces here as Py_DECREF is a macro with an if/else part.
      Py_DECREF(s);
    }

    PyErr_Restore(etype, evalue, etrace);
    PythonContext::log_python_error(strfmt("error calling %s.%s", name().c_str(), funcdef.name.c_str()).c_str());
    throw grt::module_error(strfmt("error calling Python module function %s.%s", name().c_str(), funcdef.name.c_str()),
                            strfmt("%s(\"%s\")", ename.c_str(), exc.c_str()));
  }

  ValueRef result = ctx->from_pyobject(ret, funcdef.ret_type);

  Py_XDECREF(ret);

  return result;
}

PythonModuleLoader::PythonModuleLoader(const std::string &module_path) : _pycontext(module_path) {
}

PythonModuleLoader::~PythonModuleLoader() {
}

static std::string formatStringList(PyObject *list) {
    std::string result;
    PyObject *item;

    if (!list) {
      return "";
    }
    ssize_t count = PyList_Size(list);
    for (ssize_t index = 0; index < count; ++index) {
        item = PyList_GetItem(list, index);
        result += PyUnicode_AsUTF8(item);
    }
    return result;
}

static std::string handlePyError() {
  if (!PyErr_Occurred()) 
    return "";
    
  PyObject *type, *value, *traceback;
  PyObject *pythonErrorDescryption, *moduleName, *pythonModule, *formatExceptionFunction;

  PyErr_Fetch(&type, &value, &traceback);
  pythonErrorDescryption = PyObject_Str(value);
  std::string errorDescription = PyUnicode_AsUTF8(pythonErrorDescryption);
  std::string result = "Unhandled exception in Python code: \n";

  // See if we can get a full traceback
  moduleName = PyUnicode_FromString("traceback");
  pythonModule = PyImport_Import(moduleName);
  Py_DECREF(moduleName);

  if (pythonModule) {
    formatExceptionFunction = PyObject_GetAttrString(pythonModule, "format_exception");
    
    if (formatExceptionFunction && PyCallable_Check(formatExceptionFunction)) {
      PyObject *formatExceptionFunctionResult;

      formatExceptionFunctionResult = PyObject_CallFunctionObjArgs(formatExceptionFunction, type, value, traceback, NULL);

      result += formatStringList(formatExceptionFunctionResult);
    }
  }

  return result;
}

Module *PythonModuleLoader::init_module(const std::string &path) {
  PyObject *mod;
  std::string name;

  WillEnterPython lock;

  if (path.rfind('.') != std::string::npos)
    name = path.substr(0, path.rfind('.')); // strip extension
  else
    name = path;

  name = base::basename(name);

  {
    PyObject *old_path, *sysmod, *path_list;

    // temporarily add the file's path to the module lookup pat
    sysmod = PyImport_AddModule("sys");

    path_list = PyDict_GetItemString(PyModule_GetDict(sysmod), "path");
    old_path = PyList_GetSlice(path_list, 0, PyList_Size(path_list));
    {
      PyObject *tmp = PyUnicode_FromString(base::dirname(path).c_str());
      PyList_Append(path_list, tmp);
      Py_DECREF(tmp);
    }
    // import module
    mod = PyImport_ImportModule((char *)name.c_str());
    
    // restore path
    PyDict_SetItemString(PyModule_GetDict(sysmod), "path", old_path);
    Py_DECREF(old_path);
    if (mod == NULL) {
      std::string exception = handlePyError();
      PythonContext::log_python_error(base::strfmt("Error importing Python module %s\n%s", path.c_str(), exception.c_str()).c_str());
      return 0;
    }
  }

  {
    PyObject *module_dict = PyModule_GetDict(mod);
    PyObject *moduleInfo = NULL;

    moduleInfo = PyDict_GetItemString(module_dict, "ModuleInfo");
    if (!moduleInfo) {
      PyErr_Print();
      return 0;
    }
    if (!PyDict_Check(moduleInfo)) {
      Py_XDECREF(moduleInfo);
      PyErr_Clear();
      throw grt::module_error("ModuleInfo is not an object");
    }

    grt::PythonModule *module = new grt::PythonModule(this, mod);

    module->_path = path;
    {
      PyObject *name;
      name = PyObject_GetAttrString(moduleInfo, "name");
      if (name && PyUnicode_Check(name))
        module->_name = PyUnicode_AsUTF8(name);
      else {
        PyErr_Print();
        Py_XDECREF(moduleInfo);
        delete module;
        throw grt::module_error("ModuleInfo incorrectly defined (name attribute missing)");
      }
    }

    PyObject *functions = PyObject_GetAttrString(moduleInfo, "functions");
    if (functions && PyList_Check(functions)) {
      for (Py_ssize_t c = PyList_Size(functions), i = 0; i < c; i++) {
        PyObject *item = PyList_GetItem(functions, i);
        const char *name = 0;
        PyObject *rettype;
        PyObject *argtypes;
        PyObject *callable;

        if (!PyArg_ParseTuple(item, "z(OO!)O!", &name, &rettype, &PyList_Type, &argtypes, &PyFunction_Type,
                              &callable)) {
          PySys_WriteStderr("ERROR: Invalid module function specification in %s\n", path.c_str());
          PyErr_Print();
          PyObject *tmp = PyTuple_GetItem(item, 0);
          if (tmp && PyUnicode_Check(tmp)) {
            PySys_WriteStderr("  for function %s.%s\n", module->_name.c_str(), PyUnicode_AsUTF8(tmp));
          }
          PyErr_Clear();
          delete module;
          return 0;
        }

        try {
          module->add_parse_function(name ? name : "", rettype, argtypes, callable);
        } catch (std::exception &exc) {
          delete module;
          throw grt::module_error(strfmt("Error registering Python module function: %s", exc.what()));
        }
      }
    } else
      PyErr_Print();

    PyObject *implements = PyObject_GetAttrString(moduleInfo, "implements");
    if (!implements || !PyList_Check(implements)) {
      PyErr_Print();
      delete module;
      throw grt::module_error("Invalid value for 'implements' list");
    }

    for (Py_ssize_t c = PyList_Size(implements), i = 0; i < c; i++) {
      PyObject *name = PyList_GetItem(implements, i);
      if (!PyUnicode_Check(name)) {
        PyErr_Print();
        delete module;
        throw grt::module_error("Invalid value for 'implements' list");
      }

      module->_interfaces.push_back(PyUnicode_AsUTF8(name));
    }

    PyObject *meta;

    meta = PyObject_GetAttrString(moduleInfo, "author");
    if (meta && PyUnicode_Check(meta))
      module->_meta_author = PyUnicode_AsUTF8(meta);
    else
      PyErr_Print();

    meta = PyObject_GetAttrString(moduleInfo, "version");
    if (meta && PyUnicode_Check(meta))
      module->_meta_version = PyUnicode_AsUTF8(meta);
    else
      PyErr_Print();

    meta = PyObject_GetAttrString(moduleInfo, "description");
    if (meta && PyUnicode_Check(meta))
      module->_meta_description = PyUnicode_AsUTF8(meta);
    else
      PyErr_Print();

    {
      if (g_str_has_suffix(base::dirname(path).c_str(), ".mwbplugin"))
        module->_is_bundle = true;
    }
    return module;
  }

  return 0;
}

void PythonModuleLoader::refresh() {
  _pycontext.refresh();
}

void PythonModuleLoader::add_module_dir(const std::string &dirpath) {
  WillEnterPython lock;

  PyObject *sysmod, *path_list;
  PyObject *path = PyUnicode_FromString(dirpath.c_str());

  sysmod = PyImport_AddModule("sys");

  path_list = PyDict_GetItemString(PyModule_GetDict(sysmod), "path");
  Py_ssize_t i;

  // check if the path is already in it
  for (i = PyList_Size(path_list) - 1; i >= 0; --i) {
    if (PyObject_RichCompareBool(PyList_GetItem(path_list, i), path, Py_EQ) == 1)
      break;
  }

  if (i < 0) // not found
    PyList_Append(path_list, path);

  Py_DECREF(path);
}

bool PythonModuleLoader::load_library(const std::string &file) {
  // add the path to the search path so that it can be imported
  { add_module_dir(base::dirname(file)); }

  return true;
}

bool PythonModuleLoader::run_script_file(const std::string &path) {
  if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
    return false;

  WillEnterPython lock;

  // Execute in the global environment will make it available to the interactive shell.
  return _pycontext.run_file(path, true) == 0;
}

bool PythonModuleLoader::run_script(const std::string &script) {
  return _pycontext.run_buffer(script, 0) == 0;
}

bool PythonModuleLoader::check_file_extension(const std::string &path) {
  if (g_str_has_suffix(path.c_str(), ".py"))
    return true;

  return false;
}
