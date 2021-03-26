%module mforms
%include "stdint.i"
#pragma SWIG nowarn=401,402,509

%{
#include <boost/signals2/signal.hpp>
#include <sstream>
#include <frameobject.h>
#include <base/drawing.h>
#include <base/log.h>
#include <mforms/view.h>
#include <mforms/form.h>
#include <mforms/button.h>
#include <mforms/checkbox.h>
#include <mforms/textentry.h>
#include <mforms/textbox.h>
#include <mforms/label.h>
#include <mforms/selector.h>
#include <mforms/listbox.h>
#include <mforms/tabview.h>
#include <mforms/box.h>
#include <mforms/panel.h>
#include <mforms/filechooser.h>
#include <mforms/radiobutton.h>
#include <mforms/imagebox.h>
#include <mforms/progressbar.h>
#include <mforms/table.h>
#include <mforms/scrollpanel.h>
#include <mforms/wizard.h>
#include <mforms/drawbox.h>
#include <mforms/tabswitcher.h>
#include <mforms/app.h>
#include <mforms/appview.h>
#include <mforms/utilities.h>
#include <mforms/uistyle.h>
#include <mforms/appview.h>
#include <mforms/sectionbox.h>
#include <mforms/widgets.h>
#include <mforms/menu.h>
#include <mforms/splitter.h>
#include <mforms/popup.h>
#include <mforms/code_editor.h>
#include <mforms/menubar.h>
#include <mforms/toolbar.h>
#include <mforms/task_sidebar.h>
#include <mforms/hypertext.h>
#include <mforms/popover.h>
#include <mforms/fs_object_selector.h>
#include <mforms/simpleform.h>
#include <mforms/treeview.h>
#include <mforms/dockingpoint.h>

#include "mforms_grt.h"
#include "mforms_drawbox.h"

using namespace boost::placeholders;


DEFAULT_LOG_DOMAIN("pymforms")

/// begin python specific stuff

struct PyObjectRef
{
  PyObject *object;

  PyObjectRef(PyObject *obj)
    : object(obj)
  {
    Py_XINCREF(obj);
  }

  PyObjectRef(const PyObjectRef &obj)
    : object(obj.object)
  {
    Py_XINCREF(object);
  }

  ~PyObjectRef()
  {
    WillEnterPython gil;
    Py_XDECREF(object);
  }

  operator PyObject*()
  {
    return object;
  }

  PyObjectRef &operator =(const PyObjectRef &other)
  {
    WillEnterPython gil;
    if (other.object != object)
    {
      if (object)
        Py_XDECREF(object);
      object = other.object;
      Py_XINCREF(object);
    }
    return *this;
  }
};


static std::string format_string_list(PyObject *list) {
   std::string result;
    if(!list) {
      return result;
    }
    int count = PyList_Size(list);
    for (int index = 0; index < count; ++index) {
        PyObject *item = PyList_GetItem(list, index);
        const char *text = PyUnicode_AsUTF8(item);
        if(text) {
          result += text;
        }
    }
    return result;
}

static void show_python_exception()
{
  if (!PyErr_Occurred())
    return;

  PyObject *type, *value, *traceback;
  PyObject *pythonErrorDescryption, *moduleName, *pythonModule, *formatExceptionFunction;

  PyErr_Fetch(&type, &value, &traceback);
  pythonErrorDescryption = PyObject_Str(value);
  std::string errorDescription;

  const char *description =  PyUnicode_AsUTF8(pythonErrorDescryption);
  if(description) {
    errorDescription = description;
  }
  std::string result;

  /* See if we can get a full traceback */
  moduleName = PyString_FromString("traceback");
  pythonModule = PyImport_Import(moduleName);
  Py_DECREF(moduleName);

  if (pythonModule) {
    formatExceptionFunction = PyObject_GetAttrString(pythonModule, "format_exception");

    if (formatExceptionFunction && PyCallable_Check(formatExceptionFunction)) {
      PyObject *formatExceptionFunctionResult;

      formatExceptionFunctionResult = PyObject_CallFunctionObjArgs(formatExceptionFunction, type, value, traceback, NULL);

      result = format_string_list(formatExceptionFunctionResult);
    }
  }

  logError("Unhandled exception in Python code: \n%s", result.c_str());
  mforms::Utilities::show_error("Error", std::string("Unhandled exception: ").append(errorDescription).append("\n\nCheck the log for more details."), "OK", "", "");
}

static void call_void_pycallable(PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("()");
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}

static void *call_ignoreret_voidptr_pycallable(PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("()");
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);

  return NULL;
}



