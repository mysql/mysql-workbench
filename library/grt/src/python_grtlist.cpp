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
#include "grtpp_util.h"
#include "base/string_utilities.h"

#include "python_grtlist.h"

using namespace grt;
using namespace base;

static int list_init(PyGRTListObject *self, PyObject *args, PyObject *kwds) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (ctx) {
    const char *type = nullptr, *class_name = nullptr;
    PyObject *valueptr = nullptr;
    static const char *kwlist[] = {"type", "classname", "__valueptr__", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zzO", (char **)kwlist, &type, &class_name, &valueptr))
      return -1;

    delete self->list;

    if (valueptr) {
      try {
        grt::ValueRef v = PythonContext::value_from_internal_cobject(valueptr);
        grt::BaseListRef content = grt::BaseListRef::cast_from(v);
        self->list = new grt::BaseListRef(content);
      } catch (grt::type_error &exc) {
        PythonContext::set_python_error(exc);
        return -1;
      } catch (std::exception &exc) {
        PythonContext::set_python_error(exc);
        return -1;
      }
    } else {
      if (!type)
        self->list = new grt::BaseListRef(true);
      else {
        grt::Type content_type = grt::str_to_type(type);
        if (content_type == grt::UnknownType) {
          PyErr_SetString(PyExc_TypeError, "grt type must be grt.integer, double, string, list, dict or object");
          return -1;
        }

        if (class_name) {
          if (!grt::GRT::get()->get_metaclass(class_name)) {
            PyErr_SetString(PyExc_NameError, "invalid GRT class name");
            return -1;
          }
        } else
          class_name = "";
        self->list = new grt::BaseListRef(content_type, class_name);
      }
    }
    return 0;
  }
  return -1;
}

static void list_dealloc(PyGRTListObject *self) {
  delete self->list;

  Py_TYPE(self)->tp_free(self);
}

static Py_ssize_t list_length(PyGRTListObject *self) {
  return self->list->count();
}

static PyObject *list_item(PyGRTListObject *self, Py_ssize_t index) {
  PythonContext *ctx;

  if (!(ctx = PythonContext::get_and_check()))
    return nullptr;

  if (index < 0 || index >= (int)self->list->count()) {
    PyErr_SetString(PyExc_IndexError, "list index out of range");
    return nullptr;
  }

  try {
    return ctx->from_grt(self->list->get(index));
  } catch (grt::bad_item &exc) {
    PyErr_SetString(PyExc_IndexError, exc.what());
    return nullptr;
  } catch (std::exception &exc) {
    PyErr_SetString(PyExc_RuntimeError, exc.what());
    return nullptr;
  }
}

static int list_assign(PyGRTListObject *self, Py_ssize_t index, PyObject *value) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return -1;

  if (index < 0 || index >= (int)self->list->count()) {
    PyErr_SetString(PyExc_IndexError, "list index out of range");
    return -1;
  }

  try {
    if (value == nullptr)
      self->list->remove(index);
    else
      self->list->gset(index, ctx->from_pyobject(value));
    return 0;
  } catch (grt::type_error &exc) {
    PyErr_SetString(PyExc_RuntimeError, exc.what());
  } catch (std::exception &exc) {
    PyErr_SetString(PyExc_RuntimeError, exc.what());
  }

  return -1;
}

static int list_contains(PyGRTListObject *self, PyObject *value) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return -1;

  try {
    if (self->list->get_index(ctx->from_pyobject(value)) != BaseListRef::npos)
      return 1;
  } catch (...) {
  }
  return 0;
}

static PyObject *list_inplace_concat(PyGRTListObject *self, PyObject *other) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return nullptr;

  other = PySequence_Fast(other, "argument to += must be a sequence");
  if (!other)
    return nullptr;

  for (Py_ssize_t i = 0; i < PySequence_Length(other); i++) {
    PyObject *item = PySequence_GetItem(other, i);

    try {
      self->list->ginsert(ctx->from_pyobject(item));
    } catch (grt::type_error &exc) {
      PyErr_SetString(PyExc_TypeError, base::strfmt("type of sequence contents: %s", exc.what()).c_str());
      return nullptr;
    } catch (std::exception &exc) {
      PyErr_SetString(PyExc_RuntimeError, exc.what());
      return nullptr;
    }
  }

  Py_XINCREF(self);
  return (PyObject *)self;
}

