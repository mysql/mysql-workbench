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
  #ifdef GRT_STRUCTS_DB_EXPORT
  #define GRT_STRUCTS_DB_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_DB_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_DB_PUBLIC
#endif

#include "grts/structs.h"

class db_DatabaseSyncObject;
typedef grt::Ref<db_DatabaseSyncObject> db_DatabaseSyncObjectRef;
class db_DatabaseSync;
typedef grt::Ref<db_DatabaseSync> db_DatabaseSyncRef;
class db_Script;
typedef grt::Ref<db_Script> db_ScriptRef;
class db_CharacterSet;
typedef grt::Ref<db_CharacterSet> db_CharacterSetRef;
class db_ForeignKey;
typedef grt::Ref<db_ForeignKey> db_ForeignKeyRef;
class db_IndexColumn;
typedef grt::Ref<db_IndexColumn> db_IndexColumnRef;
class db_CheckConstraint;
typedef grt::Ref<db_CheckConstraint> db_CheckConstraintRef;
class db_UserDatatype;
typedef grt::Ref<db_UserDatatype> db_UserDatatypeRef;
class db_SimpleDatatype;
typedef grt::Ref<db_SimpleDatatype> db_SimpleDatatypeRef;
class db_DatatypeGroup;
typedef grt::Ref<db_DatatypeGroup> db_DatatypeGroupRef;
class db_Column;
typedef grt::Ref<db_Column> db_ColumnRef;
class db_RolePrivilege;
typedef grt::Ref<db_RolePrivilege> db_RolePrivilegeRef;
class db_Catalog;
typedef grt::Ref<db_Catalog> db_CatalogRef;
class db_DatabaseObject;
typedef grt::Ref<db_DatabaseObject> db_DatabaseObjectRef;
class db_Sequence;
typedef grt::Ref<db_Sequence> db_SequenceRef;
class db_Synonym;
typedef grt::Ref<db_Synonym> db_SynonymRef;
class db_RoutineGroup;
typedef grt::Ref<db_RoutineGroup> db_RoutineGroupRef;
class db_Index;
typedef grt::Ref<db_Index> db_IndexRef;
class db_StructuredDatatype;
typedef grt::Ref<db_StructuredDatatype> db_StructuredDatatypeRef;
class db_Table;
typedef grt::Ref<db_Table> db_TableRef;
class db_ServerLink;
typedef grt::Ref<db_ServerLink> db_ServerLinkRef;
class db_Schema;
typedef grt::Ref<db_Schema> db_SchemaRef;
class db_Tablespace;
typedef grt::Ref<db_Tablespace> db_TablespaceRef;
class db_LogFileGroup;
typedef grt::Ref<db_LogFileGroup> db_LogFileGroupRef;
class db_User;
typedef grt::Ref<db_User> db_UserRef;
class db_Role;
typedef grt::Ref<db_Role> db_RoleRef;
class db_DatabaseDdlObject;
typedef grt::Ref<db_DatabaseDdlObject> db_DatabaseDdlObjectRef;
class db_Event;
typedef grt::Ref<db_Event> db_EventRef;
class db_Trigger;
typedef grt::Ref<db_Trigger> db_TriggerRef;
class db_Routine;
typedef grt::Ref<db_Routine> db_RoutineRef;
class db_View;
typedef grt::Ref<db_View> db_ViewRef;


class db_query_EditableResultset;
typedef grt::Ref<db_query_EditableResultset> db_query_EditableResultsetRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** an object used for object changes */
class  db_DatabaseSyncObject : public GrtObject {
  typedef GrtObject super;

public:
  db_DatabaseSyncObject(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _alterDirection(0),
      _changed(0),
      _children(this, false),
      _syncLog(this, false) {
  }

  static std::string static_class_name() {
    return "db.DatabaseSyncObject";
  }

  /**
   * Getter for attribute alterDirection
   *
   * 0 to apply the change to the database, 1 to apply the change to the model
   * \par In Python:
   *    value = obj.alterDirection
   */
  grt::IntegerRef alterDirection() const { return _alterDirection; }

  /**
   * Setter for attribute alterDirection
   * 
   * 0 to apply the change to the database, 1 to apply the change to the model
   * \par In Python:
   *   obj.alterDirection = value
   */
  virtual void alterDirection(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_alterDirection);
    _alterDirection = value;
    member_changed("alterDirection", ovalue, value);
  }

  /**
   * Getter for attribute changed
   *
   * if set to 1 the object has been modified
   * \par In Python:
   *    value = obj.changed
   */
  grt::IntegerRef changed() const { return _changed; }

  /**
   * Setter for attribute changed
   * 
   * if set to 1 the object has been modified
   * \par In Python:
   *   obj.changed = value
   */
  virtual void changed(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_changed);
    _changed = value;
    member_changed("changed", ovalue, value);
  }

  // children is owned by db_DatabaseSyncObject
  /**
   * Getter for attribute children (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.children
   */
  grt::ListRef<db_DatabaseSyncObject> children() const { return _children; }


private: // The next attribute is read-only.
  virtual void children(const grt::ListRef<db_DatabaseSyncObject> &value) {
    grt::ValueRef ovalue(_children);

    _children = value;
    owned_member_changed("children", ovalue, value);
  }
public:

  /**
   * Getter for attribute dbObject
   *
   * reference to the database object, empty if this is a new object in the model
   * \par In Python:
   *    value = obj.dbObject
   */
  GrtNamedObjectRef dbObject() const { return _dbObject; }

  /**
   * Setter for attribute dbObject
   * 
   * reference to the database object, empty if this is a new object in the model
   * \par In Python:
   *   obj.dbObject = value
   */
  virtual void dbObject(const GrtNamedObjectRef &value) {
    grt::ValueRef ovalue(_dbObject);
    _dbObject = value;
    member_changed("dbObject", ovalue, value);
  }

  /**
   * Getter for attribute modelObject
   *
   * reference to the model object, empty if this is a new object in the database
   * \par In Python:
   *    value = obj.modelObject
   */
  GrtNamedObjectRef modelObject() const { return _modelObject; }

  /**
   * Setter for attribute modelObject
   * 
   * reference to the model object, empty if this is a new object in the database
   * \par In Python:
   *   obj.modelObject = value
   */
  virtual void modelObject(const GrtNamedObjectRef &value) {
    grt::ValueRef ovalue(_modelObject);
    _modelObject = value;
    member_changed("modelObject", ovalue, value);
  }

  // syncLog is owned by db_DatabaseSyncObject
  /**
   * Getter for attribute syncLog (read-only)
   *
   * a listing of log messages generated during object synchronization
   * \par In Python:
   *    value = obj.syncLog
   */
  grt::ListRef<GrtLogObject> syncLog() const { return _syncLog; }


private: // The next attribute is read-only.
  virtual void syncLog(const grt::ListRef<GrtLogObject> &value) {
    grt::ValueRef ovalue(_syncLog);

    _syncLog = value;
    owned_member_changed("syncLog", ovalue, value);
  }
public:

protected:

  grt::IntegerRef _alterDirection;
  grt::IntegerRef _changed;
  grt::ListRef<db_DatabaseSyncObject> _children;// owned
  GrtNamedObjectRef _dbObject;
  GrtNamedObjectRef _modelObject;
  grt::ListRef<GrtLogObject> _syncLog;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_DatabaseSyncObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_DatabaseSyncObject::create);
    {
      void (db_DatabaseSyncObject::*setter)(const grt::IntegerRef &) = &db_DatabaseSyncObject::alterDirection;
      grt::IntegerRef (db_DatabaseSyncObject::*getter)() const = &db_DatabaseSyncObject::alterDirection;
      meta->bind_member("alterDirection", new grt::MetaClass::Property<db_DatabaseSyncObject,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_DatabaseSyncObject::*setter)(const grt::IntegerRef &) = &db_DatabaseSyncObject::changed;
      grt::IntegerRef (db_DatabaseSyncObject::*getter)() const = &db_DatabaseSyncObject::changed;
      meta->bind_member("changed", new grt::MetaClass::Property<db_DatabaseSyncObject,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_DatabaseSyncObject::*setter)(const grt::ListRef<db_DatabaseSyncObject> &) = &db_DatabaseSyncObject::children;
      grt::ListRef<db_DatabaseSyncObject> (db_DatabaseSyncObject::*getter)() const = &db_DatabaseSyncObject::children;
      meta->bind_member("children", new grt::MetaClass::Property<db_DatabaseSyncObject,grt::ListRef<db_DatabaseSyncObject>>(getter, setter));
    }
    {
      void (db_DatabaseSyncObject::*setter)(const GrtNamedObjectRef &) = &db_DatabaseSyncObject::dbObject;
      GrtNamedObjectRef (db_DatabaseSyncObject::*getter)() const = &db_DatabaseSyncObject::dbObject;
      meta->bind_member("dbObject", new grt::MetaClass::Property<db_DatabaseSyncObject,GrtNamedObjectRef>(getter, setter));
    }
    {
      void (db_DatabaseSyncObject::*setter)(const GrtNamedObjectRef &) = &db_DatabaseSyncObject::modelObject;
      GrtNamedObjectRef (db_DatabaseSyncObject::*getter)() const = &db_DatabaseSyncObject::modelObject;
      meta->bind_member("modelObject", new grt::MetaClass::Property<db_DatabaseSyncObject,GrtNamedObjectRef>(getter, setter));
    }
    {
      void (db_DatabaseSyncObject::*setter)(const grt::ListRef<GrtLogObject> &) = &db_DatabaseSyncObject::syncLog;
      grt::ListRef<GrtLogObject> (db_DatabaseSyncObject::*getter)() const = &db_DatabaseSyncObject::syncLog;
      meta->bind_member("syncLog", new grt::MetaClass::Property<db_DatabaseSyncObject,grt::ListRef<GrtLogObject>>(getter, setter));
    }
  }
};

/** an object used for synchronisation */
class  db_DatabaseSync : public GrtObject {
  typedef GrtObject super;

public:
  db_DatabaseSync(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "db.DatabaseSync";
  }

  // changeTree is owned by db_DatabaseSync
  /**
   * Getter for attribute changeTree
   *
   * the tree of changes to apply
   * \par In Python:
   *    value = obj.changeTree
   */
  db_DatabaseSyncObjectRef changeTree() const { return _changeTree; }

  /**
   * Setter for attribute changeTree
   * 
   * the tree of changes to apply
   * \par In Python:
   *   obj.changeTree = value
   */
  virtual void changeTree(const db_DatabaseSyncObjectRef &value) {
    grt::ValueRef ovalue(_changeTree);

    _changeTree = value;
    owned_member_changed("changeTree", ovalue, value);
  }

  // dbCatalog is owned by db_DatabaseSync
  /**
   * Getter for attribute dbCatalog
   *
   * the database's catalog
   * \par In Python:
   *    value = obj.dbCatalog
   */
  db_CatalogRef dbCatalog() const { return _dbCatalog; }

  /**
   * Setter for attribute dbCatalog
   * 
   * the database's catalog
   * \par In Python:
   *   obj.dbCatalog = value
   */
  virtual void dbCatalog(const db_CatalogRef &value) {
    grt::ValueRef ovalue(_dbCatalog);

    _dbCatalog = value;
    owned_member_changed("dbCatalog", ovalue, value);
  }

protected:

  db_DatabaseSyncObjectRef _changeTree;// owned
  db_CatalogRef _dbCatalog;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_DatabaseSync());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_DatabaseSync::create);
    {
      void (db_DatabaseSync::*setter)(const db_DatabaseSyncObjectRef &) = &db_DatabaseSync::changeTree;
      db_DatabaseSyncObjectRef (db_DatabaseSync::*getter)() const = &db_DatabaseSync::changeTree;
      meta->bind_member("changeTree", new grt::MetaClass::Property<db_DatabaseSync,db_DatabaseSyncObjectRef>(getter, setter));
    }
    {
      void (db_DatabaseSync::*setter)(const db_CatalogRef &) = &db_DatabaseSync::dbCatalog;
      db_CatalogRef (db_DatabaseSync::*getter)() const = &db_DatabaseSync::dbCatalog;
      meta->bind_member("dbCatalog", new grt::MetaClass::Property<db_DatabaseSync,db_CatalogRef>(getter, setter));
    }
  }
};

/** a SQL script */
class  db_Script : public GrtStoredNote {
  typedef GrtStoredNote super;

public:
  db_Script(grt::MetaClass *meta = nullptr)
    : GrtStoredNote(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _forwardEngineerScriptPosition(""),
      _synchronizeScriptPosition("") {
  }

  static std::string static_class_name() {
    return "db.Script";
  }

  /**
   * Getter for attribute forwardEngineerScriptPosition
   *
   * 
   * \par In Python:
   *    value = obj.forwardEngineerScriptPosition
   */
  grt::StringRef forwardEngineerScriptPosition() const { return _forwardEngineerScriptPosition; }

  /**
   * Setter for attribute forwardEngineerScriptPosition
   * 
   * 
   * \par In Python:
   *   obj.forwardEngineerScriptPosition = value
   */
  virtual void forwardEngineerScriptPosition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_forwardEngineerScriptPosition);
    _forwardEngineerScriptPosition = value;
    member_changed("forwardEngineerScriptPosition", ovalue, value);
  }

  /**
   * Getter for attribute synchronizeScriptPosition
   *
   * 
   * \par In Python:
   *    value = obj.synchronizeScriptPosition
   */
  grt::StringRef synchronizeScriptPosition() const { return _synchronizeScriptPosition; }

  /**
   * Setter for attribute synchronizeScriptPosition
   * 
   * 
   * \par In Python:
   *   obj.synchronizeScriptPosition = value
   */
  virtual void synchronizeScriptPosition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_synchronizeScriptPosition);
    _synchronizeScriptPosition = value;
    member_changed("synchronizeScriptPosition", ovalue, value);
  }

protected:

  grt::StringRef _forwardEngineerScriptPosition;
  grt::StringRef _synchronizeScriptPosition;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Script());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Script::create);
    {
      void (db_Script::*setter)(const grt::StringRef &) = &db_Script::forwardEngineerScriptPosition;
      grt::StringRef (db_Script::*getter)() const = &db_Script::forwardEngineerScriptPosition;
      meta->bind_member("forwardEngineerScriptPosition", new grt::MetaClass::Property<db_Script,grt::StringRef>(getter, setter));
    }
    {
      void (db_Script::*setter)(const grt::StringRef &) = &db_Script::synchronizeScriptPosition;
      grt::StringRef (db_Script::*getter)() const = &db_Script::synchronizeScriptPosition;
      meta->bind_member("synchronizeScriptPosition", new grt::MetaClass::Property<db_Script,grt::StringRef>(getter, setter));
    }
  }
};

class  db_CharacterSet : public GrtObject {
  typedef GrtObject super;

public:
  db_CharacterSet(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _collations(this, false),
      _defaultCollation(""),
      _description("") {
  }

  static std::string static_class_name() {
    return "db.CharacterSet";
  }

  /**
   * Getter for attribute collations (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.collations
   */
  grt::StringListRef collations() const { return _collations; }


private: // The next attribute is read-only.
  virtual void collations(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_collations);
    _collations = value;
    member_changed("collations", ovalue, value);
  }
public:

  /**
   * Getter for attribute defaultCollation
   *
   * 
   * \par In Python:
   *    value = obj.defaultCollation
   */
  grt::StringRef defaultCollation() const { return _defaultCollation; }

  /**
   * Setter for attribute defaultCollation
   * 
   * 
   * \par In Python:
   *   obj.defaultCollation = value
   */
  virtual void defaultCollation(const grt::StringRef &value) {
    grt::ValueRef ovalue(_defaultCollation);
    _defaultCollation = value;
    member_changed("defaultCollation", ovalue, value);
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

  grt::StringListRef _collations;
  grt::StringRef _defaultCollation;
  grt::StringRef _description;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_CharacterSet());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_CharacterSet::create);
    {
      void (db_CharacterSet::*setter)(const grt::StringListRef &) = &db_CharacterSet::collations;
      grt::StringListRef (db_CharacterSet::*getter)() const = &db_CharacterSet::collations;
      meta->bind_member("collations", new grt::MetaClass::Property<db_CharacterSet,grt::StringListRef>(getter, setter));
    }
    {
      void (db_CharacterSet::*setter)(const grt::StringRef &) = &db_CharacterSet::defaultCollation;
      grt::StringRef (db_CharacterSet::*getter)() const = &db_CharacterSet::defaultCollation;
      meta->bind_member("defaultCollation", new grt::MetaClass::Property<db_CharacterSet,grt::StringRef>(getter, setter));
    }
    {
      void (db_CharacterSet::*setter)(const grt::StringRef &) = &db_CharacterSet::description;
      grt::StringRef (db_CharacterSet::*getter)() const = &db_CharacterSet::description;
      meta->bind_member("description", new grt::MetaClass::Property<db_CharacterSet,grt::StringRef>(getter, setter));
    }
  }
};

class GRT_STRUCTS_DB_PUBLIC db_ForeignKey : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  db_ForeignKey(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _columns(this, false),
      _customData(this, false),
      _deferability(0),
      _deleteRule(""),
      _mandatory(1),
      _many(1),
      _modelOnly(0),
      _referencedColumns(this, false),
      _referencedMandatory(1),
      _updateRule("") {
  }

  virtual ~db_ForeignKey();

  static std::string static_class_name() {
    return "db.ForeignKey";
  }

  /**
   * Getter for attribute columns (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.columns
   */
  grt::ListRef<db_Column> columns() const { return _columns; }


private: // The next attribute is read-only.
  virtual void columns(const grt::ListRef<db_Column> &value) {
    grt::ValueRef ovalue(_columns);
    _columns = value;
    member_changed("columns", ovalue, value);
  }
public:

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

  /**
   * Getter for attribute deferability
   *
   * 
   * \par In Python:
   *    value = obj.deferability
   */
  grt::IntegerRef deferability() const { return _deferability; }

  /**
   * Setter for attribute deferability
   * 
   * 
   * \par In Python:
   *   obj.deferability = value
   */
  virtual void deferability(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_deferability);
    _deferability = value;
    member_changed("deferability", ovalue, value);
  }

  /**
   * Getter for attribute deleteRule
   *
   * 
   * \par In Python:
   *    value = obj.deleteRule
   */
  grt::StringRef deleteRule() const { return _deleteRule; }

  /**
   * Setter for attribute deleteRule
   * 
   * 
   * \par In Python:
   *   obj.deleteRule = value
   */
  virtual void deleteRule(const grt::StringRef &value) {
    grt::ValueRef ovalue(_deleteRule);
    _deleteRule = value;
    member_changed("deleteRule", ovalue, value);
  }

  /**
   * Getter for attribute index
   *
   * Index that was created for this Foreign Key. This should only be set when a index is created for the FK, in other cases (like on reverse engieer) it should be left unset. For that reason this should not be used to find the matching index for the FK.
   * \par In Python:
   *    value = obj.index
   */
  db_IndexRef index() const { return _index; }

  /**
   * Setter for attribute index
   * 
   * Index that was created for this Foreign Key. This should only be set when a index is created for the FK, in other cases (like on reverse engieer) it should be left unset. For that reason this should not be used to find the matching index for the FK.
   * \par In Python:
   *   obj.index = value
   */
  virtual void index(const db_IndexRef &value) {
    grt::ValueRef ovalue(_index);
    _index = value;
    member_changed("index", ovalue, value);
  }

  /**
   * Getter for attribute mandatory
   *
   * mandatory in the owner table
   * \par In Python:
   *    value = obj.mandatory
   */
  grt::IntegerRef mandatory() const { return _mandatory; }

  /**
   * Setter for attribute mandatory
   * 
   * mandatory in the owner table
   * \par In Python:
   *   obj.mandatory = value
   */
  virtual void mandatory(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_mandatory);
    _mandatory = value;
    member_changed("mandatory", ovalue, value);
  }

  /**
   * Getter for attribute many
   *
   * cardinality of owner table
   * \par In Python:
   *    value = obj.many
   */
  grt::IntegerRef many() const { return _many; }

  /**
   * Setter for attribute many
   * 
   * cardinality of owner table
   * \par In Python:
   *   obj.many = value
   */
  virtual void many(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_many);
    _many = value;
    member_changed("many", ovalue, value);
  }

  /**
   * Getter for attribute modelOnly
   *
   * 
   * \par In Python:
   *    value = obj.modelOnly
   */
  grt::IntegerRef modelOnly() const { return _modelOnly; }

  /**
   * Setter for attribute modelOnly
   * 
   * 
   * \par In Python:
   *   obj.modelOnly = value
   */
  virtual void modelOnly(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_modelOnly);
    _modelOnly = value;
    member_changed("modelOnly", ovalue, value);
  }

  /**
   * Getter for attribute owner
   *
   * 
   * \par In Python:
   *    value = obj.owner
   */
  db_TableRef owner() const { return db_TableRef::cast_from(_owner); }

  /**
   * Setter for attribute owner
   * 
   * 
   * \par In Python:
   *   obj.owner = value
   */
  virtual void owner(const db_TableRef &value);

  /**
   * Getter for attribute referencedColumns (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.referencedColumns
   */
  grt::ListRef<db_Column> referencedColumns() const { return _referencedColumns; }


private: // The next attribute is read-only.
  virtual void referencedColumns(const grt::ListRef<db_Column> &value) {
    grt::ValueRef ovalue(_referencedColumns);
    _referencedColumns = value;
    member_changed("referencedColumns", ovalue, value);
  }
