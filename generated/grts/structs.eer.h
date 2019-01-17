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
  #ifdef GRT_STRUCTS_EER_EXPORT
  #define GRT_STRUCTS_EER_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_EER_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_EER_PUBLIC
#endif

#include "grts/structs.h"

class eer_Datatype;
typedef grt::Ref<eer_Datatype> eer_DatatypeRef;
class eer_DatatypeGroup;
typedef grt::Ref<eer_DatatypeGroup> eer_DatatypeGroupRef;
class eer_Catalog;
typedef grt::Ref<eer_Catalog> eer_CatalogRef;
class eer_Object;
typedef grt::Ref<eer_Object> eer_ObjectRef;
class eer_Relationship;
typedef grt::Ref<eer_Relationship> eer_RelationshipRef;
class eer_Attribute;
typedef grt::Ref<eer_Attribute> eer_AttributeRef;
class eer_Entity;
typedef grt::Ref<eer_Entity> eer_EntityRef;
class eer_Schema;
typedef grt::Ref<eer_Schema> eer_SchemaRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

class  eer_Datatype : public GrtObject {
  typedef GrtObject super;

public:
  eer_Datatype(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _description("") {
  }

  static std::string static_class_name() {
    return "eer.Datatype";
  }

  /**
   * Getter for attribute caption
   *
   * 
   * \par In Python:
   *    value = obj.caption
   */
  grt::StringRef caption() const { return _caption; }

  /**
   * Setter for attribute caption
   * 
   * 
   * \par In Python:
   *   obj.caption = value
   */
  virtual void caption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_caption);
    _caption = value;
    member_changed("caption", ovalue, value);
  }

  /**
   * Getter for attribute description
   *
   * 
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * 
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

protected:

  grt::StringRef _caption;
  grt::StringRef _description;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_Datatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_Datatype::create);
    {
      void (eer_Datatype::*setter)(const grt::StringRef &) = &eer_Datatype::caption;
      grt::StringRef (eer_Datatype::*getter)() const = &eer_Datatype::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<eer_Datatype,grt::StringRef>(getter, setter));
    }
    {
      void (eer_Datatype::*setter)(const grt::StringRef &) = &eer_Datatype::description;
      grt::StringRef (eer_Datatype::*getter)() const = &eer_Datatype::description;
      meta->bind_member("description", new grt::MetaClass::Property<eer_Datatype,grt::StringRef>(getter, setter));
    }
  }
};

class  eer_DatatypeGroup : public GrtObject {
  typedef GrtObject super;

public:
  eer_DatatypeGroup(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _description("") {
  }

  static std::string static_class_name() {
    return "eer.DatatypeGroup";
  }

  /**
   * Getter for attribute caption
   *
   * 
   * \par In Python:
   *    value = obj.caption
   */
  grt::StringRef caption() const { return _caption; }

  /**
   * Setter for attribute caption
   * 
   * 
   * \par In Python:
   *   obj.caption = value
   */
  virtual void caption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_caption);
    _caption = value;
    member_changed("caption", ovalue, value);
  }

  /**
   * Getter for attribute description
   *
   * 
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * 
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

protected:

  grt::StringRef _caption;
  grt::StringRef _description;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_DatatypeGroup());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_DatatypeGroup::create);
    {
      void (eer_DatatypeGroup::*setter)(const grt::StringRef &) = &eer_DatatypeGroup::caption;
      grt::StringRef (eer_DatatypeGroup::*getter)() const = &eer_DatatypeGroup::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<eer_DatatypeGroup,grt::StringRef>(getter, setter));
    }
    {
      void (eer_DatatypeGroup::*setter)(const grt::StringRef &) = &eer_DatatypeGroup::description;
      grt::StringRef (eer_DatatypeGroup::*getter)() const = &eer_DatatypeGroup::description;
      meta->bind_member("description", new grt::MetaClass::Property<eer_DatatypeGroup,grt::StringRef>(getter, setter));
    }
  }
};

class  eer_Catalog : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  eer_Catalog(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _customData(this, false),
      _datatypes(this, false),
      _schemata(this, false),
      _userDatatypes(this, false) {
  }

  static std::string static_class_name() {
    return "eer.Catalog";
  }

  /**
   * Getter for attribute customData (read-only)
   *
   * a generic dictionary to hold additional information used by e.g. plugins
   * \par In Python:
   *    value = obj.customData
   */
  grt::DictRef customData() const { return _customData; }


private: // The next attribute is read-only.
  virtual void customData(const grt::DictRef &value) {
    grt::ValueRef ovalue(_customData);
    _customData = value;
    member_changed("customData", ovalue, value);
  }
public:

  // datatypes is owned by eer_Catalog
  /**
   * Getter for attribute datatypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.datatypes
   */
  grt::ListRef<eer_Datatype> datatypes() const { return _datatypes; }


