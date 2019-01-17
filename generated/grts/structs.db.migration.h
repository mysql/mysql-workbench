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
  #ifdef GRT_STRUCTS_DB_MIGRATION_EXPORT
  #define GRT_STRUCTS_DB_MIGRATION_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_DB_MIGRATION_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_DB_MIGRATION_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"

class db_migration_MigrationParameter;
typedef grt::Ref<db_migration_MigrationParameter> db_migration_MigrationParameterRef;
class db_migration_DatatypeMapping;
typedef grt::Ref<db_migration_DatatypeMapping> db_migration_DatatypeMappingRef;
class db_migration_DBPreferences;
typedef grt::Ref<db_migration_DBPreferences> db_migration_DBPreferencesRef;
class db_migration_Migration;
typedef grt::Ref<db_migration_Migration> db_migration_MigrationRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

class  db_migration_MigrationParameter : public GrtObject {
  typedef GrtObject super;

public:
  db_migration_MigrationParameter(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _defaultValue(""),
      _description(""),
      _paramType("") {
  }

  static std::string static_class_name() {
    return "db.migration.MigrationParameter";
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
   * Getter for attribute defaultValue
   *
   * 
   * \par In Python:
   *    value = obj.defaultValue
   */
  grt::StringRef defaultValue() const { return _defaultValue; }

  /**
   * Setter for attribute defaultValue
   * 
   * 
   * \par In Python:
   *   obj.defaultValue = value
   */
  virtual void defaultValue(const grt::StringRef &value) {
    grt::ValueRef ovalue(_defaultValue);
    _defaultValue = value;
    member_changed("defaultValue", ovalue, value);
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

  /**
   * Getter for attribute paramType
   *
   * one of string, boolean
   * \par In Python:
   *    value = obj.paramType
   */
  grt::StringRef paramType() const { return _paramType; }

  /**
   * Setter for attribute paramType
   * 
   * one of string, boolean
   * \par In Python:
   *   obj.paramType = value
   */
  virtual void paramType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_paramType);
    _paramType = value;
    member_changed("paramType", ovalue, value);
  }

protected:

  grt::StringRef _caption;
  grt::StringRef _defaultValue;
  grt::StringRef _description;
  grt::StringRef _paramType;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_migration_MigrationParameter());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_migration_MigrationParameter::create);
    {
      void (db_migration_MigrationParameter::*setter)(const grt::StringRef &) = &db_migration_MigrationParameter::caption;
      grt::StringRef (db_migration_MigrationParameter::*getter)() const = &db_migration_MigrationParameter::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<db_migration_MigrationParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_migration_MigrationParameter::*setter)(const grt::StringRef &) = &db_migration_MigrationParameter::defaultValue;
      grt::StringRef (db_migration_MigrationParameter::*getter)() const = &db_migration_MigrationParameter::defaultValue;
      meta->bind_member("defaultValue", new grt::MetaClass::Property<db_migration_MigrationParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_migration_MigrationParameter::*setter)(const grt::StringRef &) = &db_migration_MigrationParameter::description;
      grt::StringRef (db_migration_MigrationParameter::*getter)() const = &db_migration_MigrationParameter::description;
      meta->bind_member("description", new grt::MetaClass::Property<db_migration_MigrationParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_migration_MigrationParameter::*setter)(const grt::StringRef &) = &db_migration_MigrationParameter::paramType;
      grt::StringRef (db_migration_MigrationParameter::*getter)() const = &db_migration_MigrationParameter::paramType;
      meta->bind_member("paramType", new grt::MetaClass::Property<db_migration_MigrationParameter,grt::StringRef>(getter, setter));
    }
  }
};

/** mapping of a datatype from one database to another */
class  db_migration_DatatypeMapping : public GrtObject {
  typedef GrtObject super;

public:
  db_migration_DatatypeMapping(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _isUnsigned(0),
      _length(-2),
      _lengthConditionFrom(0),
      _lengthConditionTo(0),
      _precision(-2),
      _precisionConditionFrom(0),
      _precisionConditionTo(0),
      _scale(-2),
      _scaleConditionFrom(0),
      _scaleConditionTo(0),
      _sourceDatatypeName(""),
      _targetDatatypeName("") {
  }

  static std::string static_class_name() {
    return "db.migration.DatatypeMapping";
  }

  /**
   * Getter for attribute isUnsigned
   *
   * sets the unsigned flag
   * \par In Python:
   *    value = obj.isUnsigned
   */
  grt::IntegerRef isUnsigned() const { return _isUnsigned; }

  /**
   * Setter for attribute isUnsigned
   * 
   * sets the unsigned flag
   * \par In Python:
   *   obj.isUnsigned = value
   */
  virtual void isUnsigned(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isUnsigned);
    _isUnsigned = value;
    member_changed("isUnsigned", ovalue, value);
  }

  /**
   * Getter for attribute length
   *
   * overwrite length if different than -2
   * \par In Python:
   *    value = obj.length
   */
  grt::IntegerRef length() const { return _length; }

