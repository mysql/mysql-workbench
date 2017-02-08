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
#include "base/string_utilities.h"

#include "grtpp_util.h"

#include "python_grtobject.h"

#include "cppconn/exception.h"

using namespace grt;
using namespace base;

static PyObject *call_object_method(const grt::ObjectRef &object, const grt::ClassMethod *method, PyObject *args) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  Py_ssize_t a = 0;
  grt::BaseListRef grtargs(true);

  if ((int)method->arg_types.size() != PyTuple_Size(args)) {
    PyErr_SetString(PyExc_TypeError,
                    strfmt("%s.%s() takes %i arguments (%i given)", object->class_name().c_str(), method->name.c_str(),
                           (int)method->arg_types.size(), (int)PyTuple_Size(args))
                      .c_str());
    return NULL;
  }

  for (grt::ArgSpecList::const_iterator arg = method->arg_types.begin(); arg != method->arg_types.end(); ++arg) {
    PyObject *argval = PyTuple_GetItem(args, a);

    try {
      grt::ValueRef v = ctx->from_pyobject(argval, arg->type);
      grtargs.ginsert(v);
    } catch (grt::type_error) {
      PyErr_SetString(PyExc_TypeError,
                      strfmt("argument %i must be a %s", (int)(a + 1), grt::fmt_type_spec(arg->type).c_str()).c_str());
      return NULL;
    } catch (std::exception &exc) {
      PythonContext::set_python_error(exc);
      return NULL;
    }
    ++a;
  }

  try {
    grt::ValueRef result;
    {
      WillLeavePython lock;

      result = object.get_metaclass()->call_method(&object.content(), method, grtargs);
    }
    return ctx->from_grt(result);
  } catch (sql::SQLException &exc) {
    PythonContext::set_db_error(exc);
    return NULL;
  } catch (grt::python_error) {
    return NULL;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

static void method_dealloc(PyGRTMethodObject *self) {
  delete self->object;

  self->ob_type->tp_free(self);
}

static PyObject *method_call(PyGRTMethodObject *self, PyObject *args, PyObject *kw) {
  return call_object_method(*self->object, self->method, args);
}

static PyTypeObject PyGRTMethodObjectType = {
  PyObject_HEAD_INIT(&PyType_Type) // PyObject_VAR_HEAD
  0,
  "grt.Object",                 // char *tp_name; /* For printing, in format "<module>.<name>" */
  sizeof(PyGRTMethodObject), 0, // int tp_basicsize, tp_itemsize; /* For allocation */

  /* Methods to implement standard operations */

  (destructor)method_dealloc, //  destructor tp_dealloc;
  0,                          //  printfunc tp_print;
  0,                          //  getattrfunc tp_getattr;
  0,                          //  setattrfunc tp_setattr;
  0,                          //(cmpfunc)object_compare, //  cmpfunc tp_compare;
  0,                          //(reprfunc)object_repr,//  reprfunc tp_repr;

  /* Method suites for standard classes */

  0, //  PyNumberMethods *tp_as_number;
  0, //  PySequenceMethods *tp_as_sequence;
  0, //  PyMappingMethods *tp_as_mapping;

  /* More standard operations (here for binary compatibility) */

  0,                        //  hashfunc tp_hash;
  (ternaryfunc)method_call, //  ternaryfunc tp_call;
  0,                        //  reprfunc tp_str;
  PyObject_GenericGetAttr,  //  getattrofunc tp_getattro;
  PyObject_GenericSetAttr,  //  setattrofunc tp_setattro;

  /* Functions to access object as input/output buffer */
  0, //  PyBufferProcs *tp_as_buffer;

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT, //  long tp_flags;

  0, //  char *tp_doc; /* Documentation string */

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
  0,                   //  struct PyMethodDef *tp_methods;
  0,                   //  struct PyMemberDef *tp_members;
  0,                   //  struct PyGetSetDef *tp_getset;
  0,                   //  struct _typeobject *tp_base;
  0,                   //  PyObject *tp_dict;
  0,                   //  descrgetfunc tp_descr_get;
  0,                   //  descrsetfunc tp_descr_set;
  0,                   //  long tp_dictoffset;
  0,                   //  initproc tp_init;
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

//----------------------------------------------------------------------------------------------

static int object_init(PyGRTObjectObject *self, PyObject *args, PyObject *kwds) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (ctx) {
    const char *class_name = NULL;
    PyObject *valueptr = NULL;
    static const char *kwlist[] = {"classname", "__valueptr__", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "z|O", (char **)kwlist, &class_name, &valueptr))
      return -1;

    delete self->object;

    if (valueptr && valueptr != Py_None) {
      try {
        grt::ValueRef v = PythonContext::value_from_internal_cobject(valueptr);
        grt::ObjectRef content = grt::ObjectRef::cast_from(v);
        self->object = new grt::ObjectRef(content);
        self->hash = -1;
      } catch (grt::type_error &exc) {
        PythonContext::set_python_error(exc);
        return -1;
      } catch (std::exception &exc) {
        PythonContext::set_python_error(exc);
        return -1;
      }
    } else {
      if (!class_name || !grt::GRT::get()->get_metaclass(class_name)) {
        PyErr_SetString(PyExc_NameError, "invalid GRT class name");
        return -1;
      }

      self->object = new grt::ObjectRef(grt::GRT::get()->create_object<internal::Object>(class_name));
      self->hash = -1;
    }
    return 0;
  }
  return -1;
}