static void call_void_string_pycallable(const std::string &s, PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("(s)", s.c_str());
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}


static void call_void_int_pycallable(int arg, PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("(i)", arg);
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}


static void call_void_menuitem_pycallable(mforms::MenuItem *arg, PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("(O)", togrt(arg, "MenuItem"));
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}


static void call_void_int_int_pycallable(int row, int column, PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("(ii)", row, column);
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}

static void call_void_int_int_int_pycallable(int a, int b, int c, PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("(iii)", a, b, c);
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}

static void call_void_node_int_pycallable(mforms::TreeNodeRef node, int column, PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *obj;
  if (node)
    obj = SWIG_NewPointerObj((new mforms::TreeNodeRef(node)), SWIGTYPE_p_mforms__TreeNodeRef, SWIG_POINTER_OWN |  0 );
  else
  {
    obj = Py_None;
    Py_INCREF(obj);
  }
  PyObject *args = Py_BuildValue("(Oi)", obj, column);
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}

static bool call_bool_pycallable(PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("()");
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
    return false;
  }
  else
  {
    bool r = ret == Py_True;
    Py_DECREF(ret);
    return r;
  }
}

static bool call_bool_int_pycallable(int i, PyObjectRef &callable)
{
  PyObject *ret;

  WillEnterPython lock;

  PyObject *args = Py_BuildValue("(i)", i);
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
    return false;
  }
  else
  {
    bool r = ret == Py_True;
    Py_DECREF(ret);
    return r;
  }
}

/*
static void call_cell_edited_pycallable(int row, int col, const std::string &value, PyObjectRef &callable)
{
  WillEnterPython lock;
  PyObject *ret;
  PyObject *args = Py_BuildValue("(iis)", row, col, value.c_str());
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}
*/
static void call_cell_node_edited_pycallable(mforms::TreeNodeRef node, int col, const std::string &value, PyObjectRef &callable)
{
  WillEnterPython lock;
  PyObject *ret;
  PyObject *obj;
  if (node)
    obj = SWIG_NewPointerObj((new mforms::TreeNodeRef(node)), SWIGTYPE_p_mforms__TreeNodeRef, SWIG_POINTER_OWN |  0 );
  else
  {
    obj = Py_None;
    Py_INCREF(obj);
  }
  PyObject *args = Py_BuildValue("(Ois)", obj, col, value.c_str());
  ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}




static void call_toolbaritem_pycallable(const mforms::ToolBarItem *i, PyObjectRef &callable)
{
  WillEnterPython lock;

  PyObject* tbi = SWIG_NewPointerObj(SWIG_as_voidptr(i), SWIGTYPE_p_mforms__ToolBarItem, SWIG_POINTER_DISOWN | 0 );

  PyObject *args = PyTuple_Pack(1, tbi);
  PyObject *ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}


static void call_void_entryaction_pycallable(mforms::TextEntryAction action, PyObjectRef &callable)
{
  WillEnterPython lock;

  PyObject *args = Py_BuildValue("(i)",  (int)action);
  PyObject *ret = PyObject_Call(callable, args, NULL);
  Py_DECREF(args);
  if (!ret)
  {
    show_python_exception();
    PyErr_Print();
  }
  else
    Py_DECREF(ret);
}

inline boost::function<void ()> pycall_void_fun(PyObject *callable)
{
  return boost::bind(call_void_pycallable, PyObjectRef(callable));
}

inline boost::function<void* ()> pycall_ignoreret_voidptr_fun(PyObject *callable)
{
  return boost::bind(call_ignoreret_voidptr_pycallable, PyObjectRef(callable));
}

inline boost::function<void (std::string)> pycall_void_string_fun(PyObject *callable)
{
  return boost::bind(call_void_string_pycallable, _1, PyObjectRef(callable));
}

inline boost::function<void (int)> pycall_void_int_fun(PyObject *callable)
{
  return boost::bind(call_void_int_pycallable, _1, PyObjectRef(callable));
}

inline boost::function<void (mforms::MenuItem*)> pycall_void_menuitem_fun(PyObject *callable)
{
  return boost::bind(call_void_menuitem_pycallable, _1, PyObjectRef(callable));
}

