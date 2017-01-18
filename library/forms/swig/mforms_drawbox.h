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

#ifndef _MFORMS_PYDRAWBOX_H_
#define _MFORMS_PYDRAWBOX_H_

#include <mforms/drawbox.h>
#include "cairo/cairo.h"
#include "base/python_utils.h"

namespace mforms {
  class PyDrawBox : public DrawBox {
  public:
    PyDrawBox() : _self(NULL) {
    }

    void set_instance(PyObject *instance) {
      //_self = PyWeakref_NewRef(instance, NULL);
      _self = instance;
      Py_INCREF(_self);
    }

    ~PyDrawBox() {
      Py_XDECREF(_self);
    }

  protected:
    virtual void repaint(cairo_t *cr, int x, int y, int w, int h) {
      WillEnterPython lock;

      PyObject *c = SWIG_NewPointerObj(cr, SWIG_TypeQuery("cairo_t *"), 0);
      PyObject *args = Py_BuildValue("(Oiiii)", c, x, y, w, h);
      call_method("repaint", args);
      Py_XDECREF(c);
      Py_XDECREF(args);
    }

    virtual bool mouse_down(mforms::MouseButton button, int x, int y) {
      if (DrawBox::mouse_down(button, x, y))
        return true;

      WillEnterPython lock;

      PyObject *args = Py_BuildValue("(iii)", button, x, y);
      bool result = call_method("mouse_down", args);
      Py_XDECREF(args);

      return result;
    }

    virtual bool mouse_up(mforms::MouseButton button, int x, int y) {
      if (DrawBox::mouse_up(button, x, y))
        return true;

      WillEnterPython lock;

      PyObject *args = Py_BuildValue("(iii)", button, x, y);
      bool result = call_method("mouse_up", args);
      Py_XDECREF(args);

      return result;
    }

    virtual bool mouse_click(mforms::MouseButton button, int x, int y) {
      if (DrawBox::mouse_click(button, x, y))
        return true;

      WillEnterPython lock;

      PyObject *args = Py_BuildValue("(iii)", button, x, y);
      bool result = call_method("mouse_click", args);
      Py_XDECREF(args);

      return result;
    }

    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y) {
      if (DrawBox::mouse_double_click(button, x, y))
        return true;

      WillEnterPython lock;

      PyObject *args = Py_BuildValue("(iii)", button, x, y);
      bool result = call_method("mouse_double_click", args);
      Py_XDECREF(args);

      return result;
    }

    virtual bool mouse_enter() {
      if (DrawBox::mouse_enter())
        return true;

      WillEnterPython lock;

      PyObject *args = Py_BuildValue("()");
      bool result = call_method("mouse_enter", args);
      Py_XDECREF(args);

      return result;
    }

    virtual bool mouse_leave() {
      if (DrawBox::mouse_leave())
        return true;

      WillEnterPython lock;

      PyObject *args = Py_BuildValue("()");
      bool result = call_method("mouse_leave", args);
      Py_XDECREF(args);

      return result;
    }

    virtual bool mouse_move(mforms::MouseButton button, int x, int y) {
      if (DrawBox::mouse_move(button, x, y))
        return true;

      WillEnterPython lock;

      PyObject *args = Py_BuildValue("(ii)", x, y);
      bool result = call_method("mouse_move", args);
      Py_XDECREF(args);

      return result;
    }

    PyObject *_self;

    bool call_method(const char *method, PyObject *args) {
      bool result = false;

      // if (PyWeakref_CheckRef(_self))
      {
        PyObject *self = _self; // PyWeakref_GetObject(_self);
        if (self && self != Py_None && PyObject_HasAttrString(self, method)) {
          PyObject *ret = PyObject_CallMethod(self, (char *)method, (char *)"O", args, NULL);
          if (!ret) {
            PyErr_Print();
            PyErr_Clear();
          } else {
            result = ret == Py_True; // XXX: needs fix
            Py_DECREF(ret);
          }
        }
        /* else
         {
           PyObject *repr = PyObject_Repr(self);
           printf("Error calling method %s from PyDrawBox delegate %s\n", method,
                  PyString_AsString(repr));
           Py_DECREF(repr);
         }*/
      }
      return result;
    }
  };
};

#endif