public:

  /**
   * Getter for attribute referencedMandatory
   *
   * mandatory in the referenced table
   * \par In Python:
   *    value = obj.referencedMandatory
   */
  grt::IntegerRef referencedMandatory() const { return _referencedMandatory; }

  /**
   * Setter for attribute referencedMandatory
   * 
   * mandatory in the referenced table
   * \par In Python:
   *   obj.referencedMandatory = value
   */
  virtual void referencedMandatory(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_referencedMandatory);
    _referencedMandatory = value;
    member_changed("referencedMandatory", ovalue, value);
  }

  /**
   * Getter for attribute referencedTable
   *
   * 
   * \par In Python:
   *    value = obj.referencedTable
   */
  db_TableRef referencedTable() const { return _referencedTable; }

  /**
   * Setter for attribute referencedTable
   * 
   * 
   * \par In Python:
   *   obj.referencedTable = value
   */
  virtual void referencedTable(const db_TableRef &value);

  /**
   * Getter for attribute updateRule
   *
   * 
   * \par In Python:
   *    value = obj.updateRule
   */
  grt::StringRef updateRule() const { return _updateRule; }

  /**
   * Setter for attribute updateRule
   * 
   * 
   * \par In Python:
   *   obj.updateRule = value
   */
  virtual void updateRule(const grt::StringRef &value) {
    grt::ValueRef ovalue(_updateRule);
    _updateRule = value;
    member_changed("updateRule", ovalue, value);
  }

  /**
   * Method. 
   * \return 
   */
  virtual grt::IntegerRef checkCompleteness();
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:
  virtual void owned_list_item_added(grt::internal::OwnedList *list, const grt::ValueRef &value);
  virtual void owned_list_item_removed(grt::internal::OwnedList *list, const grt::ValueRef &value);

  grt::ListRef<db_Column> _columns;
  grt::DictRef _customData;
  grt::IntegerRef _deferability;
  grt::StringRef _deleteRule;
  db_IndexRef _index;
  grt::IntegerRef _mandatory;
  grt::IntegerRef _many;
  grt::IntegerRef _modelOnly;
  grt::ListRef<db_Column> _referencedColumns;
  grt::IntegerRef _referencedMandatory;
  db_TableRef _referencedTable;
  grt::StringRef _updateRule;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_ForeignKey());
  }

  static grt::ValueRef call_checkCompleteness(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_ForeignKey*>(self)->checkCompleteness(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_ForeignKey::create);
    {
      void (db_ForeignKey::*setter)(const grt::ListRef<db_Column> &) = &db_ForeignKey::columns;
      grt::ListRef<db_Column> (db_ForeignKey::*getter)() const = &db_ForeignKey::columns;
      meta->bind_member("columns", new grt::MetaClass::Property<db_ForeignKey,grt::ListRef<db_Column>>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::DictRef &) = &db_ForeignKey::customData;
      grt::DictRef (db_ForeignKey::*getter)() const = &db_ForeignKey::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<db_ForeignKey,grt::DictRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::IntegerRef &) = &db_ForeignKey::deferability;
      grt::IntegerRef (db_ForeignKey::*getter)() const = &db_ForeignKey::deferability;
      meta->bind_member("deferability", new grt::MetaClass::Property<db_ForeignKey,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::StringRef &) = &db_ForeignKey::deleteRule;
      grt::StringRef (db_ForeignKey::*getter)() const = &db_ForeignKey::deleteRule;
      meta->bind_member("deleteRule", new grt::MetaClass::Property<db_ForeignKey,grt::StringRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const db_IndexRef &) = &db_ForeignKey::index;
      db_IndexRef (db_ForeignKey::*getter)() const = &db_ForeignKey::index;
      meta->bind_member("index", new grt::MetaClass::Property<db_ForeignKey,db_IndexRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::IntegerRef &) = &db_ForeignKey::mandatory;
      grt::IntegerRef (db_ForeignKey::*getter)() const = &db_ForeignKey::mandatory;
      meta->bind_member("mandatory", new grt::MetaClass::Property<db_ForeignKey,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::IntegerRef &) = &db_ForeignKey::many;
      grt::IntegerRef (db_ForeignKey::*getter)() const = &db_ForeignKey::many;
      meta->bind_member("many", new grt::MetaClass::Property<db_ForeignKey,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::IntegerRef &) = &db_ForeignKey::modelOnly;
      grt::IntegerRef (db_ForeignKey::*getter)() const = &db_ForeignKey::modelOnly;
      meta->bind_member("modelOnly", new grt::MetaClass::Property<db_ForeignKey,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const db_TableRef &) = &db_ForeignKey::owner;
      db_TableRef (db_ForeignKey::*getter)() const = 0;
      meta->bind_member("owner", new grt::MetaClass::Property<db_ForeignKey,db_TableRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::ListRef<db_Column> &) = &db_ForeignKey::referencedColumns;
      grt::ListRef<db_Column> (db_ForeignKey::*getter)() const = &db_ForeignKey::referencedColumns;
      meta->bind_member("referencedColumns", new grt::MetaClass::Property<db_ForeignKey,grt::ListRef<db_Column>>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::IntegerRef &) = &db_ForeignKey::referencedMandatory;
      grt::IntegerRef (db_ForeignKey::*getter)() const = &db_ForeignKey::referencedMandatory;
      meta->bind_member("referencedMandatory", new grt::MetaClass::Property<db_ForeignKey,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const db_TableRef &) = &db_ForeignKey::referencedTable;
      db_TableRef (db_ForeignKey::*getter)() const = &db_ForeignKey::referencedTable;
      meta->bind_member("referencedTable", new grt::MetaClass::Property<db_ForeignKey,db_TableRef>(getter, setter));
    }
    {
      void (db_ForeignKey::*setter)(const grt::StringRef &) = &db_ForeignKey::updateRule;
      grt::StringRef (db_ForeignKey::*getter)() const = &db_ForeignKey::updateRule;
      meta->bind_member("updateRule", new grt::MetaClass::Property<db_ForeignKey,grt::StringRef>(getter, setter));
    }
    meta->bind_method("checkCompleteness", &db_ForeignKey::call_checkCompleteness);
  }
};

class  db_IndexColumn : public GrtObject {
  typedef GrtObject super;

public:
  db_IndexColumn(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _columnLength(0),
      _comment(""),
      _descend(0),
      _expression("") {
  }

  static std::string static_class_name() {
    return "db.IndexColumn";
  }

  /**
   * Getter for attribute columnLength
   *
   * 
   * \par In Python:
   *    value = obj.columnLength
   */
  grt::IntegerRef columnLength() const { return _columnLength; }

  /**
   * Setter for attribute columnLength
   * 
   * 
   * \par In Python:
   *   obj.columnLength = value
   */
  virtual void columnLength(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_columnLength);
    _columnLength = value;
    member_changed("columnLength", ovalue, value);
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
   * Getter for attribute descend
   *
   * 
   * \par In Python:
   *    value = obj.descend
   */
  grt::IntegerRef descend() const { return _descend; }

  /**
   * Setter for attribute descend
   * 
   * 
   * \par In Python:
   *   obj.descend = value
   */
  virtual void descend(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_descend);
    _descend = value;
    member_changed("descend", ovalue, value);
  }

  /**
   * Getter for attribute expression
   *
   * used when no referenced column is defined
   * \par In Python:
   *    value = obj.expression
   */
  grt::StringRef expression() const { return _expression; }

  /**
   * Setter for attribute expression
   * 
   * used when no referenced column is defined
   * \par In Python:
   *   obj.expression = value
   */
  virtual void expression(const grt::StringRef &value) {
    grt::ValueRef ovalue(_expression);
    _expression = value;
    member_changed("expression", ovalue, value);
  }

  /**
   * Getter for attribute referencedColumn
   *
   * 
   * \par In Python:
   *    value = obj.referencedColumn
   */
  db_ColumnRef referencedColumn() const { return _referencedColumn; }

  /**
   * Setter for attribute referencedColumn
   * 
   * 
   * \par In Python:
   *   obj.referencedColumn = value
   */
  virtual void referencedColumn(const db_ColumnRef &value) {
    grt::ValueRef ovalue(_referencedColumn);
    _referencedColumn = value;
    member_changed("referencedColumn", ovalue, value);
  }

protected:

  grt::IntegerRef _columnLength;
  grt::StringRef _comment;
  grt::IntegerRef _descend;
  grt::StringRef _expression;
  db_ColumnRef _referencedColumn;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_IndexColumn());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_IndexColumn::create);
    {
      void (db_IndexColumn::*setter)(const grt::IntegerRef &) = &db_IndexColumn::columnLength;
      grt::IntegerRef (db_IndexColumn::*getter)() const = &db_IndexColumn::columnLength;
      meta->bind_member("columnLength", new grt::MetaClass::Property<db_IndexColumn,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_IndexColumn::*setter)(const grt::StringRef &) = &db_IndexColumn::comment;
      grt::StringRef (db_IndexColumn::*getter)() const = &db_IndexColumn::comment;
      meta->bind_member("comment", new grt::MetaClass::Property<db_IndexColumn,grt::StringRef>(getter, setter));
    }
    {
      void (db_IndexColumn::*setter)(const grt::IntegerRef &) = &db_IndexColumn::descend;
      grt::IntegerRef (db_IndexColumn::*getter)() const = &db_IndexColumn::descend;
      meta->bind_member("descend", new grt::MetaClass::Property<db_IndexColumn,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_IndexColumn::*setter)(const grt::StringRef &) = &db_IndexColumn::expression;
      grt::StringRef (db_IndexColumn::*getter)() const = &db_IndexColumn::expression;
      meta->bind_member("expression", new grt::MetaClass::Property<db_IndexColumn,grt::StringRef>(getter, setter));
    }
    {
      void (db_IndexColumn::*setter)(const db_ColumnRef &) = &db_IndexColumn::referencedColumn;
      db_ColumnRef (db_IndexColumn::*getter)() const = &db_IndexColumn::referencedColumn;
      meta->bind_member("referencedColumn", new grt::MetaClass::Property<db_IndexColumn,db_ColumnRef>(getter, setter));
    }
  }
};

class  db_CheckConstraint : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  db_CheckConstraint(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _searchCondition("") {
  }

  static std::string static_class_name() {
    return "db.CheckConstraint";
  }

  /**
   * Getter for attribute searchCondition
   *
   * 
   * \par In Python:
   *    value = obj.searchCondition
   */
  grt::StringRef searchCondition() const { return _searchCondition; }

  /**
   * Setter for attribute searchCondition
   * 
   * 
   * \par In Python:
   *   obj.searchCondition = value
   */
  virtual void searchCondition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_searchCondition);
    _searchCondition = value;
    member_changed("searchCondition", ovalue, value);
  }

protected:

  grt::StringRef _searchCondition;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_CheckConstraint());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_CheckConstraint::create);
    {
      void (db_CheckConstraint::*setter)(const grt::StringRef &) = &db_CheckConstraint::searchCondition;
      grt::StringRef (db_CheckConstraint::*getter)() const = &db_CheckConstraint::searchCondition;
      meta->bind_member("searchCondition", new grt::MetaClass::Property<db_CheckConstraint,grt::StringRef>(getter, setter));
    }
  }
};

class  db_UserDatatype : public GrtObject {
  typedef GrtObject super;

public:
  db_UserDatatype(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _flags(""),
      _sqlDefinition("") {
  }

  static std::string static_class_name() {
    return "db.UserDatatype";
  }

  /**
   * Getter for attribute actualType
   *
   * 
   * \par In Python:
   *    value = obj.actualType
   */
  db_SimpleDatatypeRef actualType() const { return _actualType; }

  /**
   * Setter for attribute actualType
   * 
   * 
   * \par In Python:
   *   obj.actualType = value
   */
  virtual void actualType(const db_SimpleDatatypeRef &value) {
    grt::ValueRef ovalue(_actualType);
    _actualType = value;
    member_changed("actualType", ovalue, value);
  }

  /**
   * Getter for attribute flags
   *
   * 
   * \par In Python:
   *    value = obj.flags
   */
  grt::StringRef flags() const { return _flags; }

  /**
   * Setter for attribute flags
   * 
   * 
   * \par In Python:
   *   obj.flags = value
   */
  virtual void flags(const grt::StringRef &value) {
    grt::ValueRef ovalue(_flags);
    _flags = value;
    member_changed("flags", ovalue, value);
  }

  /**
   * Getter for attribute sqlDefinition
   *
   * 
   * \par In Python:
   *    value = obj.sqlDefinition
   */
  grt::StringRef sqlDefinition() const { return _sqlDefinition; }

  /**
   * Setter for attribute sqlDefinition
   * 
   * 
   * \par In Python:
   *   obj.sqlDefinition = value
   */
  virtual void sqlDefinition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_sqlDefinition);
    _sqlDefinition = value;
    member_changed("sqlDefinition", ovalue, value);
  }

protected:

  db_SimpleDatatypeRef _actualType;
  grt::StringRef _flags;
  grt::StringRef _sqlDefinition;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_UserDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_UserDatatype::create);
    {
      void (db_UserDatatype::*setter)(const db_SimpleDatatypeRef &) = &db_UserDatatype::actualType;
      db_SimpleDatatypeRef (db_UserDatatype::*getter)() const = &db_UserDatatype::actualType;
      meta->bind_member("actualType", new grt::MetaClass::Property<db_UserDatatype,db_SimpleDatatypeRef>(getter, setter));
    }
    {
      void (db_UserDatatype::*setter)(const grt::StringRef &) = &db_UserDatatype::flags;
      grt::StringRef (db_UserDatatype::*getter)() const = &db_UserDatatype::flags;
      meta->bind_member("flags", new grt::MetaClass::Property<db_UserDatatype,grt::StringRef>(getter, setter));
    }
    {
      void (db_UserDatatype::*setter)(const grt::StringRef &) = &db_UserDatatype::sqlDefinition;
      grt::StringRef (db_UserDatatype::*getter)() const = &db_UserDatatype::sqlDefinition;
      meta->bind_member("sqlDefinition", new grt::MetaClass::Property<db_UserDatatype,grt::StringRef>(getter, setter));
    }
  }
};

class  db_SimpleDatatype : public GrtObject {
  typedef GrtObject super;

public:
  db_SimpleDatatype(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _characterMaximumLength(0),
      _characterOctetLength(0),
      _dateTimePrecision(0),
      _flags(this, false),
      _needsQuotes(0),
      _numericPrecision(0),
      _numericPrecisionRadix(0),
      _numericScale(0),
      _parameterFormatType(0),
      _synonyms(this, false),
      _validity("") {
  }

  static std::string static_class_name() {
    return "db.SimpleDatatype";
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
   * Getter for attribute characterOctetLength
   *
   * maximum number of 8 bit characters this datatype can store
   * \par In Python:
   *    value = obj.characterOctetLength
   */
  grt::IntegerRef characterOctetLength() const { return _characterOctetLength; }

  /**
   * Setter for attribute characterOctetLength
   * 
   * maximum number of 8 bit characters this datatype can store
   * \par In Python:
   *   obj.characterOctetLength = value
   */
  virtual void characterOctetLength(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_characterOctetLength);
    _characterOctetLength = value;
    member_changed("characterOctetLength", ovalue, value);
  }

  /**
   * Getter for attribute dateTimePrecision
   *
   * the datetime precision the datatype can store
   * \par In Python:
   *    value = obj.dateTimePrecision
   */
  grt::IntegerRef dateTimePrecision() const { return _dateTimePrecision; }

  /**
   * Setter for attribute dateTimePrecision
   * 
   * the datetime precision the datatype can store
   * \par In Python:
   *   obj.dateTimePrecision = value
   */
  virtual void dateTimePrecision(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_dateTimePrecision);
    _dateTimePrecision = value;
    member_changed("dateTimePrecision", ovalue, value);
  }

  /**
   * Getter for attribute flags (read-only)
   *
   * additional flags like UNSIGNED, ZEROFILL, BINARY
   * \par In Python:
   *    value = obj.flags
   */
  grt::StringListRef flags() const { return _flags; }


private: // The next attribute is read-only.
  virtual void flags(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_flags);
    _flags = value;
    member_changed("flags", ovalue, value);
  }
public:

  /**
   * Getter for attribute group
   *
   * the datatype group this datatype belongs to
   * \par In Python:
   *    value = obj.group
   */
  db_DatatypeGroupRef group() const { return _group; }

  /**
   * Setter for attribute group
   * 
   * the datatype group this datatype belongs to
   * \par In Python:
   *   obj.group = value
   */
  virtual void group(const db_DatatypeGroupRef &value) {
    grt::ValueRef ovalue(_group);
    _group = value;
    member_changed("group", ovalue, value);
  }

  /**
   * Getter for attribute needsQuotes
   *
   * whether values require quotes around them
   * \par In Python:
   *    value = obj.needsQuotes
   */
  grt::IntegerRef needsQuotes() const { return _needsQuotes; }

  /**
   * Setter for attribute needsQuotes
   * 
   * whether values require quotes around them
   * \par In Python:
   *   obj.needsQuotes = value
   */
  virtual void needsQuotes(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_needsQuotes);
    _needsQuotes = value;
    member_changed("needsQuotes", ovalue, value);
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
   * Getter for attribute numericPrecisionRadix
   *
   * 
   * \par In Python:
   *    value = obj.numericPrecisionRadix
   */
  grt::IntegerRef numericPrecisionRadix() const { return _numericPrecisionRadix; }

  /**
   * Setter for attribute numericPrecisionRadix
   * 
   * 
   * \par In Python:
   *   obj.numericPrecisionRadix = value
   */
  virtual void numericPrecisionRadix(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_numericPrecisionRadix);
    _numericPrecisionRadix = value;
    member_changed("numericPrecisionRadix", ovalue, value);
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

  /**
   * Getter for attribute parameterFormatType
   *
   * 0 none, 1 (n), 2 [(n)], 3 (m,n), 4 (m[,n]), 5 [(m,n)], 6 [(m[, n])], 10 ('a','b','c')
   * \par In Python:
   *    value = obj.parameterFormatType
   */
  grt::IntegerRef parameterFormatType() const { return _parameterFormatType; }

  /**
   * Setter for attribute parameterFormatType
   * 
   * 0 none, 1 (n), 2 [(n)], 3 (m,n), 4 (m[,n]), 5 [(m,n)], 6 [(m[, n])], 10 ('a','b','c')
   * \par In Python:
   *   obj.parameterFormatType = value
   */
  virtual void parameterFormatType(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_parameterFormatType);
    _parameterFormatType = value;
    member_changed("parameterFormatType", ovalue, value);
  }

  /**
   * Getter for attribute synonyms (read-only)
   *
   * the list of names that can be used as synonym for the datatype
   * \par In Python:
   *    value = obj.synonyms
   */
  grt::StringListRef synonyms() const { return _synonyms; }


private: // The next attribute is read-only.
  virtual void synonyms(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_synonyms);
    _synonyms = value;
    member_changed("synonyms", ovalue, value);
  }
public:

  /**
   * Getter for attribute validity
   *
   * information about validity of this type. Allowed: comparison operator followed by version number
   * \par In Python:
   *    value = obj.validity
   */
  grt::StringRef validity() const { return _validity; }

  /**
   * Setter for attribute validity
   * 
   * information about validity of this type. Allowed: comparison operator followed by version number
   * \par In Python:
   *   obj.validity = value
   */
  virtual void validity(const grt::StringRef &value) {
    grt::ValueRef ovalue(_validity);
    _validity = value;
    member_changed("validity", ovalue, value);
  }