static void object_dealloc(PyGRTObjectObject *self) {
  delete self->object;

  self->ob_type->tp_free(self);
}

static PyObject *object_printable(PyGRTObjectObject *self) {
  return PyString_FromString(self->object->toString().c_str());
}

static int object_compare(PyGRTObjectObject *self, PyGRTObjectObject *other) {
  if (self == other || self->object->id() == other->object->id())
    return 0;
  if (self->object->id() > other->object->id())
    return 1;
  return -1;
}

static long object_hash(PyGRTObjectObject *self) {
  if (self->hash != -1)
    return self->hash;
  // hash the identifier of the object using the same algorithm as the built-in one for strings
  std::string id = self->object->id();
  if (id.empty())
    return 0;

  long hash = id[0] << 7;
  for (std::string::const_iterator c = id.begin(); c != id.end(); ++c)
    hash = (1000003 * hash) ^ *c;
  hash = hash ^ (int)id.length();
  if (hash == -1)
    hash = -2;

  self->hash = hash;

  return hash;
}

static PyObject *object_getattro(PyGRTObjectObject *self, PyObject *attr_name) {
  if (PyString_Check(attr_name)) {
    const char *attrname = PyString_AsString(attr_name);

    PyObject *object;
    if ((object = PyObject_GenericGetAttr((PyObject *)self, attr_name)))
      return object;
    PyErr_Clear();

    if (strcmp(attrname, "__grtclassname__") == 0)
      return Py_BuildValue("s", self->object->class_name().c_str());
    else if (strcmp(attrname, "__id__") == 0)
      return Py_BuildValue("s", self->object->id().c_str());
    else {
      if (self->object->has_member(attrname)) {
        PythonContext *ctx = PythonContext::get_and_check();
        if (!ctx)
          return NULL;

        return ctx->from_grt(self->object->get_member(attrname));
      } else if (self->object->has_method(attrname)) {
        // create a method call object and return it
        PyGRTMethodObject *method = (PyGRTMethodObject *)PyType_GenericNew(&PyGRTMethodObjectType, NULL, NULL);
        if (!method)
          return NULL;

        method->object = new grt::ObjectRef(*self->object);
        method->method = self->object->get_metaclass()->get_method_info(attrname);

        return (PyObject *)method;
      } else
        PyErr_SetString(PyExc_AttributeError, strfmt("unknown attribute '%s'", attrname).c_str());
    }
  }
  return NULL;
}

