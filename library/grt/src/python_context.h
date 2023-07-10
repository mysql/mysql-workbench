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

#pragma once

#include "base/python_utils.h"

#include "grt.h"
#include "grtpp_notifications.h"

namespace grt {
  const std::string LanguagePython = "python";

  class AutoPyObject {
  private:
    PyObject *object;
    bool autorelease;

  public:
    AutoPyObject() : object(0), autorelease(false) {
    }

    // Assigning another auto object always makes this one ref-counting as they share
    // now the same object. Same for the assignment operator.
    AutoPyObject(const AutoPyObject &other) : object(other.object), autorelease(false) {
      Py_XINCREF(object);
    }

    AutoPyObject(PyObject *py, bool retain = true) : object(py) {
      autorelease = retain;
      if (autorelease) {
        // Leave the braces in place, even though this is a one liner. They will silence LLVM.
        Py_XINCREF(object);
      }
    }

    ~AutoPyObject() {
      if (autorelease) {
        Py_XDECREF(object);
      }
    }

    AutoPyObject &operator=(PyObject *other) {
      if (object == other) // Ignore assignments of the same object.
        return *this;

      // Auto release only if we actually have increased its ref count.
      // Always make this auto object auto-releasing after that as we get an object that might
      // be shared by another instance.
      if (autorelease) {
        Py_XDECREF(object);
      }

      object = other;
      autorelease = false;
      Py_XINCREF(object);

      return *this;
    }

    operator bool() {
      return object != 0;
    }

    operator PyObject *() {
      return object;
    }

    PyObject *operator->() {
      return object;
    }
  };

  // Helper class to allow cleaning up the python context after its instance vars are finished.
  class MYSQLGRT_PUBLIC PythonContextHelper {
  private:
    PyThreadState *_main_thread_state;
#ifdef _MSC_VER
    PyConfig _config;
#endif

  protected:
    PythonContextHelper(const std::string &module_path);
    void InitPython();

  public:
    virtual ~PythonContextHelper();
  };

  class MYSQLGRT_PUBLIC PythonContext : private PythonContextHelper, public GRTObserver {
  public:
    PythonContext(const std::string &module_path);
    virtual ~PythonContext();

    static PythonContext *get();
    static PythonContext *get_and_check();

    static PyObject *internal_cobject_from_value(const ValueRef &value);
    static ValueRef value_from_internal_cobject(PyObject *value);

    static void set_wrap_pyobject_func(PyObject *(*func)(PyObject *, PyObject *));
    static void set_unwrap_pyobject_func(PyObject *(*func)(PyObject *, PyObject *));

    void add_module_path(const std::string &path, bool prepend = false);
    PyObject *import_module(const std::string &name);

    PyObject *from_grt(const ValueRef &value);
    grt::ValueRef from_pyobject(PyObject *object);
    grt::ValueRef from_pyobject(PyObject *object, const grt::TypeSpec &expected_type);
    bool pystring_to_string(PyObject *str, std::string &ret_string, bool convert = false);

    int run_file(const std::string &file, bool interactive);
    int run_buffer(const std::string &buffer, std::string *line_buffer = 0);

    int call_grt_function(const std::string &module, const std::string &function, const BaseListRef &args);

    PyObject *eval_string(const std::string &expression);

    PyObject *get_grt_module();

    PyObject *get_global(const std::string &value);
    bool set_global(const std::string &name, PyObject *value);

    int refresh();

    bool set_cwd(const std::string &path);
    std::string get_cwd() const {
      return _cwd;
    }

    std::function<std::string()> stdin_readline_slot;

    static void set_user_interrupted(const grt::user_cancelled &exc);
    static void set_db_access_denied(const grt::db_access_denied &exc);
    static void set_db_login_error(const grt::db_login_error &exc);
    static void set_db_not_conected(const grt::db_not_connected &exc);
    static void set_db_error(const grt::db_error &exc);
    static void set_python_error(const grt::type_error &exc, const std::string &location = "");
    static void set_python_error(const grt::bad_item &exc, const std::string &location = "");
    static void set_python_error(const std::exception &exc, const std::string &location = "");

    static void log_python_error(const char *message);

    PyObject *user_interrupted_error() {
      return _grt_user_interrupt_error;
    }
    PyObject *db_access_denied_error() {
      return _grt_db_access_denied_error;
    }
    PyObject *db_login_error() {
      return _grt_db_login_error;
    }
    PyObject *db_error() {
      return _grt_db_error;
    }
    PyObject *db_not_connected() {
      return _grt_db_not_connected;
    }

    void set_grt_observer_callable(PyObject *obj);
    void setEventlogCallback(PyObject *obj);
    void printResult(std::map<std::string, std::string> &output);

    static PyObject *grt_module_create();
//     static PyObject *grt_modules_module_create();

  protected:
    std::string _cwd;
    AutoPyObject _grt_module;
    AutoPyObject _grt_classes_module;
    AutoPyObject _grt_modules_module;

    AutoPyObject _grt_module_class;
    AutoPyObject _grt_function_class;

    AutoPyObject _grt_list_class;
    AutoPyObject _grt_dict_class;
    AutoPyObject _grt_object_class;
    AutoPyObject _grt_method_class;

    AutoPyObject _grt_user_interrupt_error;
    AutoPyObject _grt_db_access_denied_error;
    AutoPyObject _grt_db_login_error;
    AutoPyObject _grt_db_error;
    AutoPyObject _grt_db_not_connected;

    AutoPyObject _grt_notification_observer;

    AutoPyObject _grtEventLogNotification;

    std::map<std::string, AutoPyObject> _grt_class_wrappers;

  private:
    ValueRef simple_type_from_pyobject(PyObject *object, const grt::SimpleTypeSpec &type);

    void register_grt_module(PyObject *module);
    void register_grt_functions();
    void redirect_python_output();

    void init_grt_module_type();
    void init_grt_list_type();
    void init_grt_dict_type();
    void init_grt_object_type();

    void run_post_init_script();

    virtual void handle_grt_notification(const std::string &name, ObjectRef sender, DictRef info);
    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  };

  class python_error : public std::runtime_error {
  public:
    python_error(const std::string &what) : std::runtime_error(what) {
    }
  };
};