protected:

  grt::IntegerRef _characterMaximumLength;
  grt::IntegerRef _characterOctetLength;
  grt::IntegerRef _dateTimePrecision;
  grt::StringListRef _flags;
  db_DatatypeGroupRef _group;
  grt::IntegerRef _needsQuotes;
  grt::IntegerRef _numericPrecision;
  grt::IntegerRef _numericPrecisionRadix;
  grt::IntegerRef _numericScale;
  grt::IntegerRef _parameterFormatType;
  grt::StringListRef _synonyms;
  grt::StringRef _validity;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_SimpleDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_SimpleDatatype::create);
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::characterMaximumLength;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::characterMaximumLength;
      meta->bind_member("characterMaximumLength", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::characterOctetLength;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::characterOctetLength;
      meta->bind_member("characterOctetLength", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::dateTimePrecision;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::dateTimePrecision;
      meta->bind_member("dateTimePrecision", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::StringListRef &) = &db_SimpleDatatype::flags;
      grt::StringListRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::flags;
      meta->bind_member("flags", new grt::MetaClass::Property<db_SimpleDatatype,grt::StringListRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const db_DatatypeGroupRef &) = &db_SimpleDatatype::group;
      db_DatatypeGroupRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::group;
      meta->bind_member("group", new grt::MetaClass::Property<db_SimpleDatatype,db_DatatypeGroupRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::needsQuotes;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::needsQuotes;
      meta->bind_member("needsQuotes", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::numericPrecision;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::numericPrecision;
      meta->bind_member("numericPrecision", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::numericPrecisionRadix;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::numericPrecisionRadix;
      meta->bind_member("numericPrecisionRadix", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::numericScale;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::numericScale;
      meta->bind_member("numericScale", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::IntegerRef &) = &db_SimpleDatatype::parameterFormatType;
      grt::IntegerRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::parameterFormatType;
      meta->bind_member("parameterFormatType", new grt::MetaClass::Property<db_SimpleDatatype,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::StringListRef &) = &db_SimpleDatatype::synonyms;
      grt::StringListRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::synonyms;
      meta->bind_member("synonyms", new grt::MetaClass::Property<db_SimpleDatatype,grt::StringListRef>(getter, setter));
    }
    {
      void (db_SimpleDatatype::*setter)(const grt::StringRef &) = &db_SimpleDatatype::validity;
      grt::StringRef (db_SimpleDatatype::*getter)() const = &db_SimpleDatatype::validity;
      meta->bind_member("validity", new grt::MetaClass::Property<db_SimpleDatatype,grt::StringRef>(getter, setter));
    }
  }
};

class  db_DatatypeGroup : public GrtObject {
  typedef GrtObject super;

public:
  db_DatatypeGroup(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _description("") {
  }

  static std::string static_class_name() {
    return "db.DatatypeGroup";
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
    return grt::ObjectRef(new db_DatatypeGroup());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_DatatypeGroup::create);
    {
      void (db_DatatypeGroup::*setter)(const grt::StringRef &) = &db_DatatypeGroup::caption;
      grt::StringRef (db_DatatypeGroup::*getter)() const = &db_DatatypeGroup::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<db_DatatypeGroup,grt::StringRef>(getter, setter));
    }
    {
      void (db_DatatypeGroup::*setter)(const grt::StringRef &) = &db_DatatypeGroup::description;
      grt::StringRef (db_DatatypeGroup::*getter)() const = &db_DatatypeGroup::description;
      meta->bind_member("description", new grt::MetaClass::Property<db_DatatypeGroup,grt::StringRef>(getter, setter));
    }
  }
};

class GRT_STRUCTS_DB_PUBLIC db_Column : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  db_Column(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _characterSetName(""),
      _checks(this, false),
      _collationName(""),
      _datatypeExplicitParams(""),
      _defaultValue(""),
      _defaultValueIsNull(0),
      _flags(this, false),
      _isNotNull(0),
      _length(-1),
      _precision(-1),
      _scale(-1) {
  }

  virtual ~db_Column();

  static std::string static_class_name() {
    return "db.Column";
  }

  /**
   * Getter for attribute characterSetName
   *
   * 
   * \par In Python:
   *    value = obj.characterSetName
   */
  grt::StringRef characterSetName() const { return _characterSetName; }

  /**
   * Setter for attribute characterSetName
   * 
   * 
   * \par In Python:
   *   obj.characterSetName = value
   */
  virtual void characterSetName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_characterSetName);
    _characterSetName = value;
    member_changed("characterSetName", ovalue, value);
  }

  // checks is owned by db_Column
  /**
   * Getter for attribute checks (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.checks
   */
  grt::ListRef<db_CheckConstraint> checks() const { return _checks; }


private: // The next attribute is read-only.
  virtual void checks(const grt::ListRef<db_CheckConstraint> &value) {
    grt::ValueRef ovalue(_checks);

    _checks = value;
    owned_member_changed("checks", ovalue, value);
  }
public:

  /**
   * Getter for attribute collationName
   *
   * 
   * \par In Python:
   *    value = obj.collationName
   */
  grt::StringRef collationName() const { return _collationName; }

  /**
   * Setter for attribute collationName
   * 
   * 
   * \par In Python:
   *   obj.collationName = value
   */
  virtual void collationName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_collationName);
    _collationName = value;
    member_changed("collationName", ovalue, value);
  }

  /**
   * Getter for attribute datatypeExplicitParams
   *
   * For ENUM, SET and similar datatypes the parametes can be defined explictly. Note that brackets need to be included. This will overwrite the precision, scale and length setting
   * \par In Python:
   *    value = obj.datatypeExplicitParams
   */
  grt::StringRef datatypeExplicitParams() const { return _datatypeExplicitParams; }

  /**
   * Setter for attribute datatypeExplicitParams
   * 
   * For ENUM, SET and similar datatypes the parametes can be defined explictly. Note that brackets need to be included. This will overwrite the precision, scale and length setting
   * \par In Python:
   *   obj.datatypeExplicitParams = value
   */
  virtual void datatypeExplicitParams(const grt::StringRef &value) {
    grt::ValueRef ovalue(_datatypeExplicitParams);
    _datatypeExplicitParams = value;
    member_changed("datatypeExplicitParams", ovalue, value);
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
   * Getter for attribute defaultValueIsNull
   *
   * 
   * \par In Python:
   *    value = obj.defaultValueIsNull
   */
  grt::IntegerRef defaultValueIsNull() const { return _defaultValueIsNull; }

  /**
   * Setter for attribute defaultValueIsNull
   * 
   * 
   * \par In Python:
   *   obj.defaultValueIsNull = value
   */
  virtual void defaultValueIsNull(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_defaultValueIsNull);
    _defaultValueIsNull = value;
    member_changed("defaultValueIsNull", ovalue, value);
  }

  /**
   * Getter for attribute flags (read-only)
   *
   * additional flags like UNSIGNED, ZEROFILL, BINARY
   * \par In Python:
   *    value = obj.flags
   */
  grt::StringListRef flags() const { return _flags; }


private: // The next attribute is read-only.
  virtual void flags(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_flags);
    _flags = value;
    member_changed("flags", ovalue, value);
  }
public:

  /**
   * Getter for attribute formattedRawType (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.formattedRawType
   */
  grt::StringRef formattedRawType() const;


private: // The next attribute is read-only.
public:

  /**
   * Getter for attribute formattedType
   *
   * 
   * \par In Python:
   *    value = obj.formattedType
   */
  grt::StringRef formattedType() const;

  /**
   * Setter for attribute formattedType
   * 
   * 
   * \par In Python:
   *   obj.formattedType = value
   */
  virtual void formattedType(const grt::StringRef &value);

  /**
   * Getter for attribute isNotNull
   *
   * 
   * \par In Python:
   *    value = obj.isNotNull
   */
  grt::IntegerRef isNotNull() const { return _isNotNull; }

  /**
   * Setter for attribute isNotNull
   * 
   * 
   * \par In Python:
   *   obj.isNotNull = value
   */
  virtual void isNotNull(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isNotNull);
    _isNotNull = value;
    member_changed("isNotNull", ovalue, value);
  }

  /**
   * Getter for attribute length
   *
   * The total length of the column. For string types this referes to the number of characters that can be stored.
   * \par In Python:
   *    value = obj.length
   */
  grt::IntegerRef length() const { return _length; }

  /**
   * Setter for attribute length
   * 
   * The total length of the column. For string types this referes to the number of characters that can be stored.
   * \par In Python:
   *   obj.length = value
   */
  virtual void length(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_length);
    _length = value;
    member_changed("length", ovalue, value);
  }

  /**
   * Getter for attribute precision
   *
   * for numeric types this represents the total number of digits that are stored including digits right from the decimal point
   * \par In Python:
   *    value = obj.precision
   */
  grt::IntegerRef precision() const { return _precision; }

  /**
   * Setter for attribute precision
   * 
   * for numeric types this represents the total number of digits that are stored including digits right from the decimal point
   * \par In Python:
   *   obj.precision = value
   */
  virtual void precision(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_precision);
    _precision = value;
    member_changed("precision", ovalue, value);
  }

  /**
   * Getter for attribute scale
   *
   * the number of digits right to the decimal point
   * \par In Python:
   *    value = obj.scale
   */
  grt::IntegerRef scale() const { return _scale; }

  /**
   * Setter for attribute scale
   * 
   * the number of digits right to the decimal point
   * \par In Python:
   *   obj.scale = value
   */
  virtual void scale(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_scale);
    _scale = value;
    member_changed("scale", ovalue, value);
  }

  /**
   * Getter for attribute simpleType
   *
   * 
   * \par In Python:
   *    value = obj.simpleType
   */
  db_SimpleDatatypeRef simpleType() const { return _simpleType; }

  /**
   * Setter for attribute simpleType
   * 
   * 
   * \par In Python:
   *   obj.simpleType = value
   */
  virtual void simpleType(const db_SimpleDatatypeRef &value) {
    grt::ValueRef ovalue(_simpleType);
    _simpleType = value;
    member_changed("simpleType", ovalue, value);
  }

  /**
   * Getter for attribute structuredType
   *
   * 
   * \par In Python:
   *    value = obj.structuredType
   */
  db_StructuredDatatypeRef structuredType() const { return _structuredType; }

  /**
   * Setter for attribute structuredType
   * 
   * 
   * \par In Python:
   *   obj.structuredType = value
   */
  virtual void structuredType(const db_StructuredDatatypeRef &value) {
    grt::ValueRef ovalue(_structuredType);
    _structuredType = value;
    member_changed("structuredType", ovalue, value);
  }

  /**
   * Getter for attribute userType
   *
   * 
   * \par In Python:
   *    value = obj.userType
   */
  db_UserDatatypeRef userType() const { return _userType; }

  /**
   * Setter for attribute userType
   * 
   * 
   * \par In Python:
   *   obj.userType = value
   */
  virtual void userType(const db_UserDatatypeRef &value) {
    grt::ValueRef ovalue(_userType);
    _userType = value;
    member_changed("userType", ovalue, value);
  }

  /**
   * Method. 
   * \param type 
   * \param typeList 
   * \return 
   */
  virtual grt::IntegerRef setParseType(const std::string &type, const grt::ListRef<db_SimpleDatatype> &typeList);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::StringRef _characterSetName;
  grt::ListRef<db_CheckConstraint> _checks;// owned
  grt::StringRef _collationName;
  grt::StringRef _datatypeExplicitParams;
  grt::StringRef _defaultValue;
  grt::IntegerRef _defaultValueIsNull;
  grt::StringListRef _flags;
  grt::IntegerRef _isNotNull;
  grt::IntegerRef _length;
  grt::IntegerRef _precision;
  grt::IntegerRef _scale;
  db_SimpleDatatypeRef _simpleType;
  db_StructuredDatatypeRef _structuredType;
  db_UserDatatypeRef _userType;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Column());
  }

  static grt::ValueRef call_setParseType(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Column*>(self)->setParseType(grt::StringRef::cast_from(args[0]), grt::ListRef<db_SimpleDatatype>::cast_from(args[1])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Column::create);
    {
      void (db_Column::*setter)(const grt::StringRef &) = &db_Column::characterSetName;
      grt::StringRef (db_Column::*getter)() const = &db_Column::characterSetName;
      meta->bind_member("characterSetName", new grt::MetaClass::Property<db_Column,grt::StringRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::ListRef<db_CheckConstraint> &) = &db_Column::checks;
      grt::ListRef<db_CheckConstraint> (db_Column::*getter)() const = &db_Column::checks;
      meta->bind_member("checks", new grt::MetaClass::Property<db_Column,grt::ListRef<db_CheckConstraint>>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::StringRef &) = &db_Column::collationName;
      grt::StringRef (db_Column::*getter)() const = &db_Column::collationName;
      meta->bind_member("collationName", new grt::MetaClass::Property<db_Column,grt::StringRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::StringRef &) = &db_Column::datatypeExplicitParams;
      grt::StringRef (db_Column::*getter)() const = &db_Column::datatypeExplicitParams;
      meta->bind_member("datatypeExplicitParams", new grt::MetaClass::Property<db_Column,grt::StringRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::StringRef &) = &db_Column::defaultValue;
      grt::StringRef (db_Column::*getter)() const = &db_Column::defaultValue;
      meta->bind_member("defaultValue", new grt::MetaClass::Property<db_Column,grt::StringRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::IntegerRef &) = &db_Column::defaultValueIsNull;
      grt::IntegerRef (db_Column::*getter)() const = &db_Column::defaultValueIsNull;
      meta->bind_member("defaultValueIsNull", new grt::MetaClass::Property<db_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::StringListRef &) = &db_Column::flags;
      grt::StringListRef (db_Column::*getter)() const = &db_Column::flags;
      meta->bind_member("flags", new grt::MetaClass::Property<db_Column,grt::StringListRef>(getter, setter));
    }
    meta->bind_member("formattedRawType", new grt::MetaClass::Property<db_Column,grt::StringRef>(&db_Column::formattedRawType));
    {
      void (db_Column::*setter)(const grt::StringRef &) = &db_Column::formattedType;
      grt::StringRef (db_Column::*getter)() const = &db_Column::formattedType;
      meta->bind_member("formattedType", new grt::MetaClass::Property<db_Column,grt::StringRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::IntegerRef &) = &db_Column::isNotNull;
      grt::IntegerRef (db_Column::*getter)() const = &db_Column::isNotNull;
      meta->bind_member("isNotNull", new grt::MetaClass::Property<db_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::IntegerRef &) = &db_Column::length;
      grt::IntegerRef (db_Column::*getter)() const = &db_Column::length;
      meta->bind_member("length", new grt::MetaClass::Property<db_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::IntegerRef &) = &db_Column::precision;
      grt::IntegerRef (db_Column::*getter)() const = &db_Column::precision;
      meta->bind_member("precision", new grt::MetaClass::Property<db_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const grt::IntegerRef &) = &db_Column::scale;
      grt::IntegerRef (db_Column::*getter)() const = &db_Column::scale;
      meta->bind_member("scale", new grt::MetaClass::Property<db_Column,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const db_SimpleDatatypeRef &) = &db_Column::simpleType;
      db_SimpleDatatypeRef (db_Column::*getter)() const = &db_Column::simpleType;
      meta->bind_member("simpleType", new grt::MetaClass::Property<db_Column,db_SimpleDatatypeRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const db_StructuredDatatypeRef &) = &db_Column::structuredType;
      db_StructuredDatatypeRef (db_Column::*getter)() const = &db_Column::structuredType;
      meta->bind_member("structuredType", new grt::MetaClass::Property<db_Column,db_StructuredDatatypeRef>(getter, setter));
    }
    {
      void (db_Column::*setter)(const db_UserDatatypeRef &) = &db_Column::userType;
      db_UserDatatypeRef (db_Column::*getter)() const = &db_Column::userType;
      meta->bind_member("userType", new grt::MetaClass::Property<db_Column,db_UserDatatypeRef>(getter, setter));
    }
    meta->bind_method("setParseType", &db_Column::call_setParseType);
  }
};

class  db_RolePrivilege : public GrtObject {
  typedef GrtObject super;

public:
  db_RolePrivilege(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _databaseObjectName(""),
      _databaseObjectType(""),
      _privileges(this, false) {
  }

  static std::string static_class_name() {
    return "db.RolePrivilege";
  }

  /**
   * Getter for attribute databaseObject
   *
   * the database object this privilege is assigned to
   * \par In Python:
   *    value = obj.databaseObject
   */
  db_DatabaseObjectRef databaseObject() const { return _databaseObject; }

  /**
   * Setter for attribute databaseObject
   * 
   * the database object this privilege is assigned to
   * \par In Python:
   *   obj.databaseObject = value
   */
  virtual void databaseObject(const db_DatabaseObjectRef &value) {
    grt::ValueRef ovalue(_databaseObject);
    _databaseObject = value;
    member_changed("databaseObject", ovalue, value);
  }

  /**
   * Getter for attribute databaseObjectName
   *
   * used when wildcards are needed, like test.*
   * \par In Python:
   *    value = obj.databaseObjectName
   */
  grt::StringRef databaseObjectName() const { return _databaseObjectName; }

  /**
   * Setter for attribute databaseObjectName
   * 
   * used when wildcards are needed, like test.*
   * \par In Python:
   *   obj.databaseObjectName = value
   */
  virtual void databaseObjectName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_databaseObjectName);
    _databaseObjectName = value;
    member_changed("databaseObjectName", ovalue, value);
  }

  /**
   * Getter for attribute databaseObjectType
   *
   * specifies the type, e.g. TABLE, used when wildcards are needed, like test.*
   * \par In Python:
   *    value = obj.databaseObjectType
   */
  grt::StringRef databaseObjectType() const { return _databaseObjectType; }

  /**
   * Setter for attribute databaseObjectType
   * 
   * specifies the type, e.g. TABLE, used when wildcards are needed, like test.*
   * \par In Python:
   *   obj.databaseObjectType = value
   */
  virtual void databaseObjectType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_databaseObjectType);
    _databaseObjectType = value;
    member_changed("databaseObjectType", ovalue, value);
  }

  /**
   * Getter for attribute privileges (read-only)
   *
   * the privileges for the object, e.g. CREATE
   * \par In Python:
   *    value = obj.privileges
   */
  grt::StringListRef privileges() const { return _privileges; }


private: // The next attribute is read-only.
  virtual void privileges(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_privileges);
    _privileges = value;
    member_changed("privileges", ovalue, value);
  }
public:

protected:

  db_DatabaseObjectRef _databaseObject;
  grt::StringRef _databaseObjectName;
  grt::StringRef _databaseObjectType;
  grt::StringListRef _privileges;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_RolePrivilege());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_RolePrivilege::create);
    {
      void (db_RolePrivilege::*setter)(const db_DatabaseObjectRef &) = &db_RolePrivilege::databaseObject;
      db_DatabaseObjectRef (db_RolePrivilege::*getter)() const = &db_RolePrivilege::databaseObject;
      meta->bind_member("databaseObject", new grt::MetaClass::Property<db_RolePrivilege,db_DatabaseObjectRef>(getter, setter));
    }
    {
      void (db_RolePrivilege::*setter)(const grt::StringRef &) = &db_RolePrivilege::databaseObjectName;
      grt::StringRef (db_RolePrivilege::*getter)() const = &db_RolePrivilege::databaseObjectName;
      meta->bind_member("databaseObjectName", new grt::MetaClass::Property<db_RolePrivilege,grt::StringRef>(getter, setter));
    }
    {
      void (db_RolePrivilege::*setter)(const grt::StringRef &) = &db_RolePrivilege::databaseObjectType;
      grt::StringRef (db_RolePrivilege::*getter)() const = &db_RolePrivilege::databaseObjectType;
      meta->bind_member("databaseObjectType", new grt::MetaClass::Property<db_RolePrivilege,grt::StringRef>(getter, setter));
    }
    {
      void (db_RolePrivilege::*setter)(const grt::StringListRef &) = &db_RolePrivilege::privileges;
      grt::StringListRef (db_RolePrivilege::*getter)() const = &db_RolePrivilege::privileges;
      meta->bind_member("privileges", new grt::MetaClass::Property<db_RolePrivilege,grt::StringListRef>(getter, setter));
    }
  }
};

