/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "base/trackable.h"
#include <cstring>
#include "grt.h"
#include <typeinfo>

#include <gmodule.h>

namespace grt {

  //----------------------------------------------------------------------
  // C++ Module Support

  template <class C>
  struct traits {
    typedef C Type;
  };
  template <class C>
  struct traits<const C &> {
    typedef C Type;
  };
  template <class C>
  struct traits<C &> {
    typedef C Type;
  };

  template <class T>
  struct grt_content_type {
    static const Type id = UnknownType;
  };
  template <>
  struct grt_content_type<ListRef<internal::Integer> > {
    static const Type id = IntegerType;
  };
  template <>
  struct grt_content_type<ListRef<internal::Double> > {
    static const Type id = DoubleType;
  };
  template <>
  struct grt_content_type<ListRef<internal::String> > {
    static const Type id = StringType;
  };
  template <class T>
  struct grt_content_type<ListRef<T> > {
    static const Type id = ObjectType;
  };

  template <class T>
  struct grt_content_class_name {
    typedef T value;
  };
  template <class T>
  struct grt_content_class_name<Ref<T> > {
    typedef T value;
  };
  template <class T>
  struct grt_content_class_name<ListRef<T> > {
    typedef T value;
  };

  template <class T>
  struct grt_type_for_native {
    typedef T Type;
  };

#ifdef DEFINE_INT_FUNCTIONS
  template <>
  struct grt_type_for_native<int> {
    typedef IntegerRef Type;
  };
#endif

#ifndef DEFINE_INT_FUNCTIONS
  template <>
  struct grt_type_for_native<long int> {
    typedef IntegerRef Type;
  };
#endif

#ifdef DEFINE_UINT64_T_FUNCTIONS
  template <>
  struct grt_type_for_native<uint64_t> {
    typedef IntegerRef Type;
  };
#endif
  template <>
  struct grt_type_for_native<size_t> {
    typedef IntegerRef Type;
  };
#ifdef DEFINE_SSIZE_T_FUNCTIONS
  template <>
  struct grt_type_for_native<ssize_t> {
    typedef IntegerRef Type;
  };
#endif
  template <>
  struct grt_type_for_native<double> {
    typedef DoubleRef Type;
  };
  template <>
  struct grt_type_for_native<const std::string &> {
    typedef StringRef Type;
  };
  template <>
  struct grt_type_for_native<std::string> {
    typedef StringRef Type;
  };

  template <class T>
  struct grt_class_name_if_object {
    static std::string get() {
      return T::static_class_name();
    }
  };

  template <>
  struct grt_class_name_if_object<IntegerRef> {
    static std::string get() {
      return "";
    }
  };

  template <>
  struct grt_class_name_if_object<DoubleRef> {
    static std::string get() {
      return "";
    }
  };

  template <>
  struct grt_class_name_if_object<StringRef> {
    static std::string get() {
      return "";
    }
  };

  template <>
  struct grt_class_name_if_object<DictRef> {
    static std::string get() {
      return "";
    }
  };

  template <class T>
  struct grt_class_name_if_object<ListRef<T> > {
    static std::string get() {
      return "";
    }
  };

  template <class T, bool B>
  struct grt_content_struct_name {
    static std::string get() {
      return "";
    }
  };

  template <class T>
  struct grt_content_struct_name<T, true> {
    static std::string get() {
      return T::static_class_name();
    }
  };

  template <class T>
  struct grt_content_struct_name<Ref<T>, true> {
    static std::string get() {
      return T::static_class_name();
    }
  };

  template <class T>
  struct grt_content_struct_name<ListRef<T>, true> {
    static std::string get() {
      return T::static_class_name();
    }
  };

  // This template checks if B is cast-able to A
  // boolean value Is_super_subclass<A, B>::value
  // is available during compile time and can be used with other templates
  template <class A, class B>
  class Is_super_subclass {
    static B *makeB();
    struct CharSized {
      char c;
    };
    struct Char2Sized {
      char c, d;
    };
    static CharSized selector(...);
    static Char2Sized selector(A *);

