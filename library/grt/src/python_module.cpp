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

#include "python_module.h"

#include <cppconn/exception.h>

using namespace grt;
using namespace base;

static void function_dealloc(PyGRTFunctionObject *self) {
  Py_TYPE(self)->tp_free(self);
}

static PyObject *function_call(PyGRTFunctionObject *self, PyObject *args, PyObject *kw) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (!ctx)
    return nullptr;

  if ((int)self->function->arg_types.size() != PyTuple_Size(args)) {
    PyErr_SetString(PyExc_TypeError,
                    strfmt("%s.%s() takes %i arguments (%i given)", self->module->name().c_str(),
                           self->function->name.c_str(), (int)self->function->arg_types.size(), (int)PyTuple_Size(args))
                      .c_str());
    return nullptr;
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
        return nullptr;
      }
      grtargs.ginsert(v);
    } catch (grt::type_error &) {
      PyErr_SetString(PyExc_TypeError,
                      strfmt("%s.%s(): argument %i must be a %s", self->module->name().c_str(),
                             self->function->name.c_str(), (int)(a + 1), grt::fmt_type_spec(arg->type).c_str())
                        .c_str());
      return nullptr;
    } catch (std::exception &exc) {
      PythonContext::set_python_error(exc,
                                      strfmt("%s.%s()", self->module->name().c_str(), self->function->name.c_str()));
      return nullptr;
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
    return nullptr;
  } catch (grt::python_error &) {
    return nullptr;
  } catch (std::exception &exc) {
    PythonContext::set_python_error(exc, strfmt("%s.%s()", self->module->name().c_str(), self->function->name.c_str()));
  }

  return nullptr;
}

static PyTypeObject PyGRTFunctionObjectType = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0) // PyObject_VAR_HEAD
  "grt.Function",   //  tp_name
  sizeof(PyGRTFunctionObject),  //  tp_basicsize
  0, /* tp_itemsize For allocation */ 
  
  /* Methods to implement standard operations */

  (destructor)function_dealloc,   //  tp_dealloc
  0,  //  tp_vectorcall_offset
  0,  //  tp_getattr
  0,  //  tp_setattr
  0,  //  tp_as_async
  0,  //  tp_repr

  /* Method suites for standard classes */

  0,  //  tp_as_number
  0,  //  tp_as_sequence
  0,  //  tp_as_mapping

  /* More standard operations (here for binary compatibility) */

  0,  //  tp_hash
  (ternaryfunc)function_call, //  tp_call
  0,  //  tp_str

  PyObject_GenericGetAttr,  //  tp_getattro
  PyObject_GenericSetAttr,  //  tp_setattro

  /* Functions to access object as input/output buffer */
  0,  //  tp_as_buffer

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT, //  tp_flags

  0, /* tp_doc Documentation string */ 

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
  0,  //  tp_methods
  0,  //  tp_members
  0,  //  tp_getset
  0,  //  tp_base
  0,  //  tp_dict
  0,  //  tp_descr_get
  0,  //  tp_descr_set
  0,  //  tp_dictoffset
  0,  //  tp_init
  PyType_GenericAlloc,  //  tp_alloc
  PyType_GenericNew,  //  tp_new
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

//----------------------------------------------------------------------------------------------

static int module_init(PyGRTModuleObject *self, PyObject *args, PyObject *kwds) {
  PythonContext *ctx = PythonContext::get_and_check();
  if (ctx) {
    const char *name = nullptr;

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
  Py_TYPE(self)->tp_free(self);
}

static PyObject *module_getattro(PyGRTModuleObject *self, PyObject *attr_name) {
  if (PyUnicode_Check(attr_name)) {
    const char *attrname = PyUnicode_AsUTF8(attr_name);

    PyObject *module;
    if ((module = PyObject_GenericGetAttr((PyObject *)self, attr_name)))
      return module;
    PyErr_Clear();

    if (strcmp(attrname, "__members__") == 0) {
      return Py_BuildValue("[ssssssss]", "__doc__", "__bundlepath__", "__author__", "__name__", "__path__", "__iconpath__",
                           "__description__", "__version__");
    } else if (strcmp(attrname, "__methods__") == 0) {
      const std::vector<grt::Module::Function> &functions(self->module->get_functions());
      PyObject *methods = PyList_New(functions.size());
      for (size_t c = functions.size(), i = 0; i < c; i++) {
        PyList_SetItem(methods, i, PyUnicode_FromString(functions[i].name.c_str()));
      }
      return methods;
    } else if (strcmp(attrname, "__author__") == 0)
      return Py_BuildValue("s", self->module->author().c_str());
    else if (strcmp(attrname, "__name__") == 0)
      return Py_BuildValue("s", self->module->name().c_str());
    else if (strcmp(attrname, "__path__") == 0)
      return Py_BuildValue("s", self->module->path().c_str());
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
        PyGRTFunctionObject *method = (PyGRTFunctionObject *)PyType_GenericNew(&PyGRTFunctionObjectType, nullptr, nullptr);
        if (!method)
          return nullptr;

        method->module = self->module;
        method->function = self->module->get_function(attrname);

        return (PyObject *)method;
      } else
        PyErr_SetString(PyExc_AttributeError, strfmt("unknown attribute '%s'", attrname).c_str());
    }
  }
  return nullptr;
}

