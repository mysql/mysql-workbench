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

#include "python_module.h"

#include <cppconn/exception.h>

using namespace grt;
using namespace base;

static void function_dealloc(PyGRTFunctionObject *self) {
  self->ob_type->tp_free(self);
}

static PyObject *function_call(PyGRTFunctionObject *self, PyObject *args, PyObject *kw) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return NULL;

  if ((int)self->function->arg_types.size() != PyTuple_Size(args)) {
    PyErr_SetString(PyExc_TypeError,
                    strfmt("%s.%s() takes %i arguments (%i given)", self->module->name().c_str(),
                           self->function->name.c_str(), (int)self->function->arg_types.size(), (int)PyTuple_Size(args))
                      .c_str());
    return NULL;
  }

  Py_ssize_t a = 0;
  grt::BaseListRef grtargs(true);

  for (grt::ArgSpecList::const_iterator arg = self->function->arg_types.begin(); arg != self->function->arg_types.end();
       ++arg) {
    PyObject *argval = PyTuple_GetItem(args, a);

    try {
      grt::ValueRef v = ctx->from_pyobject(argval, arg->type);
      if (!v.is_valid() && is_simple_type(arg->type.base.type)) {
        PyErr_SetString(PyExc_TypeError,
                        strfmt("%s.%s(): argument %i must be a %s but is None", self->module->name().c_str(),
                               self->function->name.c_str(), (int)(a + 1), grt::fmt_type_spec(arg->type).c_str())
                          .c_str());
        return NULL;
      }
      grtargs.ginsert(v);
    } catch (grt::type_error) {
      PyErr_SetString(PyExc_TypeError,
                      strfmt("%s.%s(): argument %i must be a %s", self->module->name().c_str(),
                             self->function->name.c_str(), (int)(a + 1), grt::fmt_type_spec(arg->type).c_str())
                        .c_str());
      return NULL;
    } catch (std::exception &exc) {
      PythonContext::set_python_error(exc,
                                      strfmt("%s.%s()", self->module->name().c_str(), self->function->name.c_str()));
      return NULL;
    }
    ++a;
  }

  try {
    grt::ValueRef result;
    {
      WillLeavePython lock;

      result = self->module->call_function(self->function->name, grtargs);
    }
    PyObject *pyresult = ctx->from_grt(result);

    return pyresult;
  } catch (grt::user_cancelled &exc) {
    PythonContext::set_user_interrupted(exc);
  } catch (grt::db_access_denied &exc) {
    PythonContext::set_db_access_denied(exc);
  } catch (grt::db_login_error &exc) {
    PythonContext::set_db_login_error(exc);
  } catch (grt::module_error &exc) {
    if (exc.inner.empty())
      PythonContext::set_python_error(exc,
                                      strfmt("%s.%s()", self->module->name().c_str(), self->function->name.c_str()));
    else
      PythonContext::set_python_error(exc, exc.inner);
  } catch (sql::SQLException &exc) {
    PythonContext::set_python_error(exc, strfmt("%s.%s()", self->module->name().c_str(), self->function->name.c_str()));
    return NULL;
  } catch (grt::python_error) {
    return NULL;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc, strfmt("%s.%s()", self->module->name().c_str(), self->function->name.c_str()));
  }

  return NULL;
}

