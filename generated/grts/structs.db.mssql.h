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
  #ifdef GRT_STRUCTS_DB_MSSQL_EXPORT
  #define GRT_STRUCTS_DB_MSSQL_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_DB_MSSQL_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_DB_MSSQL_PUBLIC
#endif

#include "grts/structs.db.h"

class db_mssql_Sequence;
typedef grt::Ref<db_mssql_Sequence> db_mssql_SequenceRef;
class db_mssql_Synonym;
typedef grt::Ref<db_mssql_Synonym> db_mssql_SynonymRef;
class db_mssql_Routine;
typedef grt::Ref<db_mssql_Routine> db_mssql_RoutineRef;
class db_mssql_RoutineGroup;
typedef grt::Ref<db_mssql_RoutineGroup> db_mssql_RoutineGroupRef;
class db_mssql_View;
typedef grt::Ref<db_mssql_View> db_mssql_ViewRef;
class db_mssql_Trigger;
typedef grt::Ref<db_mssql_Trigger> db_mssql_TriggerRef;
class db_mssql_ForeignKey;
typedef grt::Ref<db_mssql_ForeignKey> db_mssql_ForeignKeyRef;
class db_mssql_IndexColumn;
typedef grt::Ref<db_mssql_IndexColumn> db_mssql_IndexColumnRef;
class db_mssql_Index;
typedef grt::Ref<db_mssql_Index> db_mssql_IndexRef;
class db_mssql_StructuredDatatype;
typedef grt::Ref<db_mssql_StructuredDatatype> db_mssql_StructuredDatatypeRef;
class db_mssql_UserDatatype;
typedef grt::Ref<db_mssql_UserDatatype> db_mssql_UserDatatypeRef;
class db_mssql_SimpleDatatype;
typedef grt::Ref<db_mssql_SimpleDatatype> db_mssql_SimpleDatatypeRef;
class db_mssql_Column;
typedef grt::Ref<db_mssql_Column> db_mssql_ColumnRef;
class db_mssql_Table;
typedef grt::Ref<db_mssql_Table> db_mssql_TableRef;
class db_mssql_Schema;
typedef grt::Ref<db_mssql_Schema> db_mssql_SchemaRef;
class db_mssql_Catalog;
typedef grt::Ref<db_mssql_Catalog> db_mssql_CatalogRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** a MSSQL database sequence object */
class  db_mssql_Sequence : public db_Sequence {
  typedef db_Sequence super;

public:
  db_mssql_Sequence(grt::MetaClass *meta = nullptr)
    : db_Sequence(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.Sequence";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Sequence());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Sequence::create);
  }
};

/** a MSSQL synonym object */
class  db_mssql_Synonym : public db_Synonym {
  typedef db_Synonym super;

public:
  db_mssql_Synonym(grt::MetaClass *meta = nullptr)
    : db_Synonym(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.Synonym";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Synonym());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Synonym::create);
  }
};

/** a MSSQL database routine object */
class  db_mssql_Routine : public db_Routine {
  typedef db_Routine super;

public:
  db_mssql_Routine(grt::MetaClass *meta = nullptr)
    : db_Routine(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.Routine";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Routine());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Routine::create);
  }
};

/** a MSSQL database routine group */
class  db_mssql_RoutineGroup : public db_RoutineGroup {
  typedef db_RoutineGroup super;

public:
  db_mssql_RoutineGroup(grt::MetaClass *meta = nullptr)
    : db_RoutineGroup(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.RoutineGroup";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_RoutineGroup());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_RoutineGroup::create);
  }
};

/** a MSSQL database view object */
class  db_mssql_View : public db_View {
  typedef db_View super;

public:
  db_mssql_View(grt::MetaClass *meta = nullptr)
    : db_View(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.View";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_View());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_View::create);
  }
};

class  db_mssql_Trigger : public db_Trigger {
  typedef db_Trigger super;

public:
  db_mssql_Trigger(grt::MetaClass *meta = nullptr)
    : db_Trigger(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.Trigger";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Trigger());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Trigger::create);
  }
};

class  db_mssql_ForeignKey : public db_ForeignKey {
  typedef db_ForeignKey super;

public:
  db_mssql_ForeignKey(grt::MetaClass *meta = nullptr)
    : db_ForeignKey(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.ForeignKey";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_ForeignKey());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_ForeignKey::create);
  }
};