  public:
    static const bool value = (sizeof(selector(makeB())) == sizeof(Char2Sized));
  };

  // Allows implementation of modules in C++ by sub classing

  //----------------------------------------------------------------------
  // Basic definitions for modules and interfaces
  typedef std::vector<std::string> InterfaceList;
  class ModuleFunctorBase;

  // this base class is only used by InterfaceImplBase and ModuleImplBase
  // all other subclasses only need to inherit from one of these 2
  class InterfaceData {
  public:
    InterfaceList _implemented_interfaces;

    virtual ~InterfaceData(){};
  };

  class InterfaceImplBase : virtual public InterfaceData {
  public:
    template <class InterfaceClass>
    static void Register() {
      InterfaceClass::register_interface();
    }

    virtual ~InterfaceImplBase(){};
  };

  class ModuleFunctorBase;
  class CPPModuleLoader;

  class MYSQLGRT_PUBLIC Interface : public Module {
  public:
    static Interface *create(const char *name, ...);

    bool check_conformance(const Module *module) const;

  private:
    Interface(CPPModuleLoader *loader);
  };

  class MYSQLGRT_PUBLIC CPPModule : virtual public InterfaceData, public Module, public base::trackable {
    friend class CPPModuleLoader;

  public:
    typedef CPPModuleLoader Loader;
    virtual ~CPPModule();

    virtual std::string get_module_datadir();

    std::string get_resource_file_path(const std::string &file);

    void set_name(const std::string &name);


  protected:
    CPPModule(CPPModuleLoader *loader);

    GModule *_gmodule;
    std::list<ModuleFunctorBase *> _functors;

    virtual void init_module() = 0;
    virtual void initialization_done(){};

    void register_functions(ModuleFunctorBase *first, ...);

    virtual void closeModule() noexcept override;
    virtual GModule* getModule() const override;
  };

  typedef CPPModule ModuleImplBase;

  class MYSQLGRT_PUBLIC CPPModuleLoader : public ModuleLoader {
  public:
    CPPModuleLoader();
    virtual ~CPPModuleLoader();

    virtual bool load_library(const std::string &path) {
      return false;
    }
    virtual bool run_script_file(const std::string &path) {
      return false;
    }
    virtual bool run_script(const std::string &script) {
      return false;
    }
    virtual bool check_file_extension(const std::string &path);

    virtual std::string get_loader_name() {
      return "cpp";
    }

    virtual Module *init_module(const std::string &path);
    virtual void refresh();
  };

//--------------------------------------------------------------------------------
// For Modules Implemented in C++

// this must be in interface classes to enable their registration
#define DECLARE_REGISTER_INTERFACE(the_class, ...)                                                    \
  the_class() {                                                                                       \
    std::string name = grt::get_type_name(typeid(*this));                                             \
    _implemented_interfaces.push_back(name.substr(0, name.length() - 4)); /* truncate Impl part*/     \
  }                                                                                                   \
  static void register_interface() {                                                                  \
    std::string name = grt::get_type_name(typeid(the_class));                                         \
    grt::GRT::get()->register_new_interface(grt::Interface::create(name.c_str(), __VA_ARGS__, NULL)); \
  }

// this must be put in the public section of the modules class
#define DEFINE_INIT_MODULE(VERSION, AUTHOR, parent_class, first_function, ...)\
  virtual void init_module() override \
  {\
    set_name(grt::get_type_name(typeid(*this)));\
    _meta_version= VERSION; _meta_author= AUTHOR;\
    _extends= typeid(parent_class) == typeid(grt::CPPModule) ? "" : grt::get_type_name(typeid(parent_class));\
    if (g_str_has_suffix(_extends.c_str(), "Impl"))\
      _extends= _extends.substr(0, _extends.length()-4);\
    register_functions(first_function, __VA_ARGS__, NULL);\
    initialization_done();\
  }

#define DEFINE_INIT_MODULE_DOC(VERSION, AUTHOR, DOC, parent_class, first_function, ...)\
  virtual void init_module() override \
  {\
    set_name(grt::get_type_name(typeid(*this)));\
    _meta_version= VERSION; _meta_author= AUTHOR; _meta_description= DOC;\
    _extends= typeid(parent_class) == typeid(grt::CPPModule) ? "" : grt::get_type_name(typeid(parent_class));\
    if (g_str_has_suffix(_extends.c_str(), "Impl"))\
      _extends= _extends.substr(0, _extends.length()-4);\
    register_functions(first_function, __VA_ARGS__, NULL);\
    initialization_done();\
  }

