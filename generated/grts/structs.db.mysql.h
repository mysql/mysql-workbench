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
  #ifdef GRT_STRUCTS_DB_MYSQL_EXPORT
  #define GRT_STRUCTS_DB_MYSQL_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_DB_MYSQL_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_DB_MYSQL_PUBLIC
#endif

#include "grts/structs.db.h"
#include "grts/structs.h"

class db_mysql_StorageEngine;
typedef grt::Ref<db_mysql_StorageEngine> db_mysql_StorageEngineRef;
class db_mysql_StorageEngineOption;
typedef grt::Ref<db_mysql_StorageEngineOption> db_mysql_StorageEngineOptionRef;
class db_mysql_Sequence;
typedef grt::Ref<db_mysql_Sequence> db_mysql_SequenceRef;
class db_mysql_Synonym;
typedef grt::Ref<db_mysql_Synonym> db_mysql_SynonymRef;
class db_mysql_RoutineParam;
typedef grt::Ref<db_mysql_RoutineParam> db_mysql_RoutineParamRef;
class db_mysql_Routine;
typedef grt::Ref<db_mysql_Routine> db_mysql_RoutineRef;
class db_mysql_RoutineGroup;
typedef grt::Ref<db_mysql_RoutineGroup> db_mysql_RoutineGroupRef;
class db_mysql_View;
typedef grt::Ref<db_mysql_View> db_mysql_ViewRef;
class db_mysql_Event;
typedef grt::Ref<db_mysql_Event> db_mysql_EventRef;
class db_mysql_Trigger;
typedef grt::Ref<db_mysql_Trigger> db_mysql_TriggerRef;
class db_mysql_ForeignKey;
typedef grt::Ref<db_mysql_ForeignKey> db_mysql_ForeignKeyRef;
class db_mysql_IndexColumn;
typedef grt::Ref<db_mysql_IndexColumn> db_mysql_IndexColumnRef;
class db_mysql_Index;
typedef grt::Ref<db_mysql_Index> db_mysql_IndexRef;
class db_mysql_StructuredDatatype;
typedef grt::Ref<db_mysql_StructuredDatatype> db_mysql_StructuredDatatypeRef;
class db_mysql_SimpleDatatype;
typedef grt::Ref<db_mysql_SimpleDatatype> db_mysql_SimpleDatatypeRef;
class db_mysql_Column;
typedef grt::Ref<db_mysql_Column> db_mysql_ColumnRef;
class db_mysql_Table;
typedef grt::Ref<db_mysql_Table> db_mysql_TableRef;
class db_mysql_PartitionDefinition;
typedef grt::Ref<db_mysql_PartitionDefinition> db_mysql_PartitionDefinitionRef;
class db_mysql_ServerLink;
typedef grt::Ref<db_mysql_ServerLink> db_mysql_ServerLinkRef;
class db_mysql_Tablespace;
typedef grt::Ref<db_mysql_Tablespace> db_mysql_TablespaceRef;
class db_mysql_LogFileGroup;
typedef grt::Ref<db_mysql_LogFileGroup> db_mysql_LogFileGroupRef;
class db_mysql_Schema;
typedef grt::Ref<db_mysql_Schema> db_mysql_SchemaRef;
class db_mysql_Catalog;
typedef grt::Ref<db_mysql_Catalog> db_mysql_CatalogRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** a MySQL storage engine type description */
class  db_mysql_StorageEngine : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  db_mysql_StorageEngine(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _description(""),
      _options(this, false),
      _supportsForeignKeys(0) {
  }

  static std::string static_class_name() {
    return "db.mysql.StorageEngine";
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

  // options is owned by db_mysql_StorageEngine
  /**
   * Getter for attribute options (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.options
   */
  grt::ListRef<db_mysql_StorageEngineOption> options() const { return _options; }


private: // The next attribute is read-only.
  virtual void options(const grt::ListRef<db_mysql_StorageEngineOption> &value) {
    grt::ValueRef ovalue(_options);

    _options = value;
    owned_member_changed("options", ovalue, value);
  }
public:

  /**
   * Getter for attribute supportsForeignKeys
   *
   * 
   * \par In Python:
   *    value = obj.supportsForeignKeys
   */
  grt::IntegerRef supportsForeignKeys() const { return _supportsForeignKeys; }

  /**
   * Setter for attribute supportsForeignKeys
   * 
   * 
   * \par In Python:
   *   obj.supportsForeignKeys = value
   */
  virtual void supportsForeignKeys(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_supportsForeignKeys);
    _supportsForeignKeys = value;
    member_changed("supportsForeignKeys", ovalue, value);
  }

protected:

  grt::StringRef _caption;
  grt::StringRef _description;
  grt::ListRef<db_mysql_StorageEngineOption> _options;// owned
  grt::IntegerRef _supportsForeignKeys;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_StorageEngine());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_StorageEngine::create);
    {
      void (db_mysql_StorageEngine::*setter)(const grt::StringRef &) = &db_mysql_StorageEngine::caption;
      grt::StringRef (db_mysql_StorageEngine::*getter)() const = &db_mysql_StorageEngine::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<db_mysql_StorageEngine,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_StorageEngine::*setter)(const grt::StringRef &) = &db_mysql_StorageEngine::description;
      grt::StringRef (db_mysql_StorageEngine::*getter)() const = &db_mysql_StorageEngine::description;
      meta->bind_member("description", new grt::MetaClass::Property<db_mysql_StorageEngine,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_StorageEngine::*setter)(const grt::ListRef<db_mysql_StorageEngineOption> &) = &db_mysql_StorageEngine::options;
      grt::ListRef<db_mysql_StorageEngineOption> (db_mysql_StorageEngine::*getter)() const = &db_mysql_StorageEngine::options;
      meta->bind_member("options", new grt::MetaClass::Property<db_mysql_StorageEngine,grt::ListRef<db_mysql_StorageEngineOption>>(getter, setter));
    }
    {
      void (db_mysql_StorageEngine::*setter)(const grt::IntegerRef &) = &db_mysql_StorageEngine::supportsForeignKeys;
      grt::IntegerRef (db_mysql_StorageEngine::*getter)() const = &db_mysql_StorageEngine::supportsForeignKeys;
      meta->bind_member("supportsForeignKeys", new grt::MetaClass::Property<db_mysql_StorageEngine,grt::IntegerRef>(getter, setter));
    }
  }
};

/** an option description for a MySQL storage engine */
class  db_mysql_StorageEngineOption : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  db_mysql_StorageEngineOption(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _description(""),
      _type("") {
  }

  static std::string static_class_name() {
    return "db.mysql.StorageEngineOption";
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

  /**
   * Getter for attribute type
   *
   * 
   * \par In Python:
   *    value = obj.type
   */
  grt::StringRef type() const { return _type; }

  /**
   * Setter for attribute type
   * 
   * 
   * \par In Python:
   *   obj.type = value
   */
  virtual void type(const grt::StringRef &value) {
    grt::ValueRef ovalue(_type);
    _type = value;
    member_changed("type", ovalue, value);
  }

protected:

  grt::StringRef _caption;
  grt::StringRef _description;
  grt::StringRef _type;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_StorageEngineOption());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_StorageEngineOption::create);
    {
      void (db_mysql_StorageEngineOption::*setter)(const grt::StringRef &) = &db_mysql_StorageEngineOption::caption;
      grt::StringRef (db_mysql_StorageEngineOption::*getter)() const = &db_mysql_StorageEngineOption::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<db_mysql_StorageEngineOption,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_StorageEngineOption::*setter)(const grt::StringRef &) = &db_mysql_StorageEngineOption::description;
      grt::StringRef (db_mysql_StorageEngineOption::*getter)() const = &db_mysql_StorageEngineOption::description;
      meta->bind_member("description", new grt::MetaClass::Property<db_mysql_StorageEngineOption,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_StorageEngineOption::*setter)(const grt::StringRef &) = &db_mysql_StorageEngineOption::type;
      grt::StringRef (db_mysql_StorageEngineOption::*getter)() const = &db_mysql_StorageEngineOption::type;
      meta->bind_member("type", new grt::MetaClass::Property<db_mysql_StorageEngineOption,grt::StringRef>(getter, setter));
    }
  }
};

/** a MySQL database sequence object */
class  db_mysql_Sequence : public db_Sequence {
  typedef db_Sequence super;

public:
  db_mysql_Sequence(grt::MetaClass *meta = nullptr)
    : db_Sequence(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.Sequence";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Sequence());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Sequence::create);
  }
};

/** a MySQL synonym object */
class  db_mysql_Synonym : public db_Synonym {
  typedef db_Synonym super;

public:
  db_mysql_Synonym(grt::MetaClass *meta = nullptr)
    : db_Synonym(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.Synonym";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Synonym());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Synonym::create);
  }
};

class  db_mysql_RoutineParam : public GrtObject {
  typedef GrtObject super;

public:
  db_mysql_RoutineParam(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _datatype(""),
      _paramType("") {
  }

  static std::string static_class_name() {
    return "db.mysql.RoutineParam";
  }

  /**
   * Getter for attribute datatype
   *
   * 
   * \par In Python:
   *    value = obj.datatype
   */
  grt::StringRef datatype() const { return _datatype; }

  /**
   * Setter for attribute datatype
   * 
   * 
   * \par In Python:
   *   obj.datatype = value
   */
  virtual void datatype(const grt::StringRef &value) {
    grt::ValueRef ovalue(_datatype);
    _datatype = value;
    member_changed("datatype", ovalue, value);
  }

  /**
   * Getter for attribute paramType
   *
   * 
   * \par In Python:
   *    value = obj.paramType
   */
  grt::StringRef paramType() const { return _paramType; }

  /**
   * Setter for attribute paramType
   * 
   * 
   * \par In Python:
   *   obj.paramType = value
   */
  virtual void paramType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_paramType);
    _paramType = value;
    member_changed("paramType", ovalue, value);
  }

