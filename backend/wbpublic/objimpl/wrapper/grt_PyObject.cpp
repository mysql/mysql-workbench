/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <python_context.h>
#include <grts/structs.ui.h>

#include <grtpp_util.h>

#include "grt_PyObject_impl.h"

//================================================================================
// grt_PyObject

grt::IntegerRef grt_PyObject::isEqualTo(const grt::Ref<grt_PyObject> &other) {
  if (other.is_valid())
    return grt::IntegerRef(0);
  return grt::IntegerRef(0);
}

grt::AutoPyObject pyobject_from_grt(grt_PyObjectRef object) {
  if (!object.is_valid())
    return 0;
  return *object->get_data();
}

static void release_object(grt::AutoPyObject *object) {
  delete object;
}

grt_PyObjectRef pyobject_to_grt(grt::AutoPyObject object) {
  if (object) {
    grt_PyObjectRef ref(grt::Initialized);
    ref->set_data(new grt::AutoPyObject(object), release_object);
    return ref;
  }
  return grt_PyObjectRef(grt::Initialized);
}

grt_PyObjectRef pyobject_to_grt(PyObject *object) {
  return pyobject_to_grt(grt::AutoPyObject(object));
}

static PyObject *wrap_pyobject(PyObject *self, PyObject *args) {
  grt::PythonContext *ctx;
  std::string text;

  if (!(ctx = grt::PythonContext::get_and_check()))
    return NULL;

  // wrap an arbitrary Python object into a GRT object
  PyObject *o;
  if (!PyArg_ParseTuple(args, "O", &o))
    return NULL;
  return ctx->from_grt(pyobject_to_grt(o));
}

static PyObject *unwrap_pyobject(PyObject *self, PyObject *args) {
  grt::PythonContext *ctx;
  std::string text;

  if (!(ctx = grt::PythonContext::get_and_check()))
    return NULL;

  // unwrap a grt_PyObject into a python object
  PyObject *o;
  if (!PyArg_ParseTuple(args, "O", &o))
    return NULL;

  if (o == NULL || o == Py_None)
    Py_RETURN_NONE;

  grt::ValueRef value(ctx->from_pyobject(o));
  if (value.is_valid()) {
    if (grt_PyObjectRef::can_wrap(value)) {
      grt_PyObjectRef pobj(grt_PyObjectRef::cast_from(value));
      PyObject *tmp = pyobject_from_grt(pobj);
      Py_INCREF(tmp);
      return tmp;
    } else {
      PyErr_SetString(PyExc_TypeError, "Argument to fromgrt() must be of class grt_PyObject");
      return NULL;
    }
  } else
    Py_RETURN_NONE;
}

void pyobject_initialize() {
  grt::PythonContext::set_wrap_pyobject_func(wrap_pyobject);
  grt::PythonContext::set_unwrap_pyobject_func(unwrap_pyobject);
}