private: // The next attribute is read-only.
  virtual void datatypes(const grt::ListRef<eer_Datatype> &value) {
    grt::ValueRef ovalue(_datatypes);

    _datatypes = value;
    owned_member_changed("datatypes", ovalue, value);
  }
public:

  // schemata is owned by eer_Catalog
  /**
   * Getter for attribute schemata (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.schemata
   */
  grt::ListRef<eer_Schema> schemata() const { return _schemata; }


private: // The next attribute is read-only.
  virtual void schemata(const grt::ListRef<eer_Schema> &value) {
    grt::ValueRef ovalue(_schemata);

    _schemata = value;
    owned_member_changed("schemata", ovalue, value);
  }
public:

  // userDatatypes is owned by eer_Catalog
  /**
   * Getter for attribute userDatatypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.userDatatypes
   */
  grt::ListRef<eer_Datatype> userDatatypes() const { return _userDatatypes; }


private: // The next attribute is read-only.
  virtual void userDatatypes(const grt::ListRef<eer_Datatype> &value) {
    grt::ValueRef ovalue(_userDatatypes);

    _userDatatypes = value;
    owned_member_changed("userDatatypes", ovalue, value);
  }
public:

protected:

  grt::DictRef _customData;
  grt::ListRef<eer_Datatype> _datatypes;// owned
  grt::ListRef<eer_Schema> _schemata;// owned
  grt::ListRef<eer_Datatype> _userDatatypes;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_Catalog());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_Catalog::create);
    {
      void (eer_Catalog::*setter)(const grt::DictRef &) = &eer_Catalog::customData;
      grt::DictRef (eer_Catalog::*getter)() const = &eer_Catalog::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<eer_Catalog,grt::DictRef>(getter, setter));
    }
    {
      void (eer_Catalog::*setter)(const grt::ListRef<eer_Datatype> &) = &eer_Catalog::datatypes;
      grt::ListRef<eer_Datatype> (eer_Catalog::*getter)() const = &eer_Catalog::datatypes;
      meta->bind_member("datatypes", new grt::MetaClass::Property<eer_Catalog,grt::ListRef<eer_Datatype>>(getter, setter));
    }
    {
      void (eer_Catalog::*setter)(const grt::ListRef<eer_Schema> &) = &eer_Catalog::schemata;
      grt::ListRef<eer_Schema> (eer_Catalog::*getter)() const = &eer_Catalog::schemata;
      meta->bind_member("schemata", new grt::MetaClass::Property<eer_Catalog,grt::ListRef<eer_Schema>>(getter, setter));
    }
    {
      void (eer_Catalog::*setter)(const grt::ListRef<eer_Datatype> &) = &eer_Catalog::userDatatypes;
      grt::ListRef<eer_Datatype> (eer_Catalog::*getter)() const = &eer_Catalog::userDatatypes;
      meta->bind_member("userDatatypes", new grt::MetaClass::Property<eer_Catalog,grt::ListRef<eer_Datatype>>(getter, setter));
    }
  }
};

class  eer_Object : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  eer_Object(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _commentedOut(0),
      _customData(this, false) {
  }

  static std::string static_class_name() {
    return "eer.Object";
  }

  /**
   * Getter for attribute commentedOut
   *
   * if set to 1 the object will be commented out but e.g. still be written to scripts
   * \par In Python:
   *    value = obj.commentedOut
   */
  grt::IntegerRef commentedOut() const { return _commentedOut; }

  /**
   * Setter for attribute commentedOut
   * 
   * if set to 1 the object will be commented out but e.g. still be written to scripts
   * \par In Python:
   *   obj.commentedOut = value
   */
  virtual void commentedOut(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_commentedOut);
    _commentedOut = value;
    member_changed("commentedOut", ovalue, value);
  }

  /**
   * Getter for attribute customData (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.customData
   */
  grt::DictRef customData() const { return _customData; }


private: // The next attribute is read-only.
  virtual void customData(const grt::DictRef &value) {
    grt::ValueRef ovalue(_customData);
    _customData = value;
    member_changed("customData", ovalue, value);
  }
public:

protected:

  grt::IntegerRef _commentedOut;
  grt::DictRef _customData;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_Object());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_Object::create);
    {
      void (eer_Object::*setter)(const grt::IntegerRef &) = &eer_Object::commentedOut;
      grt::IntegerRef (eer_Object::*getter)() const = &eer_Object::commentedOut;
      meta->bind_member("commentedOut", new grt::MetaClass::Property<eer_Object,grt::IntegerRef>(getter, setter));
    }
    {
      void (eer_Object::*setter)(const grt::DictRef &) = &eer_Object::customData;
      grt::DictRef (eer_Object::*getter)() const = &eer_Object::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<eer_Object,grt::DictRef>(getter, setter));
    }
  }
};

