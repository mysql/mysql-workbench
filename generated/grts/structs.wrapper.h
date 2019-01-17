/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#ifndef _MSC_VER
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include "grt.h"

#ifdef _MSC_VER
  #pragma warning(disable: 4355) // 'this' : used in base member initializer list
  #ifdef GRT_STRUCTS_WRAPPER_EXPORT
  #define GRT_STRUCTS_WRAPPER_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_WRAPPER_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_WRAPPER_PUBLIC
#endif

#include "grts/structs.h"

class parser_ContextReference;
typedef grt::Ref<parser_ContextReference> parser_ContextReferenceRef;
class mforms_ObjectReference;
typedef grt::Ref<mforms_ObjectReference> mforms_ObjectReferenceRef;
class grt_PyObject;
typedef grt::Ref<grt_PyObject> grt_PyObjectRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** wraps a parser context so it can be stored in a grt container */
class GRT_STRUCTS_WRAPPER_PUBLIC parser_ContextReference : public TransientObject {
  typedef TransientObject super;

public:
  class ImplData;
  friend class ImplData;
  parser_ContextReference(grt::MetaClass *meta = nullptr)
    : TransientObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~parser_ContextReference();

  static std::string static_class_name() {
    return "parser.ContextReference";
  }

  /**
   * Getter for attribute valid (read-only)
   *
   * whether there's a context set
   * \par In Python:
   *    value = obj.valid
   */
  grt::IntegerRef valid() const;


private: // The next attribute is read-only.
public:


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new parser_ContextReference());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&parser_ContextReference::create);
    meta->bind_member("valid", new grt::MetaClass::Property<parser_ContextReference,grt::IntegerRef>(&parser_ContextReference::valid));
  }
};

/** an object representing a reference to a mforms object */
class GRT_STRUCTS_WRAPPER_PUBLIC mforms_ObjectReference : public TransientObject {
  typedef TransientObject super;

public:
  typedef mforms::Object ImplData;
  mforms_ObjectReference(grt::MetaClass *meta = nullptr)
    : TransientObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _type(""),
      _data(nullptr), _release_data(nullptr) {
  }

  virtual ~mforms_ObjectReference() {
    if (_release_data && _data)
      _release_data(_data);
  }

  static std::string static_class_name() {
    return "mforms.ObjectReference";
  }

  /**
   * Getter for attribute type
   *
   * the specific type of mforms object
   * \par In Python:
   *    value = obj.type
   */
  grt::StringRef type() const { return _type; }

  /**
   * Setter for attribute type
   * 
   * the specific type of mforms object
   * \par In Python:
   *   obj.type = value
   */
  virtual void type(const grt::StringRef &value) {
    grt::ValueRef ovalue(_type);
    _type = value;
    member_changed("type", ovalue, value);
  }

  /**
   * Getter for attribute valid (read-only)
   *
   * whether the object is currently valid
   * \par In Python:
   *    value = obj.valid
   */
  grt::IntegerRef valid() const;


private: // The next attribute is read-only.
public:

  /**
   * Method. checks whether the reference points to the same view as another reference
   * \param other 
   * \return 
   */
  virtual grt::IntegerRef isEqualTo(const mforms_ObjectReferenceRef &other);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data, void (*release)(ImplData*)) {
    if (_data == data) return;
    if (_data && _release_data) _release_data(_data);
    _data= data;
    _release_data = release;
  }
protected:

  grt::StringRef _type;

private: // Wrapper methods for use by the grt.
  ImplData *_data;
  void (*_release_data)(ImplData *);

  static grt::ObjectRef create() {
    return grt::ObjectRef(new mforms_ObjectReference());
  }

  static grt::ValueRef call_isEqualTo(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<mforms_ObjectReference*>(self)->isEqualTo(mforms_ObjectReferenceRef::cast_from(args[0])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&mforms_ObjectReference::create);
    {
      void (mforms_ObjectReference::*setter)(const grt::StringRef &) = &mforms_ObjectReference::type;
      grt::StringRef (mforms_ObjectReference::*getter)() const = &mforms_ObjectReference::type;
      meta->bind_member("type", new grt::MetaClass::Property<mforms_ObjectReference,grt::StringRef>(getter, setter));
    }
    meta->bind_member("valid", new grt::MetaClass::Property<mforms_ObjectReference,grt::IntegerRef>(&mforms_ObjectReference::valid));
    meta->bind_method("isEqualTo", &mforms_ObjectReference::call_isEqualTo);
  }
};

/** wraps a Python object reference so it can be stored in a GRT container */
class GRT_STRUCTS_WRAPPER_PUBLIC grt_PyObject : public TransientObject {
  typedef TransientObject super;

public:
  typedef grt::AutoPyObject ImplData;
  grt_PyObject(grt::MetaClass *meta = nullptr)
    : TransientObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr), _release_data(nullptr) {
  }

  virtual ~grt_PyObject() {
    if (_release_data && _data)
      _release_data(_data);
  }

  static std::string static_class_name() {
    return "grt.PyObject";
  }

  /**
   * Method. checks whether the reference points to the same object as another refrence
   * \param other 
   * \return 
   */
  virtual grt::IntegerRef isEqualTo(const grt_PyObjectRef &other);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data, void (*release)(ImplData*)) {
    if (_data == data) return;
    if (_data && _release_data) _release_data(_data);
    _data= data;
    _release_data = release;
  }
protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;
  void (*_release_data)(ImplData *);

  static grt::ObjectRef create() {
    return grt::ObjectRef(new grt_PyObject());
  }

  static grt::ValueRef call_isEqualTo(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<grt_PyObject*>(self)->isEqualTo(grt_PyObjectRef::cast_from(args[0])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&grt_PyObject::create);
    meta->bind_method("isEqualTo", &grt_PyObject::call_isEqualTo);
  }
};



inline void register_structs_wrapper_xml() {
  grt::internal::ClassRegistry::register_class<parser_ContextReference>();
  grt::internal::ClassRegistry::register_class<mforms_ObjectReference>();
  grt::internal::ClassRegistry::register_class<grt_PyObject>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_wrapper_xml {
  _autoreg__structs_wrapper_xml() {
    register_structs_wrapper_xml();
  }
} __autoreg__structs_wrapper_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

