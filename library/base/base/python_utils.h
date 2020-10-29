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

#ifndef __MySQLWorkbench__python__
#define __MySQLWorkbench__python__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Undefine some python macros which conflict with C++ functions.
#undef isspace
#undef isupper
#undef islower
#undef isalpha
#undef isalnum
#undef toupper
#undef tolower

#include "base/common.h"

#include <string>

namespace base {
  std::string BASELIBRARY_PUBLIC_FUNC format_python_exception(std::string &summary);
};

// Must be placed when Python code will be called
struct WillEnterPython {
  PyGILState_STATE state;
  bool locked;

  WillEnterPython() : state(PyGILState_Ensure()), locked(true) {
  }

  ~WillEnterPython() {
    if (locked)
      PyGILState_Release(state);
  }

  void release() {
    if (locked)
      PyGILState_Release(state);
    locked = false;
  }
};

// Must be placed when non-python code will be called from a Python handler/callback
struct WillLeavePython {
  PyThreadState *save;

  WillLeavePython() : save(PyEval_SaveThread()) {
  }

  ~WillLeavePython() {
    PyEval_RestoreThread(save);
  }
};

#endif /* defined(__MySQLWorkbench__python__) */