inline boost::function<void (int,int)> pycall_void_int_int_fun(PyObject *callable)
{
  return boost::bind(call_void_int_int_pycallable, _1, _2, PyObjectRef(callable));
}

inline boost::function<void (int, int,int)> pycall_void_int_int_int_fun(PyObject *callable)
{
  return boost::bind(call_void_int_int_int_pycallable, _1, _2, _3, PyObjectRef(callable));
}

inline boost::function<void (int, int, int, bool)> pycall_void_int_int_int_bool_fun(PyObject *callable)
{
  return boost::bind(call_void_int_int_int_pycallable, _1, _2, _3, PyObjectRef(callable));
}

inline boost::function<void (mforms::TreeNodeRef,int)> pycall_void_node_int_fun(PyObject *callable)
{
  return boost::bind(call_void_node_int_pycallable, _1, _2, PyObjectRef(callable));
}

inline boost::function<bool ()> pycall_bool_fun(PyObject *callable)
{
  return boost::bind(call_bool_pycallable, PyObjectRef(callable));
}

inline boost::function<bool (int)> pycall_bool_int_fun(PyObject *callable)
{
  return boost::bind(call_bool_int_pycallable, _1, PyObjectRef(callable));
}


inline boost::function<void (const mforms::ToolBarItem*)> pycall_void_toolbaritem_fun(PyObject *callable)
{
  return boost::bind(call_toolbaritem_pycallable, _1, PyObjectRef(callable));
}

inline boost::function<void (mforms::TextEntryAction)> pycall_void_entryaction_fun(PyObject *callable)
{
  return boost::bind(call_void_entryaction_pycallable, _1, PyObjectRef(callable));
}

/// end python specific stuff


%}

// Macros to define methods for adding Python callbacks as signal handlers
#define SWIG_ADD_SIGNAL_VOID_CALLBACK(method, signal)\
	signal_connection_wrapper add_##method(PyObject *callback) { return signal_connection_wrapper(signal->connect(pycall_void_fun(callback))); }\
	void call_##method() { (*signal)(); }

#define SWIG_ADD_SIGNAL_VOID_STRING_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_string_fun(callback)); }\
	void call_##method(const char *s) { (*signal)(s ? s : ""); }

#define BOOST_ADD_SIGNAL_VOID_STRING_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_string_fun(callback)); }\
	void call_##method(const char *s) { (*(signal))(s ? s : ""); }

#define SWIG_ADD_SIGNAL_VOID_INT_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_int_fun(callback)); }\
	void call_##method(int i) { (*signal)(i); }

#define SWIG_ADD_SIGNAL_VOID_MENUITEM_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_menuitem_fun(callback)); }\
	void call_##method(mforms::MenuItem *item) { (*signal)(item); }


#define SWIG_ADD_SIGNAL_VOID_INT_INT_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_int_int_fun(callback)); }\
	void call_##method(int i, int j) { (*signal)(i, j); }

#define SWIG_ADD_SIGNAL_VOID_INT_INT_INT_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_int_int_int_fun(callback)); }\
	void call_##method(int i, int j, int k) { (*signal)(i, j, k); }

#define SWIG_ADD_SIGNAL_VOID_INT_INT_INT_BOOL_CALLBACK(method, signal)\
  	void add_##method(PyObject *callback) { signal->connect(pycall_void_int_int_int_bool_fun(callback)); }\
  	void call_##method(int i, int j, int k, bool l) { (*signal)(i, j, k, l); }

#define SWIG_ADD_SIGNAL_VOID_NODE_INT_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_node_int_fun(callback)); }\
	void call_##method(mforms::TreeNodeRef i, int j) { (*signal)(i, j); }

#define SWIG_ADD_SIGNAL_VOID_TOOLBARITEM_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_toolbaritem_fun(callback)); }

#define SWIG_ADD_SIGNAL_BOOL_INT_CALLBACK(method, signal)\
        void add_##method(PyObject *callback) { signal->connect(pycall_bool_int_fun(callback)); }\
        bool call_##method(int i) { return *( (*signal)(i) ); }

#define SWIG_ADD_SIGNAL_VOID_ENTRYACTION_CALLBACK(method, signal)\
	void add_##method(PyObject *callback) { signal->connect(pycall_void_entryaction_fun(callback)); }

#define SWIG_ADD_SET_BOOL_CALLBACK(method, setter)\
        void method(PyObject *callback) { if (callback != Py_None) setter(pycall_bool_fun(callback)); else setter(boost::function<bool ()>()); }


