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

#include "python_context.h"
#include "base/string_utilities.h"

#include "grtpp_util.h"

#include "python_grtdict.h"

using namespace grt;
using namespace base;

static int dict_init(PyGRTDictObject *self, PyObject *args, PyObject *kwds) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (ctx) {
    const char *type = NULL, *class_name = NULL;
    PyObject *valueptr = NULL;
    static const char *kwdict[] = {"type", "classname", "__valueptr__", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zzO", (char **)kwdict, &type, &class_name, &valueptr))
      return -1;

    delete self->dict;

    if (valueptr) {
      try {
        grt::ValueRef v = PythonContext::value_from_internal_cobject(valueptr);
        grt::DictRef content = grt::DictRef::cast_from(v);
        self->dict = new grt::DictRef(content);
      } catch (grt::type_error &exc) {
        PythonContext::set_python_error(exc);
        return -1;
      } catch (std::exception &exc) {
        PythonContext::set_python_error(exc);
        return -1;
      }
    } else {
      if (!type)
        self->dict = new grt::DictRef(true);
      else {
        grt::Type content_type = grt::str_to_type(type);
        if (content_type == grt::UnknownType) {
          PyErr_SetString(PyExc_TypeError, "grt type must be grt.integer, double, string, dict, dict or object");
          return -1;
        }

        if (class_name) {
          if (!grt::GRT::get()->get_metaclass(class_name)) {
            PyErr_SetString(PyExc_NameError, "invalid GRT class name");
            return -1;
          }
        }

        self->dict = new grt::DictRef(content_type, class_name);
      }
    }
    return 0;
  }
  return -1;
}

static void dict_dealloc(PyGRTDictObject *self) {
  delete self->dict;

  Py_TYPE(self)->tp_free(self);
}

static Py_ssize_t dict_length(PyGRTDictObject *self) {
  return self->dict->count();
}

static PyObject *dict_getattro(PyGRTDictObject *self, PyObject *attr_name) {
  AutoPyObject tmp;
  if (PyUnicode_Check(attr_name))
    attr_name = tmp = PyUnicode_AsUTF8String(attr_name);

  if (PyUnicode_Check(attr_name)) {
    const char *attrname = PyUnicode_AsUTF8(attr_name);

    PyObject *object;
    if ((object = PyObject_GenericGetAttr((PyObject *)self, attr_name)))
      return object;
    PyErr_Clear();

    if (strcmp(attrname, "__members__") == 0) {
      PyObject *members = Py_BuildValue("[s]", "__contenttype__");

      for (grt::DictRef::const_iterator iter = self->dict->begin(); iter != self->dict->end(); ++iter) {
        PyObject *tmp = PyUnicode_FromString(iter->first.c_str());
        PyList_Append(members, tmp);
        Py_DECREF(tmp);
      }
      return members;
    } else if (strcmp(attrname, "__methods__") == 0) {
      PyObject *methods = Py_BuildValue("[sssss]", "keys", "items", "values", "has_key", "update", "setdefault");
      return methods;
    } else {
      if (self->dict->has_key(attrname)) {
        PythonContext *ctx = PythonContext::get_and_check();
        if (!ctx)
          return NULL;

        return ctx->from_grt(self->dict->get(attrname));
      } else
        PyErr_SetString(PyExc_AttributeError, base::strfmt("unknown attribute '%s'", attrname).c_str());
    }
  }
  PyErr_SetString(PyExc_KeyError, "grt.Dict key must be a string");
  return NULL;
}