class  db_mssql_IndexColumn : public db_IndexColumn {
  typedef db_IndexColumn super;

public:
  db_mssql_IndexColumn(grt::MetaClass *meta = nullptr)
    : db_IndexColumn(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.IndexColumn";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_IndexColumn());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_IndexColumn::create);
  }
};

class  db_mssql_Index : public db_Index {
  typedef db_Index super;

public:
  db_mssql_Index(grt::MetaClass *meta = nullptr)
    : db_Index(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _clustered(0),
      _filterDefinition(""),
      _hasFilter(0),
      _ignoreDuplicateRows(0) {
  }

  static std::string static_class_name() {
    return "db.mssql.Index";
  }

  /**
   * Getter for attribute clustered
   *
   * 
   * \par In Python:
   *    value = obj.clustered
   */
  grt::IntegerRef clustered() const { return _clustered; }

  /**
   * Setter for attribute clustered
   * 
   * 
   * \par In Python:
   *   obj.clustered = value
   */
  virtual void clustered(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_clustered);
    _clustered = value;
    member_changed("clustered", ovalue, value);
  }

  /**
   * Getter for attribute filterDefinition
   *
   * the definition of the filter associated to the index (expression for the subset of rows included in the filtered index)
   * \par In Python:
   *    value = obj.filterDefinition
   */
  grt::StringRef filterDefinition() const { return _filterDefinition; }

  /**
   * Setter for attribute filterDefinition
   * 
   * the definition of the filter associated to the index (expression for the subset of rows included in the filtered index)
   * \par In Python:
   *   obj.filterDefinition = value
   */
  virtual void filterDefinition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_filterDefinition);
    _filterDefinition = value;
    member_changed("filterDefinition", ovalue, value);
  }

  /**
   * Getter for attribute hasFilter
   *
   * whether there is a filter associated to the index
   * \par In Python:
   *    value = obj.hasFilter
   */
  grt::IntegerRef hasFilter() const { return _hasFilter; }

  /**
   * Setter for attribute hasFilter
   * 
   * whether there is a filter associated to the index
   * \par In Python:
   *   obj.hasFilter = value
   */
  virtual void hasFilter(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_hasFilter);
    _hasFilter = value;
    member_changed("hasFilter", ovalue, value);
  }

  /**
   * Getter for attribute ignoreDuplicateRows
   *
   * 
   * \par In Python:
   *    value = obj.ignoreDuplicateRows
   */
  grt::IntegerRef ignoreDuplicateRows() const { return _ignoreDuplicateRows; }

  /**
   * Setter for attribute ignoreDuplicateRows
   * 
   * 
   * \par In Python:
   *   obj.ignoreDuplicateRows = value
   */
  virtual void ignoreDuplicateRows(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_ignoreDuplicateRows);
    _ignoreDuplicateRows = value;
    member_changed("ignoreDuplicateRows", ovalue, value);
  }

protected:

  grt::IntegerRef _clustered;
  grt::StringRef _filterDefinition;
  grt::IntegerRef _hasFilter;
  grt::IntegerRef _ignoreDuplicateRows;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Index());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Index::create);
    {
      void (db_mssql_Index::*setter)(const grt::IntegerRef &) = &db_mssql_Index::clustered;
      grt::IntegerRef (db_mssql_Index::*getter)() const = &db_mssql_Index::clustered;
      meta->bind_member("clustered", new grt::MetaClass::Property<db_mssql_Index,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mssql_Index::*setter)(const grt::StringRef &) = &db_mssql_Index::filterDefinition;
      grt::StringRef (db_mssql_Index::*getter)() const = &db_mssql_Index::filterDefinition;
      meta->bind_member("filterDefinition", new grt::MetaClass::Property<db_mssql_Index,grt::StringRef>(getter, setter));
    }
    {
      void (db_mssql_Index::*setter)(const grt::IntegerRef &) = &db_mssql_Index::hasFilter;
      grt::IntegerRef (db_mssql_Index::*getter)() const = &db_mssql_Index::hasFilter;
      meta->bind_member("hasFilter", new grt::MetaClass::Property<db_mssql_Index,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mssql_Index::*setter)(const grt::IntegerRef &) = &db_mssql_Index::ignoreDuplicateRows;
      grt::IntegerRef (db_mssql_Index::*getter)() const = &db_mssql_Index::ignoreDuplicateRows;
      meta->bind_member("ignoreDuplicateRows", new grt::MetaClass::Property<db_mssql_Index,grt::IntegerRef>(getter, setter));
    }
  }
};