static int object_setattro(PyGRTObjectObject *self, PyObject *attr_name, PyObject *attr_value) {
  if (PyString_Check(attr_name)) {
    const char *attrname = PyString_AsString(attr_name);

    if (self->object->has_member(attrname)) {
      PythonContext *ctx = PythonContext::get_and_check();
      if (!ctx)
        return -1;
      const grt::MetaClass::Member *member = self->object->get_metaclass()->get_member_info(attrname);
      if (member) {
        grt::ValueRef value;

        if (member->read_only) {
          PyErr_Format(PyExc_TypeError, "%s is read-only", attrname);
          return -1;
        }

        try {
          value = ctx->from_pyobject(attr_value, member->type);
        } catch (const std::exception &exc) {
          PythonContext::set_python_error(exc);
          return -1;
        }

        try {
          self->object->set_member(attrname, value);
        } catch (const std::exception &exc) {
          PythonContext::set_python_error(exc);
          return -1;
        }
        return 0;
      }
    }

    PyErr_Format(PyExc_AttributeError, "unknown attribute '%s'", attrname);
  }
  return -1;
}

static PyObject *object_callmethod(PyGRTObjectObject *self, PyObject *args) {
  PyObject *method_name;

  if (PyTuple_Size(args) < 1 || !(method_name = PyTuple_GetItem(args, 0)) || !PyString_Check(method_name)) {
    PyErr_SetString(PyExc_TypeError, "1st argument must be name of method to call");
    return NULL;
  }

  const grt::ClassMethod *method = self->object->get_metaclass()->get_method_info(PyString_AsString(method_name));
  if (!method) {
    PyErr_SetString(PyExc_TypeError, "invalid method");
    return NULL;
  }

  return call_object_method(*self->object, method, PyTuple_GetSlice(args, 1, PyTuple_Size(args)));
}

static PyObject *object_reset_references(PyGRTObjectObject *self, void *nothing) {
  (*self->object)->reset_references();

  Py_RETURN_NONE;
}

static PyObject *object_shallow_copy(PyGRTObjectObject *self, void *nothing) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return NULL;

  return ctx->from_grt(grt::shallow_copy_object(*self->object));
}

static PyObject *object_deep_copy(PyGRTObjectObject *self, void *nothing) {
  PythonContext *ctx;
  if (!(ctx = PythonContext::get_and_check()))
    return NULL;

  return ctx->from_grt(grt::copy_object(*self->object));
}

static PyObject *object_get_doc(PyGRTObjectObject *self, void *closure) {
  return Py_BuildValue("s", self->object->get_metaclass()->get_attribute("description").c_str());
}

static bool add_member_to_list(const grt::MetaClass::Member *member, PyObject *list) {
  PyObject *tmp = PyString_FromString(member->name.c_str());
  PyList_Append(list, tmp);
  Py_DECREF(tmp);

  return true;
}

static bool add_method_to_list(const grt::MetaClass::Method *method, PyObject *list) {
  PyObject *tmp = PyString_FromString(method->name.c_str());
  PyList_Append(list, tmp);
  Py_DECREF(tmp);

  return true;
}

static PyObject *object_get_members(PyGRTObjectObject *self, void *closure) {
  PyObject *members = PyList_New(0);
  self->object->get_metaclass()->foreach_member(std::bind(&add_member_to_list, std::placeholders::_1, members));
  return members;
}

static PyObject *object_get_methods(PyGRTObjectObject *self, void *closure) {
  PyObject *methods = PyList_New(0);
  self->object->get_metaclass()->foreach_method(std::bind(&add_method_to_list, std::placeholders::_1, methods));
  return methods;
}

PyDoc_STRVAR(PyGRTObjectDoc,
             "Object(grtclass) -> GRT Object\n\
\n\
Creates a new instance of a GRT object. grtclass specifies the name of\n\
the GRT class of the object.");

PyDoc_STRVAR(call_doc, "callmethod(method_name, ...) -> value");

static PyMethodDef PyGRTObjectMethods[] = {
  {"__callmethod__", (PyCFunction)object_callmethod, METH_VARARGS, call_doc},
  {"reset_references", (PyCFunction)object_reset_references, METH_NOARGS, NULL},
  {"deep_copy", (PyCFunction)object_deep_copy, METH_NOARGS, NULL},
  {"shallow_copy", (PyCFunction)object_shallow_copy, METH_NOARGS, NULL},
  {NULL, NULL, 0, NULL}};