static PyObject *dict_subscript(PyGRTDictObject *self, PyObject *key) {
  AutoPyObject tmp;
  if (PyUnicode_Check(key))
    key = tmp = PyUnicode_AsUTF8String(key);

  if (!PyUnicode_Check(key)) {
    PyErr_SetString(PyExc_KeyError, "grt.Dict key must be a string");
    return NULL;
  }
  const char *k = PyUnicode_AsUTF8(key);
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  try {
    return ctx->from_grt(self->dict->get(k));
  } catch (grt::bad_item &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

static int dict_ass_subscript(PyGRTDictObject *self, PyObject *key, PyObject *value) {
  AutoPyObject tmp;
  if (PyUnicode_Check(key))
    key = tmp = PyUnicode_AsUTF8String(key);

  if (!PyUnicode_Check(key)) {
    PyErr_SetString(PyExc_KeyError, "grt.Dict key must be a string");
    return -1;
  }
  const char *k = PyUnicode_AsUTF8(key);

  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return -1;

  try {
    if (value == NULL)
      self->dict->remove(k);
    else if (value == Py_None)
      self->dict->set(k, grt::ValueRef());
    else {
      grt::ValueRef v = ctx->from_pyobject(value);
      if (!v.is_valid()) {
        PyErr_SetString(PyExc_ValueError, "grt.Dict may only be assigned other GRT or string/numeric values");
        return -1;
      }
      self->dict->set(k, v);
    }
    return 0;
  } catch (grt::bad_item &exc) {
    PythonContext::set_python_error(exc);
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
  }
  return -1;
}

static PyObject *dict_keys(PyGRTDictObject *self, PyObject *args) {
  if (args) {
    PyErr_SetString(PyExc_ValueError, "method takes no arguments");
    return NULL;
  }
  PyObject *list = PyList_New(self->dict->count());

  Py_ssize_t i = 0;
  for (grt::DictRef::const_iterator iter = self->dict->begin(); iter != self->dict->end(); ++iter)
    PyList_SetItem(list, i++, PyUnicode_FromString(iter->first.c_str()));

  return list;
}

static PyObject *dict_items(PyGRTDictObject *self, PyObject *args) {
  if (args) {
    PyErr_SetString(PyExc_ValueError, "method takes no arguments");
    return NULL;
  }
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;
  PyObject *list = PyList_New(self->dict->count());

  Py_ssize_t i = 0;
  for (grt::DictRef::const_iterator iter = self->dict->begin(); iter != self->dict->end(); ++iter) {
    PyObject *tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, PyUnicode_FromString(iter->first.c_str()));
    PyTuple_SetItem(tuple, 1, ctx->from_grt(iter->second));
    PyList_SetItem(list, i++, tuple);
  }
  return list;
}

static PyObject *dict_values(PyGRTDictObject *self, PyObject *args) {
  if (args) {
    PyErr_SetString(PyExc_ValueError, "method takes no arguments");
    return NULL;
  }
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;
  PyObject *list = PyList_New(self->dict->count());

  Py_ssize_t i = 0;
  for (grt::DictRef::const_iterator iter = self->dict->begin(); iter != self->dict->end(); ++iter)
    PyList_SetItem(list, i++, ctx->from_grt(iter->second));

  return list;
}

static PyObject *dict_has_key(PyGRTDictObject *self, PyObject *arg) {
  if (!arg) {
    PyErr_SetString(PyExc_ValueError, "missing required argument");
    return NULL;
  }

  const char *key_to_find = PyUnicode_AsUTF8(arg);
  bool found = false;

  if (key_to_find)
    found = self->dict->has_key(key_to_find);

  return PyBool_FromLong(found);
}

static PyObject *dict_update(PyGRTDictObject *self, PyObject *arg) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  if (!arg) {
    PyErr_SetString(PyExc_ValueError, "dict argument required for update()");
    return NULL;
  }

  TypeSpec type;
  type.base.type = DictType;
  type.content.type = AnyType;
  grt::DictRef value;
  try {
    value = grt::DictRef::cast_from(ctx->from_pyobject(arg, type));
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc, "invalid argument");
    return NULL;
  }

  if (value.is_valid()) {
    merge_contents(*self->dict, value, true);
  } else {
    PyErr_SetString(PyExc_ValueError, "invalid argument for update()");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *dict_get(PyGRTDictObject *self, PyObject *arg) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;
  PyObject *def = NULL;
  char *key;

  if (!PyArg_ParseTuple(arg, "s|O", &key, &def))
    return NULL;

  if (key) {
    if (self->dict->has_key(key))
      return ctx->from_grt(self->dict->get(key));
    else {
      if (def) {
        Py_INCREF(def);
        return def;
      } else {
        PyErr_SetString(PyExc_KeyError, base::strfmt("invalid key '%s'", key).c_str());
      }
    }
  }

  Py_RETURN_NONE;
}