protected:

  grt::StringRef _datatype;
  grt::StringRef _paramType;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_RoutineParam());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_RoutineParam::create);
    {
      void (db_mysql_RoutineParam::*setter)(const grt::StringRef &) = &db_mysql_RoutineParam::datatype;
      grt::StringRef (db_mysql_RoutineParam::*getter)() const = &db_mysql_RoutineParam::datatype;
      meta->bind_member("datatype", new grt::MetaClass::Property<db_mysql_RoutineParam,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_RoutineParam::*setter)(const grt::StringRef &) = &db_mysql_RoutineParam::paramType;
      grt::StringRef (db_mysql_RoutineParam::*getter)() const = &db_mysql_RoutineParam::paramType;
      meta->bind_member("paramType", new grt::MetaClass::Property<db_mysql_RoutineParam,grt::StringRef>(getter, setter));
    }
  }
};

class  db_mysql_Routine : public db_Routine {
  typedef db_Routine super;

public:
  db_mysql_Routine(grt::MetaClass *meta = nullptr)
    : db_Routine(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _params(this, false),
      _returnDatatype(""),
      _security("") {
  }

  static std::string static_class_name() {
    return "db.mysql.Routine";
  }

  // params is owned by db_mysql_Routine
  /**
   * Getter for attribute params (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.params
   */
  grt::ListRef<db_mysql_RoutineParam> params() const { return _params; }


private: // The next attribute is read-only.
  virtual void params(const grt::ListRef<db_mysql_RoutineParam> &value) {
    grt::ValueRef ovalue(_params);

    _params = value;
    owned_member_changed("params", ovalue, value);
  }
public:

  /**
   * Getter for attribute returnDatatype
   *
   * 
   * \par In Python:
   *    value = obj.returnDatatype
   */
  grt::StringRef returnDatatype() const { return _returnDatatype; }

  /**
   * Setter for attribute returnDatatype
   * 
   * 
   * \par In Python:
   *   obj.returnDatatype = value
   */
  virtual void returnDatatype(const grt::StringRef &value) {
    grt::ValueRef ovalue(_returnDatatype);
    _returnDatatype = value;
    member_changed("returnDatatype", ovalue, value);
  }

  /**
   * Getter for attribute security
   *
   * 
   * \par In Python:
   *    value = obj.security
   */
  grt::StringRef security() const { return _security; }

  /**
   * Setter for attribute security
   * 
   * 
   * \par In Python:
   *   obj.security = value
   */
  virtual void security(const grt::StringRef &value) {
    grt::ValueRef ovalue(_security);
    _security = value;
    member_changed("security", ovalue, value);
  }

protected:

  grt::ListRef<db_mysql_RoutineParam> _params;// owned
  grt::StringRef _returnDatatype;
  grt::StringRef _security;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Routine());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Routine::create);
    {
      void (db_mysql_Routine::*setter)(const grt::ListRef<db_mysql_RoutineParam> &) = &db_mysql_Routine::params;
      grt::ListRef<db_mysql_RoutineParam> (db_mysql_Routine::*getter)() const = &db_mysql_Routine::params;
      meta->bind_member("params", new grt::MetaClass::Property<db_mysql_Routine,grt::ListRef<db_mysql_RoutineParam>>(getter, setter));
    }
    {
      void (db_mysql_Routine::*setter)(const grt::StringRef &) = &db_mysql_Routine::returnDatatype;
      grt::StringRef (db_mysql_Routine::*getter)() const = &db_mysql_Routine::returnDatatype;
      meta->bind_member("returnDatatype", new grt::MetaClass::Property<db_mysql_Routine,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Routine::*setter)(const grt::StringRef &) = &db_mysql_Routine::security;
      grt::StringRef (db_mysql_Routine::*getter)() const = &db_mysql_Routine::security;
      meta->bind_member("security", new grt::MetaClass::Property<db_mysql_Routine,grt::StringRef>(getter, setter));
    }
  }
};

class  db_mysql_RoutineGroup : public db_RoutineGroup {
  typedef db_RoutineGroup super;

public:
  db_mysql_RoutineGroup(grt::MetaClass *meta = nullptr)
    : db_RoutineGroup(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.RoutineGroup";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_RoutineGroup());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_RoutineGroup::create);
  }
};

class  db_mysql_View : public db_View {
  typedef db_View super;

public:
  db_mysql_View(grt::MetaClass *meta = nullptr)
    : db_View(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.View";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_View());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_View::create);
  }
};

class  db_mysql_Event : public db_Event {
  typedef db_Event super;

public:
  db_mysql_Event(grt::MetaClass *meta = nullptr)
    : db_Event(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.Event";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Event());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Event::create);
  }
};

class  db_mysql_Trigger : public db_Trigger {
  typedef db_Trigger super;

public:
  db_mysql_Trigger(grt::MetaClass *meta = nullptr)
    : db_Trigger(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.Trigger";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Trigger());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Trigger::create);
  }
};

class  db_mysql_ForeignKey : public db_ForeignKey {
  typedef db_ForeignKey super;

public:
  db_mysql_ForeignKey(grt::MetaClass *meta = nullptr)
    : db_ForeignKey(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.ForeignKey";
  }

  /**
   * Getter for attribute referencedTable
   *
   * 
   * \par In Python:
   *    value = obj.referencedTable
   */
  db_mysql_TableRef referencedTable() const { return db_mysql_TableRef::cast_from(_referencedTable); }

  /**
   * Setter for attribute referencedTable
   * 
   * 
   * \par In Python:
   *   obj.referencedTable = value
   */
  virtual void referencedTable(const db_mysql_TableRef &value) { super::referencedTable(value); }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_ForeignKey());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_ForeignKey::create);
    {
      void (db_mysql_ForeignKey::*setter)(const db_mysql_TableRef &) = 0;
      db_mysql_TableRef (db_mysql_ForeignKey::*getter)() const = 0;
      meta->bind_member("referencedTable", new grt::MetaClass::Property<db_mysql_ForeignKey,db_mysql_TableRef>(getter, setter));
    }
  }
};

class  db_mysql_IndexColumn : public db_IndexColumn {
  typedef db_IndexColumn super;

public:
  db_mysql_IndexColumn(grt::MetaClass *meta = nullptr)
    : db_IndexColumn(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.IndexColumn";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_IndexColumn());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_IndexColumn::create);
  }
};

class  db_mysql_Index : public db_Index {
  typedef db_Index super;

public:
  db_mysql_Index(grt::MetaClass *meta = nullptr)
    : db_Index(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _algorithm(""),
      _indexKind(""),
      _keyBlockSize(0),
      _lockOption(""),
      _visible(1),
      _withParser("") {
    _columns.content().__retype(grt::ObjectType, "db.mysql.IndexColumn");
  }

  static std::string static_class_name() {
    return "db.mysql.Index";
  }

  /**
   * Getter for attribute algorithm
   *
   * one of DEFAULT, INPLACE and COPY
   * \par In Python:
   *    value = obj.algorithm
   */
  grt::StringRef algorithm() const { return _algorithm; }

  /**
   * Setter for attribute algorithm
   * 
   * one of DEFAULT, INPLACE and COPY
   * \par In Python:
   *   obj.algorithm = value
   */
  virtual void algorithm(const grt::StringRef &value) {
    grt::ValueRef ovalue(_algorithm);
    _algorithm = value;
    member_changed("algorithm", ovalue, value);
  }

  // columns is owned by db_mysql_Index
  /**
   * Getter for attribute columns (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.columns
   */
  grt::ListRef<db_mysql_IndexColumn> columns() const { return grt::ListRef<db_mysql_IndexColumn>::cast_from(_columns); }


private: // The next attribute is read-only.
public:

  /**
   * Getter for attribute indexKind
   *
   * one of BTREE, RTREE and HASH
   * \par In Python:
   *    value = obj.indexKind
   */
  grt::StringRef indexKind() const { return _indexKind; }

  /**
   * Setter for attribute indexKind
   * 
   * one of BTREE, RTREE and HASH
   * \par In Python:
   *   obj.indexKind = value
   */
  virtual void indexKind(const grt::StringRef &value) {
    grt::ValueRef ovalue(_indexKind);
    _indexKind = value;
    member_changed("indexKind", ovalue, value);
  }

  /**
   * Getter for attribute keyBlockSize
   *
   * 
   * \par In Python:
   *    value = obj.keyBlockSize
   */
  grt::IntegerRef keyBlockSize() const { return _keyBlockSize; }

  /**
   * Setter for attribute keyBlockSize
   * 
   * 
   * \par In Python:
   *   obj.keyBlockSize = value
   */
  virtual void keyBlockSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_keyBlockSize);
    _keyBlockSize = value;
    member_changed("keyBlockSize", ovalue, value);
  }

  /**
   * Getter for attribute lockOption
   *
   * one of DEFAULT, NONE, SHARED and EXCLUSIVE
   * \par In Python:
   *    value = obj.lockOption
   */
  grt::StringRef lockOption() const { return _lockOption; }

  /**
   * Setter for attribute lockOption
   * 
   * one of DEFAULT, NONE, SHARED and EXCLUSIVE
   * \par In Python:
   *   obj.lockOption = value
   */
  virtual void lockOption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_lockOption);
    _lockOption = value;
    member_changed("lockOption", ovalue, value);
  }

  /**
   * Getter for attribute visible
   *
   * 
   * \par In Python:
   *    value = obj.visible
   */
  grt::IntegerRef visible() const { return _visible; }

  /**
   * Setter for attribute visible
   * 
   * 
   * \par In Python:
   *   obj.visible = value
   */
  virtual void visible(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_visible);
    _visible = value;
    member_changed("visible", ovalue, value);
  }

  /**
   * Getter for attribute withParser
   *
   * 
   * \par In Python:
   *    value = obj.withParser
   */
  grt::StringRef withParser() const { return _withParser; }

  /**
   * Setter for attribute withParser
   * 
   * 
   * \par In Python:
   *   obj.withParser = value
   */
  virtual void withParser(const grt::StringRef &value) {
    grt::ValueRef ovalue(_withParser);
    _withParser = value;
    member_changed("withParser", ovalue, value);
  }