class  eer_Relationship : public eer_Object {
  typedef eer_Object super;

public:
  eer_Relationship(grt::MetaClass *meta = nullptr)
    : eer_Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _attribute(this, false),
      _endMandatory(0),
      _startMandatory(0) {
  }

  static std::string static_class_name() {
    return "eer.Relationship";
  }

  // attribute is owned by eer_Relationship
  /**
   * Getter for attribute attribute (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.attribute
   */
  grt::ListRef<eer_Attribute> attribute() const { return _attribute; }


private: // The next attribute is read-only.
  virtual void attribute(const grt::ListRef<eer_Attribute> &value) {
    grt::ValueRef ovalue(_attribute);

    _attribute = value;
    owned_member_changed("attribute", ovalue, value);
  }
public:

  /**
   * Getter for attribute endMandatory
   *
   * mandatory in the target table
   * \par In Python:
   *    value = obj.endMandatory
   */
  grt::IntegerRef endMandatory() const { return _endMandatory; }

  /**
   * Setter for attribute endMandatory
   * 
   * mandatory in the target table
   * \par In Python:
   *   obj.endMandatory = value
   */
  virtual void endMandatory(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_endMandatory);
    _endMandatory = value;
    member_changed("endMandatory", ovalue, value);
  }

  /**
   * Getter for attribute startMandatory
   *
   * mandatory in the source table
   * \par In Python:
   *    value = obj.startMandatory
   */
  grt::IntegerRef startMandatory() const { return _startMandatory; }

  /**
   * Setter for attribute startMandatory
   * 
   * mandatory in the source table
   * \par In Python:
   *   obj.startMandatory = value
   */
  virtual void startMandatory(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_startMandatory);
    _startMandatory = value;
    member_changed("startMandatory", ovalue, value);
  }

protected:

  grt::ListRef<eer_Attribute> _attribute;// owned
  grt::IntegerRef _endMandatory;
  grt::IntegerRef _startMandatory;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_Relationship());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_Relationship::create);
    {
      void (eer_Relationship::*setter)(const grt::ListRef<eer_Attribute> &) = &eer_Relationship::attribute;
      grt::ListRef<eer_Attribute> (eer_Relationship::*getter)() const = &eer_Relationship::attribute;
      meta->bind_member("attribute", new grt::MetaClass::Property<eer_Relationship,grt::ListRef<eer_Attribute>>(getter, setter));
    }
    {
      void (eer_Relationship::*setter)(const grt::IntegerRef &) = &eer_Relationship::endMandatory;
      grt::IntegerRef (eer_Relationship::*getter)() const = &eer_Relationship::endMandatory;
      meta->bind_member("endMandatory", new grt::MetaClass::Property<eer_Relationship,grt::IntegerRef>(getter, setter));
    }
    {
      void (eer_Relationship::*setter)(const grt::IntegerRef &) = &eer_Relationship::startMandatory;
      grt::IntegerRef (eer_Relationship::*getter)() const = &eer_Relationship::startMandatory;
      meta->bind_member("startMandatory", new grt::MetaClass::Property<eer_Relationship,grt::IntegerRef>(getter, setter));
    }
  }
};

class  eer_Attribute : public eer_Object {
  typedef eer_Object super;

public:
  eer_Attribute(grt::MetaClass *meta = nullptr)
    : eer_Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _isIdentifying(0) {
  }

  static std::string static_class_name() {
    return "eer.Attribute";
  }

  // datatype is owned by eer_Attribute
  /**
   * Getter for attribute datatype
   *
   * 
   * \par In Python:
   *    value = obj.datatype
   */
  eer_DatatypeRef datatype() const { return _datatype; }

  /**
   * Setter for attribute datatype
   * 
   * 
   * \par In Python:
   *   obj.datatype = value
   */
  virtual void datatype(const eer_DatatypeRef &value) {
    grt::ValueRef ovalue(_datatype);

    _datatype = value;
    owned_member_changed("datatype", ovalue, value);
  }

  /**
   * Getter for attribute isIdentifying
   *
   * specifies if this attribute is an identifying attribute for the entity
   * \par In Python:
   *    value = obj.isIdentifying
   */
  grt::IntegerRef isIdentifying() const { return _isIdentifying; }

  /**
   * Setter for attribute isIdentifying
   * 
   * specifies if this attribute is an identifying attribute for the entity
   * \par In Python:
   *   obj.isIdentifying = value
   */
  virtual void isIdentifying(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isIdentifying);
    _isIdentifying = value;
    member_changed("isIdentifying", ovalue, value);
  }