/** a MSSQL structured datatype object */
class  db_mssql_StructuredDatatype : public db_StructuredDatatype {
  typedef db_StructuredDatatype super;

public:
  db_mssql_StructuredDatatype(grt::MetaClass *meta = nullptr)
    : db_StructuredDatatype(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.StructuredDatatype";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_StructuredDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_StructuredDatatype::create);
  }
};

class  db_mssql_UserDatatype : public db_UserDatatype {
  typedef db_UserDatatype super;

public:
  db_mssql_UserDatatype(grt::MetaClass *meta = nullptr)
    : db_UserDatatype(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _characterMaximumLength(0),
      _isNullable(0),
      _numericPrecision(0),
      _numericScale(0) {
  }

  static std::string static_class_name() {
    return "db.mssql.UserDatatype";
  }

  /**
   * Getter for attribute characterMaximumLength
   *
   * maximum number of characters this datatype can store
   * \par In Python:
   *    value = obj.characterMaximumLength
   */
  grt::IntegerRef characterMaximumLength() const { return _characterMaximumLength; }

  /**
   * Setter for attribute characterMaximumLength
   * 
   * maximum number of characters this datatype can store
   * \par In Python:
   *   obj.characterMaximumLength = value
   */
  virtual void characterMaximumLength(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_characterMaximumLength);
    _characterMaximumLength = value;
    member_changed("characterMaximumLength", ovalue, value);
  }

  /**
   * Getter for attribute isNullable
   *
   * whether NULL is a permitted value
   * \par In Python:
   *    value = obj.isNullable
   */
  grt::IntegerRef isNullable() const { return _isNullable; }

  /**
   * Setter for attribute isNullable
   * 
   * whether NULL is a permitted value
   * \par In Python:
   *   obj.isNullable = value
   */
  virtual void isNullable(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isNullable);
    _isNullable = value;
    member_changed("isNullable", ovalue, value);
  }

  /**
   * Getter for attribute numericPrecision
   *
   * maximum numbers of digits the datatype can store
   * \par In Python:
   *    value = obj.numericPrecision
   */
  grt::IntegerRef numericPrecision() const { return _numericPrecision; }

  /**
   * Setter for attribute numericPrecision
   * 
   * maximum numbers of digits the datatype can store
   * \par In Python:
   *   obj.numericPrecision = value
   */
  virtual void numericPrecision(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_numericPrecision);
    _numericPrecision = value;
    member_changed("numericPrecision", ovalue, value);
  }

  /**
   * Getter for attribute numericScale
   *
   * maximum numbers of digits right from the decimal point the datatype can store
   * \par In Python:
   *    value = obj.numericScale
   */
  grt::IntegerRef numericScale() const { return _numericScale; }

  /**
   * Setter for attribute numericScale
   * 
   * maximum numbers of digits right from the decimal point the datatype can store
   * \par In Python:
   *   obj.numericScale = value
   */
  virtual void numericScale(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_numericScale);
    _numericScale = value;
    member_changed("numericScale", ovalue, value);
  }

protected:

  grt::IntegerRef _characterMaximumLength;
  grt::IntegerRef _isNullable;
  grt::IntegerRef _numericPrecision;
  grt::IntegerRef _numericScale;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_UserDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_UserDatatype::create);
    {
      void (db_mssql_UserDatatype::*setter)(const grt::IntegerRef &) = &db_mssql_UserDatatype::characterMaximumLength;
      grt::IntegerRef (db_mssql_UserDatatype::*getter)() const = &db_mssql_UserDatatype::characterMaximumLength;
      meta->bind_member("characterMaximumLength", new grt::MetaClass::Property<db_mssql_UserDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mssql_UserDatatype::*setter)(const grt::IntegerRef &) = &db_mssql_UserDatatype::isNullable;
      grt::IntegerRef (db_mssql_UserDatatype::*getter)() const = &db_mssql_UserDatatype::isNullable;
      meta->bind_member("isNullable", new grt::MetaClass::Property<db_mssql_UserDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mssql_UserDatatype::*setter)(const grt::IntegerRef &) = &db_mssql_UserDatatype::numericPrecision;
      grt::IntegerRef (db_mssql_UserDatatype::*getter)() const = &db_mssql_UserDatatype::numericPrecision;
      meta->bind_member("numericPrecision", new grt::MetaClass::Property<db_mssql_UserDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mssql_UserDatatype::*setter)(const grt::IntegerRef &) = &db_mssql_UserDatatype::numericScale;
      grt::IntegerRef (db_mssql_UserDatatype::*getter)() const = &db_mssql_UserDatatype::numericScale;
      meta->bind_member("numericScale", new grt::MetaClass::Property<db_mssql_UserDatatype,grt::IntegerRef>(getter, setter));
    }
  }
};