static PyTypeObject PyGRTFunctionObjectType = {
  PyObject_HEAD_INIT(&PyType_Type) // PyObject_VAR_HEAD
  0,
  "grt.Function",                 // char *tp_name; /* For printing, in format "<module>.<name>" */
  sizeof(PyGRTFunctionObject), 0, // int tp_basicsize, tp_itemsize; /* For allocation */

  /* Functions to implement standard operations */

  (destructor)function_dealloc, //  destructor tp_dealloc;
  0,                            //  printfunc tp_print;
  0,                            //  getattrfunc tp_getattr;
  0,                            //  setattrfunc tp_setattr;
  0,                            //(cmpfunc)module_compare, //  cmpfunc tp_compare;
  0,                            //(reprfunc)module_repr,//  reprfunc tp_repr;

  /* Method suites for standard classes */

  0, //  PyNumberMethods *tp_as_number;
  0, //  PySequenceMethods *tp_as_sequence;
  0, //  PyMappingMethods *tp_as_mapping;

  /* More standard operations (here for binary compatibility) */

  0,                          //  hashfunc tp_hash;
  (ternaryfunc)function_call, //  ternaryfunc tp_call;
  0,                          //  reprfunc tp_str;
  PyObject_GenericGetAttr,    //  getattrofunc tp_getattro;
  PyObject_GenericSetAttr,    //  setattrofunc tp_setattro;

  /* Functions to access module as input/output buffer */
  0, //  PyBufferProcs *tp_as_buffer;

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT, //  long tp_flags;

  0, //  char *tp_doc; /* Documentation string */

  /* Assigned meaning in release 2.0 */
  /* call function for all accessible modules */
  0, //  traverseproc tp_traverse;

  /* delete references to contained modules */
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
  0,                   //  struct _typemodule *tp_base;
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
  0,                   //    destructor tp_del;
#if (PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION > 5)
  0 //  tp_version_tag
#endif
};

//----------------------------------------------------------------------------------------------

static int module_init(PyGRTModuleObject *self, PyObject *args, PyObject *kwds) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (ctx) {
    const char *name = NULL;

    if (!PyArg_ParseTuple(args, "z", &name))
      return -1;

    self->module = grt::GRT::get()->get_module(name);

    if (!self->module) {
      PyErr_SetString(PyExc_NameError, strfmt("unknown GRT module '%s'", name).c_str());
      return -1;
    }
    return 0;
  }
  return -1;
}

static void module_dealloc(PyGRTModuleObject *self) {
  self->ob_type->tp_free(self);
}

static PyObject *module_getattro(PyGRTModuleObject *self, PyObject *attr_name) {
  if (PyString_Check(attr_name)) {
    const char *attrname = PyString_AsString(attr_name);

    PyObject *module;
    if ((module = PyObject_GenericGetAttr((PyObject *)self, attr_name)))
      return module;
    PyErr_Clear();

    if (strcmp(attrname, "__members__") == 0) {
      return Py_BuildValue("[ss]", "__doc__", "__bundlepath__", "__author__", "__name__", "__iconpath__",
                           "__description__", "__version__");
    } else if (strcmp(attrname, "__methods__") == 0) {
      const std::vector<grt::Module::Function> &functions(self->module->get_functions());
      PyObject *methods = PyList_New(functions.size());
      for (size_t c = functions.size(), i = 0; i < c; i++) {
        PyList_SetItem(methods, i, PyString_FromString(functions[i].name.c_str()));
      }
      return methods;
    } else if (strcmp(attrname, "__author__") == 0)
      return Py_BuildValue("s", self->module->author().c_str());
    else if (strcmp(attrname, "__name__") == 0)
      return Py_BuildValue("s", self->module->name().c_str());
    else if (strcmp(attrname, "__iconpath__") == 0)
      return Py_BuildValue("s", self->module->default_icon_path().c_str());
    else if (strcmp(attrname, "__description__") == 0)
      return Py_BuildValue("s", self->module->description().c_str());
    else if (strcmp(attrname, "__version__") == 0)
      return Py_BuildValue("s", self->module->version().c_str());
    else if (strcmp(attrname, "__bundlepath__") == 0) {
      if (self->module->is_bundle())
        return Py_BuildValue("s", self->module->bundle_path().c_str());
      else {
        Py_INCREF(Py_None);
        return Py_None;
      }
    } else {
      if (self->module->has_function(attrname)) {
        // create a method call module and return it
        PyGRTFunctionObject *method = (PyGRTFunctionObject *)PyType_GenericNew(&PyGRTFunctionObjectType, NULL, NULL);
        if (!method)
          return NULL;

        method->module = self->module;
        method->function = self->module->get_function(attrname);

        return (PyObject *)method;
      } else
        PyErr_SetString(PyExc_AttributeError, strfmt("unknown attribute '%s'", attrname).c_str());
    }
  }
  return NULL;
}

static PyObject *module_str(PyGRTModuleObject *self) {
  return PyString_FromString(strfmt("<GRT Module '%s'>", self->module->name().c_str()).c_str());
}

static PyObject *module_get_doc(PyGRTModuleObject *self, void *closure) {
  return Py_BuildValue("s", self->module->description().c_str());
}

