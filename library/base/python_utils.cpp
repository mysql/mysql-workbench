/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#define PY_SSIZE_T_CLEAN
#if defined(_MSC_VER)
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include <frameobject.h>
#include "base/python_utils.h"
#include "base/string_utilities.h"

std::string format_python_traceback(PyObject *tb) {
  PyTracebackObject *trace = (PyTracebackObject *)tb;
  std::string stack;

  stack = "Traceback:\n";
  while (trace && trace->tb_frame) {
    PyFrameObject *frame = (PyFrameObject *)trace->tb_frame;
    PyCodeObject *code = PyFrame_GetCode(frame);
    if(code) {
      stack += base::strfmt("  File \"%s\", line %i, in %s\n", PyUnicode_AsUTF8(code->co_filename),
                            trace->tb_lineno, PyUnicode_AsUTF8(code->co_name));
      PyObject *error = PyErr_ProgramText(PyUnicode_AsUTF8(code->co_filename), trace->tb_lineno);
      if (error) {
        stack += base::strfmt("    %s", PyUnicode_AsUTF8(error));
        Py_DECREF(error);
      }
      Py_DECREF(code);
    }
    trace = trace->tb_next;
  }
  return stack;
}

std::string base::format_python_exception(std::string &summary) {
  std::string reason, stack;
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

  if (tb)
    stack = format_python_traceback(tb);
  else
    stack = "No stack information. ";

  PyErr_Restore(exc, val, tb);

  summary = reason;

  return stack + reason + "\n";
}