protected:

  eer_DatatypeRef _datatype;// owned
  grt::IntegerRef _isIdentifying;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_Attribute());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_Attribute::create);
    {
      void (eer_Attribute::*setter)(const eer_DatatypeRef &) = &eer_Attribute::datatype;
      eer_DatatypeRef (eer_Attribute::*getter)() const = &eer_Attribute::datatype;
      meta->bind_member("datatype", new grt::MetaClass::Property<eer_Attribute,eer_DatatypeRef>(getter, setter));
    }
    {
      void (eer_Attribute::*setter)(const grt::IntegerRef &) = &eer_Attribute::isIdentifying;
      grt::IntegerRef (eer_Attribute::*getter)() const = &eer_Attribute::isIdentifying;
      meta->bind_member("isIdentifying", new grt::MetaClass::Property<eer_Attribute,grt::IntegerRef>(getter, setter));
    }
  }
};

class  eer_Entity : public eer_Object {
  typedef eer_Object super;

public:
  eer_Entity(grt::MetaClass *meta = nullptr)
    : eer_Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _attribute(this, false) {
  }

  static std::string static_class_name() {
    return "eer.Entity";
  }

  // attribute is owned by eer_Entity
  /**
   * Getter for attribute attribute (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.attribute
   */
  grt::ListRef<eer_Attribute> attribute() const { return _attribute; }


private: // The next attribute is read-only.
  virtual void attribute(const grt::ListRef<eer_Attribute> &value) {
    grt::ValueRef ovalue(_attribute);

    _attribute = value;
    owned_member_changed("attribute", ovalue, value);
  }
public:

protected:

  grt::ListRef<eer_Attribute> _attribute;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_Entity());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_Entity::create);
    {
      void (eer_Entity::*setter)(const grt::ListRef<eer_Attribute> &) = &eer_Entity::attribute;
      grt::ListRef<eer_Attribute> (eer_Entity::*getter)() const = &eer_Entity::attribute;
      meta->bind_member("attribute", new grt::MetaClass::Property<eer_Entity,grt::ListRef<eer_Attribute>>(getter, setter));
    }
  }
};

class  eer_Schema : public eer_Object {
  typedef eer_Object super;

public:
  eer_Schema(grt::MetaClass *meta = nullptr)
    : eer_Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _entities(this, false),
      _relationships(this, false) {
  }

  static std::string static_class_name() {
    return "eer.Schema";
  }

  // entities is owned by eer_Schema
  /**
   * Getter for attribute entities (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.entities
   */
  grt::ListRef<eer_Entity> entities() const { return _entities; }


private: // The next attribute is read-only.
  virtual void entities(const grt::ListRef<eer_Entity> &value) {
    grt::ValueRef ovalue(_entities);

    _entities = value;
    owned_member_changed("entities", ovalue, value);
  }
public:

  // relationships is owned by eer_Schema
  /**
   * Getter for attribute relationships (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.relationships
   */
  grt::ListRef<eer_Relationship> relationships() const { return _relationships; }


private: // The next attribute is read-only.
  virtual void relationships(const grt::ListRef<eer_Relationship> &value) {
    grt::ValueRef ovalue(_relationships);

    _relationships = value;
    owned_member_changed("relationships", ovalue, value);
  }
public:

protected:

  grt::ListRef<eer_Entity> _entities;// owned
  grt::ListRef<eer_Relationship> _relationships;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new eer_Schema());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&eer_Schema::create);
    {
      void (eer_Schema::*setter)(const grt::ListRef<eer_Entity> &) = &eer_Schema::entities;
      grt::ListRef<eer_Entity> (eer_Schema::*getter)() const = &eer_Schema::entities;
      meta->bind_member("entities", new grt::MetaClass::Property<eer_Schema,grt::ListRef<eer_Entity>>(getter, setter));
    }
    {
      void (eer_Schema::*setter)(const grt::ListRef<eer_Relationship> &) = &eer_Schema::relationships;
      grt::ListRef<eer_Relationship> (eer_Schema::*getter)() const = &eer_Schema::relationships;
      meta->bind_member("relationships", new grt::MetaClass::Property<eer_Schema,grt::ListRef<eer_Relationship>>(getter, setter));
    }
  }
};



inline void register_structs_eer_xml() {
  grt::internal::ClassRegistry::register_class<eer_Datatype>();
  grt::internal::ClassRegistry::register_class<eer_DatatypeGroup>();
  grt::internal::ClassRegistry::register_class<eer_Catalog>();
  grt::internal::ClassRegistry::register_class<eer_Object>();
  grt::internal::ClassRegistry::register_class<eer_Relationship>();
  grt::internal::ClassRegistry::register_class<eer_Attribute>();
  grt::internal::ClassRegistry::register_class<eer_Entity>();
  grt::internal::ClassRegistry::register_class<eer_Schema>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_eer_xml {
  _autoreg__structs_eer_xml() {
    register_structs_eer_xml();
  }
} __autoreg__structs_eer_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