  template <class T_arg>
  ArgSpec &get_param_info(const char *argdoc = 0, int i = 0) {
    static ArgSpec p;

    if (argdoc && *argdoc) {
      const char *line_end;
      while ((line_end = strchr(argdoc, '\n')) && i > 0) {
        argdoc = line_end + 1;
        i--;
      }

      if (i == 0) {
        const char *s = strchr(argdoc, ' ');
        if (s && (!line_end || s < line_end)) {
          p.name = std::string(argdoc, s - argdoc);
          p.doc = line_end ? std::string(s + 1, line_end - s - 1) : std::string(s + 1);
        } else {
          p.name = line_end ? std::string(argdoc, line_end - argdoc) : std::string(argdoc);
          p.doc = "";
        }
      } else
        throw std::logic_error("Module function argument documentation has wrong number of items");
    } else {
      p.name = "";
      p.doc = "";
    }

    p.type.base.type = grt_type_for_native<T_arg>::Type::RefType::static_type();

    if (p.type.base.type == ObjectType) {
      if (typeid(T_arg) != typeid(internal::Object)) {
        const bool castable_to_object_value =
        Is_super_subclass<internal::Object, typename grt_content_class_name<T_arg>::value>::value;
        p.type.base.object_class =
        grt_content_struct_name<typename grt_content_class_name<T_arg>::value, castable_to_object_value>::get();
      }
    } else if (p.type.base.type == ListType) {
      p.type.content.type = grt_content_type<T_arg>::id;
      if (p.type.content.type == ObjectType) {
        const bool castable_to_object_value =
        Is_super_subclass<internal::Object, typename grt_content_class_name<T_arg>::value>::value;
        p.type.content.object_class =
        grt_content_struct_name<typename grt_content_class_name<T_arg>::value, castable_to_object_value>::get();
      }
    } else if (p.type.base.type == DictType) {
      p.type.content.type = AnyType;
    }

    return p;
  }

  class ModuleFunctorBase {
  protected:
    TypeSpec _return_type;

  public:
    typedef std::vector<ArgSpec> Function_param_list;

  protected:
    const char *_name;
    const char *_doc;
    const char *_argdoc;
    Function_param_list _signature;

  public:
    explicit ModuleFunctorBase(const char *name, const char *doc = "", const char *argdoc = "")
      : _doc(doc ? doc : ""), _argdoc(argdoc ? argdoc : "") {
      const char *c = strrchr(name, ':');
      if (c == NULL)
        c = name;
      else
        c++;
      _name = c;
    }
    virtual ~ModuleFunctorBase(){};

    const char *get_name() const {
      return _name;
    }
    const char *get_doc() const {
      return _doc;
    }
    const Function_param_list &get_signature() const {
      return _signature;
    }
    const TypeSpec &get_return_type() const {
      return _return_type;
    }

    virtual ValueRef perform_call(const BaseListRef &arglist) = 0;

    /*QQQ
    MYX_GRT_ERROR call(const BaseListRef &arglist, ValueRef &result)
    {
      try
      {
        result= perform_call(arglist);
        //if (result_value) // NULL return values are valid --alfredo
        //{
          *result= myx_grt_dict_new(NULL);
          myx_grt_dict_item_set_value(*result, "value", result_value);
          if (result_value)
            myx_grt_value_release(result_value);
        //}
        return error;
      }
      catch (const grt_runtime_error &exc)
      {
        *result= make_error(NULL, exc.what(), exc.detail());
      }
      catch (const std::exception &exc)
      {
        *result= make_error(NULL, exc.what(), "");
      }
      catch (...)
      {
        *result= make_error(NULL, "exception during function execution", "");
      }
      return MYX_GRT_FUNCTION_CALL_ERROR;
    }*/
  };