static PyGetSetDef PyGRTObjectGetSetters[] = {
  {(char *)"__doc__", (getter)object_get_doc, NULL, (char *)"Documentation of the GRT class.", NULL},
  {(char *)"__grtmembers__", (getter)object_get_members, NULL, (char *)"GRT object members", NULL},
  {(char *)"__grtmethods__", (getter)object_get_methods, NULL, (char *)"GRT object methods", NULL},
  {NULL, NULL, NULL, NULL, NULL},
};

static PyTypeObject PyGRTObjectObjectType = {
  PyObject_HEAD_INIT(&PyType_Type) // PyObject_VAR_HEAD
  0,
  "grt.Object",                 // char *tp_name; /* For printing, in format "<module>.<name>" */
  sizeof(PyGRTObjectObject), 0, // int tp_basicsize, tp_itemsize; /* For allocation */

  /* Methods to implement standard operations */

  (destructor)object_dealloc, //  destructor tp_dealloc;
  0,                          //  printfunc tp_print;
  0,                          //  getattrfunc tp_getattr;
  0,                          //  setattrfunc tp_setattr;
  (cmpfunc)object_compare,    //  cmpfunc tp_compare;
  0,                          //(reprfunc)object_repr,//  reprfunc tp_repr;

  /* Method suites for standard classes */

  0, //  PyNumberMethods *tp_as_number;
  0, //  PySequenceMethods *tp_as_sequence;
  0, //  PyMappingMethods *tp_as_mapping;

  /* More standard operations (here for binary compatibility) */

  (hashfunc)object_hash,         //  hashfunc tp_hash;
  0,                             //  ternaryfunc tp_call;
  (reprfunc)object_printable,    //  reprfunc tp_str;
  (getattrofunc)object_getattro, //  getattrofunc tp_getattro;
  (setattrofunc)object_setattro, //  setattrofunc tp_setattro;

  /* Functions to access object as input/output buffer */
  0, //  PyBufferProcs *tp_as_buffer;

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, //  long tp_flags;

  PyGRTObjectDoc, //  char *tp_doc; /* Documentation string */

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
  PyGRTObjectMethods,    //  struct PyMethodDef *tp_methods;
  0,                     //  struct PyMemberDef *tp_members;
  PyGRTObjectGetSetters, //  struct PyGetSetDef *tp_getset;
  0,                     //  struct _typeobject *tp_base;
  0,                     //  PyObject *tp_dict;
  0,                     //  descrgetfunc tp_descr_get;
  0,                     //  descrsetfunc tp_descr_set;
  0,                     //  long tp_dictoffset;
  (initproc)object_init, //  initproc tp_init;
  PyType_GenericAlloc,   //  allocfunc tp_alloc;
  PyType_GenericNew,     //  newfunc tp_new;
  0,                     //  freefunc tp_free; /* Low-level free-memory routine */
  0,                     //  inquiry tp_is_gc; /* For PyObject_IS_GC */
  0,                     //  PyObject *tp_bases;
  0,                     //  PyObject *tp_mro; /* method resolution order */
  0,                     //  PyObject *tp_cache;
  0,                     //  PyObject *tp_subclasses;
  0,                     //  PyObject *tp_weaklist;
  0,                     //  tp_del
#if (PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION > 5)
  0 //  tp_version_tag
#endif
};

void grt::PythonContext::init_grt_object_type() {
  {
    PyGRTObjectObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGRTObjectObjectType) < 0) {
      throw std::runtime_error("Could not initialize GRT Object type in python");
    }

    Py_INCREF(&PyGRTObjectObjectType);
    PyModule_AddObject(get_grt_module(), "Object", (PyObject *)&PyGRTObjectObjectType);

    _grt_object_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "Object");
  }
  {
    PyGRTMethodObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGRTMethodObjectType) < 0) {
      throw std::runtime_error("Could not initialize GRT Method type in python");
    }

    Py_INCREF(&PyGRTMethodObjectType);
    PyModule_AddObject(get_grt_module(), "Method", (PyObject *)&PyGRTMethodObjectType);

    _grt_method_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "Method");
  }
}