protected:

  grt::StringRef _algorithm;
  grt::StringRef _indexKind;
  grt::IntegerRef _keyBlockSize;
  grt::StringRef _lockOption;
  grt::IntegerRef _visible;
  grt::StringRef _withParser;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Index());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Index::create);
    {
      void (db_mysql_Index::*setter)(const grt::StringRef &) = &db_mysql_Index::algorithm;
      grt::StringRef (db_mysql_Index::*getter)() const = &db_mysql_Index::algorithm;
      meta->bind_member("algorithm", new grt::MetaClass::Property<db_mysql_Index,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Index::*setter)(const grt::ListRef<db_mysql_IndexColumn> &) = 0;
      grt::ListRef<db_mysql_IndexColumn> (db_mysql_Index::*getter)() const = 0;
      meta->bind_member("columns", new grt::MetaClass::Property<db_mysql_Index,grt::ListRef<db_mysql_IndexColumn>>(getter, setter));
    }
    {
      void (db_mysql_Index::*setter)(const grt::StringRef &) = &db_mysql_Index::indexKind;
      grt::StringRef (db_mysql_Index::*getter)() const = &db_mysql_Index::indexKind;
      meta->bind_member("indexKind", new grt::MetaClass::Property<db_mysql_Index,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Index::*setter)(const grt::IntegerRef &) = &db_mysql_Index::keyBlockSize;
      grt::IntegerRef (db_mysql_Index::*getter)() const = &db_mysql_Index::keyBlockSize;
      meta->bind_member("keyBlockSize", new grt::MetaClass::Property<db_mysql_Index,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Index::*setter)(const grt::StringRef &) = &db_mysql_Index::lockOption;
      grt::StringRef (db_mysql_Index::*getter)() const = &db_mysql_Index::lockOption;
      meta->bind_member("lockOption", new grt::MetaClass::Property<db_mysql_Index,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Index::*setter)(const grt::IntegerRef &) = &db_mysql_Index::visible;
      grt::IntegerRef (db_mysql_Index::*getter)() const = &db_mysql_Index::visible;
      meta->bind_member("visible", new grt::MetaClass::Property<db_mysql_Index,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Index::*setter)(const grt::StringRef &) = &db_mysql_Index::withParser;
      grt::StringRef (db_mysql_Index::*getter)() const = &db_mysql_Index::withParser;
      meta->bind_member("withParser", new grt::MetaClass::Property<db_mysql_Index,grt::StringRef>(getter, setter));
    }
  }
};

class  db_mysql_StructuredDatatype : public db_StructuredDatatype {
  typedef db_StructuredDatatype super;

public:
  db_mysql_StructuredDatatype(grt::MetaClass *meta = nullptr)
    : db_StructuredDatatype(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.StructuredDatatype";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_StructuredDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_StructuredDatatype::create);
  }
};

class  db_mysql_SimpleDatatype : public db_SimpleDatatype {
  typedef db_SimpleDatatype super;

public:
  db_mysql_SimpleDatatype(grt::MetaClass *meta = nullptr)
    : db_SimpleDatatype(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.SimpleDatatype";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_SimpleDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_SimpleDatatype::create);
  }
};

class  db_mysql_Column : public db_Column {
  typedef db_Column super;

public:
  db_mysql_Column(grt::MetaClass *meta = nullptr)
    : db_Column(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _autoIncrement(0),
      _expression(""),
      _generated(0),
      _generatedStorage("") {
  }

  static std::string static_class_name() {
    return "db.mysql.Column";
  }

  /**
   * Getter for attribute autoIncrement
   *
   * 
   * \par In Python:
   *    value = obj.autoIncrement
   */
  grt::IntegerRef autoIncrement() const { return _autoIncrement; }

  /**
   * Setter for attribute autoIncrement
   * 
   * 
   * \par In Python:
   *   obj.autoIncrement = value
   */
  virtual void autoIncrement(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_autoIncrement);
    _autoIncrement = value;
    member_changed("autoIncrement", ovalue, value);
  }

  /**
   * Getter for attribute expression
   *
   * The full expression for a generated column as text
   * \par In Python:
   *    value = obj.expression
   */
  grt::StringRef expression() const { return _expression; }

  /**
   * Setter for attribute expression
   * 
   * The full expression for a generated column as text
   * \par In Python:
   *   obj.expression = value
   */
  virtual void expression(const grt::StringRef &value) {
    grt::ValueRef ovalue(_expression);
    _expression = value;
    member_changed("expression", ovalue, value);
  }

  /**
   * Getter for attribute generated
   *
   * 0 or 1, 1 if generated column
   * \par In Python:
   *    value = obj.generated
   */
  grt::IntegerRef generated() const { return _generated; }

  /**
   * Setter for attribute generated
   * 
   * 0 or 1, 1 if generated column
   * \par In Python:
   *   obj.generated = value
   */
  virtual void generated(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_generated);
    _generated = value;
    member_changed("generated", ovalue, value);
  }

  /**
   * Getter for attribute generatedStorage
   *
   * VIRTUAL or STORED, for generated columns only
   * \par In Python:
   *    value = obj.generatedStorage
   */
  grt::StringRef generatedStorage() const { return _generatedStorage; }

  /**
   * Setter for attribute generatedStorage
   * 
   * VIRTUAL or STORED, for generated columns only
   * \par In Python:
   *   obj.generatedStorage = value
   */
  virtual void generatedStorage(const grt::StringRef &value) {
    grt::ValueRef ovalue(_generatedStorage);
    _generatedStorage = value;
    member_changed("generatedStorage", ovalue, value);
  }

protected:

  grt::IntegerRef _autoIncrement;
  grt::StringRef _expression;
  grt::IntegerRef _generated;
  grt::StringRef _generatedStorage;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Column());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Column::create);
    {
      void (db_mysql_Column::*setter)(const grt::IntegerRef &) = &db_mysql_Column::autoIncrement;
      grt::IntegerRef (db_mysql_Column::*getter)() const = &db_mysql_Column::autoIncrement;
      meta->bind_member("autoIncrement", new grt::MetaClass::Property<db_mysql_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Column::*setter)(const grt::StringRef &) = &db_mysql_Column::expression;
      grt::StringRef (db_mysql_Column::*getter)() const = &db_mysql_Column::expression;
      meta->bind_member("expression", new grt::MetaClass::Property<db_mysql_Column,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Column::*setter)(const grt::IntegerRef &) = &db_mysql_Column::generated;
      grt::IntegerRef (db_mysql_Column::*getter)() const = &db_mysql_Column::generated;
      meta->bind_member("generated", new grt::MetaClass::Property<db_mysql_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Column::*setter)(const grt::StringRef &) = &db_mysql_Column::generatedStorage;
      grt::StringRef (db_mysql_Column::*getter)() const = &db_mysql_Column::generatedStorage;
      meta->bind_member("generatedStorage", new grt::MetaClass::Property<db_mysql_Column,grt::StringRef>(getter, setter));
    }
  }
};

class  db_mysql_Table : public db_Table {
  typedef db_Table super;

public:
  db_mysql_Table(grt::MetaClass *meta = nullptr)
    : db_Table(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _avgRowLength(""),
      _checksum(0),
      _connectionString(""),
      _defaultCharacterSetName(""),
      _defaultCollationName(""),
      _delayKeyWrite(0),
      _keyBlockSize(""),
      _maxRows(""),
      _mergeInsert(""),
      _mergeUnion(""),
      _minRows(""),
      _nextAutoInc(""),
      _packKeys(""),
      _partitionCount(0),
      _partitionDefinitions(this, false),
      _partitionExpression(""),
      _partitionKeyAlgorithm(0),
      _partitionType(""),
      _password(""),
      _raidChunkSize(""),
      _raidChunks(""),
      _raidType(""),
      _rowFormat(""),
      _statsAutoRecalc(""),
      _statsPersistent(""),
      _statsSamplePages(0),
      _subpartitionCount(0),
      _subpartitionExpression(""),
      _subpartitionKeyAlgorithm(0),
      _subpartitionType(""),
      _tableDataDir(""),
      _tableEngine(""),
      _tableIndexDir(""),
      _tableSpace("") {
    _columns.content().__retype(grt::ObjectType, "db.mysql.Column");
    _foreignKeys.content().__retype(grt::ObjectType, "db.mysql.ForeignKey");
    _indices.content().__retype(grt::ObjectType, "db.mysql.Index");
    _triggers.content().__retype(grt::ObjectType, "db.mysql.Trigger");
  }

  static std::string static_class_name() {
    return "db.mysql.Table";
  }

  /**
   * Getter for attribute avgRowLength
   *
   * 
   * \par In Python:
   *    value = obj.avgRowLength
   */
  grt::StringRef avgRowLength() const { return _avgRowLength; }

  /**
   * Setter for attribute avgRowLength
   * 
   * 
   * \par In Python:
   *   obj.avgRowLength = value
   */
  virtual void avgRowLength(const grt::StringRef &value) {
    grt::ValueRef ovalue(_avgRowLength);
    _avgRowLength = value;
    member_changed("avgRowLength", ovalue, value);
  }

  /**
   * Getter for attribute checksum
   *
   * 
   * \par In Python:
   *    value = obj.checksum
   */
  grt::IntegerRef checksum() const { return _checksum; }

  /**
   * Setter for attribute checksum
   * 
   * 
   * \par In Python:
   *   obj.checksum = value
   */
  virtual void checksum(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_checksum);
    _checksum = value;
    member_changed("checksum", ovalue, value);
  }

  // columns is owned by db_mysql_Table
  /**
   * Getter for attribute columns (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.columns
   */
  grt::ListRef<db_mysql_Column> columns() const { return grt::ListRef<db_mysql_Column>::cast_from(_columns); }


private: // The next attribute is read-only.
public:

  // connection is owned by db_mysql_Table
  /**
   * Getter for attribute connection
   *
   * if this is a federated table the connection is set to the server link object
   * \par In Python:
   *    value = obj.connection
   */
  db_ServerLinkRef connection() const { return _connection; }

  /**
   * Setter for attribute connection
   * 
   * if this is a federated table the connection is set to the server link object
   * \par In Python:
   *   obj.connection = value
   */
  virtual void connection(const db_ServerLinkRef &value) {
    grt::ValueRef ovalue(_connection);

    _connection = value;
    owned_member_changed("connection", ovalue, value);
  }

  /**
   * Getter for attribute connectionString
   *
   * if this is a federated table the connection is set to the server link object
   * \par In Python:
   *    value = obj.connectionString
   */
  grt::StringRef connectionString() const { return _connectionString; }

