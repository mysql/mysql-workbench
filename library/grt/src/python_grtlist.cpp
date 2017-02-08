/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "python_context.h"
#include "grtpp_util.h"
#include "base/string_utilities.h"

#include "python_grtlist.h"

using namespace grt;
using namespace base;

static int list_init(PyGRTListObject *self, PyObject *args, PyObject *kwds) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (ctx) {
    const char *type = NULL, *class_name = NULL;
    PyObject *valueptr = NULL;
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

  self->ob_type->tp_free(self);
}

/*
static int list_compare(PyGRTListObject *self, PyGRTListObject *other)
{

}*/

static Py_ssize_t list_length(PyGRTListObject *self) {
  return self->list->count();
}

static PyObject *list_item(PyGRTListObject *self, Py_ssize_t index) {
  PythonContext *ctx;

  if (!(ctx = PythonContext::get_and_check()))
    return NULL;

  if (index < 0 || index >= (int)self->list->count()) {
    PyErr_SetString(PyExc_IndexError, "list index out of range");
    return NULL;
  }

  try {
    return ctx->from_grt(self->list->get(index));
  } catch (grt::bad_item &exc) {
    PyErr_SetString(PyExc_IndexError, exc.what());
    return NULL;
  } catch (std::exception &exc) {
    PyErr_SetString(PyExc_RuntimeError, exc.what());
    return NULL;
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
    if (value == NULL)
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
    return NULL;

  other = PySequence_Fast(other, "argument to += must be a sequence");
  if (!other)
    return NULL;

  for (Py_ssize_t i = 0; i < PySequence_Length(other); i++) {
    PyObject *item = PySequence_GetItem(other, i);

    try {
      self->list->ginsert(ctx->from_pyobject(item));
    } catch (grt::type_error &exc) {
      PyErr_SetString(PyExc_TypeError, base::strfmt("type of sequence contents: %s", exc.what()).c_str());
      return NULL;
    } catch (std::exception &exc) {
      PyErr_SetString(PyExc_RuntimeError, exc.what());
      return NULL;
    }
  }

  Py_INCREF(self);
  return (PyObject *)self;
}

static PyObject *list_printable(PyGRTListObject *self) {
  return PyString_FromString(self->list->toString().c_str());
}

static PyObject *list_append(PyGRTListObject *self, PyObject *v) {
  if (!v) {
    PyErr_SetString(PyExc_ValueError, "missing argument");
    return NULL;
  }
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  try {
    self->list->ginsert(ctx->from_pyobject(v));
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

static PyObject *list_insert(PyGRTListObject *self, PyObject *args) {
  int i;
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  PyObject *value;
  if (!PyArg_ParseTuple(args, "iO:insert", &i, &value))
    return NULL;

  try {
    self->list->ginsert(ctx->from_pyobject(value), i);
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

static PyObject *list_remove(PyGRTListObject *self, PyObject *v) {
  if (!v) {
    PyErr_SetString(PyExc_ValueError, "missing argument");
    return NULL;
  }
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  try {
    self->list->gremove_value(ctx->from_pyobject(v));
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

static PyObject *list_remove_all(PyGRTListObject *self) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  try {
    self->list->remove_all();
    Py_RETURN_NONE;
  } catch (grt::type_error &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

static PyObject *list_reorder(PyGRTListObject *self, PyObject *args) {
  int oldi, newi;
  if (!PyArg_ParseTuple(args, "ii:reorder", &oldi, &newi))
    return NULL;

  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  try {
    self->list->reorder(oldi, newi);
    Py_RETURN_NONE;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  }
  return NULL;
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

static PyMethodDef PyGRTListMethods[] = {
  //{"__getitem__", (PyCFunction)list_subscript, METH_O|METH_COEXIST, getitem_doc},
  {"append", (PyCFunction)list_append, METH_O, append_doc},
  {"extend", (PyCFunction)list_inplace_concat, METH_O, extend_doc},
  {"insert", (PyCFunction)list_insert, METH_VARARGS, insert_doc},
  {"reorder", (PyCFunction)list_reorder, METH_VARARGS, reorder_doc},
  {"remove", (PyCFunction)list_remove, METH_O, remove_doc},
  {"remove_all", (PyCFunction)list_remove_all, METH_NOARGS, remove_all_doc},
  {NULL, NULL, 0, NULL}};

static PyGetSetDef PyGRTListGetSetters[] = {
  {(char *)"__contenttype__", (getter)list_get_contenttype, NULL, (char *)"(content type, content object class|None)",
   NULL},
  {NULL, 0, NULL, NULL, NULL},
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
  PyObject_HEAD_INIT(&PyType_Type) // PyObject_VAR_HEAD
  0,
  "grt.List",                 // char *tp_name; /* For printing, in format "<module>.<name>" */
  sizeof(PyGRTListObject), 0, // int tp_basicsize, tp_itemsize; /* For allocation */

  /* Methods to implement standard operations */

  (destructor)list_dealloc, //  destructor tp_dealloc;
  0,                        //  printfunc tp_print;
  0,                        //  getattrfunc tp_getattr;
  0,                        //  setattrfunc tp_setattr;
  0,                        //(cmpfunc)list_compare, //  cmpfunc tp_compare;
  0,                        //(reprfunc)list_repr,//  reprfunc tp_repr;

  /* Method suites for standard classes */

  0,                            //  PyNumberMethods *tp_as_number;
  &PyGRTListObject_as_sequence, //  PySequenceMethods *tp_as_sequence;
  0,                            //  PyMappingMethods *tp_as_mapping;

  /* More standard operations (here for binary compatibility) */

  0,                        //  hashfunc tp_hash;
  0,                        //  ternaryfunc tp_call;
  (reprfunc)list_printable, //  reprfunc tp_str;
  PyObject_GenericGetAttr,  //  getattrofunc tp_getattro;
  0,                        //  setattrofunc tp_setattro;

  /* Functions to access object as input/output buffer */
  0, //  PyBufferProcs *tp_as_buffer;

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT, //  long tp_flags;

  PyGRTListDoc, //  char *tp_doc; /* Documentation string */

  /* Assigned meaning in release 2.0 */
  /* call function for all accessible objects */
  0, //  traverseproc tp_traverse;

  /* delete references to contained objects */
  0, //  inquiry tp_clear;

  /* Assigned meaning in release 2.1 */
  /* rich comparisons */
  0, //  richcmpfunc tp_richcompare;

  /* weak reference enabler */
  0, //  long tp_weaklistoffset;

  /* Added in release 2.2 */
  /* Iterators */
  0, //  getiterfunc tp_iter;
  0, //  iternextfunc tp_iternext;

  /* Attribute descriptor and subclassing stuff */
  PyGRTListMethods,    //  struct PyMethodDef *tp_methods;
  0,                   //  struct PyMemberDef *tp_members;
  PyGRTListGetSetters, //  struct PyGetSetDef *tp_getset;
  0,                   //  struct _typeobject *tp_base;
  0,                   //  PyObject *tp_dict;
  0,                   //  descrgetfunc tp_descr_get;
  0,                   //  descrsetfunc tp_descr_set;
  0,                   //  long tp_dictoffset;
  (initproc)list_init, //  initproc tp_init;
  PyType_GenericAlloc, //  allocfunc tp_alloc;
  PyType_GenericNew,   //  newfunc tp_new;
  0,                   //  freefunc tp_free; /* Low-level free-memory routine */
  0,                   //  inquiry tp_is_gc; /* For PyObject_IS_GC */
  0,                   //  PyObject *tp_bases;
  0,                   //  PyObject *tp_mro; /* method resolution order */
  0,                   //  PyObject *tp_cache;
  0,                   //  PyObject *tp_subclasses;
  0,                   //  PyObject *tp_weaklist;
  0,                   // tp_del
#if (PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION > 5)
  0 // tp_version_tag
#endif
};

void grt::PythonContext::init_grt_list_type() {
  PyGRTListObjectType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyGRTListObjectType) < 0) {
    throw std::runtime_error("Could not initialize GRT List type in python");
  }

  Py_INCREF(&PyGRTListObjectType);
  PyModule_AddObject(get_grt_module(), "List", (PyObject *)&PyGRTListObjectType);

  _grt_list_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "List");
}