  /**
   * Setter for attribute length
   * 
   * overwrite length if different than -2
   * \par In Python:
   *   obj.length = value
   */
  virtual void length(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_length);
    _length = value;
    member_changed("length", ovalue, value);
  }

  /**
   * Getter for attribute lengthConditionFrom
   *
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *    value = obj.lengthConditionFrom
   */
  grt::IntegerRef lengthConditionFrom() const { return _lengthConditionFrom; }

  /**
   * Setter for attribute lengthConditionFrom
   * 
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *   obj.lengthConditionFrom = value
   */
  virtual void lengthConditionFrom(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_lengthConditionFrom);
    _lengthConditionFrom = value;
    member_changed("lengthConditionFrom", ovalue, value);
  }

  /**
   * Getter for attribute lengthConditionTo
   *
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *    value = obj.lengthConditionTo
   */
  grt::IntegerRef lengthConditionTo() const { return _lengthConditionTo; }

  /**
   * Setter for attribute lengthConditionTo
   * 
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *   obj.lengthConditionTo = value
   */
  virtual void lengthConditionTo(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_lengthConditionTo);
    _lengthConditionTo = value;
    member_changed("lengthConditionTo", ovalue, value);
  }

  /**
   * Getter for attribute precision
   *
   * overwrite precision if different than -2
   * \par In Python:
   *    value = obj.precision
   */
  grt::IntegerRef precision() const { return _precision; }

  /**
   * Setter for attribute precision
   * 
   * overwrite precision if different than -2
   * \par In Python:
   *   obj.precision = value
   */
  virtual void precision(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_precision);
    _precision = value;
    member_changed("precision", ovalue, value);
  }

  /**
   * Getter for attribute precisionConditionFrom
   *
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *    value = obj.precisionConditionFrom
   */
  grt::IntegerRef precisionConditionFrom() const { return _precisionConditionFrom; }

  /**
   * Setter for attribute precisionConditionFrom
   * 
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *   obj.precisionConditionFrom = value
   */
  virtual void precisionConditionFrom(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_precisionConditionFrom);
    _precisionConditionFrom = value;
    member_changed("precisionConditionFrom", ovalue, value);
  }

  /**
   * Getter for attribute precisionConditionTo
   *
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *    value = obj.precisionConditionTo
   */
  grt::IntegerRef precisionConditionTo() const { return _precisionConditionTo; }

  /**
   * Setter for attribute precisionConditionTo
   * 
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *   obj.precisionConditionTo = value
   */
  virtual void precisionConditionTo(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_precisionConditionTo);
    _precisionConditionTo = value;
    member_changed("precisionConditionTo", ovalue, value);
  }

  /**
   * Getter for attribute scale
   *
   * overwrite scale if different than -2
   * \par In Python:
   *    value = obj.scale
   */
  grt::IntegerRef scale() const { return _scale; }

  /**
   * Setter for attribute scale
   * 
   * overwrite scale if different than -2
   * \par In Python:
   *   obj.scale = value
   */
  virtual void scale(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_scale);
    _scale = value;
    member_changed("scale", ovalue, value);
  }

  /**
   * Getter for attribute scaleConditionFrom
   *
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *    value = obj.scaleConditionFrom
   */
  grt::IntegerRef scaleConditionFrom() const { return _scaleConditionFrom; }

  /**
   * Setter for attribute scaleConditionFrom
   * 
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *   obj.scaleConditionFrom = value
   */
  virtual void scaleConditionFrom(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_scaleConditionFrom);
    _scaleConditionFrom = value;
    member_changed("scaleConditionFrom", ovalue, value);
  }

  /**
   * Getter for attribute scaleConditionTo
   *
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *    value = obj.scaleConditionTo
   */
  grt::IntegerRef scaleConditionTo() const { return _scaleConditionTo; }

  /**
   * Setter for attribute scaleConditionTo
   * 
   * if set to a value different than 0 this becomes a condition
   * \par In Python:
   *   obj.scaleConditionTo = value
   */
  virtual void scaleConditionTo(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_scaleConditionTo);
    _scaleConditionTo = value;
    member_changed("scaleConditionTo", ovalue, value);
  }

  /**
   * Getter for attribute sourceDatatypeName
   *
   * name of the datatype in the source database
   * \par In Python:
   *    value = obj.sourceDatatypeName
   */
  grt::StringRef sourceDatatypeName() const { return _sourceDatatypeName; }

  /**
   * Setter for attribute sourceDatatypeName
   * 
   * name of the datatype in the source database
   * \par In Python:
   *   obj.sourceDatatypeName = value
   */
  virtual void sourceDatatypeName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_sourceDatatypeName);
    _sourceDatatypeName = value;
    member_changed("sourceDatatypeName", ovalue, value);
  }

  /**
   * Getter for attribute targetDatatypeName
   *
   * name of the datatype in the target database
   * \par In Python:
   *    value = obj.targetDatatypeName
   */
  grt::StringRef targetDatatypeName() const { return _targetDatatypeName; }

  /**
   * Setter for attribute targetDatatypeName
   * 
   * name of the datatype in the target database
   * \par In Python:
   *   obj.targetDatatypeName = value
   */
  virtual void targetDatatypeName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_targetDatatypeName);
    _targetDatatypeName = value;
    member_changed("targetDatatypeName", ovalue, value);
  }