  /**
    grt_value_for_type is needed to ensure that the type we get
    (on it's output) is of a subclass of GenereicValue i.e.
    is a wrapped class
  */

  template <class T>
  ValueRef grt_value_for_type(const T &t) {
    return t;
  }

  inline ValueRef grt_value_for_type(bool t) {
    return IntegerRef(t);
  }

#ifdef DEFINE_INT_FUNCTIONS
  inline ValueRef grt_value_for_type(int t) {
    return IntegerRef(t);
  }
#endif

#ifndef DEFINE_INT_FUNCTIONS
  inline ValueRef grt_value_for_type(long int t) {
    return IntegerRef(t);
  }
#endif

#ifdef DEFINE_UINT64_T_FUNCTIONS
  inline ValueRef grt_value_for_type(uint64_t t) {
    return IntegerRef((size_t)t);
  }
#endif

  inline ValueRef grt_value_for_type(size_t t) {
    return IntegerRef(t);
  }

#ifdef DEFINE_SSIZE_T_FUNCTIONS
  inline ValueRef grt_value_for_type(ssize_t t) {
    return IntegerRef(t);
  }
#endif

  inline ValueRef grt_value_for_type(double t) {
    return DoubleRef(t);
  }

  inline ValueRef grt_value_for_type(const std::string &t) {
    return StringRef(t);
  }

  /**
    native_value_for_grt_type converts a value of ValueRef type to type T.
    T can be either a subclass of ValueRef or a simple type for with
    specialization Value<T> is available
  */

  template <typename T>
  struct native_value_for_grt_type {
    static T convert(const ValueRef &t) {
      return T::cast_from(t);
    }
  };

  template <>
  struct native_value_for_grt_type<IntegerRef> {
    static ssize_t convert(const ValueRef &t) {
      return IntegerRef::cast_from(t).operator IntegerRef::storage_type();
    }
  };

  template <>
  struct native_value_for_grt_type<int> {
    static int convert(const ValueRef &t) {
      return (int)IntegerRef::cast_from(t).operator IntegerRef::storage_type();
    }
  };

  template <>
  struct native_value_for_grt_type<bool> {
    static bool convert(const ValueRef &t) {
      return IntegerRef::cast_from(t).operator IntegerRef::storage_type() != 0;
    }
  };

#ifdef _WIN64
  template <>
  struct native_value_for_grt_type<ssize_t> {
    static ssize_t convert(const ValueRef &t) {
      return IntegerRef::cast_from(t).operator IntegerRef::storage_type();
    }
  };
#endif

  template <>
  struct native_value_for_grt_type<size_t> {
    static size_t convert(const ValueRef &t) {
      return IntegerRef::cast_from(t).operator IntegerRef::storage_type();
    }
  };

  template <>
  struct native_value_for_grt_type<DoubleRef> {
    static double convert(const ValueRef &t) {
      return DoubleRef::cast_from(t).operator double();
    }
  };

  template <>
  struct native_value_for_grt_type<double> {
    static double convert(const ValueRef &t) {
      return DoubleRef::cast_from(t).operator double();
    }
  };

  template <>
  struct native_value_for_grt_type<StringRef> {
    static std::string convert(const ValueRef &t) {
      if (t.is_valid())
        return StringRef::cast_from(t).operator std::string();
      throw std::invalid_argument("invalid null argument");
      return "";
    }
  };

  template <>
  struct native_value_for_grt_type<std::string> {
    static std::string convert(const ValueRef &t) {
      if (t.is_valid())
        return StringRef::cast_from(t).operator std::string();
      throw std::invalid_argument("invalid null argument");
      return "";
    }
  };