class  db_mssql_SimpleDatatype : public db_SimpleDatatype {
  typedef db_SimpleDatatype super;

public:
  db_mssql_SimpleDatatype(grt::MetaClass *meta = nullptr)
    : db_SimpleDatatype(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mssql.SimpleDatatype";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_SimpleDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_SimpleDatatype::create);
  }
};

class  db_mssql_Column : public db_Column {
  typedef db_Column super;

public:
  db_mssql_Column(grt::MetaClass *meta = nullptr)
    : db_Column(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _computed(0),
      _identity(0) {
  }

  static std::string static_class_name() {
    return "db.mssql.Column";
  }

  /**
   * Getter for attribute computed
   *
   * 
   * \par In Python:
   *    value = obj.computed
   */
  grt::IntegerRef computed() const { return _computed; }

  /**
   * Setter for attribute computed
   * 
   * 
   * \par In Python:
   *   obj.computed = value
   */
  virtual void computed(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_computed);
    _computed = value;
    member_changed("computed", ovalue, value);
  }

  /**
   * Getter for attribute identity
   *
   * 
   * \par In Python:
   *    value = obj.identity
   */
  grt::IntegerRef identity() const { return _identity; }

  /**
   * Setter for attribute identity
   * 
   * 
   * \par In Python:
   *   obj.identity = value
   */
  virtual void identity(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_identity);
    _identity = value;
    member_changed("identity", ovalue, value);
  }

protected:

  grt::IntegerRef _computed;
  grt::IntegerRef _identity;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Column());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Column::create);
    {
      void (db_mssql_Column::*setter)(const grt::IntegerRef &) = &db_mssql_Column::computed;
      grt::IntegerRef (db_mssql_Column::*getter)() const = &db_mssql_Column::computed;
      meta->bind_member("computed", new grt::MetaClass::Property<db_mssql_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mssql_Column::*setter)(const grt::IntegerRef &) = &db_mssql_Column::identity;
      grt::IntegerRef (db_mssql_Column::*getter)() const = &db_mssql_Column::identity;
      meta->bind_member("identity", new grt::MetaClass::Property<db_mssql_Column,grt::IntegerRef>(getter, setter));
    }
  }
};

/** a MSSQL database table object */
class  db_mssql_Table : public db_Table {
  typedef db_Table super;

public:
  db_mssql_Table(grt::MetaClass *meta = nullptr)
    : db_Table(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _createdDatetime("") {
  }

  static std::string static_class_name() {
    return "db.mssql.Table";
  }

  /**
   * Getter for attribute createdDatetime
   *
   * 
   * \par In Python:
   *    value = obj.createdDatetime
   */
  grt::StringRef createdDatetime() const { return _createdDatetime; }

  /**
   * Setter for attribute createdDatetime
   * 
   * 
   * \par In Python:
   *   obj.createdDatetime = value
   */
  virtual void createdDatetime(const grt::StringRef &value) {
    grt::ValueRef ovalue(_createdDatetime);
    _createdDatetime = value;
    member_changed("createdDatetime", ovalue, value);
  }

protected:

  grt::StringRef _createdDatetime;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Table());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Table::create);
    {
      void (db_mssql_Table::*setter)(const grt::StringRef &) = &db_mssql_Table::createdDatetime;
      grt::StringRef (db_mssql_Table::*getter)() const = &db_mssql_Table::createdDatetime;
      meta->bind_member("createdDatetime", new grt::MetaClass::Property<db_mssql_Table,grt::StringRef>(getter, setter));
    }
  }
};