static PyObject *dict_setdefault(PyGRTDictObject *self, PyObject *arg) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;
  PyObject *def = Py_None;
  char *key;

  if (!PyArg_ParseTuple(arg, "s|O", &key, &def))
    return NULL;

  if (key) {
    if (self->dict->has_key(key))
      return ctx->from_grt(self->dict->get(key));
    else {
      if (def != Py_None)
        Py_INCREF(def);
      try {
        self->dict->set(key, ctx->from_pyobject(def));
      } catch (grt::bad_item &exc) {
        PythonContext::set_python_error(exc);
      } catch (std::exception &exc) {
        PythonContext::set_python_error(exc);
      }
      return ctx->from_grt(self->dict->get(key));
    }
  }
  Py_RETURN_NONE;
}

static PyObject *dict_printable(PyGRTDictObject *self) {
  return PyUnicode_FromString(self->dict->toString().c_str());
}

static PyObject *dict_get_contenttype(PyGRTDictObject *self, void *closure) {
  return Py_BuildValue("(ss)", type_to_str(self->dict->content_type()).c_str(),
                       self->dict->content_class_name().c_str());
}

PyDoc_STRVAR(PyGRTDictDoc,
             "Dict([grttype, grtclass]) -> GRT Dict\n\
             \n\
             Creates a new instance of a GRT dict object. If a grttype argument is given,\n\
             the dict will be typed and accept values of that type only. For grt.OBJECT dicts,\n\
             you can also pass a GRT class name. grttype may be one of grt.INT, grt.DOUBLE,\n\
             grt.STRING or grt.OBJECT (ie. simple types or objects).");

static PyMethodDef PyGRTDictMethods[] = {
  //{"__getitem__", (PyCFunction)dict_subscript, METH_O|METH_COEXIST, getitem_doc},
  {"keys", (PyCFunction)dict_keys, METH_NOARGS, NULL},
  {"items", (PyCFunction)dict_items, METH_NOARGS, NULL},
  {"values", (PyCFunction)dict_values, METH_NOARGS, NULL},
  {"has_key", (PyCFunction)dict_has_key, METH_NOARGS, NULL},
  {"update", (PyCFunction)dict_update, METH_VARARGS, NULL},
  {"get", (PyCFunction)dict_get, METH_VARARGS, NULL},
  {"setdefault", (PyCFunction)dict_setdefault, METH_VARARGS, NULL},
  {NULL, NULL, 0, NULL}};

static PyGetSetDef PyGRTDictGetSetters[] = {{(char *)"contenttype", (getter)dict_get_contenttype, NULL,
                                             (char *)"(content type, content object class|None)", NULL},
                                            {NULL, 0, NULL, NULL, NULL}};

static PyMappingMethods PyGRTDictObject_as_mapping = {
  (lenfunc)dict_length,             // inquiry mp_length;
  (binaryfunc)dict_subscript,       // binaryfunc mp_subscript;
  (objobjargproc)dict_ass_subscript // objobjargproc mp_ass_subscript;
};