static PyObject *list_printable(PyGRTListObject *self) {
  return PyUnicode_FromString(self->list->toString().c_str());
}

static PyObject *list_append(PyGRTListObject *self, PyObject *v) {
  if (!v) {
    PyErr_SetString(PyExc_ValueError, "missing argument");
    return nullptr;
  }
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return nullptr;

  try {
    self->list->ginsert(ctx->from_pyobject(v));
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  }
  return nullptr;
}

static PyObject *list_insert(PyGRTListObject *self, PyObject *args) {
  int i;
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return nullptr;

  PyObject *value;
  if (!PyArg_ParseTuple(args, "iO:insert", &i, &value))
    return nullptr;

  try {
    self->list->ginsert(ctx->from_pyobject(value), i);
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  }
  return nullptr;
}

static PyObject *list_remove(PyGRTListObject *self, PyObject *v) {
  if (!v) {
    PyErr_SetString(PyExc_ValueError, "missing argument");
    return nullptr;
  }
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return nullptr;

  try {
    self->list->gremove_value(ctx->from_pyobject(v));
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  }
  return nullptr;
}

static PyObject *list_remove_all(PyGRTListObject *self) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return nullptr;

  try {
    self->list->remove_all();
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  }
  return nullptr;
}

static PyObject *list_reorder(PyGRTListObject *self, PyObject *args) {
  int oldi, newi;
  if (!PyArg_ParseTuple(args, "ii:reorder", &oldi, &newi))
    return nullptr;

  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return nullptr;

  try {
    self->list->reorder(oldi, newi);
    Py_RETURN_NONE;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return nullptr;
  }
  return nullptr;
}

static PyObject *list_get_contenttype(PyGRTListObject *self, void *closure) {
  return Py_BuildValue("(ss)", type_to_str(self->list->content_type()).c_str(),
                       self->list->content_class_name().c_str());
}

PyDoc_STRVAR(PyGRTListDoc,
             "List([grttype, grtclass]) -> GRT List\n\
             \n\
             Creates a new instance of a GRT list object. If a grttype argument is given,\n\
             the list will be typed and accept values of that type only. For grt.OBJECT lists,\n\
             you can also pass a GRT class name. grttype may be one of grt.INT, grt.DOUBLE,\n\
             grt.STRING or grt.OBJECT (ie. simple types or objects).");

PyDoc_STRVAR(append_doc, "L.append(value) -- append object to end");
PyDoc_STRVAR(insert_doc, "L.insert(index, value) -- insert object at index");
PyDoc_STRVAR(reorder_doc, "L.reorder(oindex, nindex) -- reorder object at index");
PyDoc_STRVAR(remove_doc, "L.remove(value) -- remove first occurrence of object");
PyDoc_STRVAR(remove_all_doc, "L.remove_all() -- remove all elements from the list");
PyDoc_STRVAR(extend_doc, "L.extend(list) -- add all elements from the list");

#if !defined(_MSC_VER) && !defined(__APPLE__)

#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

#endif
static PyMethodDef PyGRTListMethods[] = {
  //{"__getitem__", (PyCFunction)list_subscript, METH_O|METH_COEXIST, getitem_doc},
  {"append", (PyCFunction)list_append, METH_O, append_doc},
  {"extend", (PyCFunction)list_inplace_concat, METH_O, extend_doc},
  {"insert", (PyCFunction)list_insert, METH_VARARGS, insert_doc},
  {"reorder", (PyCFunction)list_reorder, METH_VARARGS, reorder_doc},
  {"remove", (PyCFunction)list_remove, METH_O, remove_doc},
  {"remove_all", (PyCFunction)list_remove_all, METH_NOARGS, remove_all_doc},
  {nullptr, nullptr, 0, nullptr}};
#if !defined(_MSC_VER) && !defined(__APPLE__)
#pragma GCC diagnostic pop
#endif

static PyGetSetDef PyGRTListGetSetters[] = {
  {(char *)"__contenttype__", (getter)list_get_contenttype, nullptr, (char *)"(content type, content object class|None)",
   nullptr},
  {nullptr, 0, nullptr, nullptr, nullptr},
};