#if 0
%init %{
  extern void init_mforms_bindings();
  init_mforms_bindings();
%}
#endif

%exception {
  try {
    $action
  }
  catch (std::exception &exc)
  {
    logError("exception calling mforms method $name: %s\n", exc.what());
    PyErr_Format(PyExc_SystemError, "Exception calling mforms method '$name': %s", exc.what());
    SWIG_fail;
  }
}

%feature("ref") Object "$this->retain();"
%feature("unref") Object "$this->release();"

%include std_string.i
%include std_vector.i

%typemap (in) int64_t, boost::int64_t = long long;
%typemap (out) int64_t, boost::int64_t = long long;

%typemap (in) ssize_t = long long;
%typemap (out) ssize_t = long long;

%typemap (in) size_t = unsigned long long;
%typemap (out) size_t = unsigned long long;

%typemap (out) base::Rect {
  $result = Py_BuildValue("(ffff)", $1.left(), $1.top(), $1.width(), $1.height());
}

%typemap(in) const std::string& {
  if (PyUnicode_Check($input))
  {
    PyObject *tmp = PyUnicode_AsUTF8String($input);
    $1 = new std::string(PyString_AsString(tmp));
    Py_DECREF(tmp);
  }
  else if (PyString_Check($input))
    $1 = new std::string(PyString_AsString($input));
  else
  {
    PyErr_SetString(PyExc_TypeError, "not a string");
    SWIG_fail;
  }
}

%typemap(freearg) const std::string& {
  delete $1;
}

%typemap(out) std::string {
   $result = PyUnicode_DecodeUTF8($1.data(), $1.size(), NULL);
}

%typemap(in) const std::list<std::string>& {
  if (PyList_Check($input)) {
    $1 = new std::list<std::string>();
    for (int c= PyList_Size($input), i= 0; i < c; i++)
    {
      PyObject *item = PyList_GetItem($input, i);
      if (PyUnicode_Check(item))
      {
        PyObject *tmp = PyUnicode_AsUTF8String(item);
        $1->push_back(PyString_AsString(tmp));
        Py_DECREF(tmp);
      }
      else if (PyString_Check(item))
        $1->push_back(PyString_AsString(item));
      else
      {
        delete $1;
        $1 = 0;
        SWIG_exception_fail(SWIG_TypeError, "expected list of strings");
      }
    }
  }
  else
    SWIG_exception_fail(SWIG_TypeError, "expected list of strings");
}

%typemap(out) std::vector<int> {
   $result = PyList_New(0);
   for (std::vector<int>::const_iterator iter = $1.begin(); iter != $1.end(); ++iter)
   {
     PyList_Append($result, PyInt_FromLong(*iter));
   }
}

%typemap(out) std::vector<size_t> {
   $result = PyList_New(0);
   for (std::vector<size_t>::const_iterator iter = $1.begin(); iter != $1.end(); ++iter)
   {
     PyList_Append($result, PyInt_FromLong(*iter));
   }
}

%typemap(in) const std::vector<size_t>& {
  if (PyList_Check($input)) {
    $1 = new std::vector<size_t>();
    for (int c= PyList_Size($input), i= 0; i < c; i++)
    {
      PyObject *item = PyList_GetItem($input, i);
      $1->push_back(PyInt_AsLong(item));
    }
  }
  else
    SWIG_exception_fail(SWIG_TypeError, "expected vector of size_t");
}

%typemap(out) std::pair<int, int> {
    $result = Py_BuildValue("(ii)", $1.first, $1.second);
}

%typemap(freearg) const std::list<std::string>& {
  delete $1;
}

%typemap(in) const std::vector<std::string>& {
  if (PyList_Check($input)) {
    $1 = new std::vector<std::string>();
    for (int c= PyList_Size($input), i= 0; i < c; i++)
    {
      PyObject *item = PyList_GetItem($input, i);
      if (PyUnicode_Check(item))
      {
        PyObject *tmp = PyUnicode_AsUTF8String(item);
        $1->push_back(PyString_AsString(tmp));
        Py_DECREF(tmp);
      }
      else if (PyString_Check(item))
        $1->push_back(PyString_AsString(item));
      else
      {
        delete $1;
        $1 = 0;
        SWIG_exception_fail(SWIG_TypeError, "expected vector of strings");
      }
    }
  }
  else
    SWIG_exception_fail(SWIG_TypeError, "expected vector of strings");
}