  /**
   * Setter for attribute connectionString
   * 
   * if this is a federated table the connection is set to the server link object
   * \par In Python:
   *   obj.connectionString = value
   */
  virtual void connectionString(const grt::StringRef &value) {
    grt::ValueRef ovalue(_connectionString);
    _connectionString = value;
    member_changed("connectionString", ovalue, value);
  }

  /**
   * Getter for attribute defaultCharacterSetName
   *
   * 
   * \par In Python:
   *    value = obj.defaultCharacterSetName
   */
  grt::StringRef defaultCharacterSetName() const { return _defaultCharacterSetName; }

  /**
   * Setter for attribute defaultCharacterSetName
   * 
   * 
   * \par In Python:
   *   obj.defaultCharacterSetName = value
   */
  virtual void defaultCharacterSetName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_defaultCharacterSetName);
    _defaultCharacterSetName = value;
    member_changed("defaultCharacterSetName", ovalue, value);
  }

  /**
   * Getter for attribute defaultCollationName
   *
   * 
   * \par In Python:
   *    value = obj.defaultCollationName
   */
  grt::StringRef defaultCollationName() const { return _defaultCollationName; }

  /**
   * Setter for attribute defaultCollationName
   * 
   * 
   * \par In Python:
   *   obj.defaultCollationName = value
   */
  virtual void defaultCollationName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_defaultCollationName);
    _defaultCollationName = value;
    member_changed("defaultCollationName", ovalue, value);
  }

  /**
   * Getter for attribute delayKeyWrite
   *
   * 
   * \par In Python:
   *    value = obj.delayKeyWrite
   */
  grt::IntegerRef delayKeyWrite() const { return _delayKeyWrite; }

  /**
   * Setter for attribute delayKeyWrite
   * 
   * 
   * \par In Python:
   *   obj.delayKeyWrite = value
   */
  virtual void delayKeyWrite(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_delayKeyWrite);
    _delayKeyWrite = value;
    member_changed("delayKeyWrite", ovalue, value);
  }

  // foreignKeys is owned by db_mysql_Table
  /**
   * Getter for attribute foreignKeys (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.foreignKeys
   */
  grt::ListRef<db_mysql_ForeignKey> foreignKeys() const { return grt::ListRef<db_mysql_ForeignKey>::cast_from(_foreignKeys); }


private: // The next attribute is read-only.
public:

  // indices is owned by db_mysql_Table
  /**
   * Getter for attribute indices (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.indices
   */
  grt::ListRef<db_mysql_Index> indices() const { return grt::ListRef<db_mysql_Index>::cast_from(_indices); }


private: // The next attribute is read-only.
public:

  /**
   * Getter for attribute keyBlockSize
   *
   * 
   * \par In Python:
   *    value = obj.keyBlockSize
   */
  grt::StringRef keyBlockSize() const { return _keyBlockSize; }

  /**
   * Setter for attribute keyBlockSize
   * 
   * 
   * \par In Python:
   *   obj.keyBlockSize = value
   */
  virtual void keyBlockSize(const grt::StringRef &value) {
    grt::ValueRef ovalue(_keyBlockSize);
    _keyBlockSize = value;
    member_changed("keyBlockSize", ovalue, value);
  }

  /**
   * Getter for attribute maxRows
   *
   * 
   * \par In Python:
   *    value = obj.maxRows
   */
  grt::StringRef maxRows() const { return _maxRows; }

  /**
   * Setter for attribute maxRows
   * 
   * 
   * \par In Python:
   *   obj.maxRows = value
   */
  virtual void maxRows(const grt::StringRef &value) {
    grt::ValueRef ovalue(_maxRows);
    _maxRows = value;
    member_changed("maxRows", ovalue, value);
  }

  /**
   * Getter for attribute mergeInsert
   *
   * 
   * \par In Python:
   *    value = obj.mergeInsert
   */
  grt::StringRef mergeInsert() const { return _mergeInsert; }

  /**
   * Setter for attribute mergeInsert
   * 
   * 
   * \par In Python:
   *   obj.mergeInsert = value
   */
  virtual void mergeInsert(const grt::StringRef &value) {
    grt::ValueRef ovalue(_mergeInsert);
    _mergeInsert = value;
    member_changed("mergeInsert", ovalue, value);
  }

  /**
   * Getter for attribute mergeUnion
   *
   * 
   * \par In Python:
   *    value = obj.mergeUnion
   */
  grt::StringRef mergeUnion() const { return _mergeUnion; }

  /**
   * Setter for attribute mergeUnion
   * 
   * 
   * \par In Python:
   *   obj.mergeUnion = value
   */
  virtual void mergeUnion(const grt::StringRef &value) {
    grt::ValueRef ovalue(_mergeUnion);
    _mergeUnion = value;
    member_changed("mergeUnion", ovalue, value);
  }

  /**
   * Getter for attribute minRows
   *
   * 
   * \par In Python:
   *    value = obj.minRows
   */
  grt::StringRef minRows() const { return _minRows; }

  /**
   * Setter for attribute minRows
   * 
   * 
   * \par In Python:
   *   obj.minRows = value
   */
  virtual void minRows(const grt::StringRef &value) {
    grt::ValueRef ovalue(_minRows);
    _minRows = value;
    member_changed("minRows", ovalue, value);
  }

  /**
   * Getter for attribute nextAutoInc
   *
   * 
   * \par In Python:
   *    value = obj.nextAutoInc
   */
  grt::StringRef nextAutoInc() const { return _nextAutoInc; }

  /**
   * Setter for attribute nextAutoInc
   * 
   * 
   * \par In Python:
   *   obj.nextAutoInc = value
   */
  virtual void nextAutoInc(const grt::StringRef &value) {
    grt::ValueRef ovalue(_nextAutoInc);
    _nextAutoInc = value;
    member_changed("nextAutoInc", ovalue, value);
  }

  /**
   * Getter for attribute packKeys
   *
   * DEFAULT, 0 or 1
   * \par In Python:
   *    value = obj.packKeys
   */
  grt::StringRef packKeys() const { return _packKeys; }

  /**
   * Setter for attribute packKeys
   * 
   * DEFAULT, 0 or 1
   * \par In Python:
   *   obj.packKeys = value
   */
  virtual void packKeys(const grt::StringRef &value) {
    grt::ValueRef ovalue(_packKeys);
    _packKeys = value;
    member_changed("packKeys", ovalue, value);
  }

  /**
   * Getter for attribute partitionCount
   *
   * 
   * \par In Python:
   *    value = obj.partitionCount
   */
  grt::IntegerRef partitionCount() const { return _partitionCount; }

  /**
   * Setter for attribute partitionCount
   * 
   * 
   * \par In Python:
   *   obj.partitionCount = value
   */
  virtual void partitionCount(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_partitionCount);
    _partitionCount = value;
    member_changed("partitionCount", ovalue, value);
  }

  // partitionDefinitions is owned by db_mysql_Table
  /**
   * Getter for attribute partitionDefinitions (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.partitionDefinitions
   */
  grt::ListRef<db_mysql_PartitionDefinition> partitionDefinitions() const { return _partitionDefinitions; }


private: // The next attribute is read-only.
  virtual void partitionDefinitions(const grt::ListRef<db_mysql_PartitionDefinition> &value) {
    grt::ValueRef ovalue(_partitionDefinitions);

    _partitionDefinitions = value;
    owned_member_changed("partitionDefinitions", ovalue, value);
  }