class  db_Catalog : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  db_Catalog(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _characterSets(this, false),
      _customData(this, false),
      _defaultCharacterSetName(""),
      _defaultCollationName(""),
      _logFileGroups(this, false),
      _roles(this, false),
      _schemata(this, false),
      _serverLinks(this, false),
      _simpleDatatypes(this, false),
      _tablespaces(this, false),
      _userDatatypes(this, false),
      _users(this, false) {
  }

  static std::string static_class_name() {
    return "db.Catalog";
  }

  /**
   * Getter for attribute characterSets (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.characterSets
   */
  grt::ListRef<db_CharacterSet> characterSets() const { return _characterSets; }


private: // The next attribute is read-only.
  virtual void characterSets(const grt::ListRef<db_CharacterSet> &value) {
    grt::ValueRef ovalue(_characterSets);
    _characterSets = value;
    member_changed("characterSets", ovalue, value);
  }
public:

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
   * Getter for attribute defaultSchema
   *
   * currently selected schema
   * \par In Python:
   *    value = obj.defaultSchema
   */
  db_SchemaRef defaultSchema() const { return _defaultSchema; }

  /**
   * Setter for attribute defaultSchema
   * 
   * currently selected schema
   * \par In Python:
   *   obj.defaultSchema = value
   */
  virtual void defaultSchema(const db_SchemaRef &value) {
    grt::ValueRef ovalue(_defaultSchema);
    _defaultSchema = value;
    member_changed("defaultSchema", ovalue, value);
  }

  // logFileGroups is owned by db_Catalog
  /**
   * Getter for attribute logFileGroups (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.logFileGroups
   */
  grt::ListRef<db_LogFileGroup> logFileGroups() const { return _logFileGroups; }


private: // The next attribute is read-only.
  virtual void logFileGroups(const grt::ListRef<db_LogFileGroup> &value) {
    grt::ValueRef ovalue(_logFileGroups);

    _logFileGroups = value;
    owned_member_changed("logFileGroups", ovalue, value);
  }
public:

  // roles is owned by db_Catalog
  /**
   * Getter for attribute roles (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.roles
   */
  grt::ListRef<db_Role> roles() const { return _roles; }


private: // The next attribute is read-only.
  virtual void roles(const grt::ListRef<db_Role> &value) {
    grt::ValueRef ovalue(_roles);

    _roles = value;
    owned_member_changed("roles", ovalue, value);
  }
public:

  // schemata is owned by db_Catalog
  /**
   * Getter for attribute schemata (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.schemata
   */
  grt::ListRef<db_Schema> schemata() const { return _schemata; }


private: // The next attribute is read-only.
  virtual void schemata(const grt::ListRef<db_Schema> &value) {
    grt::ValueRef ovalue(_schemata);

    _schemata = value;
    owned_member_changed("schemata", ovalue, value);
  }
public:

  // serverLinks is owned by db_Catalog
  /**
   * Getter for attribute serverLinks (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.serverLinks
   */
  grt::ListRef<db_ServerLink> serverLinks() const { return _serverLinks; }


private: // The next attribute is read-only.
  virtual void serverLinks(const grt::ListRef<db_ServerLink> &value) {
    grt::ValueRef ovalue(_serverLinks);

    _serverLinks = value;
    owned_member_changed("serverLinks", ovalue, value);
  }
public:

  /**
   * Getter for attribute simpleDatatypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.simpleDatatypes
   */
  grt::ListRef<db_SimpleDatatype> simpleDatatypes() const { return _simpleDatatypes; }


private: // The next attribute is read-only.
  virtual void simpleDatatypes(const grt::ListRef<db_SimpleDatatype> &value) {
    grt::ValueRef ovalue(_simpleDatatypes);
    _simpleDatatypes = value;
    member_changed("simpleDatatypes", ovalue, value);
  }
public:

  // tablespaces is owned by db_Catalog
  /**
   * Getter for attribute tablespaces (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.tablespaces
   */
  grt::ListRef<db_Tablespace> tablespaces() const { return _tablespaces; }


private: // The next attribute is read-only.
  virtual void tablespaces(const grt::ListRef<db_Tablespace> &value) {
    grt::ValueRef ovalue(_tablespaces);

    _tablespaces = value;
    owned_member_changed("tablespaces", ovalue, value);
  }
public:

  // userDatatypes is owned by db_Catalog
  /**
   * Getter for attribute userDatatypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.userDatatypes
   */
  grt::ListRef<db_UserDatatype> userDatatypes() const { return _userDatatypes; }


private: // The next attribute is read-only.
  virtual void userDatatypes(const grt::ListRef<db_UserDatatype> &value) {
    grt::ValueRef ovalue(_userDatatypes);

    _userDatatypes = value;
    owned_member_changed("userDatatypes", ovalue, value);
  }
public:

  // users is owned by db_Catalog
  /**
   * Getter for attribute users (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.users
   */
  grt::ListRef<db_User> users() const { return _users; }


private: // The next attribute is read-only.
  virtual void users(const grt::ListRef<db_User> &value) {
    grt::ValueRef ovalue(_users);

    _users = value;
    owned_member_changed("users", ovalue, value);
  }
public:

  // version is owned by db_Catalog
  /**
   * Getter for attribute version
   *
   * version of the catalog's database
   * \par In Python:
   *    value = obj.version
   */
  GrtVersionRef version() const { return _version; }

  /**
   * Setter for attribute version
   * 
   * version of the catalog's database
   * \par In Python:
   *   obj.version = value
   */
  virtual void version(const GrtVersionRef &value) {
    grt::ValueRef ovalue(_version);

    _version = value;
    owned_member_changed("version", ovalue, value);
  }

protected:

  grt::ListRef<db_CharacterSet> _characterSets;
  grt::DictRef _customData;
  grt::StringRef _defaultCharacterSetName;
  grt::StringRef _defaultCollationName;
  db_SchemaRef _defaultSchema;
  grt::ListRef<db_LogFileGroup> _logFileGroups;// owned
  grt::ListRef<db_Role> _roles;// owned
  grt::ListRef<db_Schema> _schemata;// owned
  grt::ListRef<db_ServerLink> _serverLinks;// owned
  grt::ListRef<db_SimpleDatatype> _simpleDatatypes;
  grt::ListRef<db_Tablespace> _tablespaces;// owned
  grt::ListRef<db_UserDatatype> _userDatatypes;// owned
  grt::ListRef<db_User> _users;// owned
  GrtVersionRef _version;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Catalog());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Catalog::create);
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_CharacterSet> &) = &db_Catalog::characterSets;
      grt::ListRef<db_CharacterSet> (db_Catalog::*getter)() const = &db_Catalog::characterSets;
      meta->bind_member("characterSets", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_CharacterSet>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::DictRef &) = &db_Catalog::customData;
      grt::DictRef (db_Catalog::*getter)() const = &db_Catalog::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<db_Catalog,grt::DictRef>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::StringRef &) = &db_Catalog::defaultCharacterSetName;
      grt::StringRef (db_Catalog::*getter)() const = &db_Catalog::defaultCharacterSetName;
      meta->bind_member("defaultCharacterSetName", new grt::MetaClass::Property<db_Catalog,grt::StringRef>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::StringRef &) = &db_Catalog::defaultCollationName;
      grt::StringRef (db_Catalog::*getter)() const = &db_Catalog::defaultCollationName;
      meta->bind_member("defaultCollationName", new grt::MetaClass::Property<db_Catalog,grt::StringRef>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const db_SchemaRef &) = &db_Catalog::defaultSchema;
      db_SchemaRef (db_Catalog::*getter)() const = &db_Catalog::defaultSchema;
      meta->bind_member("defaultSchema", new grt::MetaClass::Property<db_Catalog,db_SchemaRef>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_LogFileGroup> &) = &db_Catalog::logFileGroups;
      grt::ListRef<db_LogFileGroup> (db_Catalog::*getter)() const = &db_Catalog::logFileGroups;
      meta->bind_member("logFileGroups", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_LogFileGroup>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_Role> &) = &db_Catalog::roles;
      grt::ListRef<db_Role> (db_Catalog::*getter)() const = &db_Catalog::roles;
      meta->bind_member("roles", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_Role>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_Schema> &) = &db_Catalog::schemata;
      grt::ListRef<db_Schema> (db_Catalog::*getter)() const = &db_Catalog::schemata;
      meta->bind_member("schemata", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_Schema>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_ServerLink> &) = &db_Catalog::serverLinks;
      grt::ListRef<db_ServerLink> (db_Catalog::*getter)() const = &db_Catalog::serverLinks;
      meta->bind_member("serverLinks", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_ServerLink>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_SimpleDatatype> &) = &db_Catalog::simpleDatatypes;
      grt::ListRef<db_SimpleDatatype> (db_Catalog::*getter)() const = &db_Catalog::simpleDatatypes;
      meta->bind_member("simpleDatatypes", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_SimpleDatatype>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_Tablespace> &) = &db_Catalog::tablespaces;
      grt::ListRef<db_Tablespace> (db_Catalog::*getter)() const = &db_Catalog::tablespaces;
      meta->bind_member("tablespaces", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_Tablespace>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_UserDatatype> &) = &db_Catalog::userDatatypes;
      grt::ListRef<db_UserDatatype> (db_Catalog::*getter)() const = &db_Catalog::userDatatypes;
      meta->bind_member("userDatatypes", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_UserDatatype>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const grt::ListRef<db_User> &) = &db_Catalog::users;
      grt::ListRef<db_User> (db_Catalog::*getter)() const = &db_Catalog::users;
      meta->bind_member("users", new grt::MetaClass::Property<db_Catalog,grt::ListRef<db_User>>(getter, setter));
    }
    {
      void (db_Catalog::*setter)(const GrtVersionRef &) = &db_Catalog::version;
      GrtVersionRef (db_Catalog::*getter)() const = &db_Catalog::version;
      meta->bind_member("version", new grt::MetaClass::Property<db_Catalog,GrtVersionRef>(getter, setter));
    }
  }
};

class GRT_STRUCTS_DB_PUBLIC db_DatabaseObject : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  db_DatabaseObject(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _commentedOut(0),
      _createDate(""),
      _customData(this, false),
      _lastChangeDate(""),
      _modelOnly(0),
      _temp_sql("") {
  }

  virtual ~db_DatabaseObject();

  static std::string static_class_name() {
    return "db.DatabaseObject";
  }

  /**
   * Getter for attribute commentedOut
   *
   * if set to 1 the sql will be commented out but e.g. still be written to the script
   * \par In Python:
   *    value = obj.commentedOut
   */
  grt::IntegerRef commentedOut() const { return _commentedOut; }

  /**
   * Setter for attribute commentedOut
   * 
   * if set to 1 the sql will be commented out but e.g. still be written to the script
   * \par In Python:
   *   obj.commentedOut = value
   */
  virtual void commentedOut(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_commentedOut);
    _commentedOut = value;
    member_changed("commentedOut", ovalue, value);
  }

  /**
   * Getter for attribute createDate
   *
   * 
   * \par In Python:
   *    value = obj.createDate
   */
  grt::StringRef createDate() const { return _createDate; }

  /**
   * Setter for attribute createDate
   * 
   * 
   * \par In Python:
   *   obj.createDate = value
   */
  virtual void createDate(const grt::StringRef &value) {
    grt::ValueRef ovalue(_createDate);
    _createDate = value;
    member_changed("createDate", ovalue, value);
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

  /**
   * Getter for attribute lastChangeDate
   *
   * 
   * \par In Python:
   *    value = obj.lastChangeDate
   */
  grt::StringRef lastChangeDate() const { return _lastChangeDate; }

  /**
   * Setter for attribute lastChangeDate
   * 
   * 
   * \par In Python:
   *   obj.lastChangeDate = value
   */
  virtual void lastChangeDate(const grt::StringRef &value);

  /**
   * Getter for attribute modelOnly
   *
   * object exists in model but is not to be written to the database
   * \par In Python:
   *    value = obj.modelOnly
   */
  grt::IntegerRef modelOnly() const { return _modelOnly; }

  /**
   * Setter for attribute modelOnly
   * 
   * object exists in model but is not to be written to the database
   * \par In Python:
   *   obj.modelOnly = value
   */
  virtual void modelOnly(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_modelOnly);
    _modelOnly = value;
    member_changed("modelOnly", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * 
   * \par In Python:
   *    value = obj.name
   */
  grt::StringRef name() const { return super::name(); }

  /**
   * Setter for attribute name
   * 
   * 
   * \par In Python:
   *   obj.name = value
   */
  virtual void name(const grt::StringRef &value);

  /**
   * Getter for attribute owner
   *
   * 
   * \par In Python:
   *    value = obj.owner
   */
  GrtNamedObjectRef owner() const { return GrtNamedObjectRef::cast_from(_owner); }

  /**
   * Setter for attribute owner
   * 
   * 
   * \par In Python:
   *   obj.owner = value
   */
  virtual void owner(const GrtNamedObjectRef &value);

  /**
   * Getter for attribute temp_sql
   *
   * the generated SQL statement(s)
   * \par In Python:
   *    value = obj.temp_sql
   */
  grt::StringRef temp_sql() const { return _temp_sql; }

  /**
   * Setter for attribute temp_sql
   * 
   * the generated SQL statement(s)
   * \par In Python:
   *   obj.temp_sql = value
   */
  virtual void temp_sql(const grt::StringRef &value) {
    grt::ValueRef ovalue(_temp_sql);
    _temp_sql = value;
    member_changed("temp_sql", ovalue, value);
  }

  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::IntegerRef _commentedOut;
  grt::StringRef _createDate;
  grt::DictRef _customData;
  grt::StringRef _lastChangeDate;
  grt::IntegerRef _modelOnly;
  grt::StringRef _temp_sql;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_DatabaseObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_DatabaseObject::create);
    {
      void (db_DatabaseObject::*setter)(const grt::IntegerRef &) = &db_DatabaseObject::commentedOut;
      grt::IntegerRef (db_DatabaseObject::*getter)() const = &db_DatabaseObject::commentedOut;
      meta->bind_member("commentedOut", new grt::MetaClass::Property<db_DatabaseObject,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_DatabaseObject::*setter)(const grt::StringRef &) = &db_DatabaseObject::createDate;
      grt::StringRef (db_DatabaseObject::*getter)() const = &db_DatabaseObject::createDate;
      meta->bind_member("createDate", new grt::MetaClass::Property<db_DatabaseObject,grt::StringRef>(getter, setter));
    }
    {
      void (db_DatabaseObject::*setter)(const grt::DictRef &) = &db_DatabaseObject::customData;
      grt::DictRef (db_DatabaseObject::*getter)() const = &db_DatabaseObject::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<db_DatabaseObject,grt::DictRef>(getter, setter));
    }
    {
      void (db_DatabaseObject::*setter)(const grt::StringRef &) = &db_DatabaseObject::lastChangeDate;
      grt::StringRef (db_DatabaseObject::*getter)() const = &db_DatabaseObject::lastChangeDate;
      meta->bind_member("lastChangeDate", new grt::MetaClass::Property<db_DatabaseObject,grt::StringRef>(getter, setter));
    }
    {
      void (db_DatabaseObject::*setter)(const grt::IntegerRef &) = &db_DatabaseObject::modelOnly;
      grt::IntegerRef (db_DatabaseObject::*getter)() const = &db_DatabaseObject::modelOnly;
      meta->bind_member("modelOnly", new grt::MetaClass::Property<db_DatabaseObject,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_DatabaseObject::*setter)(const grt::StringRef &) = &db_DatabaseObject::name;
      grt::StringRef (db_DatabaseObject::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<db_DatabaseObject,grt::StringRef>(getter, setter));
    }
    {
      void (db_DatabaseObject::*setter)(const GrtNamedObjectRef &) = &db_DatabaseObject::owner;
      GrtNamedObjectRef (db_DatabaseObject::*getter)() const = 0;
      meta->bind_member("owner", new grt::MetaClass::Property<db_DatabaseObject,GrtNamedObjectRef>(getter, setter));
    }
    {
      void (db_DatabaseObject::*setter)(const grt::StringRef &) = &db_DatabaseObject::temp_sql;
      grt::StringRef (db_DatabaseObject::*getter)() const = &db_DatabaseObject::temp_sql;
      meta->bind_member("temp_sql", new grt::MetaClass::Property<db_DatabaseObject,grt::StringRef>(getter, setter));
    }
  }
};

/** a database sequence object */
class  db_Sequence : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_Sequence(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _cacheSize(""),
      _cycleFlag(0),
      _incrementBy(""),
      _lastNumber(""),
      _maxValue(""),
      _minValue(""),
      _orderFlag(0),
      _startValue("") {
  }

  static std::string static_class_name() {
    return "db.Sequence";
  }

  /**
   * Getter for attribute cacheSize
   *
   * Number of sequence values that are loaded into cache simultaneously
   * \par In Python:
   *    value = obj.cacheSize
   */
  grt::StringRef cacheSize() const { return _cacheSize; }

  /**
   * Setter for attribute cacheSize
   * 
   * Number of sequence values that are loaded into cache simultaneously
   * \par In Python:
   *   obj.cacheSize = value
   */
  virtual void cacheSize(const grt::StringRef &value) {
    grt::ValueRef ovalue(_cacheSize);
    _cacheSize = value;
    member_changed("cacheSize", ovalue, value);
  }

  /**
   * Getter for attribute cycleFlag
   *
   * Does sequence begin again with minimum value once maximum value has been reached?
   * \par In Python:
   *    value = obj.cycleFlag
   */
  grt::IntegerRef cycleFlag() const { return _cycleFlag; }

  /**
   * Setter for attribute cycleFlag
   * 
   * Does sequence begin again with minimum value once maximum value has been reached?
   * \par In Python:
   *   obj.cycleFlag = value
   */
  virtual void cycleFlag(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_cycleFlag);
    _cycleFlag = value;
    member_changed("cycleFlag", ovalue, value);
  }

  /**
   * Getter for attribute incrementBy
   *
   * Value by which sequence is increased
   * \par In Python:
   *    value = obj.incrementBy
   */
  grt::StringRef incrementBy() const { return _incrementBy; }

  /**
   * Setter for attribute incrementBy
   * 
   * Value by which sequence is increased
   * \par In Python:
   *   obj.incrementBy = value
   */
  virtual void incrementBy(const grt::StringRef &value) {
    grt::ValueRef ovalue(_incrementBy);
    _incrementBy = value;
    member_changed("incrementBy", ovalue, value);
  }

  /**
   * Getter for attribute lastNumber
   *
   * Last sequence value that was saved
   * \par In Python:
   *    value = obj.lastNumber
   */
  grt::StringRef lastNumber() const { return _lastNumber; }

  /**
   * Setter for attribute lastNumber
   * 
   * Last sequence value that was saved
   * \par In Python:
   *   obj.lastNumber = value
   */
  virtual void lastNumber(const grt::StringRef &value) {
    grt::ValueRef ovalue(_lastNumber);
    _lastNumber = value;
    member_changed("lastNumber", ovalue, value);
  }

  /**
   * Getter for attribute maxValue
   *
   * Maximum value of sequence
   * \par In Python:
   *    value = obj.maxValue
   */
  grt::StringRef maxValue() const { return _maxValue; }

  /**
   * Setter for attribute maxValue
   * 
   * Maximum value of sequence
   * \par In Python:
   *   obj.maxValue = value
   */
  virtual void maxValue(const grt::StringRef &value) {
    grt::ValueRef ovalue(_maxValue);
    _maxValue = value;
    member_changed("maxValue", ovalue, value);
  }

  /**
   * Getter for attribute minValue
   *
   * Minimum value of sequence
   * \par In Python:
   *    value = obj.minValue
   */
  grt::StringRef minValue() const { return _minValue; }

  /**
   * Setter for attribute minValue
   * 
   * Minimum value of sequence
   * \par In Python:
   *   obj.minValue = value
   */
  virtual void minValue(const grt::StringRef &value) {
    grt::ValueRef ovalue(_minValue);
    _minValue = value;
    member_changed("minValue", ovalue, value);
  }

  /**
   * Getter for attribute orderFlag
   *
   * Are the sequence values granted in the order of the request?
   * \par In Python:
   *    value = obj.orderFlag
   */
  grt::IntegerRef orderFlag() const { return _orderFlag; }

  /**
   * Setter for attribute orderFlag
   * 
   * Are the sequence values granted in the order of the request?
   * \par In Python:
   *   obj.orderFlag = value
   */
  virtual void orderFlag(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_orderFlag);
    _orderFlag = value;
    member_changed("orderFlag", ovalue, value);
  }

  /**
   * Getter for attribute startValue
   *
   * The value that starts the sequence
   * \par In Python:
   *    value = obj.startValue
   */
  grt::StringRef startValue() const { return _startValue; }

  /**
   * Setter for attribute startValue
   * 
   * The value that starts the sequence
   * \par In Python:
   *   obj.startValue = value
   */
  virtual void startValue(const grt::StringRef &value) {
    grt::ValueRef ovalue(_startValue);
    _startValue = value;
    member_changed("startValue", ovalue, value);
  }

protected:

  grt::StringRef _cacheSize;
  grt::IntegerRef _cycleFlag;
  grt::StringRef _incrementBy;
  grt::StringRef _lastNumber;
  grt::StringRef _maxValue;
  grt::StringRef _minValue;
  grt::IntegerRef _orderFlag;
  grt::StringRef _startValue;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Sequence());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Sequence::create);
    {
      void (db_Sequence::*setter)(const grt::StringRef &) = &db_Sequence::cacheSize;
      grt::StringRef (db_Sequence::*getter)() const = &db_Sequence::cacheSize;
      meta->bind_member("cacheSize", new grt::MetaClass::Property<db_Sequence,grt::StringRef>(getter, setter));
    }
    {
      void (db_Sequence::*setter)(const grt::IntegerRef &) = &db_Sequence::cycleFlag;
      grt::IntegerRef (db_Sequence::*getter)() const = &db_Sequence::cycleFlag;
      meta->bind_member("cycleFlag", new grt::MetaClass::Property<db_Sequence,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Sequence::*setter)(const grt::StringRef &) = &db_Sequence::incrementBy;
      grt::StringRef (db_Sequence::*getter)() const = &db_Sequence::incrementBy;
      meta->bind_member("incrementBy", new grt::MetaClass::Property<db_Sequence,grt::StringRef>(getter, setter));
    }
    {
      void (db_Sequence::*setter)(const grt::StringRef &) = &db_Sequence::lastNumber;
      grt::StringRef (db_Sequence::*getter)() const = &db_Sequence::lastNumber;
      meta->bind_member("lastNumber", new grt::MetaClass::Property<db_Sequence,grt::StringRef>(getter, setter));
    }
    {
      void (db_Sequence::*setter)(const grt::StringRef &) = &db_Sequence::maxValue;
      grt::StringRef (db_Sequence::*getter)() const = &db_Sequence::maxValue;
      meta->bind_member("maxValue", new grt::MetaClass::Property<db_Sequence,grt::StringRef>(getter, setter));
    }
    {
      void (db_Sequence::*setter)(const grt::StringRef &) = &db_Sequence::minValue;
      grt::StringRef (db_Sequence::*getter)() const = &db_Sequence::minValue;
      meta->bind_member("minValue", new grt::MetaClass::Property<db_Sequence,grt::StringRef>(getter, setter));
    }
    {
      void (db_Sequence::*setter)(const grt::IntegerRef &) = &db_Sequence::orderFlag;
      grt::IntegerRef (db_Sequence::*getter)() const = &db_Sequence::orderFlag;
      meta->bind_member("orderFlag", new grt::MetaClass::Property<db_Sequence,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Sequence::*setter)(const grt::StringRef &) = &db_Sequence::startValue;
      grt::StringRef (db_Sequence::*getter)() const = &db_Sequence::startValue;
      meta->bind_member("startValue", new grt::MetaClass::Property<db_Sequence,grt::StringRef>(getter, setter));
    }
  }
};

class  db_Synonym : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_Synonym(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _isPublic(0),
      _referencedObjectName(""),
      _referencedSchemaName("") {
  }

  static std::string static_class_name() {
    return "db.Synonym";
  }

  /**
   * Getter for attribute isPublic
   *
   * 
   * \par In Python:
   *    value = obj.isPublic
   */
  grt::IntegerRef isPublic() const { return _isPublic; }

  /**
   * Setter for attribute isPublic
   * 
   * 
   * \par In Python:
   *   obj.isPublic = value
   */
  virtual void isPublic(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isPublic);
    _isPublic = value;
    member_changed("isPublic", ovalue, value);
  }

  /**
   * Getter for attribute referencedObject
   *
   * 
   * \par In Python:
   *    value = obj.referencedObject
   */
  db_DatabaseObjectRef referencedObject() const { return _referencedObject; }

  /**
   * Setter for attribute referencedObject
   * 
   * 
   * \par In Python:
   *   obj.referencedObject = value
   */
  virtual void referencedObject(const db_DatabaseObjectRef &value) {
    grt::ValueRef ovalue(_referencedObject);
    _referencedObject = value;
    member_changed("referencedObject", ovalue, value);
  }

  /**
   * Getter for attribute referencedObjectName
   *
   * 
   * \par In Python:
   *    value = obj.referencedObjectName
   */
  grt::StringRef referencedObjectName() const { return _referencedObjectName; }

  /**
   * Setter for attribute referencedObjectName
   * 
   * 
   * \par In Python:
   *   obj.referencedObjectName = value
   */
  virtual void referencedObjectName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_referencedObjectName);
    _referencedObjectName = value;
    member_changed("referencedObjectName", ovalue, value);
  }

  /**
   * Getter for attribute referencedSchemaName
   *
   * 
   * \par In Python:
   *    value = obj.referencedSchemaName
   */
  grt::StringRef referencedSchemaName() const { return _referencedSchemaName; }

  /**
   * Setter for attribute referencedSchemaName
   * 
   * 
   * \par In Python:
   *   obj.referencedSchemaName = value
   */
  virtual void referencedSchemaName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_referencedSchemaName);
    _referencedSchemaName = value;
    member_changed("referencedSchemaName", ovalue, value);
  }