%typemap(freearg) const std::vector<std::string>& {
  delete $1;
}

%typemap(argout) std::string &ret_password {
    PyObject *o= PyUnicode_DecodeUTF8(($1)->data(), ($1)->size(), NULL);
    $result= SWIG_Python_AppendOutput($result, o);
}

%typemap(in,numinputs=0) std::string &ret_password(std::string temp) {
    $1 = &temp;
}

%typemap(argout) std::string &ret_value {
    PyObject *o= PyUnicode_DecodeUTF8(($1)->data(), ($1)->size(), NULL);
    $result= SWIG_Python_AppendOutput($result, o);
}

%typemap(in,numinputs=0) std::string &ret_value(std::string temp) {
    $1 = &temp;
}


%typemap(argout) bool &ret_store {
    if (*$1) Py_INCREF(Py_True); else Py_INCREF(Py_False);
    $result= SWIG_Python_AppendOutput($result, *$1 ? Py_True : Py_False);
}

%typemap(in,numinputs=0) bool &ret_store(bool temp) {
    temp = false;
    $1 = &temp;
}


%typemap(in,numinputs=0) int *w (int temp), int *h (int temp) {
    $1 = &temp;
}

%typemap(argout) (int *w, int *h) {
    $result = Py_BuildValue("(ii)", *$1, *$2);
}

%typemap(in,numinputs=0) int &retx (int temp), int &rety (int temp) {
    $1 = &temp;
}

%typemap(argout) (int &retx, int &rety) {
    $result = Py_BuildValue("(ii)", *$1, *$2);
}

%typemap(out) mforms::TreeNodeRef {
  if ($1)
    $result = SWIG_NewPointerObj((new mforms::TreeNodeRef($1)), SWIGTYPE_p_mforms__TreeNodeRef, SWIG_POINTER_OWN |  0 );
  else
  {
    $result = Py_None;
    Py_INCREF($result);
  }
}

%typemap(in) mforms::TreeNodeRef {
  if ($input == Py_None)
    $1 = mforms::TreeNodeRef();
  else
  {
    void *argp;
    int r = SWIG_ConvertPtr($input, &argp, SWIGTYPE_p_mforms__TreeNodeRef,  0  | 0);
    if (!SWIG_IsOK(r)) {
      SWIG_exception_fail(SWIG_ArgError(r), "in method '" "TreeView_select_node" "', argument of type '" "mforms::TreeNodeRef""'");
    }
    if (!argp) {
      SWIG_exception_fail(SWIG_ValueError, "invalid null reference " "in method '" "TreeView_select_node" "', argument " "2"" of type '" "mforms::TreeNodeRef""'");
    } else {
      mforms::TreeNodeRef * temp = reinterpret_cast< mforms::TreeNodeRef * >(argp);
      $1 = *temp;
      if (SWIG_IsNewObj(r)) delete temp;
    }
  }
}


%typemap(out) std::list<mforms::TreeNodeRef> {
   $result = PyList_New(0);
   for (std::list<mforms::TreeNodeRef>::const_iterator iter = $1.begin(); iter != $1.end(); ++iter)
   {
     PyObject *obj = SWIG_NewPointerObj((new mforms::TreeNodeRef(*iter)), SWIGTYPE_p_mforms__TreeNodeRef, SWIG_POINTER_OWN |  0 );
     PyList_Append($result, obj);
     Py_DECREF(obj);
   }
}


%include std_list.i

namespace std {
%template(doubleList) list<double>;
};

%typemap(in) std::list<double> {
    if (PyList_Check($input)) {
    for (int c= PyList_Size($input), i= 0; i < c; i++)
    {
      PyObject *item = PyList_GetItem($input, i);
      if (PyFloat_Check(item))
        $1.push_back(PyFloat_AsDouble(item));
      else if (PyInt_Check(item))
        $1.push_back(PyInt_AsLong(item));
      else
      {
        SWIG_exception_fail(SWIG_TypeError, "expected list of doubles");
      }
    }
  }
  else
    SWIG_exception_fail(SWIG_TypeError, "expected list of doubles");
}