  template <class T_ret, class T_obj>
  class ModuleFunctor0 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)();
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &arglist) {
      return grt_value_for_type((_obj->*_funcptr)());
    }

  public:
    ModuleFunctor0(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc, argdoc), _funcptr(func), _obj(obj) {
      _return_type = get_param_info<T_ret>().type;
    }
  };

  template <class T_ret, class T_obj, class T_arg1>
  class ModuleFunctor1 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)(T_arg1);
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &args) {
      //    BaseListRef args(arglist);
      T_arg1 arg1 = native_value_for_grt_type<typename traits<T_arg1>::Type>::convert(args[0]);
      return grt_value_for_type((_obj->*_funcptr)(arg1));
    }

  public:
    ModuleFunctor1(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc, argdoc), _funcptr(func), _obj(obj) {
      _signature.push_back(get_param_info<typename traits<T_arg1>::Type>(argdoc, 0));
      _return_type = get_param_info<typename traits<T_ret>::Type>().type;
    }
  };

  template <class T_ret, class T_obj, class T_arg1, class T_arg2>
  class ModuleFunctor2 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)(T_arg1, T_arg2);
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &args) {
      //    BaseListRef args(arglist);
      T_arg1 arg1 = native_value_for_grt_type<typename traits<T_arg1>::Type>::convert(args[0]);
      T_arg2 arg2 = native_value_for_grt_type<typename traits<T_arg2>::Type>::convert(args[1]);
      return grt_value_for_type((_obj->*_funcptr)(arg1, arg2));
    }

  public:
    ModuleFunctor2(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc), _funcptr(func), _obj(obj) {
      _signature.push_back(get_param_info<typename traits<T_arg1>::Type>(argdoc, 0));
      _signature.push_back(get_param_info<typename traits<T_arg2>::Type>(argdoc, 1));
      _return_type = get_param_info<typename traits<T_ret>::Type>().type;
    }
  };

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3>
  class ModuleFunctor3 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)(T_arg1, T_arg2, T_arg3);
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &args) {
      //    BaseListRef args(arglist);
      T_arg1 arg1 = native_value_for_grt_type<typename traits<T_arg1>::Type>::convert(args[0]);
      T_arg2 arg2 = native_value_for_grt_type<typename traits<T_arg2>::Type>::convert(args[1]);
      T_arg3 arg3 = native_value_for_grt_type<typename traits<T_arg3>::Type>::convert(args[2]);
      return grt_value_for_type((_obj->*_funcptr)(arg1, arg2, arg3));
    }

  public:
    ModuleFunctor3(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc, argdoc), _funcptr(func), _obj(obj) {
      _signature.push_back(get_param_info<typename traits<T_arg1>::Type>(argdoc, 0));
      _signature.push_back(get_param_info<typename traits<T_arg2>::Type>(argdoc, 1));
      _signature.push_back(get_param_info<typename traits<T_arg3>::Type>(argdoc, 2));
      _return_type = get_param_info<typename traits<T_ret>::Type>().type;
    }
  };

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4>
  class ModuleFunctor4 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)(T_arg1, T_arg2, T_arg3, T_arg4);
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &args) {
      //    BaseListRef args(arglist);
      T_arg1 arg1 = native_value_for_grt_type<typename traits<T_arg1>::Type>::convert(args[0]);
      T_arg2 arg2 = native_value_for_grt_type<typename traits<T_arg2>::Type>::convert(args[1]);
      T_arg3 arg3 = native_value_for_grt_type<typename traits<T_arg3>::Type>::convert(args[2]);
      T_arg4 arg4 = native_value_for_grt_type<typename traits<T_arg4>::Type>::convert(args[3]);
      return grt_value_for_type((_obj->*_funcptr)(arg1, arg2, arg3, arg4));
    }

  public:
    ModuleFunctor4(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc, argdoc), _funcptr(func), _obj(obj) {
      _signature.push_back(get_param_info<typename traits<T_arg1>::Type>(argdoc, 0));
      _signature.push_back(get_param_info<typename traits<T_arg2>::Type>(argdoc, 1));
      _signature.push_back(get_param_info<typename traits<T_arg3>::Type>(argdoc, 2));
      _signature.push_back(get_param_info<typename traits<T_arg4>::Type>(argdoc, 3));
      _return_type = get_param_info<typename traits<T_ret>::Type>().type;
    }
  };

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5>
  class ModuleFunctor5 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5);
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &args) {
      //    BaseListRef args(arglist);
      T_arg1 arg1 = native_value_for_grt_type<typename traits<T_arg1>::Type>::convert(args[0]);
      T_arg2 arg2 = native_value_for_grt_type<typename traits<T_arg2>::Type>::convert(args[1]);
      T_arg3 arg3 = native_value_for_grt_type<typename traits<T_arg3>::Type>::convert(args[2]);
      T_arg4 arg4 = native_value_for_grt_type<typename traits<T_arg4>::Type>::convert(args[3]);
      T_arg5 arg5 = native_value_for_grt_type<typename traits<T_arg5>::Type>::convert(args[4]);
      return grt_value_for_type((_obj->*_funcptr)(arg1, arg2, arg3, arg4, arg5));
    }

  public:
    ModuleFunctor5(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc, argdoc), _funcptr(func), _obj(obj) {
      _signature.push_back(get_param_info<typename traits<T_arg1>::Type>(argdoc, 0));
      _signature.push_back(get_param_info<typename traits<T_arg2>::Type>(argdoc, 1));
      _signature.push_back(get_param_info<typename traits<T_arg3>::Type>(argdoc, 2));
      _signature.push_back(get_param_info<typename traits<T_arg4>::Type>(argdoc, 3));
      _signature.push_back(get_param_info<typename traits<T_arg5>::Type>(argdoc, 4));
      _return_type = get_param_info<typename traits<T_ret>::Type>().type;
    }
  };

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5,
            class T_arg6>
  class ModuleFunctor6 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6);
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &args) {
      //    BaseListRef args(arglist);
      T_arg1 arg1 = native_value_for_grt_type<typename traits<T_arg1>::Type>::convert(args[0]);
      T_arg2 arg2 = native_value_for_grt_type<typename traits<T_arg2>::Type>::convert(args[1]);
      T_arg3 arg3 = native_value_for_grt_type<typename traits<T_arg3>::Type>::convert(args[2]);
      T_arg4 arg4 = native_value_for_grt_type<typename traits<T_arg4>::Type>::convert(args[3]);
      T_arg5 arg5 = native_value_for_grt_type<typename traits<T_arg5>::Type>::convert(args[4]);
      T_arg6 arg6 = native_value_for_grt_type<typename traits<T_arg6>::Type>::convert(args[5]);
      return grt_value_for_type((_obj->*_funcptr)(arg1, arg2, arg3, arg4, arg5, arg6));
    }

  public:
    ModuleFunctor6(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc, argdoc), _funcptr(func), _obj(obj) {
      _signature.push_back(get_param_info<typename traits<T_arg1>::Type>(argdoc, 0));
      _signature.push_back(get_param_info<typename traits<T_arg2>::Type>(argdoc, 1));
      _signature.push_back(get_param_info<typename traits<T_arg3>::Type>(argdoc, 2));
      _signature.push_back(get_param_info<typename traits<T_arg4>::Type>(argdoc, 3));
      _signature.push_back(get_param_info<typename traits<T_arg5>::Type>(argdoc, 4));
      _signature.push_back(get_param_info<typename traits<T_arg6>::Type>(argdoc, 5));
      _return_type = get_param_info<typename traits<T_ret>::Type>().type;
    }
  };

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5,
            class T_arg6, class T_arg7>
  class ModuleFunctor7 : public ModuleFunctorBase {
    typedef T_ret (T_obj::*function_type)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6, T_arg7);
    function_type _funcptr;
    T_obj *_obj;

  protected:
    virtual ValueRef perform_call(const BaseListRef &args) {
      //    BaseListRef args(arglist);
      T_arg1 arg1 = native_value_for_grt_type<typename traits<T_arg1>::Type>::convert(args[0]);
      T_arg2 arg2 = native_value_for_grt_type<typename traits<T_arg2>::Type>::convert(args[1]);
      T_arg3 arg3 = native_value_for_grt_type<typename traits<T_arg3>::Type>::convert(args[2]);
      T_arg4 arg4 = native_value_for_grt_type<typename traits<T_arg4>::Type>::convert(args[3]);
      T_arg5 arg5 = native_value_for_grt_type<typename traits<T_arg5>::Type>::convert(args[4]);
      T_arg6 arg6 = native_value_for_grt_type<typename traits<T_arg6>::Type>::convert(args[5]);
      T_arg7 arg7 = native_value_for_grt_type<typename traits<T_arg7>::Type>::convert(args[6]);
      return grt_value_for_type((_obj->*_funcptr)(arg1, arg2, arg3, arg4, arg5, arg6, arg7));
    }

  public:
    ModuleFunctor7(const char *name, T_obj *obj, function_type func, const char *doc = "", const char *argdoc = "")
      : ModuleFunctorBase(name, doc, argdoc), _funcptr(func), _obj(obj) {
      _signature.push_back(get_param_info<typename traits<T_arg1>::Type>(argdoc, 0));
      _signature.push_back(get_param_info<typename traits<T_arg2>::Type>(argdoc, 1));
      _signature.push_back(get_param_info<typename traits<T_arg3>::Type>(argdoc, 2));
      _signature.push_back(get_param_info<typename traits<T_arg4>::Type>(argdoc, 3));
      _signature.push_back(get_param_info<typename traits<T_arg5>::Type>(argdoc, 4));
      _signature.push_back(get_param_info<typename traits<T_arg6>::Type>(argdoc, 5));
      _signature.push_back(get_param_info<typename traits<T_arg7>::Type>(argdoc, 6));
      _return_type = get_param_info<typename traits<T_ret>::Type>().type;
    }
  };

  // module functor wrappers (0 - 7 args)

  template <class T_ret, class T_obj>
  ModuleFunctor0<T_ret, T_obj> *module_fun(T_obj *obj, T_ret (T_obj::*func)(), const char *name, const char *doc = "",
                                           const char *argdoc = "") {
    return new ModuleFunctor0<T_ret, T_obj>(name, obj, func, doc, argdoc);
  }

  template <class T_ret, class T_obj, class T_arg1>
  ModuleFunctor1<T_ret, T_obj, T_arg1> *module_fun(T_obj *obj, T_ret (T_obj::*func)(T_arg1), const char *name,
                                                   const char *doc = "", const char *argdoc = "") {
    return new ModuleFunctor1<T_ret, T_obj, T_arg1>(name, obj, func, doc, argdoc);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2>
  ModuleFunctor2<T_ret, T_obj, T_arg1, T_arg2> *module_fun(T_obj *obj, T_ret (T_obj::*func)(T_arg1, T_arg2),
                                                           const char *name, const char *doc = "",
                                                           const char *argdoc = "") {
    return new ModuleFunctor2<T_ret, T_obj, T_arg1, T_arg2>(name, obj, func, doc, argdoc);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3>
  ModuleFunctor3<T_ret, T_obj, T_arg1, T_arg2, T_arg3> *module_fun(T_obj *obj,
                                                                   T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3),
                                                                   const char *name, const char *doc = "",
                                                                   const char *argdoc = "") {
    return new ModuleFunctor3<T_ret, T_obj, T_arg1, T_arg2, T_arg3>(name, obj, func, doc, argdoc);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4>
  ModuleFunctor4<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4> *module_fun(
    T_obj *obj, T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3, T_arg4), const char *name, const char *doc = "",
    const char *argdoc = "") {
    return new ModuleFunctor4<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4>(name, obj, func, doc, argdoc);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5>
  ModuleFunctor5<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5> *module_fun(
    T_obj *obj, T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5), const char *name, const char *doc = "",
    const char *argdoc = "") {
    return new ModuleFunctor5<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5>(name, obj, func, doc, argdoc);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5,
            class T_arg6>
  ModuleFunctor6<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6> *module_fun(
    T_obj *obj, T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6), const char *name,
    const char *doc = "", const char *argdoc = "") {
    return new ModuleFunctor6<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6>(name, obj, func, doc,
                                                                                            argdoc);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5,
            class T_arg6, class T_arg7>
  ModuleFunctor7<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6, T_arg7> *module_fun(
    T_obj *obj, T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6, T_arg7), const char *name,
    const char *doc = "", const char *argdoc = "") {
    return new ModuleFunctor7<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6, T_arg7>(name, obj, func,
                                                                                                    doc, argdoc);
  }

  // interface functor wrappers (0 - 7 args)

  template <class T_ret, class T_obj>
  ModuleFunctor0<T_ret, T_obj> *interface_fun(T_ret (T_obj::*func)(), const char *name) {
    return new ModuleFunctor0<T_ret, T_obj>(name, NULL, func);
  }

  template <class T_ret, class T_obj, class T_arg1>
  ModuleFunctor1<T_ret, T_obj, T_arg1> *interface_fun(T_ret (T_obj::*func)(T_arg1), const char *name) {
    return new ModuleFunctor1<T_ret, T_obj, T_arg1>(name, NULL, func);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2>
  ModuleFunctor2<T_ret, T_obj, T_arg1, T_arg2> *interface_fun(T_ret (T_obj::*func)(T_arg1, T_arg2), const char *name) {
    return new ModuleFunctor2<T_ret, T_obj, T_arg1, T_arg2>(name, NULL, func);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3>
  ModuleFunctor3<T_ret, T_obj, T_arg1, T_arg2, T_arg3> *interface_fun(T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3),
                                                                      const char *name) {
    return new ModuleFunctor3<T_ret, T_obj, T_arg1, T_arg2, T_arg3>(name, NULL, func);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4>
  ModuleFunctor4<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4> *interface_fun(T_ret (T_obj::*func)(T_arg1, T_arg2,
                                                                                                   T_arg3, T_arg4),
                                                                              const char *name) {
    return new ModuleFunctor4<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4>(name, NULL, func);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5>
  ModuleFunctor5<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5> *interface_fun(
    T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5), const char *name) {
    return new ModuleFunctor5<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5>(name, NULL, func);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5,
            class T_arg6>
  ModuleFunctor6<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6> *interface_fun(
    T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6), const char *name) {
    return new ModuleFunctor6<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6>(name, NULL, func);
  }

  template <class T_ret, class T_obj, class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5,
            class T_arg6, class T_arg7>
  ModuleFunctor7<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6, T_arg7> *interface_fun(
    T_ret (T_obj::*func)(T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6, T_arg7), const char *name) {
    return new ModuleFunctor7<T_ret, T_obj, T_arg1, T_arg2, T_arg3, T_arg4, T_arg5, T_arg6, T_arg7>(name, NULL, func);
  }

#define DECLARE_INTERFACE_FUNCTION(qualified_name) interface_fun(&qualified_name, #qualified_name)
#define DECLARE_MODULE_FUNCTION(qualified_name) module_fun(this, &qualified_name, #qualified_name)
#define DECLARE_MODULE_FUNCTION_DOC(qualified_name, doc, argdoc) \
  module_fun(this, &qualified_name, #qualified_name, doc, argdoc)

#define DECLARE_NAMED_INTERFACE_FUNCTION(qualified_name, name) interface_fun(&qualified_name, name)
#define DECLARE_NAMED_MODULE_FUNCTION(qualified_name, name) module_fun(this, &qualified_name, name)
#define DECLARE_NAMED_MODULE_FUNCTION_DOC(qualified_name, name, doc) module_fun(this, &qualified_name, name, doc)

//----------------------------------------------------------------------------

// DLL modules must declare the following
#if 1
#if defined(_MSC_VER)
#define GRT_MODULE_ENTRY_POINT_PUBLIC __declspec(dllexport)
#else
#define GRT_MODULE_ENTRY_POINT_PUBLIC
#endif

#define GRT_MODULE_ENTRY_POINT(moduleName)                                                                            \
  extern "C" {                                                                                                        \
  GRT_MODULE_ENTRY_POINT_PUBLIC grt::Module *grt_module_init(grt::CPPModuleLoader *loader, const char *grt_version) { \
    moduleName *module = new moduleName(loader);                                                                      \
    module->init_module();                                                                                            \
    return module;                                                                                                    \
  }                                                                                                                   \
  }
#endif
};