protected:

  grt::IntegerRef _isPublic;
  db_DatabaseObjectRef _referencedObject;
  grt::StringRef _referencedObjectName;
  grt::StringRef _referencedSchemaName;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Synonym());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Synonym::create);
    {
      void (db_Synonym::*setter)(const grt::IntegerRef &) = &db_Synonym::isPublic;
      grt::IntegerRef (db_Synonym::*getter)() const = &db_Synonym::isPublic;
      meta->bind_member("isPublic", new grt::MetaClass::Property<db_Synonym,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Synonym::*setter)(const db_DatabaseObjectRef &) = &db_Synonym::referencedObject;
      db_DatabaseObjectRef (db_Synonym::*getter)() const = &db_Synonym::referencedObject;
      meta->bind_member("referencedObject", new grt::MetaClass::Property<db_Synonym,db_DatabaseObjectRef>(getter, setter));
    }
    {
      void (db_Synonym::*setter)(const grt::StringRef &) = &db_Synonym::referencedObjectName;
      grt::StringRef (db_Synonym::*getter)() const = &db_Synonym::referencedObjectName;
      meta->bind_member("referencedObjectName", new grt::MetaClass::Property<db_Synonym,grt::StringRef>(getter, setter));
    }
    {
      void (db_Synonym::*setter)(const grt::StringRef &) = &db_Synonym::referencedSchemaName;
      grt::StringRef (db_Synonym::*getter)() const = &db_Synonym::referencedSchemaName;
      meta->bind_member("referencedSchemaName", new grt::MetaClass::Property<db_Synonym,grt::StringRef>(getter, setter));
    }
  }
};

/** a logical group of routines */
class GRT_STRUCTS_DB_PUBLIC db_RoutineGroup : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_RoutineGroup(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _routineExpandedHeights(this, false),
      _routineExpandedStates(this, false),
      _routines(this, false) {
  }

  virtual ~db_RoutineGroup();

  static std::string static_class_name() {
    return "db.RoutineGroup";
  }

  // args: 
  boost::signals2::signal<void ()>* signal_contentChanged() { return &_signal_contentChanged; }
  /**
   * Getter for attribute routineExpandedHeights (read-only)
   *
   * specifies the n-th routine height in the editor, 0 for automatic height
   * \par In Python:
   *    value = obj.routineExpandedHeights
   */
  grt::IntegerListRef routineExpandedHeights() const { return _routineExpandedHeights; }


private: // The next attribute is read-only.
  virtual void routineExpandedHeights(const grt::IntegerListRef &value) {
    grt::ValueRef ovalue(_routineExpandedHeights);
    _routineExpandedHeights = value;
    member_changed("routineExpandedHeights", ovalue, value);
  }
public:

  /**
   * Getter for attribute routineExpandedStates (read-only)
   *
   * specifies if the n-th routine is expanded in the editor, 0 if collapsed
   * \par In Python:
   *    value = obj.routineExpandedStates
   */
  grt::IntegerListRef routineExpandedStates() const { return _routineExpandedStates; }


private: // The next attribute is read-only.
  virtual void routineExpandedStates(const grt::IntegerListRef &value) {
    grt::ValueRef ovalue(_routineExpandedStates);
    _routineExpandedStates = value;
    member_changed("routineExpandedStates", ovalue, value);
  }
public:

  // routines is owned by db_RoutineGroup
  /**
   * Getter for attribute routines (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.routines
   */
  grt::ListRef<db_Routine> routines() const { return _routines; }


private: // The next attribute is read-only.
  virtual void routines(const grt::ListRef<db_Routine> &value) {
    grt::ValueRef ovalue(_routines);

    _routines = value;
    owned_member_changed("routines", ovalue, value);
  }
public:

  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:
  boost::signals2::signal<void ()> _signal_contentChanged;

  grt::IntegerListRef _routineExpandedHeights;
  grt::IntegerListRef _routineExpandedStates;
  grt::ListRef<db_Routine> _routines;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_RoutineGroup());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_RoutineGroup::create);
    {
      void (db_RoutineGroup::*setter)(const grt::IntegerListRef &) = &db_RoutineGroup::routineExpandedHeights;
      grt::IntegerListRef (db_RoutineGroup::*getter)() const = &db_RoutineGroup::routineExpandedHeights;
      meta->bind_member("routineExpandedHeights", new grt::MetaClass::Property<db_RoutineGroup,grt::IntegerListRef>(getter, setter));
    }
    {
      void (db_RoutineGroup::*setter)(const grt::IntegerListRef &) = &db_RoutineGroup::routineExpandedStates;
      grt::IntegerListRef (db_RoutineGroup::*getter)() const = &db_RoutineGroup::routineExpandedStates;
      meta->bind_member("routineExpandedStates", new grt::MetaClass::Property<db_RoutineGroup,grt::IntegerListRef>(getter, setter));
    }
    {
      void (db_RoutineGroup::*setter)(const grt::ListRef<db_Routine> &) = &db_RoutineGroup::routines;
      grt::ListRef<db_Routine> (db_RoutineGroup::*getter)() const = &db_RoutineGroup::routines;
      meta->bind_member("routines", new grt::MetaClass::Property<db_RoutineGroup,grt::ListRef<db_Routine>>(getter, setter));
    }
  }
};

class GRT_STRUCTS_DB_PUBLIC db_Index : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_Index(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _columns(this, false),
      _deferability(0),
      _indexType(""),
      _isPrimary(0),
      _unique(0) {
  }

  virtual ~db_Index();

  static std::string static_class_name() {
    return "db.Index";
  }

  // columns is owned by db_Index
  /**
   * Getter for attribute columns (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.columns
   */
  grt::ListRef<db_IndexColumn> columns() const { return _columns; }


private: // The next attribute is read-only.
  virtual void columns(const grt::ListRef<db_IndexColumn> &value) {
    grt::ValueRef ovalue(_columns);

    _columns = value;
    owned_member_changed("columns", ovalue, value);
  }
public:

  /**
   * Getter for attribute comment
   *
   * 
   * \par In Python:
   *    value = obj.comment
   */

  /**
   * Setter for attribute comment
   * 
   * 
   * \par In Python:
   *   obj.comment = value
   */

  /**
   * Getter for attribute deferability
   *
   * 
   * \par In Python:
   *    value = obj.deferability
   */
  grt::IntegerRef deferability() const { return _deferability; }

  /**
   * Setter for attribute deferability
   * 
   * 
   * \par In Python:
   *   obj.deferability = value
   */
  virtual void deferability(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_deferability);
    _deferability = value;
    member_changed("deferability", ovalue, value);
  }

  /**
   * Getter for attribute indexType
   *
   * one of INDEX, PRIMARY, UNIQUE, FULLTEXT and SPATIAL
   * \par In Python:
   *    value = obj.indexType
   */
  grt::StringRef indexType() const { return _indexType; }

  /**
   * Setter for attribute indexType
   * 
   * one of INDEX, PRIMARY, UNIQUE, FULLTEXT and SPATIAL
   * \par In Python:
   *   obj.indexType = value
   */
  virtual void indexType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_indexType);
    _indexType = value;
    member_changed("indexType", ovalue, value);
  }

  /**
   * Getter for attribute isPrimary
   *
   * 
   * \par In Python:
   *    value = obj.isPrimary
   */
  grt::IntegerRef isPrimary() const { return _isPrimary; }

  /**
   * Setter for attribute isPrimary
   * 
   * 
   * \par In Python:
   *   obj.isPrimary = value
   */
  virtual void isPrimary(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isPrimary);
    _isPrimary = value;
    member_changed("isPrimary", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * 
   * \par In Python:
   *    value = obj.name
   */
  grt::StringRef name() const { return super::name(); }

  /**
   * Setter for attribute name
   * 
   * 
   * \par In Python:
   *   obj.name = value
   */
  virtual void name(const grt::StringRef &value);

  /**
   * Getter for attribute unique
   *
   * 
   * \par In Python:
   *    value = obj.unique
   */
  grt::IntegerRef unique() const { return _unique; }

  /**
   * Setter for attribute unique
   * 
   * 
   * \par In Python:
   *   obj.unique = value
   */
  virtual void unique(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_unique);
    _unique = value;
    member_changed("unique", ovalue, value);
  }

  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::ListRef<db_IndexColumn> _columns;// owned
  grt::IntegerRef _deferability;
  grt::StringRef _indexType;
  grt::IntegerRef _isPrimary;
  grt::IntegerRef _unique;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Index());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Index::create);
    {
      void (db_Index::*setter)(const grt::ListRef<db_IndexColumn> &) = &db_Index::columns;
      grt::ListRef<db_IndexColumn> (db_Index::*getter)() const = &db_Index::columns;
      meta->bind_member("columns", new grt::MetaClass::Property<db_Index,grt::ListRef<db_IndexColumn>>(getter, setter));
    }
    {
      void (db_Index::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_Index::*getter)() const = 0;
      meta->bind_member("comment", new grt::MetaClass::Property<db_Index,grt::StringRef>(getter, setter));
    }
    {
      void (db_Index::*setter)(const grt::IntegerRef &) = &db_Index::deferability;
      grt::IntegerRef (db_Index::*getter)() const = &db_Index::deferability;
      meta->bind_member("deferability", new grt::MetaClass::Property<db_Index,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Index::*setter)(const grt::StringRef &) = &db_Index::indexType;
      grt::StringRef (db_Index::*getter)() const = &db_Index::indexType;
      meta->bind_member("indexType", new grt::MetaClass::Property<db_Index,grt::StringRef>(getter, setter));
    }
    {
      void (db_Index::*setter)(const grt::IntegerRef &) = &db_Index::isPrimary;
      grt::IntegerRef (db_Index::*getter)() const = &db_Index::isPrimary;
      meta->bind_member("isPrimary", new grt::MetaClass::Property<db_Index,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Index::*setter)(const grt::StringRef &) = &db_Index::name;
      grt::StringRef (db_Index::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<db_Index,grt::StringRef>(getter, setter));
    }
    {
      void (db_Index::*setter)(const grt::IntegerRef &) = &db_Index::unique;
      grt::IntegerRef (db_Index::*getter)() const = &db_Index::unique;
      meta->bind_member("unique", new grt::MetaClass::Property<db_Index,grt::IntegerRef>(getter, setter));
    }
  }
};

class  db_StructuredDatatype : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_StructuredDatatype(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _distinctTypes(this, false) {
  }

  static std::string static_class_name() {
    return "db.StructuredDatatype";
  }

  // distinctTypes is owned by db_StructuredDatatype
  /**
   * Getter for attribute distinctTypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.distinctTypes
   */
  grt::ListRef<db_Column> distinctTypes() const { return _distinctTypes; }


private: // The next attribute is read-only.
  virtual void distinctTypes(const grt::ListRef<db_Column> &value) {
    grt::ValueRef ovalue(_distinctTypes);

    _distinctTypes = value;
    owned_member_changed("distinctTypes", ovalue, value);
  }
public:

  /**
   * Getter for attribute parentType
   *
   * 
   * \par In Python:
   *    value = obj.parentType
   */
  db_StructuredDatatypeRef parentType() const { return _parentType; }

  /**
   * Setter for attribute parentType
   * 
   * 
   * \par In Python:
   *   obj.parentType = value
   */
  virtual void parentType(const db_StructuredDatatypeRef &value) {
    grt::ValueRef ovalue(_parentType);
    _parentType = value;
    member_changed("parentType", ovalue, value);
  }

protected:

  grt::ListRef<db_Column> _distinctTypes;// owned
  db_StructuredDatatypeRef _parentType;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_StructuredDatatype());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_StructuredDatatype::create);
    {
      void (db_StructuredDatatype::*setter)(const grt::ListRef<db_Column> &) = &db_StructuredDatatype::distinctTypes;
      grt::ListRef<db_Column> (db_StructuredDatatype::*getter)() const = &db_StructuredDatatype::distinctTypes;
      meta->bind_member("distinctTypes", new grt::MetaClass::Property<db_StructuredDatatype,grt::ListRef<db_Column>>(getter, setter));
    }
    {
      void (db_StructuredDatatype::*setter)(const db_StructuredDatatypeRef &) = &db_StructuredDatatype::parentType;
      db_StructuredDatatypeRef (db_StructuredDatatype::*getter)() const = &db_StructuredDatatype::parentType;
      meta->bind_member("parentType", new grt::MetaClass::Property<db_StructuredDatatype,db_StructuredDatatypeRef>(getter, setter));
    }
  }
};

/** an object that stores information about a database schema table */
class GRT_STRUCTS_DB_PUBLIC db_Table : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_Table(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _columns(this, false),
      _foreignKeys(this, false),
      _indices(this, false),
      _isStub(0),
      _isSystem(0),
      _isTemporary(0),
      _temporaryScope(""),
      _triggers(this, false) {
  }

  virtual ~db_Table();

  static std::string static_class_name() {
    return "db.Table";
  }

  // args: 
  boost::signals2::signal<void (std::string)>* signal_refreshDisplay() { return &_signal_refreshDisplay; }
  // args: 
  boost::signals2::signal<void (db_ForeignKeyRef)>* signal_foreignKeyChanged() { return &_signal_foreignKeyChanged; }
  // columns is owned by db_Table
  /**
   * Getter for attribute columns (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.columns
   */
  grt::ListRef<db_Column> columns() const { return _columns; }


private: // The next attribute is read-only.
  virtual void columns(const grt::ListRef<db_Column> &value) {
    grt::ValueRef ovalue(_columns);

    _columns = value;
    owned_member_changed("columns", ovalue, value);
  }
public:

  // foreignKeys is owned by db_Table
  /**
   * Getter for attribute foreignKeys (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.foreignKeys
   */
  grt::ListRef<db_ForeignKey> foreignKeys() const { return _foreignKeys; }


private: // The next attribute is read-only.
  virtual void foreignKeys(const grt::ListRef<db_ForeignKey> &value) {
    grt::ValueRef ovalue(_foreignKeys);

    _foreignKeys = value;
    owned_member_changed("foreignKeys", ovalue, value);
  }
public:

  // indices is owned by db_Table
  /**
   * Getter for attribute indices (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.indices
   */
  grt::ListRef<db_Index> indices() const { return _indices; }


private: // The next attribute is read-only.
  virtual void indices(const grt::ListRef<db_Index> &value) {
    grt::ValueRef ovalue(_indices);

    _indices = value;
    owned_member_changed("indices", ovalue, value);
  }
public:

  /**
   * Getter for attribute isStub
   *
   * whether this table is a stub intended for foreign keys and triggers that refer to tables external to this model
   * \par In Python:
   *    value = obj.isStub
   */
  grt::IntegerRef isStub() const { return _isStub; }

  /**
   * Setter for attribute isStub
   * 
   * whether this table is a stub intended for foreign keys and triggers that refer to tables external to this model
   * \par In Python:
   *   obj.isStub = value
   */
  virtual void isStub(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isStub);
    _isStub = value;
    member_changed("isStub", ovalue, value);
  }

  /**
   * Getter for attribute isSystem
   *
   * 
   * \par In Python:
   *    value = obj.isSystem
   */
  grt::IntegerRef isSystem() const { return _isSystem; }

  /**
   * Setter for attribute isSystem
   * 
   * 
   * \par In Python:
   *   obj.isSystem = value
   */
  virtual void isSystem(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isSystem);
    _isSystem = value;
    member_changed("isSystem", ovalue, value);
  }

  /**
   * Getter for attribute isTemporary
   *
   * 
   * \par In Python:
   *    value = obj.isTemporary
   */
  grt::IntegerRef isTemporary() const { return _isTemporary; }

  /**
   * Setter for attribute isTemporary
   * 
   * 
   * \par In Python:
   *   obj.isTemporary = value
   */
  virtual void isTemporary(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isTemporary);
    _isTemporary = value;
    member_changed("isTemporary", ovalue, value);
  }

  /**
   * Getter for attribute primaryKey
   *
   * 
   * \par In Python:
   *    value = obj.primaryKey
   */
  db_IndexRef primaryKey() const { return _primaryKey; }

  /**
   * Setter for attribute primaryKey
   * 
   * 
   * \par In Python:
   *   obj.primaryKey = value
   */
  virtual void primaryKey(const db_IndexRef &value) {
    grt::ValueRef ovalue(_primaryKey);
    _primaryKey = value;
    member_changed("primaryKey", ovalue, value);
  }

  /**
   * Getter for attribute temporaryScope
   *
   * 
   * \par In Python:
   *    value = obj.temporaryScope
   */
  grt::StringRef temporaryScope() const { return _temporaryScope; }

  /**
   * Setter for attribute temporaryScope
   * 
   * 
   * \par In Python:
   *   obj.temporaryScope = value
   */
  virtual void temporaryScope(const grt::StringRef &value) {
    grt::ValueRef ovalue(_temporaryScope);
    _temporaryScope = value;
    member_changed("temporaryScope", ovalue, value);
  }

  // triggers is owned by db_Table
  /**
   * Getter for attribute triggers (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.triggers
   */
  grt::ListRef<db_Trigger> triggers() const { return _triggers; }


private: // The next attribute is read-only.
  virtual void triggers(const grt::ListRef<db_Trigger> &value) {
    grt::ValueRef ovalue(_triggers);

    _triggers = value;
    owned_member_changed("triggers", ovalue, value);
  }