static PyObject *module_str(PyGRTModuleObject *self) {
  return PyUnicode_FromString(strfmt("<GRT Module '%s'>", self->module->name().c_str()).c_str());
}

static PyObject *module_get_doc(PyGRTModuleObject *self, void *closure) {
  return Py_BuildValue("s", self->module->description().c_str());
}

PyDoc_STRVAR(PyGRTModuleDoc,
             "Module(name) -> GRT Module\n\
             \n\
             Creates a wrapper for a GRT module.");

static PyGetSetDef PyGRTModuleGetSetters[] = {
  {(char *)"__doc__", (getter)module_get_doc, nullptr, (char *)"Documentation of the GRT module.", 0}, {0, 0, 0, 0, 0},
};

static PyTypeObject PyGRTModuleObjectType = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0) // PyModule_VAR_HEAD
  "grt.Module",     //  tp_name
  sizeof(PyGRTModuleObject),  //  tp_basicsize
  0, /* tp_itemsize For allocation */
  
  /* Methods to implement standard operations */

  (destructor)module_dealloc,   //  tp_dealloc
  0,                            //  tp_print
  0,                            //  tp_getattr
  0,                            //  tp_setattr
  0,                            //  tp_as_async
  0,                            //  tp_repr

  /* Method suites for standard classes */

  0,              //  tp_as_number
  0,              //  tp_as_sequence
  0,              //  tp_as_mapping

  /* More standard operations (here for binary compatibility) */

  0,  //  tp_hash
  0,  //  tp_call
  (reprfunc)module_str,   //  tp_str

  (getattrofunc)module_getattro,  //  tp_getattro
  PyObject_GenericSetAttr,  //  tp_setattro

  /* Functions to access object as input/output buffer */
  0,  //  tp_as_buffer

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT,   //  tp_flags

  PyGRTModuleDoc, /* tp_doc Documentation string */

  /* Assigned meaning in release 2.0 */
  /* call function for all accessible objects */
  0,  //  tp_traverse

  /* delete references to contained objects */
  0,  //  tp_clear

  /* Assigned meaning in release 2.1 */
  /* rich comparisons */
  0,    //  tp_richcompare

  /* weak reference enabler */
  0,    //  tp_weaklistoffset

  /* Iterators */
  0,    //  tp_iter
  0,    //  tp_iternext

  /* Attribute descriptor and subclassing stuff */
  0,    //  tp_methods
  0,    //  tp_members
  PyGRTModuleGetSetters,  //  tp_getset
  0,    //  tp_base
  0,    //  tp_dict
  0,    //  tp_descr_get
  0,    //  tp_descr_set
  0,    //  tp_dictoffset
  (initproc)module_init,  //  tp_init
  PyType_GenericAlloc,    //  tp_alloc
  PyType_GenericNew,      //  tp_new
  0, /* tp_free Low-level free-memory routine */
  0, /* tp_is_gc For PyObject_IS_GC */
  0,    //  tp_bases
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

void grt::PythonContext::init_grt_module_type() {
  {
//     PyGRTModuleObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGRTModuleObjectType) < 0) {
      throw std::runtime_error("Could not initialize GRT Module type in python");
    }

    Py_INCREF(&PyGRTModuleObjectType);
    PyModule_AddObject(get_grt_module(), "Module", (PyObject *)&PyGRTModuleObjectType);

    _grt_module_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "Module");
  }

  {
//     PyGRTFunctionObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGRTFunctionObjectType) < 0) {
      throw std::runtime_error("Could not initialize GRT function type in python");
    }

    Py_INCREF(&PyGRTFunctionObjectType);
    PyModule_AddObject(get_grt_module(), "Function", (PyObject *)&PyGRTFunctionObjectType);

    _grt_function_class = PyDict_GetItemString(PyModule_GetDict(get_grt_module()), "Function");
  }
}