// static PyTypeObject PyGRTDictObjectType = {
//   PyVarObject_HEAD_INIT(&PyType_Type, 0) // PyObject_VAR_HEAD
//   "grt.Dict",   //  tp_name
//   sizeof(PyGRTDictObject),  //  tp_basicsize
//   0, /* tp_itemsize For allocation */
//   
//   /* Methods to implement standard operations */
// 
//   (destructor)dict_dealloc, //  tp_dealloc destructor tp_dealloc;
//   0,  //  tp_print
//   0,  //  tp_getattr
//   0,  //  tp_setattr
//   0,  //  tp_as_async
//   0,  //  tp_repr
// 
//   /* Method suites for standard classes */
// 
//   0,  //  tp_as_number
//   0,  //  tp_as_sequence
//   &PyGRTDictObject_as_mapping,    //  tp_as_mapping
// 
//   /* More standard operations (here for binary compatibility) */
// 
//   0,  //  tp_hash
//   0,  //  tp_call
//   (reprfunc)dict_printable,   //  tp_str
// 
//   (getattrofunc)dict_getattro,  //  tp_getattro
//   0,  //  tp_setattro
// 
//   /* Functions to access object as input/output buffer */
//   0,  //  tp_as_buffer
// 
//   /* Flags to define presence of optional/expanded features */
//   Py_TPFLAGS_DEFAULT, //  tp_flags
// 
//   PyGRTDictDoc, /* tp_doc Documentation string */
// 
//   /* Assigned meaning in release 2.0 */
//   /* call function for all accessible objects */
//   0,  //  tp_traverse
// 
//   /* delete references to contained objects */
//   0,  //  tp_clear
// 
//   /* Assigned meaning in release 2.1 */
//   /* rich comparisons */
//   0,  //  tp_richcompare
// 
//   /* weak reference enabler */
//   0,  //  tp_weaklistoffset
// 
//   /* Iterators */
//   0,    //  tp_iter
//   0,    //  tp_iternext
// 
//   /* Attribute descriptor and subclassing stuff */
//   PyGRTDictMethods, //  tp_methods
//   0,  //  tp_members
//   PyGRTDictGetSetters,  //  tp_getset
//   0,  //  tp_base
//   0,  //  tp_dict
//   0,  //  tp_descr_get
//   0,  //  tp_descr_set
//   0,  //  tp_dictoffset
//   (initproc)dict_init,  //  tp_init
//   PyType_GenericAlloc,  //  tp_alloc
//   PyType_GenericNew,    //  tp_new
//   0, /* tp_free Low-level free-memory routine */
//   0, /* tp_is_gc For PyObject_IS_GC */
//   0,  //  tp_bases
//   0, /* tp_mro method resolution order */
//   0,  //  tp_cache
//   0,  //  tp_subclasses
//   0,  //  tp_weaklist
//   0,  //  tp_del
// 
//   /* Type attribute cache version tag. Added in version 2.6 */
//   0,  //  tp_version_tag
// 
//   0, //  tp_finalize
//   nullptr,  //  tp_vectorcall
//   nullptr   //  tp_print
// };
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <iostream>
static PyTypeObject PyGRTDictObjectType = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0) // PyObject_VAR_HEAD
  .tp_name = "grt.Dict",
  .tp_basicsize = sizeof(PyGRTDictObject),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)dict_dealloc, //  destructor tp_dealloc;
  .tp_vectorcall_offset = 0,
  .tp_as_mapping = &PyGRTDictObject_as_mapping,
  .tp_str = (reprfunc)dict_printable,
  .tp_getattro = (getattrofunc)dict_getattro,
  .tp_flags = Py_TPFLAGS_DEFAULT,  
  .tp_doc = PyGRTDictDoc,
  .tp_methods = PyGRTDictMethods,
  .tp_getset = PyGRTDictGetSetters,
  .tp_init = (initproc)dict_init,
  .tp_alloc = PyType_GenericAlloc,
  .tp_new = PyType_GenericNew
};


void grt::PythonContext::init_grt_dict_type() {
  if (PyType_Ready(&PyGRTDictObjectType) < 0) {
    PyErr_Print();
    throw std::runtime_error("Could not initialize GRT Dict type in python");
  }

  Py_INCREF(&PyGRTDictObjectType);
  PyModule_AddObject(get_grt_module(), "Dict", (PyObject *)&PyGRTDictObjectType);

  _grt_dict_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "Dict");
}