public:

  /**
   * Method. 
   * \param column 
   * \return 
   */
  virtual void addColumn(const db_ColumnRef &column);
  /**
   * Method. 
   * \param index 
   * \return 
   */
  virtual void addIndex(const db_IndexRef &index);
  /**
   * Method. 
   * \param column 
   * \return 
   */
  virtual void addPrimaryKeyColumn(const db_ColumnRef &column);
  /**
   * Method. 
   * \param name 
   * \return 
   */
  virtual db_ForeignKeyRef createForeignKey(const std::string &name);
  /**
   * Method. creates a grid object representing the inserts data, suitable for browsing and editing its contents
   * \return 
   */
  virtual db_query_EditableResultsetRef createInsertsEditor();
  /**
   * Method. 
   * \return 
   */
  virtual grt::StringRef inserts();
  /**
   * Method. 
   * \return 
   */
  virtual grt::IntegerRef isDependantTable();
  /**
   * Method. 
   * \param column 
   * \return 
   */
  virtual grt::IntegerRef isForeignKeyColumn(const db_ColumnRef &column);
  /**
   * Method. 
   * \param column 
   * \return 
   */
  virtual grt::IntegerRef isPrimaryKeyColumn(const db_ColumnRef &column);
  /**
   * Method. 
   * \param column 
   * \return 
   */
  virtual void removeColumn(const db_ColumnRef &column);
  /**
   * Method. 
   * \param fk 
   * \param removeColumns 
   * \return 
   */
  virtual void removeForeignKey(const db_ForeignKeyRef &fk, ssize_t removeColumns);
  /**
   * Method. 
   * \param index 
   * \return 
   */
  virtual void removeIndex(const db_IndexRef &index);
  /**
   * Method. 
   * \param column 
   * \return 
   */
  virtual void removePrimaryKeyColumn(const db_ColumnRef &column);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:
  boost::signals2::signal<void (std::string)> _signal_refreshDisplay;
  boost::signals2::signal<void (db_ForeignKeyRef)> _signal_foreignKeyChanged;

  grt::ListRef<db_Column> _columns;// owned
  grt::ListRef<db_ForeignKey> _foreignKeys;// owned
  grt::ListRef<db_Index> _indices;// owned
  grt::IntegerRef _isStub;
  grt::IntegerRef _isSystem;
  grt::IntegerRef _isTemporary;
  db_IndexRef _primaryKey;
  grt::StringRef _temporaryScope;
  grt::ListRef<db_Trigger> _triggers;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Table());
  }

  static grt::ValueRef call_addColumn(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Table*>(self)->addColumn(db_ColumnRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_addIndex(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Table*>(self)->addIndex(db_IndexRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_addPrimaryKeyColumn(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Table*>(self)->addPrimaryKeyColumn(db_ColumnRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_createForeignKey(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Table*>(self)->createForeignKey(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_createInsertsEditor(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Table*>(self)->createInsertsEditor(); }

  static grt::ValueRef call_inserts(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Table*>(self)->inserts(); }

  static grt::ValueRef call_isDependantTable(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Table*>(self)->isDependantTable(); }

  static grt::ValueRef call_isForeignKeyColumn(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Table*>(self)->isForeignKeyColumn(db_ColumnRef::cast_from(args[0])); }

  static grt::ValueRef call_isPrimaryKeyColumn(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Table*>(self)->isPrimaryKeyColumn(db_ColumnRef::cast_from(args[0])); }

  static grt::ValueRef call_removeColumn(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Table*>(self)->removeColumn(db_ColumnRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_removeForeignKey(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Table*>(self)->removeForeignKey(db_ForeignKeyRef::cast_from(args[0]), grt::IntegerRef::cast_from(args[1])); return grt::ValueRef(); }

  static grt::ValueRef call_removeIndex(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Table*>(self)->removeIndex(db_IndexRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_removePrimaryKeyColumn(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Table*>(self)->removePrimaryKeyColumn(db_ColumnRef::cast_from(args[0])); return grt::ValueRef(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Table::create);
    {
      void (db_Table::*setter)(const grt::ListRef<db_Column> &) = &db_Table::columns;
      grt::ListRef<db_Column> (db_Table::*getter)() const = &db_Table::columns;
      meta->bind_member("columns", new grt::MetaClass::Property<db_Table,grt::ListRef<db_Column>>(getter, setter));
    }
    {
      void (db_Table::*setter)(const grt::ListRef<db_ForeignKey> &) = &db_Table::foreignKeys;
      grt::ListRef<db_ForeignKey> (db_Table::*getter)() const = &db_Table::foreignKeys;
      meta->bind_member("foreignKeys", new grt::MetaClass::Property<db_Table,grt::ListRef<db_ForeignKey>>(getter, setter));
    }
    {
      void (db_Table::*setter)(const grt::ListRef<db_Index> &) = &db_Table::indices;
      grt::ListRef<db_Index> (db_Table::*getter)() const = &db_Table::indices;
      meta->bind_member("indices", new grt::MetaClass::Property<db_Table,grt::ListRef<db_Index>>(getter, setter));
    }
    {
      void (db_Table::*setter)(const grt::IntegerRef &) = &db_Table::isStub;
      grt::IntegerRef (db_Table::*getter)() const = &db_Table::isStub;
      meta->bind_member("isStub", new grt::MetaClass::Property<db_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Table::*setter)(const grt::IntegerRef &) = &db_Table::isSystem;
      grt::IntegerRef (db_Table::*getter)() const = &db_Table::isSystem;
      meta->bind_member("isSystem", new grt::MetaClass::Property<db_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Table::*setter)(const grt::IntegerRef &) = &db_Table::isTemporary;
      grt::IntegerRef (db_Table::*getter)() const = &db_Table::isTemporary;
      meta->bind_member("isTemporary", new grt::MetaClass::Property<db_Table,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Table::*setter)(const db_IndexRef &) = &db_Table::primaryKey;
      db_IndexRef (db_Table::*getter)() const = &db_Table::primaryKey;
      meta->bind_member("primaryKey", new grt::MetaClass::Property<db_Table,db_IndexRef>(getter, setter));
    }
    {
      void (db_Table::*setter)(const grt::StringRef &) = &db_Table::temporaryScope;
      grt::StringRef (db_Table::*getter)() const = &db_Table::temporaryScope;
      meta->bind_member("temporaryScope", new grt::MetaClass::Property<db_Table,grt::StringRef>(getter, setter));
    }
    {
      void (db_Table::*setter)(const grt::ListRef<db_Trigger> &) = &db_Table::triggers;
      grt::ListRef<db_Trigger> (db_Table::*getter)() const = &db_Table::triggers;
      meta->bind_member("triggers", new grt::MetaClass::Property<db_Table,grt::ListRef<db_Trigger>>(getter, setter));
    }
    meta->bind_method("addColumn", &db_Table::call_addColumn);
    meta->bind_method("addIndex", &db_Table::call_addIndex);
    meta->bind_method("addPrimaryKeyColumn", &db_Table::call_addPrimaryKeyColumn);
    meta->bind_method("createForeignKey", &db_Table::call_createForeignKey);
    meta->bind_method("createInsertsEditor", &db_Table::call_createInsertsEditor);
    meta->bind_method("inserts", &db_Table::call_inserts);
    meta->bind_method("isDependantTable", &db_Table::call_isDependantTable);
    meta->bind_method("isForeignKeyColumn", &db_Table::call_isForeignKeyColumn);
    meta->bind_method("isPrimaryKeyColumn", &db_Table::call_isPrimaryKeyColumn);
    meta->bind_method("removeColumn", &db_Table::call_removeColumn);
    meta->bind_method("removeForeignKey", &db_Table::call_removeForeignKey);
    meta->bind_method("removeIndex", &db_Table::call_removeIndex);
    meta->bind_method("removePrimaryKeyColumn", &db_Table::call_removePrimaryKeyColumn);
  }
};

class  db_ServerLink : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_ServerLink(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _host(""),
      _ownerUser(""),
      _password(""),
      _port(""),
      _schema(""),
      _socket(""),
      _user(""),
      _wrapperName("") {
  }

  static std::string static_class_name() {
    return "db.ServerLink";
  }

  /**
   * Getter for attribute host
   *
   * the host name the server runs on
   * \par In Python:
   *    value = obj.host
   */
  grt::StringRef host() const { return _host; }

  /**
   * Setter for attribute host
   * 
   * the host name the server runs on
   * \par In Python:
   *   obj.host = value
   */
  virtual void host(const grt::StringRef &value) {
    grt::ValueRef ovalue(_host);
    _host = value;
    member_changed("host", ovalue, value);
  }

  /**
   * Getter for attribute ownerUser
   *
   * the owner
   * \par In Python:
   *    value = obj.ownerUser
   */
  grt::StringRef ownerUser() const { return _ownerUser; }

  /**
   * Setter for attribute ownerUser
   * 
   * the owner
   * \par In Python:
   *   obj.ownerUser = value
   */
  virtual void ownerUser(const grt::StringRef &value) {
    grt::ValueRef ovalue(_ownerUser);
    _ownerUser = value;
    member_changed("ownerUser", ovalue, value);
  }

  /**
   * Getter for attribute password
   *
   * the password to connect with
   * \par In Python:
   *    value = obj.password
   */
  grt::StringRef password() const { return _password; }

  /**
   * Setter for attribute password
   * 
   * the password to connect with
   * \par In Python:
   *   obj.password = value
   */
  virtual void password(const grt::StringRef &value) {
    grt::ValueRef ovalue(_password);
    _password = value;
    member_changed("password", ovalue, value);
  }

  /**
   * Getter for attribute port
   *
   * the port the server runs on
   * \par In Python:
   *    value = obj.port
   */
  grt::StringRef port() const { return _port; }

  /**
   * Setter for attribute port
   * 
   * the port the server runs on
   * \par In Python:
   *   obj.port = value
   */
  virtual void port(const grt::StringRef &value) {
    grt::ValueRef ovalue(_port);
    _port = value;
    member_changed("port", ovalue, value);
  }

  /**
   * Getter for attribute schema
   *
   * the name of the schema to use
   * \par In Python:
   *    value = obj.schema
   */
  grt::StringRef schema() const { return _schema; }

  /**
   * Setter for attribute schema
   * 
   * the name of the schema to use
   * \par In Python:
   *   obj.schema = value
   */
  virtual void schema(const grt::StringRef &value) {
    grt::ValueRef ovalue(_schema);
    _schema = value;
    member_changed("schema", ovalue, value);
  }

  /**
   * Getter for attribute socket
   *
   * the socket the server runs on
   * \par In Python:
   *    value = obj.socket
   */
  grt::StringRef socket() const { return _socket; }

  /**
   * Setter for attribute socket
   * 
   * the socket the server runs on
   * \par In Python:
   *   obj.socket = value
   */
  virtual void socket(const grt::StringRef &value) {
    grt::ValueRef ovalue(_socket);
    _socket = value;
    member_changed("socket", ovalue, value);
  }

  /**
   * Getter for attribute user
   *
   * the user to connect with
   * \par In Python:
   *    value = obj.user
   */
  grt::StringRef user() const { return _user; }

  /**
   * Setter for attribute user
   * 
   * the user to connect with
   * \par In Python:
   *   obj.user = value
   */
  virtual void user(const grt::StringRef &value) {
    grt::ValueRef ovalue(_user);
    _user = value;
    member_changed("user", ovalue, value);
  }

  /**
   * Getter for attribute wrapperName
   *
   * the type of database server to connect to
   * \par In Python:
   *    value = obj.wrapperName
   */
  grt::StringRef wrapperName() const { return _wrapperName; }

  /**
   * Setter for attribute wrapperName
   * 
   * the type of database server to connect to
   * \par In Python:
   *   obj.wrapperName = value
   */
  virtual void wrapperName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_wrapperName);
    _wrapperName = value;
    member_changed("wrapperName", ovalue, value);
  }

protected:

  grt::StringRef _host;
  grt::StringRef _ownerUser;
  grt::StringRef _password;
  grt::StringRef _port;
  grt::StringRef _schema;
  grt::StringRef _socket;
  grt::StringRef _user;
  grt::StringRef _wrapperName;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_ServerLink());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_ServerLink::create);
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::host;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::host;
      meta->bind_member("host", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::ownerUser;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::ownerUser;
      meta->bind_member("ownerUser", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::password;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::password;
      meta->bind_member("password", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::port;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::port;
      meta->bind_member("port", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::schema;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::schema;
      meta->bind_member("schema", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::socket;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::socket;
      meta->bind_member("socket", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::user;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::user;
      meta->bind_member("user", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
    {
      void (db_ServerLink::*setter)(const grt::StringRef &) = &db_ServerLink::wrapperName;
      grt::StringRef (db_ServerLink::*getter)() const = &db_ServerLink::wrapperName;
      meta->bind_member("wrapperName", new grt::MetaClass::Property<db_ServerLink,grt::StringRef>(getter, setter));
    }
  }
};

class GRT_STRUCTS_DB_PUBLIC db_Schema : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_Schema(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _defaultCharacterSetName(""),
      _defaultCollationName(""),
      _events(this, false),
      _routineGroups(this, false),
      _routines(this, false),
      _sequences(this, false),
      _structuredTypes(this, false),
      _synonyms(this, false),
      _tables(this, false),
      _views(this, false) {
  }

  virtual ~db_Schema();

  static std::string static_class_name() {
    return "db.Schema";
  }

  // args: 
  boost::signals2::signal<void (db_DatabaseObjectRef)>* signal_refreshDisplay() { return &_signal_refreshDisplay; }
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

  // events is owned by db_Schema
  /**
   * Getter for attribute events (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.events
   */
  grt::ListRef<db_Event> events() const { return _events; }


private: // The next attribute is read-only.
  virtual void events(const grt::ListRef<db_Event> &value) {
    grt::ValueRef ovalue(_events);

    _events = value;
    owned_member_changed("events", ovalue, value);
  }
public:

  // routineGroups is owned by db_Schema
  /**
   * Getter for attribute routineGroups (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.routineGroups
   */
  grt::ListRef<db_RoutineGroup> routineGroups() const { return _routineGroups; }


private: // The next attribute is read-only.
  virtual void routineGroups(const grt::ListRef<db_RoutineGroup> &value) {
    grt::ValueRef ovalue(_routineGroups);

    _routineGroups = value;
    owned_member_changed("routineGroups", ovalue, value);
  }
public:

  // routines is owned by db_Schema
  /**
   * Getter for attribute routines (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.routines
   */
  grt::ListRef<db_Routine> routines() const { return _routines; }


private: // The next attribute is read-only.
  virtual void routines(const grt::ListRef<db_Routine> &value) {
    grt::ValueRef ovalue(_routines);

    _routines = value;
    owned_member_changed("routines", ovalue, value);
  }
public:

  // sequences is owned by db_Schema
  /**
   * Getter for attribute sequences (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.sequences
   */
  grt::ListRef<db_Sequence> sequences() const { return _sequences; }


private: // The next attribute is read-only.
  virtual void sequences(const grt::ListRef<db_Sequence> &value) {
    grt::ValueRef ovalue(_sequences);

    _sequences = value;
    owned_member_changed("sequences", ovalue, value);
  }
public:

  // structuredTypes is owned by db_Schema
  /**
   * Getter for attribute structuredTypes (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.structuredTypes
   */
  grt::ListRef<db_StructuredDatatype> structuredTypes() const { return _structuredTypes; }


private: // The next attribute is read-only.
  virtual void structuredTypes(const grt::ListRef<db_StructuredDatatype> &value) {
    grt::ValueRef ovalue(_structuredTypes);

    _structuredTypes = value;
    owned_member_changed("structuredTypes", ovalue, value);
  }
public:

  // synonyms is owned by db_Schema
  /**
   * Getter for attribute synonyms (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.synonyms
   */
  grt::ListRef<db_Synonym> synonyms() const { return _synonyms; }


private: // The next attribute is read-only.
  virtual void synonyms(const grt::ListRef<db_Synonym> &value) {
    grt::ValueRef ovalue(_synonyms);

    _synonyms = value;
    owned_member_changed("synonyms", ovalue, value);
  }
public:

  // tables is owned by db_Schema
  /**
   * Getter for attribute tables (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.tables
   */
  grt::ListRef<db_Table> tables() const { return _tables; }


private: // The next attribute is read-only.
  virtual void tables(const grt::ListRef<db_Table> &value) {
    grt::ValueRef ovalue(_tables);

    _tables = value;
    owned_member_changed("tables", ovalue, value);
  }
public:

  // views is owned by db_Schema
  /**
   * Getter for attribute views (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.views
   */
  grt::ListRef<db_View> views() const { return _views; }


private: // The next attribute is read-only.
  virtual void views(const grt::ListRef<db_View> &value) {
    grt::ValueRef ovalue(_views);

    _views = value;
    owned_member_changed("views", ovalue, value);
  }
public:

  /**
   * Method. 
   * \param dbpackage 
   * \return 
   */
  virtual db_RoutineRef addNewRoutine(const std::string &dbpackage);
  /**
   * Method. 
   * \param dbpackage 
   * \return 
   */
  virtual db_RoutineGroupRef addNewRoutineGroup(const std::string &dbpackage);
  /**
   * Method. create and add a new table to the schema. For MySQL tables, pass db.mysql as the dbpackage argument
   * \param dbpackage 
   * \return 
   */
  virtual db_TableRef addNewTable(const std::string &dbpackage);
  /**
   * Method. 
   * \param dbpackage 
   * \return 
   */
  virtual db_ViewRef addNewView(const std::string &dbpackage);
  /**
   * Method. 
   * \param table 
   * \return 
   */
  virtual grt::ListRef<db_ForeignKey> getForeignKeysReferencingTable(const db_TableRef &table);
  /**
   * Method. 
   * \param table 
   * \return 
   */
  virtual void removeTable(const db_TableRef &table);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:
  boost::signals2::signal<void (db_DatabaseObjectRef)> _signal_refreshDisplay;

  grt::StringRef _defaultCharacterSetName;
  grt::StringRef _defaultCollationName;
  grt::ListRef<db_Event> _events;// owned
  grt::ListRef<db_RoutineGroup> _routineGroups;// owned
  grt::ListRef<db_Routine> _routines;// owned
  grt::ListRef<db_Sequence> _sequences;// owned
  grt::ListRef<db_StructuredDatatype> _structuredTypes;// owned
  grt::ListRef<db_Synonym> _synonyms;// owned
  grt::ListRef<db_Table> _tables;// owned
  grt::ListRef<db_View> _views;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Schema());
  }

  static grt::ValueRef call_addNewRoutine(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Schema*>(self)->addNewRoutine(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_addNewRoutineGroup(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Schema*>(self)->addNewRoutineGroup(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_addNewTable(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Schema*>(self)->addNewTable(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_addNewView(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Schema*>(self)->addNewView(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_getForeignKeysReferencingTable(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_Schema*>(self)->getForeignKeysReferencingTable(db_TableRef::cast_from(args[0])); }

  static grt::ValueRef call_removeTable(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_Schema*>(self)->removeTable(db_TableRef::cast_from(args[0])); return grt::ValueRef(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Schema::create);
    {
      void (db_Schema::*setter)(const grt::StringRef &) = &db_Schema::defaultCharacterSetName;
      grt::StringRef (db_Schema::*getter)() const = &db_Schema::defaultCharacterSetName;
      meta->bind_member("defaultCharacterSetName", new grt::MetaClass::Property<db_Schema,grt::StringRef>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::StringRef &) = &db_Schema::defaultCollationName;
      grt::StringRef (db_Schema::*getter)() const = &db_Schema::defaultCollationName;
      meta->bind_member("defaultCollationName", new grt::MetaClass::Property<db_Schema,grt::StringRef>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_Event> &) = &db_Schema::events;
      grt::ListRef<db_Event> (db_Schema::*getter)() const = &db_Schema::events;
      meta->bind_member("events", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_Event>>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_RoutineGroup> &) = &db_Schema::routineGroups;
      grt::ListRef<db_RoutineGroup> (db_Schema::*getter)() const = &db_Schema::routineGroups;
      meta->bind_member("routineGroups", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_RoutineGroup>>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_Routine> &) = &db_Schema::routines;
      grt::ListRef<db_Routine> (db_Schema::*getter)() const = &db_Schema::routines;
      meta->bind_member("routines", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_Routine>>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_Sequence> &) = &db_Schema::sequences;
      grt::ListRef<db_Sequence> (db_Schema::*getter)() const = &db_Schema::sequences;
      meta->bind_member("sequences", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_Sequence>>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_StructuredDatatype> &) = &db_Schema::structuredTypes;
      grt::ListRef<db_StructuredDatatype> (db_Schema::*getter)() const = &db_Schema::structuredTypes;
      meta->bind_member("structuredTypes", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_StructuredDatatype>>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_Synonym> &) = &db_Schema::synonyms;
      grt::ListRef<db_Synonym> (db_Schema::*getter)() const = &db_Schema::synonyms;
      meta->bind_member("synonyms", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_Synonym>>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_Table> &) = &db_Schema::tables;
      grt::ListRef<db_Table> (db_Schema::*getter)() const = &db_Schema::tables;
      meta->bind_member("tables", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_Table>>(getter, setter));
    }
    {
      void (db_Schema::*setter)(const grt::ListRef<db_View> &) = &db_Schema::views;
      grt::ListRef<db_View> (db_Schema::*getter)() const = &db_Schema::views;
      meta->bind_member("views", new grt::MetaClass::Property<db_Schema,grt::ListRef<db_View>>(getter, setter));
    }
    meta->bind_method("addNewRoutine", &db_Schema::call_addNewRoutine);
    meta->bind_method("addNewRoutineGroup", &db_Schema::call_addNewRoutineGroup);
    meta->bind_method("addNewTable", &db_Schema::call_addNewTable);
    meta->bind_method("addNewView", &db_Schema::call_addNewView);
    meta->bind_method("getForeignKeysReferencingTable", &db_Schema::call_getForeignKeysReferencingTable);
    meta->bind_method("removeTable", &db_Schema::call_removeTable);
  }
};

class  db_Tablespace : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_Tablespace(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _autoExtendSize(0),
      _dataFile(""),
      _encryption(""),
      _engine(""),
      _extentSize(0),
      _fileBlockSize(0),
      _initialSize(0),
      _maxSize(0),
      _nodeGroup(0),
      _wait(0) {
  }

  static std::string static_class_name() {
    return "db.Tablespace";
  }

  /**
   * Getter for attribute autoExtendSize
   *
   * 
   * \par In Python:
   *    value = obj.autoExtendSize
   */
  grt::IntegerRef autoExtendSize() const { return _autoExtendSize; }

  /**
   * Setter for attribute autoExtendSize
   * 
   * 
   * \par In Python:
   *   obj.autoExtendSize = value
   */
  virtual void autoExtendSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_autoExtendSize);
    _autoExtendSize = value;
    member_changed("autoExtendSize", ovalue, value);
  }

  /**
   * Getter for attribute dataFile
   *
   * 
   * \par In Python:
   *    value = obj.dataFile
   */
  grt::StringRef dataFile() const { return _dataFile; }

  /**
   * Setter for attribute dataFile
   * 
   * 
   * \par In Python:
   *   obj.dataFile = value
   */
  virtual void dataFile(const grt::StringRef &value) {
    grt::ValueRef ovalue(_dataFile);
    _dataFile = value;
    member_changed("dataFile", ovalue, value);
  }

  /**
   * Getter for attribute encryption
   *
   * 
   * \par In Python:
   *    value = obj.encryption
   */
  grt::StringRef encryption() const { return _encryption; }

  /**
   * Setter for attribute encryption
   * 
   * 
   * \par In Python:
   *   obj.encryption = value
   */
  virtual void encryption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_encryption);
    _encryption = value;
    member_changed("encryption", ovalue, value);
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
   * Getter for attribute extentSize
   *
   * 
   * \par In Python:
   *    value = obj.extentSize
   */
  grt::IntegerRef extentSize() const { return _extentSize; }

  /**
   * Setter for attribute extentSize
   * 
   * 
   * \par In Python:
   *   obj.extentSize = value
   */
  virtual void extentSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_extentSize);
    _extentSize = value;
    member_changed("extentSize", ovalue, value);
  }

  /**
   * Getter for attribute fileBlockSize
   *
   * 
   * \par In Python:
   *    value = obj.fileBlockSize
   */
  grt::IntegerRef fileBlockSize() const { return _fileBlockSize; }

  /**
   * Setter for attribute fileBlockSize
   * 
   * 
   * \par In Python:
   *   obj.fileBlockSize = value
   */
  virtual void fileBlockSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_fileBlockSize);
    _fileBlockSize = value;
    member_changed("fileBlockSize", ovalue, value);
  }

  /**
   * Getter for attribute initialSize
   *
   * 
   * \par In Python:
   *    value = obj.initialSize
   */
  grt::IntegerRef initialSize() const { return _initialSize; }

  /**
   * Setter for attribute initialSize
   * 
   * 
   * \par In Python:
   *   obj.initialSize = value
   */
  virtual void initialSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_initialSize);
    _initialSize = value;
    member_changed("initialSize", ovalue, value);
  }

  /**
   * Getter for attribute logFileGroup
   *
   * the log file group that is used for this tablespace
   * \par In Python:
   *    value = obj.logFileGroup
   */
  db_LogFileGroupRef logFileGroup() const { return _logFileGroup; }

  /**
   * Setter for attribute logFileGroup
   * 
   * the log file group that is used for this tablespace
   * \par In Python:
   *   obj.logFileGroup = value
   */
  virtual void logFileGroup(const db_LogFileGroupRef &value) {
    grt::ValueRef ovalue(_logFileGroup);
    _logFileGroup = value;
    member_changed("logFileGroup", ovalue, value);
  }

  /**
   * Getter for attribute maxSize
   *
   * 
   * \par In Python:
   *    value = obj.maxSize
   */
  grt::IntegerRef maxSize() const { return _maxSize; }

  /**
   * Setter for attribute maxSize
   * 
   * 
   * \par In Python:
   *   obj.maxSize = value
   */
  virtual void maxSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_maxSize);
    _maxSize = value;
    member_changed("maxSize", ovalue, value);
  }

  /**
   * Getter for attribute nodeGroup
   *
   * 
   * \par In Python:
   *    value = obj.nodeGroup
   */
  grt::IntegerRef nodeGroup() const { return _nodeGroup; }

  /**
   * Setter for attribute nodeGroup
   * 
   * 
   * \par In Python:
   *   obj.nodeGroup = value
   */
  virtual void nodeGroup(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_nodeGroup);
    _nodeGroup = value;
    member_changed("nodeGroup", ovalue, value);
  }

  /**
   * Getter for attribute wait
   *
   * 
   * \par In Python:
   *    value = obj.wait
   */
  grt::IntegerRef wait() const { return _wait; }

  /**
   * Setter for attribute wait
   * 
   * 
   * \par In Python:
   *   obj.wait = value
   */
  virtual void wait(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_wait);
    _wait = value;
    member_changed("wait", ovalue, value);
  }

protected:

  grt::IntegerRef _autoExtendSize;
  grt::StringRef _dataFile;
  grt::StringRef _encryption;
  grt::StringRef _engine;
  grt::IntegerRef _extentSize;
  grt::IntegerRef _fileBlockSize;
  grt::IntegerRef _initialSize;
  db_LogFileGroupRef _logFileGroup;
  grt::IntegerRef _maxSize;
  grt::IntegerRef _nodeGroup;
  grt::IntegerRef _wait;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Tablespace());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Tablespace::create);
    {
      void (db_Tablespace::*setter)(const grt::IntegerRef &) = &db_Tablespace::autoExtendSize;
      grt::IntegerRef (db_Tablespace::*getter)() const = &db_Tablespace::autoExtendSize;
      meta->bind_member("autoExtendSize", new grt::MetaClass::Property<db_Tablespace,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::StringRef &) = &db_Tablespace::dataFile;
      grt::StringRef (db_Tablespace::*getter)() const = &db_Tablespace::dataFile;
      meta->bind_member("dataFile", new grt::MetaClass::Property<db_Tablespace,grt::StringRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::StringRef &) = &db_Tablespace::encryption;
      grt::StringRef (db_Tablespace::*getter)() const = &db_Tablespace::encryption;
      meta->bind_member("encryption", new grt::MetaClass::Property<db_Tablespace,grt::StringRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::StringRef &) = &db_Tablespace::engine;
      grt::StringRef (db_Tablespace::*getter)() const = &db_Tablespace::engine;
      meta->bind_member("engine", new grt::MetaClass::Property<db_Tablespace,grt::StringRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::IntegerRef &) = &db_Tablespace::extentSize;
      grt::IntegerRef (db_Tablespace::*getter)() const = &db_Tablespace::extentSize;
      meta->bind_member("extentSize", new grt::MetaClass::Property<db_Tablespace,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::IntegerRef &) = &db_Tablespace::fileBlockSize;
      grt::IntegerRef (db_Tablespace::*getter)() const = &db_Tablespace::fileBlockSize;
      meta->bind_member("fileBlockSize", new grt::MetaClass::Property<db_Tablespace,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::IntegerRef &) = &db_Tablespace::initialSize;
      grt::IntegerRef (db_Tablespace::*getter)() const = &db_Tablespace::initialSize;
      meta->bind_member("initialSize", new grt::MetaClass::Property<db_Tablespace,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const db_LogFileGroupRef &) = &db_Tablespace::logFileGroup;
      db_LogFileGroupRef (db_Tablespace::*getter)() const = &db_Tablespace::logFileGroup;
      meta->bind_member("logFileGroup", new grt::MetaClass::Property<db_Tablespace,db_LogFileGroupRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::IntegerRef &) = &db_Tablespace::maxSize;
      grt::IntegerRef (db_Tablespace::*getter)() const = &db_Tablespace::maxSize;
      meta->bind_member("maxSize", new grt::MetaClass::Property<db_Tablespace,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::IntegerRef &) = &db_Tablespace::nodeGroup;
      grt::IntegerRef (db_Tablespace::*getter)() const = &db_Tablespace::nodeGroup;
      meta->bind_member("nodeGroup", new grt::MetaClass::Property<db_Tablespace,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Tablespace::*setter)(const grt::IntegerRef &) = &db_Tablespace::wait;
      grt::IntegerRef (db_Tablespace::*getter)() const = &db_Tablespace::wait;
      meta->bind_member("wait", new grt::MetaClass::Property<db_Tablespace,grt::IntegerRef>(getter, setter));
    }
  }
};

class  db_LogFileGroup : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_LogFileGroup(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _engine(""),
      _initialSize(0),
      _nodeGroup(0),
      _redoBufferSize(0),
      _undoBufferSize(0),
      _undoFile(""),
      _wait(0) {
  }

  static std::string static_class_name() {
    return "db.LogFileGroup";
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
   * Getter for attribute initialSize
   *
   * 
   * \par In Python:
   *    value = obj.initialSize
   */
  grt::IntegerRef initialSize() const { return _initialSize; }

  /**
   * Setter for attribute initialSize
   * 
   * 
   * \par In Python:
   *   obj.initialSize = value
   */
  virtual void initialSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_initialSize);
    _initialSize = value;
    member_changed("initialSize", ovalue, value);
  }

  /**
   * Getter for attribute nodeGroup
   *
   * 
   * \par In Python:
   *    value = obj.nodeGroup
   */
  grt::IntegerRef nodeGroup() const { return _nodeGroup; }

  /**
   * Setter for attribute nodeGroup
   * 
   * 
   * \par In Python:
   *   obj.nodeGroup = value
   */
  virtual void nodeGroup(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_nodeGroup);
    _nodeGroup = value;
    member_changed("nodeGroup", ovalue, value);
  }

  /**
   * Getter for attribute redoBufferSize
   *
   * 
   * \par In Python:
   *    value = obj.redoBufferSize
   */
  grt::IntegerRef redoBufferSize() const { return _redoBufferSize; }

  /**
   * Setter for attribute redoBufferSize
   * 
   * 
   * \par In Python:
   *   obj.redoBufferSize = value
   */
  virtual void redoBufferSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_redoBufferSize);
    _redoBufferSize = value;
    member_changed("redoBufferSize", ovalue, value);
  }

  /**
   * Getter for attribute undoBufferSize
   *
   * 
   * \par In Python:
   *    value = obj.undoBufferSize
   */
  grt::IntegerRef undoBufferSize() const { return _undoBufferSize; }

  /**
   * Setter for attribute undoBufferSize
   * 
   * 
   * \par In Python:
   *   obj.undoBufferSize = value
   */
  virtual void undoBufferSize(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_undoBufferSize);
    _undoBufferSize = value;
    member_changed("undoBufferSize", ovalue, value);
  }

  /**
   * Getter for attribute undoFile
   *
   * 
   * \par In Python:
   *    value = obj.undoFile
   */
  grt::StringRef undoFile() const { return _undoFile; }

  /**
   * Setter for attribute undoFile
   * 
   * 
   * \par In Python:
   *   obj.undoFile = value
   */
  virtual void undoFile(const grt::StringRef &value) {
    grt::ValueRef ovalue(_undoFile);
    _undoFile = value;
    member_changed("undoFile", ovalue, value);
  }

  /**
   * Getter for attribute wait
   *
   * 
   * \par In Python:
   *    value = obj.wait
   */
  grt::IntegerRef wait() const { return _wait; }

  /**
   * Setter for attribute wait
   * 
   * 
   * \par In Python:
   *   obj.wait = value
   */
  virtual void wait(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_wait);
    _wait = value;
    member_changed("wait", ovalue, value);
  }

