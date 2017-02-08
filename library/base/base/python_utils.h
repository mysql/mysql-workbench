/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef __MySQLWorkbench__python__
#define __MySQLWorkbench__python__

#if defined(_WIN32)
#include <Python/Python.h>
#else
#include <Python.h>
#endif

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