PyDoc_STRVAR(PyGRTModuleDoc,
             "Module(name) -> GRT Module\n\
             \n\
             Creates a wrapper for a GRT module.");

static PyGetSetDef PyGRTModuleGetSetters[] = {
  {(char *)"__doc__", (getter)module_get_doc, NULL, (char *)"Documentation of the GRT module.", 0}, {0, 0, 0, 0, 0},
};

static PyTypeObject PyGRTModuleObjectType = {
  PyObject_HEAD_INIT(&PyType_Type) // PyModule_VAR_HEAD
  0,
  "grt.Module",                 // char *tp_name; /* For printing, in format "<module>.<name>" */
  sizeof(PyGRTModuleObject), 0, // int tp_basicsize, tp_itemsize; /* For allocation */

  /* Methods to implement standard operations */

  (destructor)module_dealloc, //  destructor tp_dealloc;
  0,                          //  printfunc tp_print;
  0,                          //  getattrfunc tp_getattr;
  0,                          //  setattrfunc tp_setattr;
  0,                          //(cmpfunc)module_compare, //  cmpfunc tp_compare;
  0,                          //(reprfunc)module_repr,//  reprfunc tp_repr;

  /* Method suites for standard classes */

  0, //  PyNumberMethods *tp_as_number;
  0, //  PySequenceMethods *tp_as_sequence;
  0, //  PyMappingMethods *tp_as_mapping;

  /* More standard operations (here for binary compatibility) */

  0,                             //  hashfunc tp_hash;
  0,                             //  ternaryfunc tp_call;
  (reprfunc)module_str,          //  reprfunc tp_str;
  (getattrofunc)module_getattro, //  getattrofunc tp_getattro;
  PyObject_GenericSetAttr,       //  setattrofunc tp_setattro;

  /* Functions to access module as input/output buffer */
  0, //  PyBufferProcs *tp_as_buffer;

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT, //  long tp_flags;

  PyGRTModuleDoc, //  char *tp_doc; /* Documentation string */

  /* Assigned meaning in release 2.0 */
  /* call function for all accessible modules */
  0, //  traverseproc tp_traverse;

  /* delete references to contained modules */
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
  0,                     //  struct PyMethodDef *tp_methods;
  0,                     //  struct PyMemberDef *tp_members;
  PyGRTModuleGetSetters, //  struct PyGetSetDef *tp_getset;
  0,                     //  struct _typemodule *tp_base;
  0,                     //  PyModule *tp_dict;
  0,                     //  descrgetfunc tp_descr_get;
  0,                     //  descrsetfunc tp_descr_set;
  0,                     //  long tp_dictoffset;
  (initproc)module_init, //  initproc tp_init;
  PyType_GenericAlloc,   //  allocfunc tp_alloc;
  PyType_GenericNew,     //  newfunc tp_new;
  0,                     //  freefunc tp_free; /* Low-level free-memory routine */
  0,                     //  inquiry tp_is_gc; /* For PyModule_IS_GC */
  0,                     //  PyModule *tp_bases;
  0,                     //  PyModule *tp_mro; /* method resolution order */
  0,                     //  PyModule *tp_cache;
  0,                     //  PyModule *tp_subclasses;
  0,                     //  PyModule *tp_weaklist;
  0,                     //  destructor tp_del;
#if (PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION > 5)
  0 // tp_version_tag
#endif
};

void grt::PythonContext::init_grt_module_type() {
  {
    PyGRTModuleObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGRTModuleObjectType) < 0) {
      throw std::runtime_error("Could not initialize GRT Module type in python");
    }

    Py_INCREF(&PyGRTModuleObjectType);
    PyModule_AddObject(get_grt_module(), "Module", (PyObject *)&PyGRTModuleObjectType);

    _grt_module_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "Module");
  }

  {
    PyGRTFunctionObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGRTFunctionObjectType) < 0) {
      throw std::runtime_error("Could not initialize GRT function type in python");
    }

    Py_INCREF(&PyGRTFunctionObjectType);
    PyModule_AddObject(get_grt_module(), "Function", (PyObject *)&PyGRTFunctionObjectType);

    _grt_function_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "Function");
  }
}