protected:

  grt::StringRef _engine;
  grt::IntegerRef _initialSize;
  grt::IntegerRef _nodeGroup;
  grt::IntegerRef _redoBufferSize;
  grt::IntegerRef _undoBufferSize;
  grt::StringRef _undoFile;
  grt::IntegerRef _wait;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_LogFileGroup());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_LogFileGroup::create);
    {
      void (db_LogFileGroup::*setter)(const grt::StringRef &) = &db_LogFileGroup::engine;
      grt::StringRef (db_LogFileGroup::*getter)() const = &db_LogFileGroup::engine;
      meta->bind_member("engine", new grt::MetaClass::Property<db_LogFileGroup,grt::StringRef>(getter, setter));
    }
    {
      void (db_LogFileGroup::*setter)(const grt::IntegerRef &) = &db_LogFileGroup::initialSize;
      grt::IntegerRef (db_LogFileGroup::*getter)() const = &db_LogFileGroup::initialSize;
      meta->bind_member("initialSize", new grt::MetaClass::Property<db_LogFileGroup,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_LogFileGroup::*setter)(const grt::IntegerRef &) = &db_LogFileGroup::nodeGroup;
      grt::IntegerRef (db_LogFileGroup::*getter)() const = &db_LogFileGroup::nodeGroup;
      meta->bind_member("nodeGroup", new grt::MetaClass::Property<db_LogFileGroup,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_LogFileGroup::*setter)(const grt::IntegerRef &) = &db_LogFileGroup::redoBufferSize;
      grt::IntegerRef (db_LogFileGroup::*getter)() const = &db_LogFileGroup::redoBufferSize;
      meta->bind_member("redoBufferSize", new grt::MetaClass::Property<db_LogFileGroup,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_LogFileGroup::*setter)(const grt::IntegerRef &) = &db_LogFileGroup::undoBufferSize;
      grt::IntegerRef (db_LogFileGroup::*getter)() const = &db_LogFileGroup::undoBufferSize;
      meta->bind_member("undoBufferSize", new grt::MetaClass::Property<db_LogFileGroup,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_LogFileGroup::*setter)(const grt::StringRef &) = &db_LogFileGroup::undoFile;
      grt::StringRef (db_LogFileGroup::*getter)() const = &db_LogFileGroup::undoFile;
      meta->bind_member("undoFile", new grt::MetaClass::Property<db_LogFileGroup,grt::StringRef>(getter, setter));
    }
    {
      void (db_LogFileGroup::*setter)(const grt::IntegerRef &) = &db_LogFileGroup::wait;
      grt::IntegerRef (db_LogFileGroup::*getter)() const = &db_LogFileGroup::wait;
      meta->bind_member("wait", new grt::MetaClass::Property<db_LogFileGroup,grt::IntegerRef>(getter, setter));
    }
  }
};

class  db_User : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_User(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _password(""),
      _roles(this, false) {
  }

  static std::string static_class_name() {
    return "db.User";
  }

  /**
   * Getter for attribute password
   *
   * the password assigned to the user
   * \par In Python:
   *    value = obj.password
   */
  grt::StringRef password() const { return _password; }

  /**
   * Setter for attribute password
   * 
   * the password assigned to the user
   * \par In Python:
   *   obj.password = value
   */
  virtual void password(const grt::StringRef &value) {
    grt::ValueRef ovalue(_password);
    _password = value;
    member_changed("password", ovalue, value);
  }

  /**
   * Getter for attribute roles (read-only)
   *
   * the list of assigned roles
   * \par In Python:
   *    value = obj.roles
   */
  grt::ListRef<db_Role> roles() const { return _roles; }


private: // The next attribute is read-only.
  virtual void roles(const grt::ListRef<db_Role> &value) {
    grt::ValueRef ovalue(_roles);
    _roles = value;
    member_changed("roles", ovalue, value);
  }
public:

protected:

  grt::StringRef _password;
  grt::ListRef<db_Role> _roles;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_User());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_User::create);
    {
      void (db_User::*setter)(const grt::StringRef &) = &db_User::password;
      grt::StringRef (db_User::*getter)() const = &db_User::password;
      meta->bind_member("password", new grt::MetaClass::Property<db_User,grt::StringRef>(getter, setter));
    }
    {
      void (db_User::*setter)(const grt::ListRef<db_Role> &) = &db_User::roles;
      grt::ListRef<db_Role> (db_User::*getter)() const = &db_User::roles;
      meta->bind_member("roles", new grt::MetaClass::Property<db_User,grt::ListRef<db_Role>>(getter, setter));
    }
  }
};

class  db_Role : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_Role(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _childRoles(this, false),
      _privileges(this, false) {
  }

  static std::string static_class_name() {
    return "db.Role";
  }

  /**
   * Getter for attribute childRoles (read-only)
   *
   * the list of roles that derive from this one. They will have all privileges from this role and it's parents.
   * \par In Python:
   *    value = obj.childRoles
   */
  grt::ListRef<db_Role> childRoles() const { return _childRoles; }


private: // The next attribute is read-only.
  virtual void childRoles(const grt::ListRef<db_Role> &value) {
    grt::ValueRef ovalue(_childRoles);
    _childRoles = value;
    member_changed("childRoles", ovalue, value);
  }
public:

  /**
   * Getter for attribute parentRole
   *
   * role that this role derives from or empty if there is no parent role. All privileges which has parent present for current Role
   * \par In Python:
   *    value = obj.parentRole
   */
  db_RoleRef parentRole() const { return _parentRole; }

  /**
   * Setter for attribute parentRole
   * 
   * role that this role derives from or empty if there is no parent role. All privileges which has parent present for current Role
   * \par In Python:
   *   obj.parentRole = value
   */
  virtual void parentRole(const db_RoleRef &value) {
    grt::ValueRef ovalue(_parentRole);
    _parentRole = value;
    member_changed("parentRole", ovalue, value);
  }

  // privileges is owned by db_Role
  /**
   * Getter for attribute privileges (read-only)
   *
   * the list of privileges available for this role
   * \par In Python:
   *    value = obj.privileges
   */
  grt::ListRef<db_RolePrivilege> privileges() const { return _privileges; }


private: // The next attribute is read-only.
  virtual void privileges(const grt::ListRef<db_RolePrivilege> &value) {
    grt::ValueRef ovalue(_privileges);

    _privileges = value;
    owned_member_changed("privileges", ovalue, value);
  }
public:

protected:

  grt::ListRef<db_Role> _childRoles;
  db_RoleRef _parentRole;
  grt::ListRef<db_RolePrivilege> _privileges;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Role());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Role::create);
    {
      void (db_Role::*setter)(const grt::ListRef<db_Role> &) = &db_Role::childRoles;
      grt::ListRef<db_Role> (db_Role::*getter)() const = &db_Role::childRoles;
      meta->bind_member("childRoles", new grt::MetaClass::Property<db_Role,grt::ListRef<db_Role>>(getter, setter));
    }
    {
      void (db_Role::*setter)(const db_RoleRef &) = &db_Role::parentRole;
      db_RoleRef (db_Role::*getter)() const = &db_Role::parentRole;
      meta->bind_member("parentRole", new grt::MetaClass::Property<db_Role,db_RoleRef>(getter, setter));
    }
    {
      void (db_Role::*setter)(const grt::ListRef<db_RolePrivilege> &) = &db_Role::privileges;
      grt::ListRef<db_RolePrivilege> (db_Role::*getter)() const = &db_Role::privileges;
      meta->bind_member("privileges", new grt::MetaClass::Property<db_Role,grt::ListRef<db_RolePrivilege>>(getter, setter));
    }
  }
};

class  db_DatabaseDdlObject : public db_DatabaseObject {
  typedef db_DatabaseObject super;

public:
  db_DatabaseDdlObject(grt::MetaClass *meta = nullptr)
    : db_DatabaseObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _definer(""),
      _sqlBody(""),
      _sqlDefinition("") {
  }

  static std::string static_class_name() {
    return "db.DatabaseDdlObject";
  }

  /**
   * Getter for attribute definer
   *
   * 
   * \par In Python:
   *    value = obj.definer
   */
  grt::StringRef definer() const { return _definer; }

  /**
   * Setter for attribute definer
   * 
   * 
   * \par In Python:
   *   obj.definer = value
   */
  virtual void definer(const grt::StringRef &value) {
    grt::ValueRef ovalue(_definer);
    _definer = value;
    member_changed("definer", ovalue, value);
  }

  /**
   * Getter for attribute sqlBody
   *
   * 
   * \par In Python:
   *    value = obj.sqlBody
   */
  grt::StringRef sqlBody() const { return _sqlBody; }

  /**
   * Setter for attribute sqlBody
   * 
   * 
   * \par In Python:
   *   obj.sqlBody = value
   */
  virtual void sqlBody(const grt::StringRef &value) {
    grt::ValueRef ovalue(_sqlBody);
    _sqlBody = value;
    member_changed("sqlBody", ovalue, value);
  }

  /**
   * Getter for attribute sqlDefinition
   *
   * 
   * \par In Python:
   *    value = obj.sqlDefinition
   */
  grt::StringRef sqlDefinition() const { return _sqlDefinition; }

  /**
   * Setter for attribute sqlDefinition
   * 
   * 
   * \par In Python:
   *   obj.sqlDefinition = value
   */
  virtual void sqlDefinition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_sqlDefinition);
    _sqlDefinition = value;
    member_changed("sqlDefinition", ovalue, value);
  }

protected:

  grt::StringRef _definer;
  grt::StringRef _sqlBody;
  grt::StringRef _sqlDefinition;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_DatabaseDdlObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_DatabaseDdlObject::create);
    {
      void (db_DatabaseDdlObject::*setter)(const grt::StringRef &) = &db_DatabaseDdlObject::definer;
      grt::StringRef (db_DatabaseDdlObject::*getter)() const = &db_DatabaseDdlObject::definer;
      meta->bind_member("definer", new grt::MetaClass::Property<db_DatabaseDdlObject,grt::StringRef>(getter, setter));
    }
    {
      void (db_DatabaseDdlObject::*setter)(const grt::StringRef &) = &db_DatabaseDdlObject::sqlBody;
      grt::StringRef (db_DatabaseDdlObject::*getter)() const = &db_DatabaseDdlObject::sqlBody;
      meta->bind_member("sqlBody", new grt::MetaClass::Property<db_DatabaseDdlObject,grt::StringRef>(getter, setter));
    }
    {
      void (db_DatabaseDdlObject::*setter)(const grt::StringRef &) = &db_DatabaseDdlObject::sqlDefinition;
      grt::StringRef (db_DatabaseDdlObject::*getter)() const = &db_DatabaseDdlObject::sqlDefinition;
      meta->bind_member("sqlDefinition", new grt::MetaClass::Property<db_DatabaseDdlObject,grt::StringRef>(getter, setter));
    }
  }
};

class  db_Event : public db_DatabaseDdlObject {
  typedef db_DatabaseDdlObject super;

public:
  db_Event(grt::MetaClass *meta = nullptr)
    : db_DatabaseDdlObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _at(""),
      _enabled(0),
      _interval(""),
      _intervalEnd(""),
      _intervalStart(""),
      _intervalUnit(""),
      _preserved(0),
      _useInterval(0) {
  }

  static std::string static_class_name() {
    return "db.Event";
  }

  /**
   * Getter for attribute at
   *
   * the expression to define an execution timestamp, mutually exclusive with the interval* members
   * \par In Python:
   *    value = obj.at
   */
  grt::StringRef at() const { return _at; }

  /**
   * Setter for attribute at
   * 
   * the expression to define an execution timestamp, mutually exclusive with the interval* members
   * \par In Python:
   *   obj.at = value
   */
  virtual void at(const grt::StringRef &value) {
    grt::ValueRef ovalue(_at);
    _at = value;
    member_changed("at", ovalue, value);
  }

  /**
   * Getter for attribute comment
   *
   * 
   * \par In Python:
   *    value = obj.comment
   */

  /**
   * Setter for attribute comment
   * 
   * 
   * \par In Python:
   *   obj.comment = value
   */

  /**
   * Getter for attribute definer
   *
   * a full user name ('user'@'host') or CURRENT_USER
   * \par In Python:
   *    value = obj.definer
   */

  /**
   * Setter for attribute definer
   * 
   * a full user name ('user'@'host') or CURRENT_USER
   * \par In Python:
   *   obj.definer = value
   */

  /**
   * Getter for attribute enabled
   *
   * 
   * \par In Python:
   *    value = obj.enabled
   */
  grt::IntegerRef enabled() const { return _enabled; }