protected:

  grt::IntegerRef _isUnsigned;
  grt::IntegerRef _length;
  grt::IntegerRef _lengthConditionFrom;
  grt::IntegerRef _lengthConditionTo;
  grt::IntegerRef _precision;
  grt::IntegerRef _precisionConditionFrom;
  grt::IntegerRef _precisionConditionTo;
  grt::IntegerRef _scale;
  grt::IntegerRef _scaleConditionFrom;
  grt::IntegerRef _scaleConditionTo;
  grt::StringRef _sourceDatatypeName;
  grt::StringRef _targetDatatypeName;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_migration_DatatypeMapping());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_migration_DatatypeMapping::create);
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::isUnsigned;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::isUnsigned;
      meta->bind_member("isUnsigned", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::length;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::length;
      meta->bind_member("length", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::lengthConditionFrom;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::lengthConditionFrom;
      meta->bind_member("lengthConditionFrom", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::lengthConditionTo;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::lengthConditionTo;
      meta->bind_member("lengthConditionTo", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::precision;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::precision;
      meta->bind_member("precision", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::precisionConditionFrom;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::precisionConditionFrom;
      meta->bind_member("precisionConditionFrom", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::precisionConditionTo;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::precisionConditionTo;
      meta->bind_member("precisionConditionTo", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::scale;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::scale;
      meta->bind_member("scale", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::scaleConditionFrom;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::scaleConditionFrom;
      meta->bind_member("scaleConditionFrom", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::IntegerRef &) = &db_migration_DatatypeMapping::scaleConditionTo;
      grt::IntegerRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::scaleConditionTo;
      meta->bind_member("scaleConditionTo", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::StringRef &) = &db_migration_DatatypeMapping::sourceDatatypeName;
      grt::StringRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::sourceDatatypeName;
      meta->bind_member("sourceDatatypeName", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::StringRef>(getter, setter));
    }
    {
      void (db_migration_DatatypeMapping::*setter)(const grt::StringRef &) = &db_migration_DatatypeMapping::targetDatatypeName;
      grt::StringRef (db_migration_DatatypeMapping::*getter)() const = &db_migration_DatatypeMapping::targetDatatypeName;
      meta->bind_member("targetDatatypeName", new grt::MetaClass::Property<db_migration_DatatypeMapping,grt::StringRef>(getter, setter));
    }
  }
};

class  db_migration_DBPreferences : public GrtObject {
  typedef GrtObject super;

public:
  db_migration_DBPreferences(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _characterSetMapping(this, false),
      _datatypeMapping(this, false),
      _defaultValueMapping(this, false),
      _options(this, false) {
  }

  static std::string static_class_name() {
    return "db.migration.DBPreferences";
  }

  /**
   * Getter for attribute characterSetMapping (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.characterSetMapping
   */
  grt::DictRef characterSetMapping() const { return _characterSetMapping; }


private: // The next attribute is read-only.
  virtual void characterSetMapping(const grt::DictRef &value) {
    grt::ValueRef ovalue(_characterSetMapping);
    _characterSetMapping = value;
    member_changed("characterSetMapping", ovalue, value);
  }
public:

  // datatypeMapping is owned by db_migration_DBPreferences
  /**
   * Getter for attribute datatypeMapping (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.datatypeMapping
   */
  grt::ListRef<db_migration_DatatypeMapping> datatypeMapping() const { return _datatypeMapping; }


private: // The next attribute is read-only.
  virtual void datatypeMapping(const grt::ListRef<db_migration_DatatypeMapping> &value) {
    grt::ValueRef ovalue(_datatypeMapping);

    _datatypeMapping = value;
    owned_member_changed("datatypeMapping", ovalue, value);
  }
public:

  /**
   * Getter for attribute defaultValueMapping (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.defaultValueMapping
   */
  grt::DictRef defaultValueMapping() const { return _defaultValueMapping; }


private: // The next attribute is read-only.
  virtual void defaultValueMapping(const grt::DictRef &value) {
    grt::ValueRef ovalue(_defaultValueMapping);
    _defaultValueMapping = value;
    member_changed("defaultValueMapping", ovalue, value);
  }
public:

  /**
   * Getter for attribute options (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.options
   */
  grt::DictRef options() const { return _options; }


private: // The next attribute is read-only.
  virtual void options(const grt::DictRef &value) {
    grt::ValueRef ovalue(_options);
    _options = value;
    member_changed("options", ovalue, value);
  }
public:

  /**
   * Getter for attribute sourceRdbms
   *
   * 
   * \par In Python:
   *    value = obj.sourceRdbms
   */
  db_mgmt_RdbmsRef sourceRdbms() const { return _sourceRdbms; }

  /**
   * Setter for attribute sourceRdbms
   * 
   * 
   * \par In Python:
   *   obj.sourceRdbms = value
   */
  virtual void sourceRdbms(const db_mgmt_RdbmsRef &value) {
    grt::ValueRef ovalue(_sourceRdbms);
    _sourceRdbms = value;
    member_changed("sourceRdbms", ovalue, value);
  }

protected:

  grt::DictRef _characterSetMapping;
  grt::ListRef<db_migration_DatatypeMapping> _datatypeMapping;// owned
  grt::DictRef _defaultValueMapping;
  grt::DictRef _options;
  db_mgmt_RdbmsRef _sourceRdbms;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_migration_DBPreferences());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_migration_DBPreferences::create);
    {
      void (db_migration_DBPreferences::*setter)(const grt::DictRef &) = &db_migration_DBPreferences::characterSetMapping;
      grt::DictRef (db_migration_DBPreferences::*getter)() const = &db_migration_DBPreferences::characterSetMapping;
      meta->bind_member("characterSetMapping", new grt::MetaClass::Property<db_migration_DBPreferences,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_DBPreferences::*setter)(const grt::ListRef<db_migration_DatatypeMapping> &) = &db_migration_DBPreferences::datatypeMapping;
      grt::ListRef<db_migration_DatatypeMapping> (db_migration_DBPreferences::*getter)() const = &db_migration_DBPreferences::datatypeMapping;
      meta->bind_member("datatypeMapping", new grt::MetaClass::Property<db_migration_DBPreferences,grt::ListRef<db_migration_DatatypeMapping>>(getter, setter));
    }
    {
      void (db_migration_DBPreferences::*setter)(const grt::DictRef &) = &db_migration_DBPreferences::defaultValueMapping;
      grt::DictRef (db_migration_DBPreferences::*getter)() const = &db_migration_DBPreferences::defaultValueMapping;
      meta->bind_member("defaultValueMapping", new grt::MetaClass::Property<db_migration_DBPreferences,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_DBPreferences::*setter)(const grt::DictRef &) = &db_migration_DBPreferences::options;
      grt::DictRef (db_migration_DBPreferences::*getter)() const = &db_migration_DBPreferences::options;
      meta->bind_member("options", new grt::MetaClass::Property<db_migration_DBPreferences,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_DBPreferences::*setter)(const db_mgmt_RdbmsRef &) = &db_migration_DBPreferences::sourceRdbms;
      db_mgmt_RdbmsRef (db_migration_DBPreferences::*getter)() const = &db_migration_DBPreferences::sourceRdbms;
      meta->bind_member("sourceRdbms", new grt::MetaClass::Property<db_migration_DBPreferences,db_mgmt_RdbmsRef>(getter, setter));
    }
  }
};

/** an object to store information needed during the migration process */
class GRT_STRUCTS_DB_MIGRATION_PUBLIC db_migration_Migration : public GrtObject {
  typedef GrtObject super;

public:
  class ImplData;
  friend class ImplData;
  db_migration_Migration(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _applicationData(this, false),
      _creationLog(this, false),
      _dataBulkTransferParams(this, false),
      _dataTransferLog(this, false),
      _defaultColumnValueMappings(this, false),
      _genericDatatypeMappings(this, false),
      _ignoreList(this, false),
      _migrationLog(this, false),
      _objectCreationParams(this, false),
      _objectMigrationParams(this, false),
      _selectedSchemataNames(this, false),
      _sourceObjects(this, false),
      _sourceSchemataNames(this, false),
      _data(nullptr) {
  }

  virtual ~db_migration_Migration();

  static std::string static_class_name() {
    return "db.migration.Migration";
  }

  /**
   * Getter for attribute applicationData (read-only)
   *
   * internal parameters set by the migration tool
   * \par In Python:
   *    value = obj.applicationData
   */
  grt::DictRef applicationData() const { return _applicationData; }


private: // The next attribute is read-only.
  virtual void applicationData(const grt::DictRef &value) {
    grt::ValueRef ovalue(_applicationData);
    _applicationData = value;
    member_changed("applicationData", ovalue, value);
  }
public:

  // creationLog is owned by db_migration_Migration
  /**
   * Getter for attribute creationLog (read-only)
   *
   * a listing of log messages generated during object creation
   * \par In Python:
   *    value = obj.creationLog
   */
  grt::ListRef<GrtLogObject> creationLog() const { return _creationLog; }


private: // The next attribute is read-only.
  virtual void creationLog(const grt::ListRef<GrtLogObject> &value) {
    grt::ValueRef ovalue(_creationLog);

    _creationLog = value;
    owned_member_changed("creationLog", ovalue, value);
  }
public:

  /**
   * Getter for attribute dataBulkTransferParams (read-only)
   *
   * the dictionary of parameters used during the bulk data transfer
   * \par In Python:
   *    value = obj.dataBulkTransferParams
   */
  grt::DictRef dataBulkTransferParams() const { return _dataBulkTransferParams; }


private: // The next attribute is read-only.
  virtual void dataBulkTransferParams(const grt::DictRef &value) {
    grt::ValueRef ovalue(_dataBulkTransferParams);
    _dataBulkTransferParams = value;
    member_changed("dataBulkTransferParams", ovalue, value);
  }
public:

  // dataTransferLog is owned by db_migration_Migration
  /**
   * Getter for attribute dataTransferLog (read-only)
   *
   * a listing of log messages generated during data transfer
   * \par In Python:
   *    value = obj.dataTransferLog
   */
  grt::ListRef<GrtLogObject> dataTransferLog() const { return _dataTransferLog; }


private: // The next attribute is read-only.
  virtual void dataTransferLog(const grt::ListRef<GrtLogObject> &value) {
    grt::ValueRef ovalue(_dataTransferLog);

    _dataTransferLog = value;
    owned_member_changed("dataTransferLog", ovalue, value);
  }
public:

  /**
   * Getter for attribute defaultColumnValueMappings (read-only)
   *
   * a mapping of default column values for the selected source RDBMS. Default values that match one of the values in the dict will be automatically translated.
   * \par In Python:
   *    value = obj.defaultColumnValueMappings
   */
  grt::DictRef defaultColumnValueMappings() const { return _defaultColumnValueMappings; }


private: // The next attribute is read-only.
  virtual void defaultColumnValueMappings(const grt::DictRef &value) {
    grt::ValueRef ovalue(_defaultColumnValueMappings);
    _defaultColumnValueMappings = value;
    member_changed("defaultColumnValueMappings", ovalue, value);
  }
public:

  // genericDatatypeMappings is owned by db_migration_Migration
  /**
   * Getter for attribute genericDatatypeMappings (read-only)
   *
   * datatype mapping for generic migration
   * \par In Python:
   *    value = obj.genericDatatypeMappings
   */
  grt::ListRef<db_migration_DatatypeMapping> genericDatatypeMappings() const { return _genericDatatypeMappings; }


private: // The next attribute is read-only.
  virtual void genericDatatypeMappings(const grt::ListRef<db_migration_DatatypeMapping> &value) {
    grt::ValueRef ovalue(_genericDatatypeMappings);

    _genericDatatypeMappings = value;
    owned_member_changed("genericDatatypeMappings", ovalue, value);
  }
public:

  /**
   * Getter for attribute ignoreList (read-only)
   *
   * list of objects that should not be migrated in the form objecttype:schemaname.objectname
   * \par In Python:
   *    value = obj.ignoreList
   */
  grt::StringListRef ignoreList() const { return _ignoreList; }


private: // The next attribute is read-only.
  virtual void ignoreList(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_ignoreList);
    _ignoreList = value;
    member_changed("ignoreList", ovalue, value);
  }
public:

  // migrationLog is owned by db_migration_Migration
  /**
   * Getter for attribute migrationLog (read-only)
   *
   * a listing of log messages generated during object migration
   * \par In Python:
   *    value = obj.migrationLog
   */
  grt::ListRef<GrtLogObject> migrationLog() const { return _migrationLog; }


private: // The next attribute is read-only.
  virtual void migrationLog(const grt::ListRef<GrtLogObject> &value) {
    grt::ValueRef ovalue(_migrationLog);

    _migrationLog = value;
    owned_member_changed("migrationLog", ovalue, value);
  }
public:

  /**
   * Getter for attribute objectCreationParams (read-only)
   *
   * the dictionary of parameters used during the object creation
   * \par In Python:
   *    value = obj.objectCreationParams
   */
  grt::DictRef objectCreationParams() const { return _objectCreationParams; }


private: // The next attribute is read-only.
  virtual void objectCreationParams(const grt::DictRef &value) {
    grt::ValueRef ovalue(_objectCreationParams);
    _objectCreationParams = value;
    member_changed("objectCreationParams", ovalue, value);
  }
public:

  /**
   * Getter for attribute objectMigrationParams (read-only)
   *
   * the dictionary of parameters used during object migration
   * \par In Python:
   *    value = obj.objectMigrationParams
   */
  grt::DictRef objectMigrationParams() const { return _objectMigrationParams; }


private: // The next attribute is read-only.
  virtual void objectMigrationParams(const grt::DictRef &value) {
    grt::ValueRef ovalue(_objectMigrationParams);
    _objectMigrationParams = value;
    member_changed("objectMigrationParams", ovalue, value);
  }
public:

  /**
   * Getter for attribute selectedSchemataNames (read-only)
   *
   * list of selected schemata names to reverse engineer
   * \par In Python:
   *    value = obj.selectedSchemataNames
   */
  grt::StringListRef selectedSchemataNames() const { return _selectedSchemataNames; }


private: // The next attribute is read-only.
  virtual void selectedSchemataNames(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_selectedSchemataNames);
    _selectedSchemataNames = value;
    member_changed("selectedSchemataNames", ovalue, value);
  }
public:

  // sourceCatalog is owned by db_migration_Migration
  /**
   * Getter for attribute sourceCatalog
   *
   * a catalog object reflecting the reverse engineered assets from the source database
   * \par In Python:
   *    value = obj.sourceCatalog
   */
  db_CatalogRef sourceCatalog() const { return _sourceCatalog; }

  /**
   * Setter for attribute sourceCatalog
   * 
   * a catalog object reflecting the reverse engineered assets from the source database
   * \par In Python:
   *   obj.sourceCatalog = value
   */
  virtual void sourceCatalog(const db_CatalogRef &value) {
    grt::ValueRef ovalue(_sourceCatalog);

    _sourceCatalog = value;
    owned_member_changed("sourceCatalog", ovalue, value);
  }

  // sourceConnection is owned by db_migration_Migration
  /**
   * Getter for attribute sourceConnection
   *
   * connection used for the source database
   * \par In Python:
   *    value = obj.sourceConnection
   */
  db_mgmt_ConnectionRef sourceConnection() const { return _sourceConnection; }

  /**
   * Setter for attribute sourceConnection
   * 
   * connection used for the source database
   * \par In Python:
   *   obj.sourceConnection = value
   */
  virtual void sourceConnection(const db_mgmt_ConnectionRef &value) {
    grt::ValueRef ovalue(_sourceConnection);

    _sourceConnection = value;
    owned_member_changed("sourceConnection", ovalue, value);
  }

  // sourceDBVersion is owned by db_migration_Migration
  /**
   * Getter for attribute sourceDBVersion
   *
   * 
   * \par In Python:
   *    value = obj.sourceDBVersion
   */
  GrtVersionRef sourceDBVersion() const { return _sourceDBVersion; }

  /**
   * Setter for attribute sourceDBVersion
   * 
   * 
   * \par In Python:
   *   obj.sourceDBVersion = value
   */
  virtual void sourceDBVersion(const GrtVersionRef &value) {
    grt::ValueRef ovalue(_sourceDBVersion);

    _sourceDBVersion = value;
    owned_member_changed("sourceDBVersion", ovalue, value);
  }

  // sourceObjects is owned by db_migration_Migration
  /**
   * Getter for attribute sourceObjects (read-only)
   *
   * temporary list of objects that should be migrated
   * \par In Python:
   *    value = obj.sourceObjects
   */
  grt::ListRef<GrtObject> sourceObjects() const { return _sourceObjects; }


private: // The next attribute is read-only.
  virtual void sourceObjects(const grt::ListRef<GrtObject> &value) {
    grt::ValueRef ovalue(_sourceObjects);

    _sourceObjects = value;
    owned_member_changed("sourceObjects", ovalue, value);
  }
public:

  /**
   * Getter for attribute sourceSchemataNames (read-only)
   *
   * list of available schemata names in the source database
   * \par In Python:
   *    value = obj.sourceSchemataNames
   */
  grt::StringListRef sourceSchemataNames() const { return _sourceSchemataNames; }


private: // The next attribute is read-only.
  virtual void sourceSchemataNames(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_sourceSchemataNames);
    _sourceSchemataNames = value;
    member_changed("sourceSchemataNames", ovalue, value);
  }
public:

  // targetCatalog is owned by db_migration_Migration
  /**
   * Getter for attribute targetCatalog
   *
   * the migrated target catalog
   * \par In Python:
   *    value = obj.targetCatalog
   */
  db_CatalogRef targetCatalog() const { return _targetCatalog; }

  /**
   * Setter for attribute targetCatalog
   * 
   * the migrated target catalog
   * \par In Python:
   *   obj.targetCatalog = value
   */
  virtual void targetCatalog(const db_CatalogRef &value) {
    grt::ValueRef ovalue(_targetCatalog);

    _targetCatalog = value;
    owned_member_changed("targetCatalog", ovalue, value);
  }

  // targetConnection is owned by db_migration_Migration
  /**
   * Getter for attribute targetConnection
   *
   * connection used for the target database
   * \par In Python:
   *    value = obj.targetConnection
   */
  db_mgmt_ConnectionRef targetConnection() const { return _targetConnection; }

  /**
   * Setter for attribute targetConnection
   * 
   * connection used for the target database
   * \par In Python:
   *   obj.targetConnection = value
   */
  virtual void targetConnection(const db_mgmt_ConnectionRef &value) {
    grt::ValueRef ovalue(_targetConnection);

    _targetConnection = value;
    owned_member_changed("targetConnection", ovalue, value);
  }

  // targetDBVersion is owned by db_migration_Migration
  /**
   * Getter for attribute targetDBVersion
   *
   * 
   * \par In Python:
   *    value = obj.targetDBVersion
   */
  GrtVersionRef targetDBVersion() const { return _targetDBVersion; }

  /**
   * Setter for attribute targetDBVersion
   * 
   * 
   * \par In Python:
   *   obj.targetDBVersion = value
   */
  virtual void targetDBVersion(const GrtVersionRef &value) {
    grt::ValueRef ovalue(_targetDBVersion);

    _targetDBVersion = value;
    owned_member_changed("targetDBVersion", ovalue, value);
  }

  // targetVersion is owned by db_migration_Migration
  /**
   * Getter for attribute targetVersion
   *
   * the version that the target catalog should have
   * \par In Python:
   *    value = obj.targetVersion
   */
  GrtVersionRef targetVersion() const { return _targetVersion; }

  /**
   * Setter for attribute targetVersion
   * 
   * the version that the target catalog should have
   * \par In Python:
   *   obj.targetVersion = value
   */
  virtual void targetVersion(const GrtVersionRef &value) {
    grt::ValueRef ovalue(_targetVersion);

    _targetVersion = value;
    owned_member_changed("targetVersion", ovalue, value);
  }

  /**
   * Method. 
   * \param type 
   * \param sourceObject 
   * \param targetObject 
   * \param message 
   * \return 
   */
  virtual GrtLogObjectRef addMigrationLogEntry(ssize_t type, const GrtObjectRef &sourceObject, const GrtObjectRef &targetObject, const std::string &message);
  /**
   * Method. 
   * \param sourceObject 
   * \param targetObject 
   * \return 
   */
  virtual GrtLogObjectRef findMigrationLogEntry(const GrtObjectRef &sourceObject, const GrtObjectRef &targetObject);
  /**
   * Method. 
   * \param sourceObject 
   * \return 
   */
  virtual GrtObjectRef lookupMigratedObject(const GrtObjectRef &sourceObject);
  /**
   * Method. 
   * \param targetObject 
   * \return 
   */
  virtual GrtObjectRef lookupSourceObject(const GrtObjectRef &targetObject);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::DictRef _applicationData;
  grt::ListRef<GrtLogObject> _creationLog;// owned
  grt::DictRef _dataBulkTransferParams;
  grt::ListRef<GrtLogObject> _dataTransferLog;// owned
  grt::DictRef _defaultColumnValueMappings;
  grt::ListRef<db_migration_DatatypeMapping> _genericDatatypeMappings;// owned
  grt::StringListRef _ignoreList;
  grt::ListRef<GrtLogObject> _migrationLog;// owned
  grt::DictRef _objectCreationParams;
  grt::DictRef _objectMigrationParams;
  grt::StringListRef _selectedSchemataNames;
  db_CatalogRef _sourceCatalog;// owned
  db_mgmt_ConnectionRef _sourceConnection;// owned
  GrtVersionRef _sourceDBVersion;// owned
  grt::ListRef<GrtObject> _sourceObjects;// owned
  grt::StringListRef _sourceSchemataNames;
  db_CatalogRef _targetCatalog;// owned
  db_mgmt_ConnectionRef _targetConnection;// owned
  GrtVersionRef _targetDBVersion;// owned
  GrtVersionRef _targetVersion;// owned

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_migration_Migration());
  }

  static grt::ValueRef call_addMigrationLogEntry(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_migration_Migration*>(self)->addMigrationLogEntry(grt::IntegerRef::cast_from(args[0]), GrtObjectRef::cast_from(args[1]), GrtObjectRef::cast_from(args[2]), grt::StringRef::cast_from(args[3])); }

  static grt::ValueRef call_findMigrationLogEntry(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_migration_Migration*>(self)->findMigrationLogEntry(GrtObjectRef::cast_from(args[0]), GrtObjectRef::cast_from(args[1])); }

  static grt::ValueRef call_lookupMigratedObject(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_migration_Migration*>(self)->lookupMigratedObject(GrtObjectRef::cast_from(args[0])); }

  static grt::ValueRef call_lookupSourceObject(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_migration_Migration*>(self)->lookupSourceObject(GrtObjectRef::cast_from(args[0])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_migration_Migration::create);
    {
      void (db_migration_Migration::*setter)(const grt::DictRef &) = &db_migration_Migration::applicationData;
      grt::DictRef (db_migration_Migration::*getter)() const = &db_migration_Migration::applicationData;
      meta->bind_member("applicationData", new grt::MetaClass::Property<db_migration_Migration,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::ListRef<GrtLogObject> &) = &db_migration_Migration::creationLog;
      grt::ListRef<GrtLogObject> (db_migration_Migration::*getter)() const = &db_migration_Migration::creationLog;
      meta->bind_member("creationLog", new grt::MetaClass::Property<db_migration_Migration,grt::ListRef<GrtLogObject>>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::DictRef &) = &db_migration_Migration::dataBulkTransferParams;
      grt::DictRef (db_migration_Migration::*getter)() const = &db_migration_Migration::dataBulkTransferParams;
      meta->bind_member("dataBulkTransferParams", new grt::MetaClass::Property<db_migration_Migration,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::ListRef<GrtLogObject> &) = &db_migration_Migration::dataTransferLog;
      grt::ListRef<GrtLogObject> (db_migration_Migration::*getter)() const = &db_migration_Migration::dataTransferLog;
      meta->bind_member("dataTransferLog", new grt::MetaClass::Property<db_migration_Migration,grt::ListRef<GrtLogObject>>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::DictRef &) = &db_migration_Migration::defaultColumnValueMappings;
      grt::DictRef (db_migration_Migration::*getter)() const = &db_migration_Migration::defaultColumnValueMappings;
      meta->bind_member("defaultColumnValueMappings", new grt::MetaClass::Property<db_migration_Migration,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::ListRef<db_migration_DatatypeMapping> &) = &db_migration_Migration::genericDatatypeMappings;
      grt::ListRef<db_migration_DatatypeMapping> (db_migration_Migration::*getter)() const = &db_migration_Migration::genericDatatypeMappings;
      meta->bind_member("genericDatatypeMappings", new grt::MetaClass::Property<db_migration_Migration,grt::ListRef<db_migration_DatatypeMapping>>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::StringListRef &) = &db_migration_Migration::ignoreList;
      grt::StringListRef (db_migration_Migration::*getter)() const = &db_migration_Migration::ignoreList;
      meta->bind_member("ignoreList", new grt::MetaClass::Property<db_migration_Migration,grt::StringListRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::ListRef<GrtLogObject> &) = &db_migration_Migration::migrationLog;
      grt::ListRef<GrtLogObject> (db_migration_Migration::*getter)() const = &db_migration_Migration::migrationLog;
      meta->bind_member("migrationLog", new grt::MetaClass::Property<db_migration_Migration,grt::ListRef<GrtLogObject>>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::DictRef &) = &db_migration_Migration::objectCreationParams;
      grt::DictRef (db_migration_Migration::*getter)() const = &db_migration_Migration::objectCreationParams;
      meta->bind_member("objectCreationParams", new grt::MetaClass::Property<db_migration_Migration,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::DictRef &) = &db_migration_Migration::objectMigrationParams;
      grt::DictRef (db_migration_Migration::*getter)() const = &db_migration_Migration::objectMigrationParams;
      meta->bind_member("objectMigrationParams", new grt::MetaClass::Property<db_migration_Migration,grt::DictRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::StringListRef &) = &db_migration_Migration::selectedSchemataNames;
      grt::StringListRef (db_migration_Migration::*getter)() const = &db_migration_Migration::selectedSchemataNames;
      meta->bind_member("selectedSchemataNames", new grt::MetaClass::Property<db_migration_Migration,grt::StringListRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const db_CatalogRef &) = &db_migration_Migration::sourceCatalog;
      db_CatalogRef (db_migration_Migration::*getter)() const = &db_migration_Migration::sourceCatalog;
      meta->bind_member("sourceCatalog", new grt::MetaClass::Property<db_migration_Migration,db_CatalogRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const db_mgmt_ConnectionRef &) = &db_migration_Migration::sourceConnection;
      db_mgmt_ConnectionRef (db_migration_Migration::*getter)() const = &db_migration_Migration::sourceConnection;
      meta->bind_member("sourceConnection", new grt::MetaClass::Property<db_migration_Migration,db_mgmt_ConnectionRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const GrtVersionRef &) = &db_migration_Migration::sourceDBVersion;
      GrtVersionRef (db_migration_Migration::*getter)() const = &db_migration_Migration::sourceDBVersion;
      meta->bind_member("sourceDBVersion", new grt::MetaClass::Property<db_migration_Migration,GrtVersionRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::ListRef<GrtObject> &) = &db_migration_Migration::sourceObjects;
      grt::ListRef<GrtObject> (db_migration_Migration::*getter)() const = &db_migration_Migration::sourceObjects;
      meta->bind_member("sourceObjects", new grt::MetaClass::Property<db_migration_Migration,grt::ListRef<GrtObject>>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const grt::StringListRef &) = &db_migration_Migration::sourceSchemataNames;
      grt::StringListRef (db_migration_Migration::*getter)() const = &db_migration_Migration::sourceSchemataNames;
      meta->bind_member("sourceSchemataNames", new grt::MetaClass::Property<db_migration_Migration,grt::StringListRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const db_CatalogRef &) = &db_migration_Migration::targetCatalog;
      db_CatalogRef (db_migration_Migration::*getter)() const = &db_migration_Migration::targetCatalog;
      meta->bind_member("targetCatalog", new grt::MetaClass::Property<db_migration_Migration,db_CatalogRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const db_mgmt_ConnectionRef &) = &db_migration_Migration::targetConnection;
      db_mgmt_ConnectionRef (db_migration_Migration::*getter)() const = &db_migration_Migration::targetConnection;
      meta->bind_member("targetConnection", new grt::MetaClass::Property<db_migration_Migration,db_mgmt_ConnectionRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const GrtVersionRef &) = &db_migration_Migration::targetDBVersion;
      GrtVersionRef (db_migration_Migration::*getter)() const = &db_migration_Migration::targetDBVersion;
      meta->bind_member("targetDBVersion", new grt::MetaClass::Property<db_migration_Migration,GrtVersionRef>(getter, setter));
    }
    {
      void (db_migration_Migration::*setter)(const GrtVersionRef &) = &db_migration_Migration::targetVersion;
      GrtVersionRef (db_migration_Migration::*getter)() const = &db_migration_Migration::targetVersion;
      meta->bind_member("targetVersion", new grt::MetaClass::Property<db_migration_Migration,GrtVersionRef>(getter, setter));
    }
    meta->bind_method("addMigrationLogEntry", &db_migration_Migration::call_addMigrationLogEntry);
    meta->bind_method("findMigrationLogEntry", &db_migration_Migration::call_findMigrationLogEntry);
    meta->bind_method("lookupMigratedObject", &db_migration_Migration::call_lookupMigratedObject);
    meta->bind_method("lookupSourceObject", &db_migration_Migration::call_lookupSourceObject);
  }
};



inline void register_structs_db_migration_xml() {
  grt::internal::ClassRegistry::register_class<db_migration_MigrationParameter>();
  grt::internal::ClassRegistry::register_class<db_migration_DatatypeMapping>();
  grt::internal::ClassRegistry::register_class<db_migration_DBPreferences>();
  grt::internal::ClassRegistry::register_class<db_migration_Migration>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_db_migration_xml {
  _autoreg__structs_db_migration_xml() {
    register_structs_db_migration_xml();
  }
} __autoreg__structs_db_migration_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