%pythoncode %{
# flake8: noqa

def newLabel(*args):
    c = Label(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newButton(*args):
    c = Button(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newCheckBox(*args):
    c = CheckBox(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newCodeEditor(*args):
    c = CodeEditor(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newTextEntry(*args):
    c = TextEntry(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newTextBox(*args):
    c = TextBox(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newSelector(*args):
    c = Selector(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newListBox(*args):
    c = ListBox(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newTabView(*args):
    c = TabView(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newBox(*args):
    c = Box(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newPanel(*args):
    c = Panel(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newFileChooser(*args):
    c = FileChooser(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newRadioButton(*args):
    c = RadioButton(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newImageBox(*args):
    c = ImageBox(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newProgressBar(*args):
    c = ProgressBar(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newTable(*args):
    c = Table(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newScrollPanel(*args):
    c = ScrollPanel(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newTreeView(*args):
    c = TreeView(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newAppView(*args):
    c = AppView(*args)
    return c

def newDrawBox(*args):
    c = DrawBox(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newTabSwitcher(*args):
    c = TabSwitcher(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newSectionBox(*args):
    c = SectionBox(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newWidgetSeparator(*args):
    c = WidgetSeparator(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newHeartbeatWidget(*args):
    c = HeartbeatWidget(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newServerStatusWidget(*args):
    c = ServerStatusWidget(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newBarGraphWidget(*args):
    c = BarGraphWidget(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newLineDiagramWidget(*args):
    c = LineDiagramWidget(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newPopup(*args):
    c = Popup(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newPopover(*args):
    c = Popover(*args)
    c.set_managed()
    c.release()
    return c

def newSplitter(*args):
    c = Splitter(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newTaskSidebar(*args):
    c = TaskSidebar.create(*args)
    c.set_managed()
    c.set_release_on_add()
    return c


def newHyperText(*args):
    c = HyperText(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newContextMenu(*args):
    c = ContextMenu(*args)
    c.set_managed()
    return c

def newMenuItem(*args):
    c = MenuItem(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

def newToolBar(*args):
    c = ToolBar(*args)
    c.set_managed()
    return c

def newToolBarItem(*args):
    c = ToolBarItem(*args)
    c.set_managed()
    c.set_release_on_add()
    return c

%}

%include "../mforms/base.h"
%include "../mforms/view.h"
%include "../mforms/container.h"
%include "../mforms/form.h"
%include "../mforms/button.h"
%include "../mforms/checkbox.h"
%include "../mforms/textentry.h"
%include "../mforms/textbox.h"
%include "../mforms/label.h"
%include "../mforms/selector.h"
%include "../mforms/listbox.h"
%include "../mforms/tabview.h"
%include "../mforms/box.h"
%include "../mforms/panel.h"
%include "../mforms/toolbar.h"
%include "../mforms/filechooser.h"
%include "../mforms/radiobutton.h"
%include "../mforms/imagebox.h"
%include "../mforms/progressbar.h"
%include "../mforms/table.h"
%include "../mforms/scrollpanel.h"
%include "../mforms/wizard.h"
%include "../mforms/utilities.h"
%include "../mforms/appview.h"
%include "../mforms/dockingpoint.h"
%include "../mforms/app.h"
%include "../mforms/drawbox.h"
%include "../mforms/tabswitcher.h"
%include "../mforms/sectionbox.h"
%include "../mforms/widgets.h"
%include "../mforms/popup.h"
%include "../mforms/popover.h"
%include "../mforms/menubar.h"
%include "../mforms/splitter.h"
%include "../mforms/code_editor.h"
%include "../mforms/task_sidebar.h"
%include "../mforms/hypertext.h"
%include "../mforms/treeview.h"

%include "../../base/base/common.h"
%include "../../base/base/drawing.h"

%include "mforms_grt.h"
%include "mforms_drawbox.h"

%warnfilter(362) signal_connection_wrapper;

// Extend classes to include callback support

%extend mforms::Button {
SWIG_ADD_SIGNAL_VOID_CALLBACK(clicked_callback, self->signal_clicked());
}

%extend mforms::ListBox {
SWIG_ADD_SIGNAL_VOID_CALLBACK(changed_callback, self->signal_changed());
}

%extend mforms::CodeEditor {
SWIG_ADD_SIGNAL_VOID_INT_INT_INT_BOOL_CALLBACK(changed_callback, self->signal_changed());
}

%extend mforms::TreeView {
SWIG_ADD_SIGNAL_VOID_NODE_INT_CALLBACK(activated_callback, self->signal_node_activated());
SWIG_ADD_SIGNAL_VOID_CALLBACK(changed_callback, self->signal_changed());
SWIG_ADD_SIGNAL_VOID_INT_CALLBACK(column_resized_callback, self->signal_column_resized());

void set_cell_edited_callback(PyObject *callable)
{
  self->set_cell_edit_handler(boost::bind(call_cell_node_edited_pycallable, _1, _2, _3, PyObjectRef(callable)));
}
}


%extend mforms::RadioButton {
SWIG_ADD_SIGNAL_VOID_CALLBACK(clicked_callback, self->signal_clicked());
}

%extend mforms::Selector {
SWIG_ADD_SIGNAL_VOID_CALLBACK(changed_callback, self->signal_changed());
}

%extend mforms::Form {
SWIG_ADD_SET_BOOL_CALLBACK(set_on_close, self->set_on_close);
SWIG_ADD_SIGNAL_VOID_CALLBACK(closed_callback, self->signal_closed());
SWIG_ADD_SIGNAL_VOID_CALLBACK(activated_callback, self->signal_activated());
SWIG_ADD_SIGNAL_VOID_CALLBACK(deactivated_callback, self->signal_deactivated());
}

%extend mforms::TextBox {
SWIG_ADD_SIGNAL_VOID_CALLBACK(changed_callback, self->signal_changed());
 void append_text_and_scroll(const std::string &text, bool scroll_to_end)
 {
   self->append_text(text, scroll_to_end);
 }
}

%extend mforms::TextEntry {
SWIG_ADD_SIGNAL_VOID_CALLBACK(changed_callback, self->signal_changed());
SWIG_ADD_SIGNAL_VOID_ENTRYACTION_CALLBACK(action_callback, self->signal_action());
}

%extend mforms::Wizard {
SWIG_ADD_SIGNAL_VOID_CALLBACK(next_callback, self->signal_next_clicked());
SWIG_ADD_SIGNAL_VOID_CALLBACK(back_callback, self->signal_back_clicked());
SWIG_ADD_SIGNAL_VOID_CALLBACK(extra_callback, self->signal_extra_clicked());
}

%extend mforms::AppView {
SWIG_ADD_SET_BOOL_CALLBACK(on_close, self->set_on_close);
}

%extend mforms::TabView {
SWIG_ADD_SIGNAL_VOID_CALLBACK(tab_changed_callback, self->signal_tab_changed());
SWIG_ADD_SIGNAL_BOOL_INT_CALLBACK(tab_closing_callback, self->signal_tab_closing());
}

%extend mforms::TabSwitcher {
SWIG_ADD_SIGNAL_VOID_CALLBACK(changed_callback, self->signal_changed());
}


%extend mforms::Utilities {
static mforms::TimeoutHandle add_timeout(float interval, PyObject *callback) { return mforms::Utilities::add_timeout(interval, (pycall_bool_fun(callback))); }

static void perform_from_main_thread(PyObject *callable, bool wait) {
  WillLeavePython gil;
  mforms::Utilities::perform_from_main_thread(pycall_ignoreret_voidptr_fun(callable), wait); }
}

%extend mforms::MenuBase {
  mforms::MenuItem *add_item_with_title(const std::string &title, PyObject *callable, const std::string &name = "", const std::string &internalName = "")
  {
     return self->add_item_with_title(title, pycall_void_fun(callable), name, internalName);
  }

  mforms::MenuItem *add_check_item_with_title(const std::string &title, PyObject *callable, const std::string &name, const std::string &internalName)
  {
     return self->add_check_item_with_title(title, pycall_void_fun(callable), name, internalName);
  }
}

%extend mforms::MenuItem {
SWIG_ADD_SIGNAL_VOID_CALLBACK(clicked_callback, self->signal_clicked());
}

%extend mforms::ContextMenu {
SWIG_ADD_SIGNAL_VOID_MENUITEM_CALLBACK(will_show_callback, self->signal_will_show());
}


%extend mforms::TaskSidebar {
SWIG_ADD_SIGNAL_VOID_STRING_CALLBACK(on_section_command_callback, self->on_section_command());
}


%extend mforms::HyperText {
BOOST_ADD_SIGNAL_VOID_STRING_CALLBACK(link_click_callback, self->signal_link_click());
}

%extend mforms::ToolBarItem {
SWIG_ADD_SIGNAL_VOID_TOOLBARITEM_CALLBACK(activated_callback, self->signal_activated());
}

%extend mforms::Popover {
SWIG_ADD_SIGNAL_VOID_CALLBACK(close_callback, self->signal_close());
}