  /**
   * Setter for attribute enabled
   * 
   * 
   * \par In Python:
   *   obj.enabled = value
   */
  virtual void enabled(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_enabled);
    _enabled = value;
    member_changed("enabled", ovalue, value);
  }

  /**
   * Getter for attribute interval
   *
   * the expression to define an interval, mutually exclusive with the at member
   * \par In Python:
   *    value = obj.interval
   */
  grt::StringRef interval() const { return _interval; }

  /**
   * Setter for attribute interval
   * 
   * the expression to define an interval, mutually exclusive with the at member
   * \par In Python:
   *   obj.interval = value
   */
  virtual void interval(const grt::StringRef &value) {
    grt::ValueRef ovalue(_interval);
    _interval = value;
    member_changed("interval", ovalue, value);
  }

  /**
   * Getter for attribute intervalEnd
   *
   * optional expression for an end timestamp
   * \par In Python:
   *    value = obj.intervalEnd
   */
  grt::StringRef intervalEnd() const { return _intervalEnd; }

  /**
   * Setter for attribute intervalEnd
   * 
   * optional expression for an end timestamp
   * \par In Python:
   *   obj.intervalEnd = value
   */
  virtual void intervalEnd(const grt::StringRef &value) {
    grt::ValueRef ovalue(_intervalEnd);
    _intervalEnd = value;
    member_changed("intervalEnd", ovalue, value);
  }

  /**
   * Getter for attribute intervalStart
   *
   * optional expression for a start timestamp
   * \par In Python:
   *    value = obj.intervalStart
   */
  grt::StringRef intervalStart() const { return _intervalStart; }

  /**
   * Setter for attribute intervalStart
   * 
   * optional expression for a start timestamp
   * \par In Python:
   *   obj.intervalStart = value
   */
  virtual void intervalStart(const grt::StringRef &value) {
    grt::ValueRef ovalue(_intervalStart);
    _intervalStart = value;
    member_changed("intervalStart", ovalue, value);
  }

  /**
   * Getter for attribute intervalUnit
   *
   * one of the interval units, except microseconds, e.g. SECOND, HOUR etc.
   * \par In Python:
   *    value = obj.intervalUnit
   */
  grt::StringRef intervalUnit() const { return _intervalUnit; }

  /**
   * Setter for attribute intervalUnit
   * 
   * one of the interval units, except microseconds, e.g. SECOND, HOUR etc.
   * \par In Python:
   *   obj.intervalUnit = value
   */
  virtual void intervalUnit(const grt::StringRef &value) {
    grt::ValueRef ovalue(_intervalUnit);
    _intervalUnit = value;
    member_changed("intervalUnit", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * 
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * 
   * \par In Python:
   *   obj.name = value
   */

  /**
   * Getter for attribute preserved
   *
   * 0 if the event is automatically dropped after last occurrence
   * \par In Python:
   *    value = obj.preserved
   */
  grt::IntegerRef preserved() const { return _preserved; }

  /**
   * Setter for attribute preserved
   * 
   * 0 if the event is automatically dropped after last occurrence
   * \par In Python:
   *   obj.preserved = value
   */
  virtual void preserved(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_preserved);
    _preserved = value;
    member_changed("preserved", ovalue, value);
  }

  /**
   * Getter for attribute sqlBody
   *
   * the sql code to execute on each invocation of the event
   * \par In Python:
   *    value = obj.sqlBody
   */

  /**
   * Setter for attribute sqlBody
   * 
   * the sql code to execute on each invocation of the event
   * \par In Python:
   *   obj.sqlBody = value
   */

  /**
   * Getter for attribute useInterval
   *
   * 1 if to use the interval* members, otherwise for at
   * \par In Python:
   *    value = obj.useInterval
   */
  grt::IntegerRef useInterval() const { return _useInterval; }

  /**
   * Setter for attribute useInterval
   * 
   * 1 if to use the interval* members, otherwise for at
   * \par In Python:
   *   obj.useInterval = value
   */
  virtual void useInterval(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_useInterval);
    _useInterval = value;
    member_changed("useInterval", ovalue, value);
  }

protected:

  grt::StringRef _at;
  grt::IntegerRef _enabled;
  grt::StringRef _interval;
  grt::StringRef _intervalEnd;
  grt::StringRef _intervalStart;
  grt::StringRef _intervalUnit;
  grt::IntegerRef _preserved;
  grt::IntegerRef _useInterval;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Event());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Event::create);
    {
      void (db_Event::*setter)(const grt::StringRef &) = &db_Event::at;
      grt::StringRef (db_Event::*getter)() const = &db_Event::at;
      meta->bind_member("at", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_Event::*getter)() const = 0;
      meta->bind_member("comment", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_Event::*getter)() const = 0;
      meta->bind_member("definer", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::IntegerRef &) = &db_Event::enabled;
      grt::IntegerRef (db_Event::*getter)() const = &db_Event::enabled;
      meta->bind_member("enabled", new grt::MetaClass::Property<db_Event,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = &db_Event::interval;
      grt::StringRef (db_Event::*getter)() const = &db_Event::interval;
      meta->bind_member("interval", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = &db_Event::intervalEnd;
      grt::StringRef (db_Event::*getter)() const = &db_Event::intervalEnd;
      meta->bind_member("intervalEnd", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = &db_Event::intervalStart;
      grt::StringRef (db_Event::*getter)() const = &db_Event::intervalStart;
      meta->bind_member("intervalStart", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = &db_Event::intervalUnit;
      grt::StringRef (db_Event::*getter)() const = &db_Event::intervalUnit;
      meta->bind_member("intervalUnit", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_Event::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::IntegerRef &) = &db_Event::preserved;
      grt::IntegerRef (db_Event::*getter)() const = &db_Event::preserved;
      meta->bind_member("preserved", new grt::MetaClass::Property<db_Event,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_Event::*getter)() const = 0;
      meta->bind_member("sqlBody", new grt::MetaClass::Property<db_Event,grt::StringRef>(getter, setter));
    }
    {
      void (db_Event::*setter)(const grt::IntegerRef &) = &db_Event::useInterval;
      grt::IntegerRef (db_Event::*getter)() const = &db_Event::useInterval;
      meta->bind_member("useInterval", new grt::MetaClass::Property<db_Event,grt::IntegerRef>(getter, setter));
    }
  }
};

class GRT_STRUCTS_DB_PUBLIC db_Trigger : public db_DatabaseDdlObject {
  typedef db_DatabaseDdlObject super;

public:
  db_Trigger(grt::MetaClass *meta = nullptr)
    : db_DatabaseDdlObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _enabled(0),
      _event(""),
      _ordering(""),
      _otherTrigger(""),
      _timing("") {
  }

  virtual ~db_Trigger();

  static std::string static_class_name() {
    return "db.Trigger";
  }

  /**
   * Getter for attribute enabled
   *
   * 
   * \par In Python:
   *    value = obj.enabled
   */
  grt::IntegerRef enabled() const { return _enabled; }

  /**
   * Setter for attribute enabled
   * 
   * 
   * \par In Python:
   *   obj.enabled = value
   */
  virtual void enabled(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_enabled);
    _enabled = value;
    member_changed("enabled", ovalue, value);
  }

  /**
   * Getter for attribute event
   *
   * what fires the trigger (INSERT, UPDATE or DELETE)
   * \par In Python:
   *    value = obj.event
   */
  grt::StringRef event() const { return _event; }

  /**
   * Setter for attribute event
   * 
   * what fires the trigger (INSERT, UPDATE or DELETE)
   * \par In Python:
   *   obj.event = value
   */
  virtual void event(const grt::StringRef &value);

  /**
   * Getter for attribute name
   *
   * 
   * \par In Python:
   *    value = obj.name
   */
  grt::StringRef name() const { return super::name(); }

  /**
   * Setter for attribute name
   * 
   * 
   * \par In Python:
   *   obj.name = value
   */
  virtual void name(const grt::StringRef &value);

  /**
   * Getter for attribute ordering
   *
   * the order in which triggers of the same event and timing are executed (FOLLOWS or PRECEDES)
   * \par In Python:
   *    value = obj.ordering
   */
  grt::StringRef ordering() const { return _ordering; }

  /**
   * Setter for attribute ordering
   * 
   * the order in which triggers of the same event and timing are executed (FOLLOWS or PRECEDES)
   * \par In Python:
   *   obj.ordering = value
   */
  virtual void ordering(const grt::StringRef &value) {
    grt::ValueRef ovalue(_ordering);
    _ordering = value;
    member_changed("ordering", ovalue, value);
  }

  /**
   * Getter for attribute otherTrigger
   *
   * the name of the trigger to which order is relative
   * \par In Python:
   *    value = obj.otherTrigger
   */
  grt::StringRef otherTrigger() const { return _otherTrigger; }

  /**
   * Setter for attribute otherTrigger
   * 
   * the name of the trigger to which order is relative
   * \par In Python:
   *   obj.otherTrigger = value
   */
  virtual void otherTrigger(const grt::StringRef &value) {
    grt::ValueRef ovalue(_otherTrigger);
    _otherTrigger = value;
    member_changed("otherTrigger", ovalue, value);
  }

  /**
   * Getter for attribute timing
   *
   * when the trigger fires (AFTER or BEFORE)
   * \par In Python:
   *    value = obj.timing
   */
  grt::StringRef timing() const { return _timing; }

  /**
   * Setter for attribute timing
   * 
   * when the trigger fires (AFTER or BEFORE)
   * \par In Python:
   *   obj.timing = value
   */
  virtual void timing(const grt::StringRef &value);

  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::IntegerRef _enabled;
  grt::StringRef _event;
  grt::StringRef _ordering;
  grt::StringRef _otherTrigger;
  grt::StringRef _timing;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Trigger());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Trigger::create);
    {
      void (db_Trigger::*setter)(const grt::IntegerRef &) = &db_Trigger::enabled;
      grt::IntegerRef (db_Trigger::*getter)() const = &db_Trigger::enabled;
      meta->bind_member("enabled", new grt::MetaClass::Property<db_Trigger,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_Trigger::*setter)(const grt::StringRef &) = &db_Trigger::event;
      grt::StringRef (db_Trigger::*getter)() const = &db_Trigger::event;
      meta->bind_member("event", new grt::MetaClass::Property<db_Trigger,grt::StringRef>(getter, setter));
    }
    {
      void (db_Trigger::*setter)(const grt::StringRef &) = &db_Trigger::name;
      grt::StringRef (db_Trigger::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<db_Trigger,grt::StringRef>(getter, setter));
    }
    {
      void (db_Trigger::*setter)(const grt::StringRef &) = &db_Trigger::ordering;
      grt::StringRef (db_Trigger::*getter)() const = &db_Trigger::ordering;
      meta->bind_member("ordering", new grt::MetaClass::Property<db_Trigger,grt::StringRef>(getter, setter));
    }
    {
      void (db_Trigger::*setter)(const grt::StringRef &) = &db_Trigger::otherTrigger;
      grt::StringRef (db_Trigger::*getter)() const = &db_Trigger::otherTrigger;
      meta->bind_member("otherTrigger", new grt::MetaClass::Property<db_Trigger,grt::StringRef>(getter, setter));
    }
    {
      void (db_Trigger::*setter)(const grt::StringRef &) = &db_Trigger::timing;
      grt::StringRef (db_Trigger::*getter)() const = &db_Trigger::timing;
      meta->bind_member("timing", new grt::MetaClass::Property<db_Trigger,grt::StringRef>(getter, setter));
    }
  }
};

/** an object that stores information about a database schema routine */
class  db_Routine : public db_DatabaseDdlObject {
  typedef db_DatabaseDdlObject super;

public:
  db_Routine(grt::MetaClass *meta = nullptr)
    : db_DatabaseDdlObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _routineType(""),
      _sequenceNumber(0) {
  }

  static std::string static_class_name() {
    return "db.Routine";
  }

  /**
   * Getter for attribute name
   *
   * the current name of the object
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * the current name of the object
   * \par In Python:
   *   obj.name = value
   */

  /**
   * Getter for attribute routineType
   *
   * 
   * \par In Python:
   *    value = obj.routineType
   */
  grt::StringRef routineType() const { return _routineType; }

  /**
   * Setter for attribute routineType
   * 
   * 
   * \par In Python:
   *   obj.routineType = value
   */
  virtual void routineType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_routineType);
    _routineType = value;
    member_changed("routineType", ovalue, value);
  }

  /**
   * Getter for attribute sequenceNumber
   *
   * defines position in editor
   * \par In Python:
   *    value = obj.sequenceNumber
   */
  grt::IntegerRef sequenceNumber() const { return _sequenceNumber; }

  /**
   * Setter for attribute sequenceNumber
   * 
   * defines position in editor
   * \par In Python:
   *   obj.sequenceNumber = value
   */
  virtual void sequenceNumber(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_sequenceNumber);
    _sequenceNumber = value;
    member_changed("sequenceNumber", ovalue, value);
  }

protected:

  grt::StringRef _routineType;
  grt::IntegerRef _sequenceNumber;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_Routine());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_Routine::create);
    {
      void (db_Routine::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_Routine::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<db_Routine,grt::StringRef>(getter, setter));
    }
    {
      void (db_Routine::*setter)(const grt::StringRef &) = &db_Routine::routineType;
      grt::StringRef (db_Routine::*getter)() const = &db_Routine::routineType;
      meta->bind_member("routineType", new grt::MetaClass::Property<db_Routine,grt::StringRef>(getter, setter));
    }
    {
      void (db_Routine::*setter)(const grt::IntegerRef &) = &db_Routine::sequenceNumber;
      grt::IntegerRef (db_Routine::*getter)() const = &db_Routine::sequenceNumber;
      meta->bind_member("sequenceNumber", new grt::MetaClass::Property<db_Routine,grt::IntegerRef>(getter, setter));
    }
  }
};

/** a object that stores information about a database schema view */
class  db_View : public db_DatabaseDdlObject {
  typedef db_DatabaseDdlObject super;

public:
  db_View(grt::MetaClass *meta = nullptr)
    : db_DatabaseDdlObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _algorithm(0),
      _columns(this, false),
      _isReadOnly(0),
      _oldModelSqlDefinition(""),
      _oldServerSqlDefinition(""),
      _withCheckCondition(0) {
  }

  static std::string static_class_name() {
    return "db.View";
  }

  /**
   * Getter for attribute algorithm
   *
   * 
   * \par In Python:
   *    value = obj.algorithm
   */
  grt::IntegerRef algorithm() const { return _algorithm; }

  /**
   * Setter for attribute algorithm
   * 
   * 
   * \par In Python:
   *   obj.algorithm = value
   */
  virtual void algorithm(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_algorithm);
    _algorithm = value;
    member_changed("algorithm", ovalue, value);
  }

  /**
   * Getter for attribute columns (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.columns
   */
  grt::StringListRef columns() const { return _columns; }


private: // The next attribute is read-only.
  virtual void columns(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_columns);
    _columns = value;
    member_changed("columns", ovalue, value);
  }
public:

  /**
   * Getter for attribute isReadOnly
   *
   * 
   * \par In Python:
   *    value = obj.isReadOnly
   */
  grt::IntegerRef isReadOnly() const { return _isReadOnly; }

  /**
   * Setter for attribute isReadOnly
   * 
   * 
   * \par In Python:
   *   obj.isReadOnly = value
   */
  virtual void isReadOnly(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isReadOnly);
    _isReadOnly = value;
    member_changed("isReadOnly", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * the current name of the object
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * the current name of the object
   * \par In Python:
   *   obj.name = value
   */

  /**
   * Getter for attribute oldModelSqlDefinition
   *
   * this is set at the time of the last sync/rev-eng/fwd-eng to be able to tell if the SQL has been altered when the next sync is performed
   * \par In Python:
   *    value = obj.oldModelSqlDefinition
   */
  grt::StringRef oldModelSqlDefinition() const { return _oldModelSqlDefinition; }

  /**
   * Setter for attribute oldModelSqlDefinition
   * 
   * this is set at the time of the last sync/rev-eng/fwd-eng to be able to tell if the SQL has been altered when the next sync is performed
   * \par In Python:
   *   obj.oldModelSqlDefinition = value
   */
  virtual void oldModelSqlDefinition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_oldModelSqlDefinition);
    _oldModelSqlDefinition = value;
    member_changed("oldModelSqlDefinition", ovalue, value);
  }

  /**
   * Getter for attribute oldServerSqlDefinition
   *
   * this is set at the time of the last sync/rev-eng/fwd-eng to be able to tell if the SQL has been altered when the next sync is performed
   * \par In Python:
   *    value = obj.oldServerSqlDefinition
   */
  grt::StringRef oldServerSqlDefinition() const { return _oldServerSqlDefinition; }

  /**
   * Setter for attribute oldServerSqlDefinition
   * 
   * this is set at the time of the last sync/rev-eng/fwd-eng to be able to tell if the SQL has been altered when the next sync is performed
   * \par In Python:
   *   obj.oldServerSqlDefinition = value
   */
  virtual void oldServerSqlDefinition(const grt::StringRef &value) {
    grt::ValueRef ovalue(_oldServerSqlDefinition);
    _oldServerSqlDefinition = value;
    member_changed("oldServerSqlDefinition", ovalue, value);
  }

  /**
   * Getter for attribute withCheckCondition
   *
   * 
   * \par In Python:
   *    value = obj.withCheckCondition
   */
  grt::IntegerRef withCheckCondition() const { return _withCheckCondition; }

  /**
   * Setter for attribute withCheckCondition
   * 
   * 
   * \par In Python:
   *   obj.withCheckCondition = value
   */
  virtual void withCheckCondition(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_withCheckCondition);
    _withCheckCondition = value;
    member_changed("withCheckCondition", ovalue, value);
  }

protected:

  grt::IntegerRef _algorithm;
  grt::StringListRef _columns;
  grt::IntegerRef _isReadOnly;
  grt::StringRef _oldModelSqlDefinition;
  grt::StringRef _oldServerSqlDefinition;
  grt::IntegerRef _withCheckCondition;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_View());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_View::create);
    {
      void (db_View::*setter)(const grt::IntegerRef &) = &db_View::algorithm;
      grt::IntegerRef (db_View::*getter)() const = &db_View::algorithm;
      meta->bind_member("algorithm", new grt::MetaClass::Property<db_View,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_View::*setter)(const grt::StringListRef &) = &db_View::columns;
      grt::StringListRef (db_View::*getter)() const = &db_View::columns;
      meta->bind_member("columns", new grt::MetaClass::Property<db_View,grt::StringListRef>(getter, setter));
    }
    {
      void (db_View::*setter)(const grt::IntegerRef &) = &db_View::isReadOnly;
      grt::IntegerRef (db_View::*getter)() const = &db_View::isReadOnly;
      meta->bind_member("isReadOnly", new grt::MetaClass::Property<db_View,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_View::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (db_View::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<db_View,grt::StringRef>(getter, setter));
    }
    {
      void (db_View::*setter)(const grt::StringRef &) = &db_View::oldModelSqlDefinition;
      grt::StringRef (db_View::*getter)() const = &db_View::oldModelSqlDefinition;
      meta->bind_member("oldModelSqlDefinition", new grt::MetaClass::Property<db_View,grt::StringRef>(getter, setter));
    }
    {
      void (db_View::*setter)(const grt::StringRef &) = &db_View::oldServerSqlDefinition;
      grt::StringRef (db_View::*getter)() const = &db_View::oldServerSqlDefinition;
      meta->bind_member("oldServerSqlDefinition", new grt::MetaClass::Property<db_View,grt::StringRef>(getter, setter));
    }
    {
      void (db_View::*setter)(const grt::IntegerRef &) = &db_View::withCheckCondition;
      grt::IntegerRef (db_View::*getter)() const = &db_View::withCheckCondition;
      meta->bind_member("withCheckCondition", new grt::MetaClass::Property<db_View,grt::IntegerRef>(getter, setter));
    }
  }
};



inline void register_structs_db_xml() {
  grt::internal::ClassRegistry::register_class<db_DatabaseSyncObject>();
  grt::internal::ClassRegistry::register_class<db_DatabaseSync>();
  grt::internal::ClassRegistry::register_class<db_Script>();
  grt::internal::ClassRegistry::register_class<db_CharacterSet>();
  grt::internal::ClassRegistry::register_class<db_ForeignKey>();
  grt::internal::ClassRegistry::register_class<db_IndexColumn>();
  grt::internal::ClassRegistry::register_class<db_CheckConstraint>();
  grt::internal::ClassRegistry::register_class<db_UserDatatype>();
  grt::internal::ClassRegistry::register_class<db_SimpleDatatype>();
  grt::internal::ClassRegistry::register_class<db_DatatypeGroup>();
  grt::internal::ClassRegistry::register_class<db_Column>();
  grt::internal::ClassRegistry::register_class<db_RolePrivilege>();
  grt::internal::ClassRegistry::register_class<db_Catalog>();
  grt::internal::ClassRegistry::register_class<db_DatabaseObject>();
  grt::internal::ClassRegistry::register_class<db_Sequence>();
  grt::internal::ClassRegistry::register_class<db_Synonym>();
  grt::internal::ClassRegistry::register_class<db_RoutineGroup>();
  grt::internal::ClassRegistry::register_class<db_Index>();
  grt::internal::ClassRegistry::register_class<db_StructuredDatatype>();
  grt::internal::ClassRegistry::register_class<db_Table>();
  grt::internal::ClassRegistry::register_class<db_ServerLink>();
  grt::internal::ClassRegistry::register_class<db_Schema>();
  grt::internal::ClassRegistry::register_class<db_Tablespace>();
  grt::internal::ClassRegistry::register_class<db_LogFileGroup>();
  grt::internal::ClassRegistry::register_class<db_User>();
  grt::internal::ClassRegistry::register_class<db_Role>();
  grt::internal::ClassRegistry::register_class<db_DatabaseDdlObject>();
  grt::internal::ClassRegistry::register_class<db_Event>();
  grt::internal::ClassRegistry::register_class<db_Trigger>();
  grt::internal::ClassRegistry::register_class<db_Routine>();
  grt::internal::ClassRegistry::register_class<db_View>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_db_xml {
  _autoreg__structs_db_xml() {
    register_structs_db_xml();
  }
} __autoreg__structs_db_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