class  db_mssql_Schema : public db_Schema {
  typedef db_Schema super;

public:
  db_mssql_Schema(grt::MetaClass *meta = nullptr)
    : db_Schema(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
    _routineGroups.content().__retype(grt::ObjectType, "db.mssql.RoutineGroup");
    _routines.content().__retype(grt::ObjectType, "db.mssql.Routine");
    _sequences.content().__retype(grt::ObjectType, "db.mssql.Sequence");
    _structuredTypes.content().__retype(grt::ObjectType, "db.mssql.StructuredDatatype");
    _synonyms.content().__retype(grt::ObjectType, "db.mssql.Synonym");
    _tables.content().__retype(grt::ObjectType, "db.mssql.Table");
    _views.content().__retype(grt::ObjectType, "db.mssql.View");
  }

  static std::string static_class_name() {
    return "db.mssql.Schema";
  }

  // routineGroups is owned by db_mssql_Schema
  /**
   * Getter for attribute routineGroups (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.routineGroups
   */
  grt::ListRef<db_mssql_RoutineGroup> routineGroups() const { return grt::ListRef<db_mssql_RoutineGroup>::cast_from(_routineGroups); }


private: // The next attribute is read-only.
public:

  // routines is owned by db_mssql_Schema
  /**
   * Getter for attribute routines (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.routines
   */
  grt::ListRef<db_mssql_Routine> routines() const { return grt::ListRef<db_mssql_Routine>::cast_from(_routines); }


private: // The next attribute is read-only.
public:

  // sequences is owned by db_mssql_Schema
  /**
   * Getter for attribute sequences (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.sequences
   */
  grt::ListRef<db_mssql_Sequence> sequences() const { return grt::ListRef<db_mssql_Sequence>::cast_from(_sequences); }


private: // The next attribute is read-only.
public:

  // structuredTypes is owned by db_mssql_Schema
  /**
   * Getter for attribute structuredTypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.structuredTypes
   */
  grt::ListRef<db_mssql_StructuredDatatype> structuredTypes() const { return grt::ListRef<db_mssql_StructuredDatatype>::cast_from(_structuredTypes); }


private: // The next attribute is read-only.
public:

  // synonyms is owned by db_mssql_Schema
  /**
   * Getter for attribute synonyms (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.synonyms
   */
  grt::ListRef<db_mssql_Synonym> synonyms() const { return grt::ListRef<db_mssql_Synonym>::cast_from(_synonyms); }


private: // The next attribute is read-only.
public:

  // tables is owned by db_mssql_Schema
  /**
   * Getter for attribute tables (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.tables
   */
  grt::ListRef<db_mssql_Table> tables() const { return grt::ListRef<db_mssql_Table>::cast_from(_tables); }


private: // The next attribute is read-only.
public:

  // views is owned by db_mssql_Schema
  /**
   * Getter for attribute views (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.views
   */
  grt::ListRef<db_mssql_View> views() const { return grt::ListRef<db_mssql_View>::cast_from(_views); }


private: // The next attribute is read-only.
public:

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Schema());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Schema::create);
    {
      void (db_mssql_Schema::*setter)(const grt::ListRef<db_mssql_RoutineGroup> &) = 0;
      grt::ListRef<db_mssql_RoutineGroup> (db_mssql_Schema::*getter)() const = 0;
      meta->bind_member("routineGroups", new grt::MetaClass::Property<db_mssql_Schema,grt::ListRef<db_mssql_RoutineGroup>>(getter, setter));
    }
    {
      void (db_mssql_Schema::*setter)(const grt::ListRef<db_mssql_Routine> &) = 0;
      grt::ListRef<db_mssql_Routine> (db_mssql_Schema::*getter)() const = 0;
      meta->bind_member("routines", new grt::MetaClass::Property<db_mssql_Schema,grt::ListRef<db_mssql_Routine>>(getter, setter));
    }
    {
      void (db_mssql_Schema::*setter)(const grt::ListRef<db_mssql_Sequence> &) = 0;
      grt::ListRef<db_mssql_Sequence> (db_mssql_Schema::*getter)() const = 0;
      meta->bind_member("sequences", new grt::MetaClass::Property<db_mssql_Schema,grt::ListRef<db_mssql_Sequence>>(getter, setter));
    }
    {
      void (db_mssql_Schema::*setter)(const grt::ListRef<db_mssql_StructuredDatatype> &) = 0;
      grt::ListRef<db_mssql_StructuredDatatype> (db_mssql_Schema::*getter)() const = 0;
      meta->bind_member("structuredTypes", new grt::MetaClass::Property<db_mssql_Schema,grt::ListRef<db_mssql_StructuredDatatype>>(getter, setter));
    }
    {
      void (db_mssql_Schema::*setter)(const grt::ListRef<db_mssql_Synonym> &) = 0;
      grt::ListRef<db_mssql_Synonym> (db_mssql_Schema::*getter)() const = 0;
      meta->bind_member("synonyms", new grt::MetaClass::Property<db_mssql_Schema,grt::ListRef<db_mssql_Synonym>>(getter, setter));
    }
    {
      void (db_mssql_Schema::*setter)(const grt::ListRef<db_mssql_Table> &) = 0;
      grt::ListRef<db_mssql_Table> (db_mssql_Schema::*getter)() const = 0;
      meta->bind_member("tables", new grt::MetaClass::Property<db_mssql_Schema,grt::ListRef<db_mssql_Table>>(getter, setter));
    }
    {
      void (db_mssql_Schema::*setter)(const grt::ListRef<db_mssql_View> &) = 0;
      grt::ListRef<db_mssql_View> (db_mssql_Schema::*getter)() const = 0;
      meta->bind_member("views", new grt::MetaClass::Property<db_mssql_Schema,grt::ListRef<db_mssql_View>>(getter, setter));
    }
  }
};

class  db_mssql_Catalog : public db_Catalog {
  typedef db_Catalog super;

public:
  db_mssql_Catalog(grt::MetaClass *meta = nullptr)
    : db_Catalog(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
    _schemata.content().__retype(grt::ObjectType, "db.mssql.Schema");
  }

  static std::string static_class_name() {
    return "db.mssql.Catalog";
  }

  // schemata is owned by db_mssql_Catalog
  /**
   * Getter for attribute schemata (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.schemata
   */
  grt::ListRef<db_mssql_Schema> schemata() const { return grt::ListRef<db_mssql_Schema>::cast_from(_schemata); }


private: // The next attribute is read-only.
public:

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mssql_Catalog());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mssql_Catalog::create);
    {
      void (db_mssql_Catalog::*setter)(const grt::ListRef<db_mssql_Schema> &) = 0;
      grt::ListRef<db_mssql_Schema> (db_mssql_Catalog::*getter)() const = 0;
      meta->bind_member("schemata", new grt::MetaClass::Property<db_mssql_Catalog,grt::ListRef<db_mssql_Schema>>(getter, setter));
    }
  }
};



inline void register_structs_db_mssql_xml() {
  grt::internal::ClassRegistry::register_class<db_mssql_Sequence>();
  grt::internal::ClassRegistry::register_class<db_mssql_Synonym>();
  grt::internal::ClassRegistry::register_class<db_mssql_Routine>();
  grt::internal::ClassRegistry::register_class<db_mssql_RoutineGroup>();
  grt::internal::ClassRegistry::register_class<db_mssql_View>();
  grt::internal::ClassRegistry::register_class<db_mssql_Trigger>();
  grt::internal::ClassRegistry::register_class<db_mssql_ForeignKey>();
  grt::internal::ClassRegistry::register_class<db_mssql_IndexColumn>();
  grt::internal::ClassRegistry::register_class<db_mssql_Index>();
  grt::internal::ClassRegistry::register_class<db_mssql_StructuredDatatype>();
  grt::internal::ClassRegistry::register_class<db_mssql_UserDatatype>();
  grt::internal::ClassRegistry::register_class<db_mssql_SimpleDatatype>();
  grt::internal::ClassRegistry::register_class<db_mssql_Column>();
  grt::internal::ClassRegistry::register_class<db_mssql_Table>();
  grt::internal::ClassRegistry::register_class<db_mssql_Schema>();
  grt::internal::ClassRegistry::register_class<db_mssql_Catalog>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_db_mssql_xml {
  _autoreg__structs_db_mssql_xml() {
    register_structs_db_mssql_xml();
  }
} __autoreg__structs_db_mssql_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