public:

  /**
   * Getter for attribute partitionExpression
   *
   * a generic expression or a column list
   * \par In Python:
   *    value = obj.partitionExpression
   */
  grt::StringRef partitionExpression() const { return _partitionExpression; }

  /**
   * Setter for attribute partitionExpression
   * 
   * a generic expression or a column list
   * \par In Python:
   *   obj.partitionExpression = value
   */
  virtual void partitionExpression(const grt::StringRef &value) {
    grt::ValueRef ovalue(_partitionExpression);
    _partitionExpression = value;
    member_changed("partitionExpression", ovalue, value);
  }

  /**
   * Getter for attribute partitionKeyAlgorithm
   *
   * algorithm used for KEY partition type, can be 1 or 2
   * \par In Python:
   *    value = obj.partitionKeyAlgorithm
   */
  grt::IntegerRef partitionKeyAlgorithm() const { return _partitionKeyAlgorithm; }

  /**
   * Setter for attribute partitionKeyAlgorithm
   * 
   * algorithm used for KEY partition type, can be 1 or 2
   * \par In Python:
   *   obj.partitionKeyAlgorithm = value
   */
  virtual void partitionKeyAlgorithm(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_partitionKeyAlgorithm);
    _partitionKeyAlgorithm = value;
    member_changed("partitionKeyAlgorithm", ovalue, value);
  }

  /**
   * Getter for attribute partitionType
   *
   * 
   * \par In Python:
   *    value = obj.partitionType
   */
  grt::StringRef partitionType() const { return _partitionType; }

  /**
   * Setter for attribute partitionType
   * 
   * 
   * \par In Python:
   *   obj.partitionType = value
   */
  virtual void partitionType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_partitionType);
    _partitionType = value;
    member_changed("partitionType", ovalue, value);
  }

  /**
   * Getter for attribute password
   *
   * 
   * \par In Python:
   *    value = obj.password
   */
  grt::StringRef password() const { return _password; }

  /**
   * Setter for attribute password
   * 
   * 
   * \par In Python:
   *   obj.password = value
   */
  virtual void password(const grt::StringRef &value) {
    grt::ValueRef ovalue(_password);
    _password = value;
    member_changed("password", ovalue, value);
  }

  /**
   * Getter for attribute primaryKey
   *
   * 
   * \par In Python:
   *    value = obj.primaryKey
   */
  db_mysql_IndexRef primaryKey() const { return db_mysql_IndexRef::cast_from(_primaryKey); }

  /**
   * Setter for attribute primaryKey
   * 
   * 
   * \par In Python:
   *   obj.primaryKey = value
   */
  virtual void primaryKey(const db_mysql_IndexRef &value) { super::primaryKey(value); }

  /**
   * Getter for attribute raidChunkSize
   *
   * 
   * \par In Python:
   *    value = obj.raidChunkSize
   */
  grt::StringRef raidChunkSize() const { return _raidChunkSize; }

  /**
   * Setter for attribute raidChunkSize
   * 
   * 
   * \par In Python:
   *   obj.raidChunkSize = value
   */
  virtual void raidChunkSize(const grt::StringRef &value) {
    grt::ValueRef ovalue(_raidChunkSize);
    _raidChunkSize = value;
    member_changed("raidChunkSize", ovalue, value);
  }

  /**
   * Getter for attribute raidChunks
   *
   * 
   * \par In Python:
   *    value = obj.raidChunks
   */
  grt::StringRef raidChunks() const { return _raidChunks; }

  /**
   * Setter for attribute raidChunks
   * 
   * 
   * \par In Python:
   *   obj.raidChunks = value
   */
  virtual void raidChunks(const grt::StringRef &value) {
    grt::ValueRef ovalue(_raidChunks);
    _raidChunks = value;
    member_changed("raidChunks", ovalue, value);
  }

  /**
   * Getter for attribute raidType
   *
   * 
   * \par In Python:
   *    value = obj.raidType
   */
  grt::StringRef raidType() const { return _raidType; }

  /**
   * Setter for attribute raidType
   * 
   * 
   * \par In Python:
   *   obj.raidType = value
   */
  virtual void raidType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_raidType);
    _raidType = value;
    member_changed("raidType", ovalue, value);
  }

  /**
   * Getter for attribute rowFormat
   *
   * 
   * \par In Python:
   *    value = obj.rowFormat
   */
  grt::StringRef rowFormat() const { return _rowFormat; }

  /**
   * Setter for attribute rowFormat
   * 
   * 
   * \par In Python:
   *   obj.rowFormat = value
   */
  virtual void rowFormat(const grt::StringRef &value) {
    grt::ValueRef ovalue(_rowFormat);
    _rowFormat = value;
    member_changed("rowFormat", ovalue, value);
  }

  /**
   * Getter for attribute statsAutoRecalc
   *
   * DEFAULT, 0 or 1
   * \par In Python:
   *    value = obj.statsAutoRecalc
   */
  grt::StringRef statsAutoRecalc() const { return _statsAutoRecalc; }

  /**
   * Setter for attribute statsAutoRecalc
   * 
   * DEFAULT, 0 or 1
   * \par In Python:
   *   obj.statsAutoRecalc = value
   */
  virtual void statsAutoRecalc(const grt::StringRef &value) {
    grt::ValueRef ovalue(_statsAutoRecalc);
    _statsAutoRecalc = value;
    member_changed("statsAutoRecalc", ovalue, value);
  }

  /**
   * Getter for attribute statsPersistent
   *
   * DEFAULT, 0 or 1
   * \par In Python:
   *    value = obj.statsPersistent
   */
  grt::StringRef statsPersistent() const { return _statsPersistent; }

  /**
   * Setter for attribute statsPersistent
   * 
   * DEFAULT, 0 or 1
   * \par In Python:
   *   obj.statsPersistent = value
   */
  virtual void statsPersistent(const grt::StringRef &value) {
    grt::ValueRef ovalue(_statsPersistent);
    _statsPersistent = value;
    member_changed("statsPersistent", ovalue, value);
  }

  /**
   * Getter for attribute statsSamplePages
   *
   * 
   * \par In Python:
   *    value = obj.statsSamplePages
   */
  grt::IntegerRef statsSamplePages() const { return _statsSamplePages; }

  /**
   * Setter for attribute statsSamplePages
   * 
   * 
   * \par In Python:
   *   obj.statsSamplePages = value
   */
  virtual void statsSamplePages(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_statsSamplePages);
    _statsSamplePages = value;
    member_changed("statsSamplePages", ovalue, value);
  }

  /**
   * Getter for attribute subpartitionCount
   *
   * 
   * \par In Python:
   *    value = obj.subpartitionCount
   */
  grt::IntegerRef subpartitionCount() const { return _subpartitionCount; }

  /**
   * Setter for attribute subpartitionCount
   * 
   * 
   * \par In Python:
   *   obj.subpartitionCount = value
   */
  virtual void subpartitionCount(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_subpartitionCount);
    _subpartitionCount = value;
    member_changed("subpartitionCount", ovalue, value);
  }

  /**
   * Getter for attribute subpartitionExpression
   *
   * 
   * \par In Python:
   *    value = obj.subpartitionExpression
   */
  grt::StringRef subpartitionExpression() const { return _subpartitionExpression; }

  /**
   * Setter for attribute subpartitionExpression
   * 
   * 
   * \par In Python:
   *   obj.subpartitionExpression = value
   */
  virtual void subpartitionExpression(const grt::StringRef &value) {
    grt::ValueRef ovalue(_subpartitionExpression);
    _subpartitionExpression = value;
    member_changed("subpartitionExpression", ovalue, value);
  }

  /**
   * Getter for attribute subpartitionKeyAlgorithm
   *
   * algorithm used for KEY partition type, can be 1 or 2
   * \par In Python:
   *    value = obj.subpartitionKeyAlgorithm
   */
  grt::IntegerRef subpartitionKeyAlgorithm() const { return _subpartitionKeyAlgorithm; }

  /**
   * Setter for attribute subpartitionKeyAlgorithm
   * 
   * algorithm used for KEY partition type, can be 1 or 2
   * \par In Python:
   *   obj.subpartitionKeyAlgorithm = value
   */
  virtual void subpartitionKeyAlgorithm(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_subpartitionKeyAlgorithm);
    _subpartitionKeyAlgorithm = value;
    member_changed("subpartitionKeyAlgorithm", ovalue, value);
  }

  /**
   * Getter for attribute subpartitionType
   *
   * 
   * \par In Python:
   *    value = obj.subpartitionType
   */
  grt::StringRef subpartitionType() const { return _subpartitionType; }

  /**
   * Setter for attribute subpartitionType
   * 
   * 
   * \par In Python:
   *   obj.subpartitionType = value
   */
  virtual void subpartitionType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_subpartitionType);
    _subpartitionType = value;
    member_changed("subpartitionType", ovalue, value);
  }

  /**
   * Getter for attribute tableDataDir
   *
   * 
   * \par In Python:
   *    value = obj.tableDataDir
   */
  grt::StringRef tableDataDir() const { return _tableDataDir; }

  /**
   * Setter for attribute tableDataDir
   * 
   * 
   * \par In Python:
   *   obj.tableDataDir = value
   */
  virtual void tableDataDir(const grt::StringRef &value) {
    grt::ValueRef ovalue(_tableDataDir);
    _tableDataDir = value;
    member_changed("tableDataDir", ovalue, value);
  }

  /**
   * Getter for attribute tableEngine
   *
   * 
   * \par In Python:
   *    value = obj.tableEngine
   */
  grt::StringRef tableEngine() const { return _tableEngine; }

  /**
   * Setter for attribute tableEngine
   * 
   * 
   * \par In Python:
   *   obj.tableEngine = value
   */
  virtual void tableEngine(const grt::StringRef &value) {
    grt::ValueRef ovalue(_tableEngine);
    _tableEngine = value;
    member_changed("tableEngine", ovalue, value);
  }

  /**
   * Getter for attribute tableIndexDir
   *
   * 
   * \par In Python:
   *    value = obj.tableIndexDir
   */
  grt::StringRef tableIndexDir() const { return _tableIndexDir; }

  /**
   * Setter for attribute tableIndexDir
   * 
   * 
   * \par In Python:
   *   obj.tableIndexDir = value
   */
  virtual void tableIndexDir(const grt::StringRef &value) {
    grt::ValueRef ovalue(_tableIndexDir);
    _tableIndexDir = value;
    member_changed("tableIndexDir", ovalue, value);
  }

  /**
   * Getter for attribute tableSpace
   *
   * 
   * \par In Python:
   *    value = obj.tableSpace
   */
  grt::StringRef tableSpace() const { return _tableSpace; }

  /**
   * Setter for attribute tableSpace
   * 
   * 
   * \par In Python:
   *   obj.tableSpace = value
   */
  virtual void tableSpace(const grt::StringRef &value) {
    grt::ValueRef ovalue(_tableSpace);
    _tableSpace = value;
    member_changed("tableSpace", ovalue, value);
  }

  // triggers is owned by db_mysql_Table
  /**
   * Getter for attribute triggers (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.triggers
   */
  grt::ListRef<db_mysql_Trigger> triggers() const { return grt::ListRef<db_mysql_Trigger>::cast_from(_triggers); }


private: // The next attribute is read-only.
public:

protected:

  grt::StringRef _avgRowLength;
  grt::IntegerRef _checksum;
  db_ServerLinkRef _connection;// owned
  grt::StringRef _connectionString;
  grt::StringRef _defaultCharacterSetName;
  grt::StringRef _defaultCollationName;
  grt::IntegerRef _delayKeyWrite;
  grt::StringRef _keyBlockSize;
  grt::StringRef _maxRows;
  grt::StringRef _mergeInsert;
  grt::StringRef _mergeUnion;
  grt::StringRef _minRows;
  grt::StringRef _nextAutoInc;
  grt::StringRef _packKeys;
  grt::IntegerRef _partitionCount;
  grt::ListRef<db_mysql_PartitionDefinition> _partitionDefinitions;// owned
  grt::StringRef _partitionExpression;
  grt::IntegerRef _partitionKeyAlgorithm;
  grt::StringRef _partitionType;
  grt::StringRef _password;
  grt::StringRef _raidChunkSize;
  grt::StringRef _raidChunks;
  grt::StringRef _raidType;
  grt::StringRef _rowFormat;
  grt::StringRef _statsAutoRecalc;
  grt::StringRef _statsPersistent;
  grt::IntegerRef _statsSamplePages;
  grt::IntegerRef _subpartitionCount;
  grt::StringRef _subpartitionExpression;
  grt::IntegerRef _subpartitionKeyAlgorithm;
  grt::StringRef _subpartitionType;
  grt::StringRef _tableDataDir;
  grt::StringRef _tableEngine;
  grt::StringRef _tableIndexDir;
  grt::StringRef _tableSpace;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Table());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Table::create);
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::avgRowLength;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::avgRowLength;
      meta->bind_member("avgRowLength", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::IntegerRef &) = &db_mysql_Table::checksum;
      grt::IntegerRef (db_mysql_Table::*getter)() const = &db_mysql_Table::checksum;
      meta->bind_member("checksum", new grt::MetaClass::Property<db_mysql_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::ListRef<db_mysql_Column> &) = 0;
      grt::ListRef<db_mysql_Column> (db_mysql_Table::*getter)() const = 0;
      meta->bind_member("columns", new grt::MetaClass::Property<db_mysql_Table,grt::ListRef<db_mysql_Column>>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const db_ServerLinkRef &) = &db_mysql_Table::connection;
      db_ServerLinkRef (db_mysql_Table::*getter)() const = &db_mysql_Table::connection;
      meta->bind_member("connection", new grt::MetaClass::Property<db_mysql_Table,db_ServerLinkRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::connectionString;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::connectionString;
      meta->bind_member("connectionString", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::defaultCharacterSetName;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::defaultCharacterSetName;
      meta->bind_member("defaultCharacterSetName", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::defaultCollationName;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::defaultCollationName;
      meta->bind_member("defaultCollationName", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::IntegerRef &) = &db_mysql_Table::delayKeyWrite;
      grt::IntegerRef (db_mysql_Table::*getter)() const = &db_mysql_Table::delayKeyWrite;
      meta->bind_member("delayKeyWrite", new grt::MetaClass::Property<db_mysql_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::ListRef<db_mysql_ForeignKey> &) = 0;
      grt::ListRef<db_mysql_ForeignKey> (db_mysql_Table::*getter)() const = 0;
      meta->bind_member("foreignKeys", new grt::MetaClass::Property<db_mysql_Table,grt::ListRef<db_mysql_ForeignKey>>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::ListRef<db_mysql_Index> &) = 0;
      grt::ListRef<db_mysql_Index> (db_mysql_Table::*getter)() const = 0;
      meta->bind_member("indices", new grt::MetaClass::Property<db_mysql_Table,grt::ListRef<db_mysql_Index>>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::keyBlockSize;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::keyBlockSize;
      meta->bind_member("keyBlockSize", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::maxRows;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::maxRows;
      meta->bind_member("maxRows", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::mergeInsert;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::mergeInsert;
      meta->bind_member("mergeInsert", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::mergeUnion;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::mergeUnion;
      meta->bind_member("mergeUnion", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::minRows;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::minRows;
      meta->bind_member("minRows", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::nextAutoInc;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::nextAutoInc;
      meta->bind_member("nextAutoInc", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::packKeys;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::packKeys;
      meta->bind_member("packKeys", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::IntegerRef &) = &db_mysql_Table::partitionCount;
      grt::IntegerRef (db_mysql_Table::*getter)() const = &db_mysql_Table::partitionCount;
      meta->bind_member("partitionCount", new grt::MetaClass::Property<db_mysql_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::ListRef<db_mysql_PartitionDefinition> &) = &db_mysql_Table::partitionDefinitions;
      grt::ListRef<db_mysql_PartitionDefinition> (db_mysql_Table::*getter)() const = &db_mysql_Table::partitionDefinitions;
      meta->bind_member("partitionDefinitions", new grt::MetaClass::Property<db_mysql_Table,grt::ListRef<db_mysql_PartitionDefinition>>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::partitionExpression;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::partitionExpression;
      meta->bind_member("partitionExpression", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::IntegerRef &) = &db_mysql_Table::partitionKeyAlgorithm;
      grt::IntegerRef (db_mysql_Table::*getter)() const = &db_mysql_Table::partitionKeyAlgorithm;
      meta->bind_member("partitionKeyAlgorithm", new grt::MetaClass::Property<db_mysql_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::partitionType;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::partitionType;
      meta->bind_member("partitionType", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::password;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::password;
      meta->bind_member("password", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const db_mysql_IndexRef &) = 0;
      db_mysql_IndexRef (db_mysql_Table::*getter)() const = 0;
      meta->bind_member("primaryKey", new grt::MetaClass::Property<db_mysql_Table,db_mysql_IndexRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::raidChunkSize;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::raidChunkSize;
      meta->bind_member("raidChunkSize", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::raidChunks;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::raidChunks;
      meta->bind_member("raidChunks", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::raidType;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::raidType;
      meta->bind_member("raidType", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::rowFormat;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::rowFormat;
      meta->bind_member("rowFormat", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::statsAutoRecalc;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::statsAutoRecalc;
      meta->bind_member("statsAutoRecalc", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::statsPersistent;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::statsPersistent;
      meta->bind_member("statsPersistent", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::IntegerRef &) = &db_mysql_Table::statsSamplePages;
      grt::IntegerRef (db_mysql_Table::*getter)() const = &db_mysql_Table::statsSamplePages;
      meta->bind_member("statsSamplePages", new grt::MetaClass::Property<db_mysql_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::IntegerRef &) = &db_mysql_Table::subpartitionCount;
      grt::IntegerRef (db_mysql_Table::*getter)() const = &db_mysql_Table::subpartitionCount;
      meta->bind_member("subpartitionCount", new grt::MetaClass::Property<db_mysql_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::subpartitionExpression;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::subpartitionExpression;
      meta->bind_member("subpartitionExpression", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::IntegerRef &) = &db_mysql_Table::subpartitionKeyAlgorithm;
      grt::IntegerRef (db_mysql_Table::*getter)() const = &db_mysql_Table::subpartitionKeyAlgorithm;
      meta->bind_member("subpartitionKeyAlgorithm", new grt::MetaClass::Property<db_mysql_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::subpartitionType;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::subpartitionType;
      meta->bind_member("subpartitionType", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::tableDataDir;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::tableDataDir;
      meta->bind_member("tableDataDir", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::tableEngine;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::tableEngine;
      meta->bind_member("tableEngine", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::tableIndexDir;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::tableIndexDir;
      meta->bind_member("tableIndexDir", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::StringRef &) = &db_mysql_Table::tableSpace;
      grt::StringRef (db_mysql_Table::*getter)() const = &db_mysql_Table::tableSpace;
      meta->bind_member("tableSpace", new grt::MetaClass::Property<db_mysql_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Table::*setter)(const grt::ListRef<db_mysql_Trigger> &) = 0;
      grt::ListRef<db_mysql_Trigger> (db_mysql_Table::*getter)() const = 0;
      meta->bind_member("triggers", new grt::MetaClass::Property<db_mysql_Table,grt::ListRef<db_mysql_Trigger>>(getter, setter));
    }
  }
};

class  db_mysql_PartitionDefinition : public GrtObject {
  typedef GrtObject super;

public:
  db_mysql_PartitionDefinition(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _comment(""),
      _dataDirectory(""),
      _engine(""),
      _indexDirectory(""),
      _maxRows(""),
      _minRows(""),
      _nodeGroupId(0),
      _subpartitionDefinitions(this, false),
      _tableSpace(""),
      _value("") {
  }

  static std::string static_class_name() {
    return "db.mysql.PartitionDefinition";
  }

  /**
   * Getter for attribute comment
   *
   * 
   * \par In Python:
   *    value = obj.comment
   */
  grt::StringRef comment() const { return _comment; }

  /**
   * Setter for attribute comment
   * 
   * 
   * \par In Python:
   *   obj.comment = value
   */
  virtual void comment(const grt::StringRef &value) {
    grt::ValueRef ovalue(_comment);
    _comment = value;
    member_changed("comment", ovalue, value);
  }

  /**
   * Getter for attribute dataDirectory
   *
   * 
   * \par In Python:
   *    value = obj.dataDirectory
   */
  grt::StringRef dataDirectory() const { return _dataDirectory; }

  /**
   * Setter for attribute dataDirectory
   * 
   * 
   * \par In Python:
   *   obj.dataDirectory = value
   */
  virtual void dataDirectory(const grt::StringRef &value) {
    grt::ValueRef ovalue(_dataDirectory);
    _dataDirectory = value;
    member_changed("dataDirectory", ovalue, value);
  }

  /**
   * Getter for attribute engine
   *
   * 
   * \par In Python:
   *    value = obj.engine
   */
  grt::StringRef engine() const { return _engine; }

  /**
   * Setter for attribute engine
   * 
   * 
   * \par In Python:
   *   obj.engine = value
   */
  virtual void engine(const grt::StringRef &value) {
    grt::ValueRef ovalue(_engine);
    _engine = value;
    member_changed("engine", ovalue, value);
  }

  /**
   * Getter for attribute indexDirectory
   *
   * 
   * \par In Python:
   *    value = obj.indexDirectory
   */
  grt::StringRef indexDirectory() const { return _indexDirectory; }

  /**
   * Setter for attribute indexDirectory
   * 
   * 
   * \par In Python:
   *   obj.indexDirectory = value
   */
  virtual void indexDirectory(const grt::StringRef &value) {
    grt::ValueRef ovalue(_indexDirectory);
    _indexDirectory = value;
    member_changed("indexDirectory", ovalue, value);
  }

  /**
   * Getter for attribute maxRows
   *
   * 
   * \par In Python:
   *    value = obj.maxRows
   */
  grt::StringRef maxRows() const { return _maxRows; }

  /**
   * Setter for attribute maxRows
   * 
   * 
   * \par In Python:
   *   obj.maxRows = value
   */
  virtual void maxRows(const grt::StringRef &value) {
    grt::ValueRef ovalue(_maxRows);
    _maxRows = value;
    member_changed("maxRows", ovalue, value);
  }

  /**
   * Getter for attribute minRows
   *
   * 
   * \par In Python:
   *    value = obj.minRows
   */
  grt::StringRef minRows() const { return _minRows; }

  /**
   * Setter for attribute minRows
   * 
   * 
   * \par In Python:
   *   obj.minRows = value
   */
  virtual void minRows(const grt::StringRef &value) {
    grt::ValueRef ovalue(_minRows);
    _minRows = value;
    member_changed("minRows", ovalue, value);
  }

  /**
   * Getter for attribute nodeGroupId
   *
   * 
   * \par In Python:
   *    value = obj.nodeGroupId
   */
  grt::IntegerRef nodeGroupId() const { return _nodeGroupId; }

  /**
   * Setter for attribute nodeGroupId
   * 
   * 
   * \par In Python:
   *   obj.nodeGroupId = value
   */
  virtual void nodeGroupId(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_nodeGroupId);
    _nodeGroupId = value;
    member_changed("nodeGroupId", ovalue, value);
  }

  // subpartitionDefinitions is owned by db_mysql_PartitionDefinition
  /**
   * Getter for attribute subpartitionDefinitions (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.subpartitionDefinitions
   */
  grt::ListRef<db_mysql_PartitionDefinition> subpartitionDefinitions() const { return _subpartitionDefinitions; }


private: // The next attribute is read-only.
  virtual void subpartitionDefinitions(const grt::ListRef<db_mysql_PartitionDefinition> &value) {
    grt::ValueRef ovalue(_subpartitionDefinitions);

    _subpartitionDefinitions = value;
    owned_member_changed("subpartitionDefinitions", ovalue, value);
  }
public:

  /**
   * Getter for attribute tableSpace
   *
   * 
   * \par In Python:
   *    value = obj.tableSpace
   */
  grt::StringRef tableSpace() const { return _tableSpace; }

  /**
   * Setter for attribute tableSpace
   * 
   * 
   * \par In Python:
   *   obj.tableSpace = value
   */
  virtual void tableSpace(const grt::StringRef &value) {
    grt::ValueRef ovalue(_tableSpace);
    _tableSpace = value;
    member_changed("tableSpace", ovalue, value);
  }

  /**
   * Getter for attribute value
   *
   * 
   * \par In Python:
   *    value = obj.value
   */
  grt::StringRef value() const { return _value; }

  /**
   * Setter for attribute value
   * 
   * 
   * \par In Python:
   *   obj.value = value
   */
  virtual void value(const grt::StringRef &value) {
    grt::ValueRef ovalue(_value);
    _value = value;
    member_changed("value", ovalue, value);
  }

protected:

  grt::StringRef _comment;
  grt::StringRef _dataDirectory;
  grt::StringRef _engine;
  grt::StringRef _indexDirectory;
  grt::StringRef _maxRows;
  grt::StringRef _minRows;
  grt::IntegerRef _nodeGroupId;
  grt::ListRef<db_mysql_PartitionDefinition> _subpartitionDefinitions;// owned
  grt::StringRef _tableSpace;
  grt::StringRef _value;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_PartitionDefinition());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_PartitionDefinition::create);
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::comment;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::comment;
      meta->bind_member("comment", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::dataDirectory;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::dataDirectory;
      meta->bind_member("dataDirectory", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::engine;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::engine;
      meta->bind_member("engine", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::indexDirectory;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::indexDirectory;
      meta->bind_member("indexDirectory", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::maxRows;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::maxRows;
      meta->bind_member("maxRows", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::minRows;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::minRows;
      meta->bind_member("minRows", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::IntegerRef &) = &db_mysql_PartitionDefinition::nodeGroupId;
      grt::IntegerRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::nodeGroupId;
      meta->bind_member("nodeGroupId", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::ListRef<db_mysql_PartitionDefinition> &) = &db_mysql_PartitionDefinition::subpartitionDefinitions;
      grt::ListRef<db_mysql_PartitionDefinition> (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::subpartitionDefinitions;
      meta->bind_member("subpartitionDefinitions", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::ListRef<db_mysql_PartitionDefinition>>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::tableSpace;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::tableSpace;
      meta->bind_member("tableSpace", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_PartitionDefinition::*setter)(const grt::StringRef &) = &db_mysql_PartitionDefinition::value;
      grt::StringRef (db_mysql_PartitionDefinition::*getter)() const = &db_mysql_PartitionDefinition::value;
      meta->bind_member("value", new grt::MetaClass::Property<db_mysql_PartitionDefinition,grt::StringRef>(getter, setter));
    }
  }
};

class  db_mysql_ServerLink : public db_ServerLink {
  typedef db_ServerLink super;

public:
  db_mysql_ServerLink(grt::MetaClass *meta = nullptr)
    : db_ServerLink(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.mysql.ServerLink";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_ServerLink());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_ServerLink::create);
  }
};

class  db_mysql_Tablespace : public db_Tablespace {
  typedef db_Tablespace super;

public:
  db_mysql_Tablespace(grt::MetaClass *meta = nullptr)
    : db_Tablespace(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _nodeGroupId(0) {
  }

  static std::string static_class_name() {
    return "db.mysql.Tablespace";
  }

  /**
   * Getter for attribute engine
   *
   * NDB and InnoDB are supported
   * \par In Python:
   *    value = obj.engine
   */

  /**
   * Setter for attribute engine
   * 
   * NDB and InnoDB are supported
   * \par In Python:
   *   obj.engine = value
   */

  /**
   * Getter for attribute nodeGroupId
   *
   * the same id as used for a logfile group
   * \par In Python:
   *    value = obj.nodeGroupId
   */
  grt::IntegerRef nodeGroupId() const { return _nodeGroupId; }

  /**
   * Setter for attribute nodeGroupId
   * 
   * the same id as used for a logfile group
   * \par In Python:
   *   obj.nodeGroupId = value
   */
  virtual void nodeGroupId(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_nodeGroupId);
    _nodeGroupId = value;
    member_changed("nodeGroupId", ovalue, value);
  }

  /**
   * Getter for attribute wait
   *
   * no documentation yet
   * \par In Python:
   *    value = obj.wait
   */

  /**
   * Setter for attribute wait
   * 
   * no documentation yet
   * \par In Python:
   *   obj.wait = value
   */

protected:

  grt::IntegerRef _nodeGroupId;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Tablespace());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Tablespace::create);
    {
      void (db_mysql_Tablespace::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_mysql_Tablespace::*getter)() const = 0;
      meta->bind_member("engine", new grt::MetaClass::Property<db_mysql_Tablespace,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_Tablespace::*setter)(const grt::IntegerRef &) = &db_mysql_Tablespace::nodeGroupId;
      grt::IntegerRef (db_mysql_Tablespace::*getter)() const = &db_mysql_Tablespace::nodeGroupId;
      meta->bind_member("nodeGroupId", new grt::MetaClass::Property<db_mysql_Tablespace,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_Tablespace::*setter)(const grt::IntegerRef &) = 0;
      grt::IntegerRef (db_mysql_Tablespace::*getter)() const = 0;
      meta->bind_member("wait", new grt::MetaClass::Property<db_mysql_Tablespace,grt::IntegerRef>(getter, setter));
    }
  }
};

class  db_mysql_LogFileGroup : public db_LogFileGroup {
  typedef db_LogFileGroup super;

public:
  db_mysql_LogFileGroup(grt::MetaClass *meta = nullptr)
    : db_LogFileGroup(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _nodeGroupId(0) {
  }

  static std::string static_class_name() {
    return "db.mysql.LogFileGroup";
  }

  /**
   * Getter for attribute engine
   *
   * usually only NDB makes sense
   * \par In Python:
   *    value = obj.engine
   */

  /**
   * Setter for attribute engine
   * 
   * usually only NDB makes sense
   * \par In Python:
   *   obj.engine = value
   */

  /**
   * Getter for attribute nodeGroupId
   *
   * a unique id for the group, used in a tablespace
   * \par In Python:
   *    value = obj.nodeGroupId
   */
  grt::IntegerRef nodeGroupId() const { return _nodeGroupId; }

  /**
   * Setter for attribute nodeGroupId
   * 
   * a unique id for the group, used in a tablespace
   * \par In Python:
   *   obj.nodeGroupId = value
   */
  virtual void nodeGroupId(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_nodeGroupId);
    _nodeGroupId = value;
    member_changed("nodeGroupId", ovalue, value);
  }

  /**
   * Getter for attribute wait
   *
   * no documentation yet
   * \par In Python:
   *    value = obj.wait
   */

  /**
   * Setter for attribute wait
   * 
   * no documentation yet
   * \par In Python:
   *   obj.wait = value
   */

protected:

  grt::IntegerRef _nodeGroupId;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_LogFileGroup());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_LogFileGroup::create);
    {
      void (db_mysql_LogFileGroup::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_mysql_LogFileGroup::*getter)() const = 0;
      meta->bind_member("engine", new grt::MetaClass::Property<db_mysql_LogFileGroup,grt::StringRef>(getter, setter));
    }
    {
      void (db_mysql_LogFileGroup::*setter)(const grt::IntegerRef &) = &db_mysql_LogFileGroup::nodeGroupId;
      grt::IntegerRef (db_mysql_LogFileGroup::*getter)() const = &db_mysql_LogFileGroup::nodeGroupId;
      meta->bind_member("nodeGroupId", new grt::MetaClass::Property<db_mysql_LogFileGroup,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mysql_LogFileGroup::*setter)(const grt::IntegerRef &) = 0;
      grt::IntegerRef (db_mysql_LogFileGroup::*getter)() const = 0;
      meta->bind_member("wait", new grt::MetaClass::Property<db_mysql_LogFileGroup,grt::IntegerRef>(getter, setter));
    }
  }
};

class  db_mysql_Schema : public db_Schema {
  typedef db_Schema super;

public:
  db_mysql_Schema(grt::MetaClass *meta = nullptr)
    : db_Schema(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
    _routineGroups.content().__retype(grt::ObjectType, "db.mysql.RoutineGroup");
    _routines.content().__retype(grt::ObjectType, "db.mysql.Routine");
    _sequences.content().__retype(grt::ObjectType, "db.mysql.Sequence");
    _structuredTypes.content().__retype(grt::ObjectType, "db.mysql.StructuredDatatype");
    _synonyms.content().__retype(grt::ObjectType, "db.mysql.Synonym");
    _tables.content().__retype(grt::ObjectType, "db.mysql.Table");
    _views.content().__retype(grt::ObjectType, "db.mysql.View");
  }

  static std::string static_class_name() {
    return "db.mysql.Schema";
  }

  // routineGroups is owned by db_mysql_Schema
  /**
   * Getter for attribute routineGroups (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.routineGroups
   */
  grt::ListRef<db_mysql_RoutineGroup> routineGroups() const { return grt::ListRef<db_mysql_RoutineGroup>::cast_from(_routineGroups); }


private: // The next attribute is read-only.
public:

  // routines is owned by db_mysql_Schema
  /**
   * Getter for attribute routines (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.routines
   */
  grt::ListRef<db_mysql_Routine> routines() const { return grt::ListRef<db_mysql_Routine>::cast_from(_routines); }


private: // The next attribute is read-only.
public:

  // sequences is owned by db_mysql_Schema
  /**
   * Getter for attribute sequences (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.sequences
   */
  grt::ListRef<db_mysql_Sequence> sequences() const { return grt::ListRef<db_mysql_Sequence>::cast_from(_sequences); }


private: // The next attribute is read-only.
public:

  // structuredTypes is owned by db_mysql_Schema
  /**
   * Getter for attribute structuredTypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.structuredTypes
   */
  grt::ListRef<db_mysql_StructuredDatatype> structuredTypes() const { return grt::ListRef<db_mysql_StructuredDatatype>::cast_from(_structuredTypes); }


private: // The next attribute is read-only.
public:

  // synonyms is owned by db_mysql_Schema
  /**
   * Getter for attribute synonyms (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.synonyms
   */
  grt::ListRef<db_mysql_Synonym> synonyms() const { return grt::ListRef<db_mysql_Synonym>::cast_from(_synonyms); }


private: // The next attribute is read-only.
public:

  // tables is owned by db_mysql_Schema
  /**
   * Getter for attribute tables (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.tables
   */
  grt::ListRef<db_mysql_Table> tables() const { return grt::ListRef<db_mysql_Table>::cast_from(_tables); }


private: // The next attribute is read-only.
public:

  // views is owned by db_mysql_Schema
  /**
   * Getter for attribute views (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.views
   */
  grt::ListRef<db_mysql_View> views() const { return grt::ListRef<db_mysql_View>::cast_from(_views); }


private: // The next attribute is read-only.
public:

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Schema());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Schema::create);
    {
      void (db_mysql_Schema::*setter)(const grt::ListRef<db_mysql_RoutineGroup> &) = 0;
      grt::ListRef<db_mysql_RoutineGroup> (db_mysql_Schema::*getter)() const = 0;
      meta->bind_member("routineGroups", new grt::MetaClass::Property<db_mysql_Schema,grt::ListRef<db_mysql_RoutineGroup>>(getter, setter));
    }
    {
      void (db_mysql_Schema::*setter)(const grt::ListRef<db_mysql_Routine> &) = 0;
      grt::ListRef<db_mysql_Routine> (db_mysql_Schema::*getter)() const = 0;
      meta->bind_member("routines", new grt::MetaClass::Property<db_mysql_Schema,grt::ListRef<db_mysql_Routine>>(getter, setter));
    }
    {
      void (db_mysql_Schema::*setter)(const grt::ListRef<db_mysql_Sequence> &) = 0;
      grt::ListRef<db_mysql_Sequence> (db_mysql_Schema::*getter)() const = 0;
      meta->bind_member("sequences", new grt::MetaClass::Property<db_mysql_Schema,grt::ListRef<db_mysql_Sequence>>(getter, setter));
    }
    {
      void (db_mysql_Schema::*setter)(const grt::ListRef<db_mysql_StructuredDatatype> &) = 0;
      grt::ListRef<db_mysql_StructuredDatatype> (db_mysql_Schema::*getter)() const = 0;
      meta->bind_member("structuredTypes", new grt::MetaClass::Property<db_mysql_Schema,grt::ListRef<db_mysql_StructuredDatatype>>(getter, setter));
    }
    {
      void (db_mysql_Schema::*setter)(const grt::ListRef<db_mysql_Synonym> &) = 0;
      grt::ListRef<db_mysql_Synonym> (db_mysql_Schema::*getter)() const = 0;
      meta->bind_member("synonyms", new grt::MetaClass::Property<db_mysql_Schema,grt::ListRef<db_mysql_Synonym>>(getter, setter));
    }
    {
      void (db_mysql_Schema::*setter)(const grt::ListRef<db_mysql_Table> &) = 0;
      grt::ListRef<db_mysql_Table> (db_mysql_Schema::*getter)() const = 0;
      meta->bind_member("tables", new grt::MetaClass::Property<db_mysql_Schema,grt::ListRef<db_mysql_Table>>(getter, setter));
    }
    {
      void (db_mysql_Schema::*setter)(const grt::ListRef<db_mysql_View> &) = 0;
      grt::ListRef<db_mysql_View> (db_mysql_Schema::*getter)() const = 0;
      meta->bind_member("views", new grt::MetaClass::Property<db_mysql_Schema,grt::ListRef<db_mysql_View>>(getter, setter));
    }
  }
};

class  db_mysql_Catalog : public db_Catalog {
  typedef db_Catalog super;

public:
  db_mysql_Catalog(grt::MetaClass *meta = nullptr)
    : db_Catalog(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
    _logFileGroups.content().__retype(grt::ObjectType, "db.mysql.LogFileGroup");
    _schemata.content().__retype(grt::ObjectType, "db.mysql.Schema");
    _serverLinks.content().__retype(grt::ObjectType, "db.mysql.ServerLink");
    _tablespaces.content().__retype(grt::ObjectType, "db.mysql.Tablespace");
  }

  static std::string static_class_name() {
    return "db.mysql.Catalog";
  }

  // logFileGroups is owned by db_mysql_Catalog
  /**
   * Getter for attribute logFileGroups (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.logFileGroups
   */
  grt::ListRef<db_mysql_LogFileGroup> logFileGroups() const { return grt::ListRef<db_mysql_LogFileGroup>::cast_from(_logFileGroups); }


private: // The next attribute is read-only.
public:

  // schemata is owned by db_mysql_Catalog
  /**
   * Getter for attribute schemata (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.schemata
   */
  grt::ListRef<db_mysql_Schema> schemata() const { return grt::ListRef<db_mysql_Schema>::cast_from(_schemata); }


private: // The next attribute is read-only.
public:

  // serverLinks is owned by db_mysql_Catalog
  /**
   * Getter for attribute serverLinks (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.serverLinks
   */
  grt::ListRef<db_mysql_ServerLink> serverLinks() const { return grt::ListRef<db_mysql_ServerLink>::cast_from(_serverLinks); }


private: // The next attribute is read-only.
public:

  // tablespaces is owned by db_mysql_Catalog
  /**
   * Getter for attribute tablespaces (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.tablespaces
   */
  grt::ListRef<db_mysql_Tablespace> tablespaces() const { return grt::ListRef<db_mysql_Tablespace>::cast_from(_tablespaces); }


private: // The next attribute is read-only.
public:

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mysql_Catalog());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mysql_Catalog::create);
    {
      void (db_mysql_Catalog::*setter)(const grt::ListRef<db_mysql_LogFileGroup> &) = 0;
      grt::ListRef<db_mysql_LogFileGroup> (db_mysql_Catalog::*getter)() const = 0;
      meta->bind_member("logFileGroups", new grt::MetaClass::Property<db_mysql_Catalog,grt::ListRef<db_mysql_LogFileGroup>>(getter, setter));
    }
    {
      void (db_mysql_Catalog::*setter)(const grt::ListRef<db_mysql_Schema> &) = 0;
      grt::ListRef<db_mysql_Schema> (db_mysql_Catalog::*getter)() const = 0;
      meta->bind_member("schemata", new grt::MetaClass::Property<db_mysql_Catalog,grt::ListRef<db_mysql_Schema>>(getter, setter));
    }
    {
      void (db_mysql_Catalog::*setter)(const grt::ListRef<db_mysql_ServerLink> &) = 0;
      grt::ListRef<db_mysql_ServerLink> (db_mysql_Catalog::*getter)() const = 0;
      meta->bind_member("serverLinks", new grt::MetaClass::Property<db_mysql_Catalog,grt::ListRef<db_mysql_ServerLink>>(getter, setter));
    }
    {
      void (db_mysql_Catalog::*setter)(const grt::ListRef<db_mysql_Tablespace> &) = 0;
      grt::ListRef<db_mysql_Tablespace> (db_mysql_Catalog::*getter)() const = 0;
      meta->bind_member("tablespaces", new grt::MetaClass::Property<db_mysql_Catalog,grt::ListRef<db_mysql_Tablespace>>(getter, setter));
    }
  }
};



inline void register_structs_db_mysql_xml() {
  grt::internal::ClassRegistry::register_class<db_mysql_StorageEngine>();
  grt::internal::ClassRegistry::register_class<db_mysql_StorageEngineOption>();
  grt::internal::ClassRegistry::register_class<db_mysql_Sequence>();
  grt::internal::ClassRegistry::register_class<db_mysql_Synonym>();
  grt::internal::ClassRegistry::register_class<db_mysql_RoutineParam>();
  grt::internal::ClassRegistry::register_class<db_mysql_Routine>();
  grt::internal::ClassRegistry::register_class<db_mysql_RoutineGroup>();
  grt::internal::ClassRegistry::register_class<db_mysql_View>();
  grt::internal::ClassRegistry::register_class<db_mysql_Event>();
  grt::internal::ClassRegistry::register_class<db_mysql_Trigger>();
  grt::internal::ClassRegistry::register_class<db_mysql_ForeignKey>();
  grt::internal::ClassRegistry::register_class<db_mysql_IndexColumn>();
  grt::internal::ClassRegistry::register_class<db_mysql_Index>();
  grt::internal::ClassRegistry::register_class<db_mysql_StructuredDatatype>();
  grt::internal::ClassRegistry::register_class<db_mysql_SimpleDatatype>();
  grt::internal::ClassRegistry::register_class<db_mysql_Column>();
  grt::internal::ClassRegistry::register_class<db_mysql_Table>();
  grt::internal::ClassRegistry::register_class<db_mysql_PartitionDefinition>();
  grt::internal::ClassRegistry::register_class<db_mysql_ServerLink>();
  grt::internal::ClassRegistry::register_class<db_mysql_Tablespace>();
  grt::internal::ClassRegistry::register_class<db_mysql_LogFileGroup>();
  grt::internal::ClassRegistry::register_class<db_mysql_Schema>();
  grt::internal::ClassRegistry::register_class<db_mysql_Catalog>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_db_mysql_xml {
  _autoreg__structs_db_mysql_xml() {
    register_structs_db_mysql_xml();
  }
} __autoreg__structs_db_mysql_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

