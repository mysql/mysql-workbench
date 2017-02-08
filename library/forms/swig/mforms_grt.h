/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

// GRT support

#include "grt.h"
#include "python_context.h"
#include "grts/structs.ui.h"
#include "mforms/dockingpoint.h"

#include "objimpl/wrapper/mforms_ObjectReference_impl.h"

//--------------------------------------------------------------------------------------------------

// The throw clause gives a warning in VS (as it is not support, except to indicate this is not a nothrow function).
PyObject *fromgrt(PyObject *object) // throw (std::runtime_error, std::invalid_argument, std::logic_error)
{
  grt::PythonContext *ctx = grt::PythonContext::get();
  if (!ctx)
    throw std::runtime_error("Internal error, could not get internal Python context");
  grt::ValueRef value = ctx->from_pyobject(object);
  if (!value.is_valid())
    throw std::invalid_argument("Invalid None argument to fromgrt()");

  if (mforms_ObjectReferenceRef::can_wrap(value)) {
    mforms_ObjectReferenceRef oref = mforms_ObjectReferenceRef::cast_from(value);
    swig_type_info *type = SWIG_TypeQuery(std::string("mforms::" + *oref->type() + " *").c_str());
    if (!type)
      throw std::logic_error(std::string("Internal error converting mforms.ObjectReference to a Python object: ") +
                             *oref->type());
    return SWIG_NewPointerObj(mforms_from_grt(oref), type, 0);
  }

  throw std::invalid_argument("Invalid argument to fromgrt(), not a mforms_ObjectReference instance");
}

//--------------------------------------------------------------------------------------------------

PyObject *togrt(mforms::Object *object,
                const std::string &mforms_type_name) // throw (std::runtime_error, std::invalid_argument)
{
  if (object != NULL) {
    grt::PythonContext *ctx = grt::PythonContext::get();
    if (!ctx)
      throw std::runtime_error("Internal error, could not get internal Python context");

    if (!SWIG_TypeQuery(std::string("mforms::" + mforms_type_name + " *").c_str()))
      throw std::invalid_argument(mforms_type_name + " is not a valid mforms class name");

    return ctx->from_grt(mforms_to_grt(object, mforms_type_name));
  } else
    Py_RETURN_NONE;
}

//--------------------------------------------------------------------------------------------------

struct signal_connection_wrapper {
#ifndef SWIG
  boost::signals2::connection connection;

  signal_connection_wrapper() {
  }
  signal_connection_wrapper(const signal_connection_wrapper &o) : connection(o.connection) {
  }
  signal_connection_wrapper(const boost::signals2::connection &c) : connection(c) {
  }

  signal_connection_wrapper &operator=(const signal_connection_wrapper &o) {
    connection = o.connection;
    return *this;
  }
#endif
  void disconnect() {
    connection.disconnect();
  }
};

//--------------------------------------------------------------------------------------------------