static PySequenceMethods PyGRTListObject_as_sequence = {
  (lenfunc)list_length,         // lenfunc sq_length;
  0,                            // binaryfunc sq_concat;
  0,                            // ssizeargfunc sq_repeat;
  (ssizeargfunc)list_item,      // ssizeargfunc sq_item;
  0,                            //(ssizessizeargfunc)list_slice, // ssizessizeargfunc sq_slice;
  (ssizeobjargproc)list_assign, // ssizeobjargproc sq_ass_item;
  0,                            ///(ssizessizeobjargproc)list_assign_slice,// ssizessizeobjargproc sq_ass_slice;
  (objobjproc)list_contains,    // objobjproc sq_contains;
  /* Added in release 2.0 */
  (binaryfunc)list_inplace_concat, // binaryfunc sq_inplace_concat;
  0                                // ssizeargfunc sq_inplace_repeat;
};

static PyTypeObject PyGRTListObjectType = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0) // PyObject_VAR_HEAD
  "grt.List",   //  tp_name
  sizeof(PyGRTListObject),  //  tp_basicsize
  0, /* tp_itemsize For allocation */
  
  /* Methods to implement standard operations */

  (destructor)list_dealloc,   //  tp_dealloc
  0,  //  tp_vectorcall_offset
  0,  //  tp_getattr
  0,  //  tp_setattr
  0,  //  tp_as_async
  0,  //  tp_repr

  /* Method suites for standard classes */

  0,  //  tp_as_number
  &PyGRTListObject_as_sequence, //  tp_as_sequence
  0,  //  tp_as_mapping

  /* More standard operations (here for binary compatibility) */

  0,  //  tp_hash
  0,  //  tp_call
  (reprfunc)list_printable, //  tp_str

  PyObject_GenericGetAttr,  //  tp_getattro
  0,  //  tp_setattro

  /* Functions to access object as input/output buffer */
  0,  //  tp_as_buffer

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT, //  tp_flags

  PyGRTListDoc, /* tp_doc Documentation string */

  /* Assigned meaning in release 2.0 */
  /* call function for all accessible objects */
  0,  //  tp_traverse

  /* delete references to contained objects */
  0,  //  tp_clear

  /* Assigned meaning in release 2.1 */
  /* rich comparisons */
  0,  //  tp_richcompare

  /* weak reference enabler */
  0,  //  tp_weaklistoffset

  /* Iterators */
  0,  //  tp_iter
  0,  //  tp_iternext

  /* Attribute descriptor and subclassing stuff */
  PyGRTListMethods, //  tp_methods
  0,  //  tp_members
  PyGRTListGetSetters,  //  tp_getset
  0,  //  tp_base
  0,  //  tp_dict
  0,  //  tp_descr_get
  0,  //  tp_descr_set
  0,  //  tp_dictoffset
  (initproc)list_init,  //  tp_init
  PyType_GenericAlloc,  //  tp_alloc
  PyType_GenericNew,    //  tp_new
  0, /* tp_free Low-level free-memory routine */
  0, /* tp_is_gc For PyObject_IS_GC */
  0,  //  tp_bases
  0, /* tp_mro method resolution order */
  0,  //  tp_cache
  0,  //  tp_subclasses
  0,  //  tp_weaklist
  0,  //  tp_del

  /* Type attribute cache version tag. Added in version 2.6 */
  0,  //  tp_version_tag
  0, //  tp_finalize
#if PY_VERSION_HEX >= 0x03090000
  nullptr   //  tp_vectorcall
#elif PY_VERSION_HEX >= 0x03080000
  nullptr,  //  tp_vectorcall
  nullptr   //  tp_print
#endif
};

void grt::PythonContext::init_grt_list_type() {
  PyGRTListObjectType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyGRTListObjectType) < 0) {
    throw std::runtime_error("Could not initialize GRT List type in python");
  }

  Py_XINCREF(&PyGRTListObjectType);
  PyModule_AddObject(get_grt_module(), "List", (PyObject *)&PyGRTListObjectType);

  _grt_list_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "List");
}
