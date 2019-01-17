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
  #ifdef GRT_STRUCTS_DB_MGMT_EXPORT
  #define GRT_STRUCTS_DB_MGMT_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_DB_MGMT_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_DB_MGMT_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.db.h"

class db_mgmt_SyncProfile;
typedef grt::Ref<db_mgmt_SyncProfile> db_mgmt_SyncProfileRef;
class db_mgmt_ServerInstance;
typedef grt::Ref<db_mgmt_ServerInstance> db_mgmt_ServerInstanceRef;
class db_mgmt_SSHFile;
typedef grt::Ref<db_mgmt_SSHFile> db_mgmt_SSHFileRef;
class db_mgmt_SSHConnection;
typedef grt::Ref<db_mgmt_SSHConnection> db_mgmt_SSHConnectionRef;
class db_mgmt_Connection;
typedef grt::Ref<db_mgmt_Connection> db_mgmt_ConnectionRef;
class db_mgmt_DriverParameter;
typedef grt::Ref<db_mgmt_DriverParameter> db_mgmt_DriverParameterRef;
class db_mgmt_Driver;
typedef grt::Ref<db_mgmt_Driver> db_mgmt_DriverRef;
class db_mgmt_PythonDBAPIDriver;
typedef grt::Ref<db_mgmt_PythonDBAPIDriver> db_mgmt_PythonDBAPIDriverRef;
class db_mgmt_PrivilegeMapping;
typedef grt::Ref<db_mgmt_PrivilegeMapping> db_mgmt_PrivilegeMappingRef;
class db_mgmt_Rdbms;
typedef grt::Ref<db_mgmt_Rdbms> db_mgmt_RdbmsRef;
class db_mgmt_Management;
typedef grt::Ref<db_mgmt_Management> db_mgmt_ManagementRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** DB synchronization profile containing a list last known names for each model object in a equivalent schema in the server */
class  db_mgmt_SyncProfile : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_SyncProfile(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _lastKnownDBNames(this, false),
      _lastKnownViewDefinitions(this, false),
      _lastSyncDate(""),
      _targetHostIdentifier(""),
      _targetSchemaName("") {
  }

  static std::string static_class_name() {
    return "db.mgmt.SyncProfile";
  }

  /**
   * Getter for attribute lastKnownDBNames (read-only)
   *
   * dictionary of object-id to object name values that were last seen in the target DB
   * \par In Python:
   *    value = obj.lastKnownDBNames
   */
  grt::DictRef lastKnownDBNames() const { return _lastKnownDBNames; }


private: // The next attribute is read-only.
  virtual void lastKnownDBNames(const grt::DictRef &value) {
    grt::ValueRef ovalue(_lastKnownDBNames);
    _lastKnownDBNames = value;
    member_changed("lastKnownDBNames", ovalue, value);
  }
public:

  /**
   * Getter for attribute lastKnownViewDefinitions (read-only)
   *
   * dictionary of view object-id to the checksums of the view definitions in both model and server (object-id:model, object-id:server). The canonical location for these values in the object is in oldServerSqlDefinition and oldModelSqlDefinition.
   * \par In Python:
   *    value = obj.lastKnownViewDefinitions
   */
  grt::DictRef lastKnownViewDefinitions() const { return _lastKnownViewDefinitions; }


private: // The next attribute is read-only.
  virtual void lastKnownViewDefinitions(const grt::DictRef &value) {
    grt::ValueRef ovalue(_lastKnownViewDefinitions);
    _lastKnownViewDefinitions = value;
    member_changed("lastKnownViewDefinitions", ovalue, value);
  }
public:

  /**
   * Getter for attribute lastSyncDate
   *
   * last date/time that the model was synchronized to this target
   * \par In Python:
   *    value = obj.lastSyncDate
   */
  grt::StringRef lastSyncDate() const { return _lastSyncDate; }

  /**
   * Setter for attribute lastSyncDate
   * 
   * last date/time that the model was synchronized to this target
   * \par In Python:
   *   obj.lastSyncDate = value
   */
  virtual void lastSyncDate(const grt::StringRef &value) {
    grt::ValueRef ovalue(_lastSyncDate);
    _lastSyncDate = value;
    member_changed("lastSyncDate", ovalue, value);
  }

  /**
   * Getter for attribute targetHostIdentifier
   *
   * identifier for the target DB server
   * \par In Python:
   *    value = obj.targetHostIdentifier
   */
  grt::StringRef targetHostIdentifier() const { return _targetHostIdentifier; }

  /**
   * Setter for attribute targetHostIdentifier
   * 
   * identifier for the target DB server
   * \par In Python:
   *   obj.targetHostIdentifier = value
   */
  virtual void targetHostIdentifier(const grt::StringRef &value) {
    grt::ValueRef ovalue(_targetHostIdentifier);
    _targetHostIdentifier = value;
    member_changed("targetHostIdentifier", ovalue, value);
  }

  /**
   * Getter for attribute targetSchemaName
   *
   * name of the target schema in the DB server
   * \par In Python:
   *    value = obj.targetSchemaName
   */
  grt::StringRef targetSchemaName() const { return _targetSchemaName; }

  /**
   * Setter for attribute targetSchemaName
   * 
   * name of the target schema in the DB server
   * \par In Python:
   *   obj.targetSchemaName = value
   */
  virtual void targetSchemaName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_targetSchemaName);
    _targetSchemaName = value;
    member_changed("targetSchemaName", ovalue, value);
  }

protected:

  grt::DictRef _lastKnownDBNames;
  grt::DictRef _lastKnownViewDefinitions;
  grt::StringRef _lastSyncDate;
  grt::StringRef _targetHostIdentifier;
  grt::StringRef _targetSchemaName;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_SyncProfile());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_SyncProfile::create);
    {
      void (db_mgmt_SyncProfile::*setter)(const grt::DictRef &) = &db_mgmt_SyncProfile::lastKnownDBNames;
      grt::DictRef (db_mgmt_SyncProfile::*getter)() const = &db_mgmt_SyncProfile::lastKnownDBNames;
      meta->bind_member("lastKnownDBNames", new grt::MetaClass::Property<db_mgmt_SyncProfile,grt::DictRef>(getter, setter));
    }
    {
      void (db_mgmt_SyncProfile::*setter)(const grt::DictRef &) = &db_mgmt_SyncProfile::lastKnownViewDefinitions;
      grt::DictRef (db_mgmt_SyncProfile::*getter)() const = &db_mgmt_SyncProfile::lastKnownViewDefinitions;
      meta->bind_member("lastKnownViewDefinitions", new grt::MetaClass::Property<db_mgmt_SyncProfile,grt::DictRef>(getter, setter));
    }
    {
      void (db_mgmt_SyncProfile::*setter)(const grt::StringRef &) = &db_mgmt_SyncProfile::lastSyncDate;
      grt::StringRef (db_mgmt_SyncProfile::*getter)() const = &db_mgmt_SyncProfile::lastSyncDate;
      meta->bind_member("lastSyncDate", new grt::MetaClass::Property<db_mgmt_SyncProfile,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_SyncProfile::*setter)(const grt::StringRef &) = &db_mgmt_SyncProfile::targetHostIdentifier;
      grt::StringRef (db_mgmt_SyncProfile::*getter)() const = &db_mgmt_SyncProfile::targetHostIdentifier;
      meta->bind_member("targetHostIdentifier", new grt::MetaClass::Property<db_mgmt_SyncProfile,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_SyncProfile::*setter)(const grt::StringRef &) = &db_mgmt_SyncProfile::targetSchemaName;
      grt::StringRef (db_mgmt_SyncProfile::*getter)() const = &db_mgmt_SyncProfile::targetSchemaName;
      meta->bind_member("targetSchemaName", new grt::MetaClass::Property<db_mgmt_SyncProfile,grt::StringRef>(getter, setter));
    }
  }
};

/** DB server connection and management information */
class  db_mgmt_ServerInstance : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_ServerInstance(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _loginInfo(this, false),
      _serverInfo(this, false) {
  }

  static std::string static_class_name() {
    return "db.mgmt.ServerInstance";
  }

  /**
   * Getter for attribute connection
   *
   * 
   * \par In Python:
   *    value = obj.connection
   */
  db_mgmt_ConnectionRef connection() const { return _connection; }

  /**
   * Setter for attribute connection
   * 
   * 
   * \par In Python:
   *   obj.connection = value
   */
  virtual void connection(const db_mgmt_ConnectionRef &value) {
    grt::ValueRef ovalue(_connection);
    _connection = value;
    member_changed("connection", ovalue, value);
  }

  /**
   * Getter for attribute loginInfo (read-only)
   *
   * login information to the server
   * \par In Python:
   *    value = obj.loginInfo
   */
  grt::DictRef loginInfo() const { return _loginInfo; }


private: // The next attribute is read-only.
  virtual void loginInfo(const grt::DictRef &value) {
    grt::ValueRef ovalue(_loginInfo);
    _loginInfo = value;
    member_changed("loginInfo", ovalue, value);
  }
public:

  /**
   * Getter for attribute serverInfo (read-only)
   *
   * server configuration information
   * \par In Python:
   *    value = obj.serverInfo
   */
  grt::DictRef serverInfo() const { return _serverInfo; }


private: // The next attribute is read-only.
  virtual void serverInfo(const grt::DictRef &value) {
    grt::ValueRef ovalue(_serverInfo);
    _serverInfo = value;
    member_changed("serverInfo", ovalue, value);
  }
public:

protected:

  db_mgmt_ConnectionRef _connection;
  grt::DictRef _loginInfo;
  grt::DictRef _serverInfo;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_ServerInstance());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_ServerInstance::create);
    {
      void (db_mgmt_ServerInstance::*setter)(const db_mgmt_ConnectionRef &) = &db_mgmt_ServerInstance::connection;
      db_mgmt_ConnectionRef (db_mgmt_ServerInstance::*getter)() const = &db_mgmt_ServerInstance::connection;
      meta->bind_member("connection", new grt::MetaClass::Property<db_mgmt_ServerInstance,db_mgmt_ConnectionRef>(getter, setter));
    }
    {
      void (db_mgmt_ServerInstance::*setter)(const grt::DictRef &) = &db_mgmt_ServerInstance::loginInfo;
      grt::DictRef (db_mgmt_ServerInstance::*getter)() const = &db_mgmt_ServerInstance::loginInfo;
      meta->bind_member("loginInfo", new grt::MetaClass::Property<db_mgmt_ServerInstance,grt::DictRef>(getter, setter));
    }
    {
      void (db_mgmt_ServerInstance::*setter)(const grt::DictRef &) = &db_mgmt_ServerInstance::serverInfo;
      grt::DictRef (db_mgmt_ServerInstance::*getter)() const = &db_mgmt_ServerInstance::serverInfo;
      meta->bind_member("serverInfo", new grt::MetaClass::Property<db_mgmt_ServerInstance,grt::DictRef>(getter, setter));
    }
  }
};

/** a proxy to and instance that provide access to remote file. This object cannot be instantiated directly. */
class GRT_STRUCTS_DB_MGMT_PUBLIC db_mgmt_SSHFile : public GrtObject {
  typedef GrtObject super;

public:
  class ImplData;
  friend class ImplData;
  db_mgmt_SSHFile(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~db_mgmt_SSHFile();

  static std::string static_class_name() {
    return "db.mgmt.SSHFile";
  }

  /**
   * Method. get path for the file
   * \return 
   */
  virtual grt::StringRef getPath();
  /**
   * Method. read up to length bytes from this file.
   * \param length 
   * \return 
   */
  virtual grt::StringRef read(ssize_t length);
  /**
   * Method. read from file until line termination is found '\n'
   * \return 
   */
  virtual grt::StringRef readline();
  /**
   * Method. reposition the file's current position.
   * \param offset 
   * \return 
   */
  virtual grt::IntegerRef seek(ssize_t offset);
  /**
   * Method. return the file's current position.
   * \return 
   */
  virtual grt::IntegerRef tell();

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_SSHFile());
  }

  static grt::ValueRef call_getPath(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHFile*>(self)->getPath(); }

  static grt::ValueRef call_read(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHFile*>(self)->read(grt::IntegerRef::cast_from(args[0])); }

  static grt::ValueRef call_readline(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHFile*>(self)->readline(); }

  static grt::ValueRef call_seek(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHFile*>(self)->seek(grt::IntegerRef::cast_from(args[0])); }

  static grt::ValueRef call_tell(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHFile*>(self)->tell(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_SSHFile::create);
    meta->bind_method("getPath", &db_mgmt_SSHFile::call_getPath);
    meta->bind_method("read", &db_mgmt_SSHFile::call_read);
    meta->bind_method("readline", &db_mgmt_SSHFile::call_readline);
    meta->bind_method("seek", &db_mgmt_SSHFile::call_seek);
    meta->bind_method("tell", &db_mgmt_SSHFile::call_tell);
  }
};

/** a proxy to and instance that provide access to remote server. This object cannot be instantiated directly. */
class GRT_STRUCTS_DB_MGMT_PUBLIC db_mgmt_SSHConnection : public GrtObject {
  typedef GrtObject super;

public:
  class ImplData;
  friend class ImplData;
  db_mgmt_SSHConnection(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~db_mgmt_SSHConnection();

  static std::string static_class_name() {
    return "db.mgmt.SSHConnection";
  }

  /**
   * Method. change current working directory
   * \param directory new location
   * \return indicator whenever change was successfull
   */
  virtual grt::IntegerRef cd(const std::string &directory);
  /**
   * Method. make connection to remote server
   * \return 
   */
  virtual grt::IntegerRef connect();
  /**
   * Method. disconnect ssh connection
   * \return 
   */
  virtual void disconnect();
  /**
   * Method. execute command on the remote server
   * \param text the command to be executed on the server
   * \return command output
   */
  virtual grt::DictRef executeCommand(const std::string &text);
  /**
   * Method. execute command on the remote server using sudo
   * \param command the command to be executed on the server
   * \param user the user which should execute the command
   * \return command output
   */
  virtual grt::DictRef executeSudoCommand(const std::string &command, const std::string &user);
  /**
   * Method. check if given filename exists
   * \param path path to remote file
   * \return indicator whenever file exists
   */
  virtual grt::IntegerRef fileExists(const std::string &path);
  /**
   * Method. download remote file
   * \param src remote file path
   * \param dest local file path
   * \return 
   */
  virtual void get(const std::string &src, const std::string &dest);
  /**
   * Method. fetch remote file into variable
   * \param src remote file path
   * \return remote file content
   */
  virtual grt::StringRef getContent(const std::string &src);
  /**
   * Method. check if connection is active
   * \return 
   */
  virtual grt::IntegerRef isConnected();
  /**
   * Method. list remote directory contents
   * \param path remote location
   * \return 
   */
  virtual grt::DictListRef ls(const std::string &path);
  /**
   * Method. create new directory on remote host
   * \param directory new directory name or absolute path to the new directory
   * \return 
   */
  virtual void mkdir(const std::string &directory);
  /**
   * Method. open remote file
   * \param path remote file location
   * \return 
   */
  virtual db_mgmt_SSHFileRef open(const std::string &path);
  /**
   * Method. upload file to remote location
   * \param src local file path
   * \param dest remote file path
   * \return 
   */
  virtual void put(const std::string &src, const std::string &dest);
  /**
   * Method. get current working directory
   * \return current working directory
   */
  virtual grt::StringRef pwd();
  /**
   * Method. remove remote directory
   * \param directory directory name or absolute path to the directory
   * \return 
   */
  virtual void rmdir(const std::string &directory);
  /**
   * Method. create remote file with content
   * \param path remote file path
   * \param content remote file content
   * \return 
   */
  virtual void setContent(const std::string &path, const std::string &content);
  /**
   * Method. get remote path details
   * \param path path to remote directory or file
   * \return Dict object with file attributes
   */
  virtual grt::DictRef stat(const std::string &path);
  /**
   * Method. remove remote file
   * \param file filename or absolute path to the file
   * \return 
   */
  virtual void unlink(const std::string &file);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_SSHConnection());
  }

  static grt::ValueRef call_cd(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->cd(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_connect(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->connect(); }

  static grt::ValueRef call_disconnect(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_mgmt_SSHConnection*>(self)->disconnect(); return grt::ValueRef(); }

  static grt::ValueRef call_executeCommand(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->executeCommand(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_executeSudoCommand(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->executeSudoCommand(grt::StringRef::cast_from(args[0]), grt::StringRef::cast_from(args[1])); }

  static grt::ValueRef call_fileExists(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->fileExists(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_get(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_mgmt_SSHConnection*>(self)->get(grt::StringRef::cast_from(args[0]), grt::StringRef::cast_from(args[1])); return grt::ValueRef(); }

  static grt::ValueRef call_getContent(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->getContent(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_isConnected(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->isConnected(); }

  static grt::ValueRef call_ls(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->ls(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_mkdir(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_mgmt_SSHConnection*>(self)->mkdir(grt::StringRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_open(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->open(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_put(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_mgmt_SSHConnection*>(self)->put(grt::StringRef::cast_from(args[0]), grt::StringRef::cast_from(args[1])); return grt::ValueRef(); }

  static grt::ValueRef call_pwd(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->pwd(); }

  static grt::ValueRef call_rmdir(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_mgmt_SSHConnection*>(self)->rmdir(grt::StringRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_setContent(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_mgmt_SSHConnection*>(self)->setContent(grt::StringRef::cast_from(args[0]), grt::StringRef::cast_from(args[1])); return grt::ValueRef(); }

  static grt::ValueRef call_stat(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<db_mgmt_SSHConnection*>(self)->stat(grt::StringRef::cast_from(args[0])); }

  static grt::ValueRef call_unlink(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_mgmt_SSHConnection*>(self)->unlink(grt::StringRef::cast_from(args[0])); return grt::ValueRef(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_SSHConnection::create);
    meta->bind_method("cd", &db_mgmt_SSHConnection::call_cd);
    meta->bind_method("connect", &db_mgmt_SSHConnection::call_connect);
    meta->bind_method("disconnect", &db_mgmt_SSHConnection::call_disconnect);
    meta->bind_method("executeCommand", &db_mgmt_SSHConnection::call_executeCommand);
    meta->bind_method("executeSudoCommand", &db_mgmt_SSHConnection::call_executeSudoCommand);
    meta->bind_method("fileExists", &db_mgmt_SSHConnection::call_fileExists);
    meta->bind_method("get", &db_mgmt_SSHConnection::call_get);
    meta->bind_method("getContent", &db_mgmt_SSHConnection::call_getContent);
    meta->bind_method("isConnected", &db_mgmt_SSHConnection::call_isConnected);
    meta->bind_method("ls", &db_mgmt_SSHConnection::call_ls);
    meta->bind_method("mkdir", &db_mgmt_SSHConnection::call_mkdir);
    meta->bind_method("open", &db_mgmt_SSHConnection::call_open);
    meta->bind_method("put", &db_mgmt_SSHConnection::call_put);
    meta->bind_method("pwd", &db_mgmt_SSHConnection::call_pwd);
    meta->bind_method("rmdir", &db_mgmt_SSHConnection::call_rmdir);
    meta->bind_method("setContent", &db_mgmt_SSHConnection::call_setContent);
    meta->bind_method("stat", &db_mgmt_SSHConnection::call_stat);
    meta->bind_method("unlink", &db_mgmt_SSHConnection::call_unlink);
  }
};

/** a stored RDBMS connection */
class  db_mgmt_Connection : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_Connection(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _hostIdentifier(""),
      _isDefault(0),
      _modules(this, false),
      _parameterValues(this, false) {
  }

  static std::string static_class_name() {
    return "db.mgmt.Connection";
  }

  /**
   * Getter for attribute driver
   *
   * the driver used to connect
   * \par In Python:
   *    value = obj.driver
   */
  db_mgmt_DriverRef driver() const { return _driver; }

  /**
   * Setter for attribute driver
   * 
   * the driver used to connect
   * \par In Python:
   *   obj.driver = value
   */
  virtual void driver(const db_mgmt_DriverRef &value) {
    grt::ValueRef ovalue(_driver);
    _driver = value;
    member_changed("driver", ovalue, value);
  }

  /**
   * Getter for attribute hostIdentifier
   *
   * identifier to be used for storing password
   * \par In Python:
   *    value = obj.hostIdentifier
   */
  grt::StringRef hostIdentifier() const { return _hostIdentifier; }

  /**
   * Setter for attribute hostIdentifier
   * 
   * identifier to be used for storing password
   * \par In Python:
   *   obj.hostIdentifier = value
   */
  virtual void hostIdentifier(const grt::StringRef &value) {
    grt::ValueRef ovalue(_hostIdentifier);
    _hostIdentifier = value;
    member_changed("hostIdentifier", ovalue, value);
  }

  /**
   * Getter for attribute isDefault
   *
   * 
   * \par In Python:
   *    value = obj.isDefault
   */
  grt::IntegerRef isDefault() const { return _isDefault; }

  /**
   * Setter for attribute isDefault
   * 
   * 
   * \par In Python:
   *   obj.isDefault = value
   */
  virtual void isDefault(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_isDefault);
    _isDefault = value;
    member_changed("isDefault", ovalue, value);
  }

  /**
   * Getter for attribute modules (read-only)
   *
   * the modules used for this connection
   * \par In Python:
   *    value = obj.modules
   */
  grt::DictRef modules() const { return _modules; }


private: // The next attribute is read-only.
  virtual void modules(const grt::DictRef &value) {
    grt::ValueRef ovalue(_modules);
    _modules = value;
    member_changed("modules", ovalue, value);
  }
public:

  /**
   * Getter for attribute parameterValues (read-only)
   *
   * the parameters the user entered
   * \par In Python:
   *    value = obj.parameterValues
   */
  grt::DictRef parameterValues() const { return _parameterValues; }


private: // The next attribute is read-only.
  virtual void parameterValues(const grt::DictRef &value) {
    grt::ValueRef ovalue(_parameterValues);
    _parameterValues = value;
    member_changed("parameterValues", ovalue, value);
  }
public:

protected:

  db_mgmt_DriverRef _driver;
  grt::StringRef _hostIdentifier;
  grt::IntegerRef _isDefault;
  grt::DictRef _modules;
  grt::DictRef _parameterValues;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_Connection());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_Connection::create);
    {
      void (db_mgmt_Connection::*setter)(const db_mgmt_DriverRef &) = &db_mgmt_Connection::driver;
      db_mgmt_DriverRef (db_mgmt_Connection::*getter)() const = &db_mgmt_Connection::driver;
      meta->bind_member("driver", new grt::MetaClass::Property<db_mgmt_Connection,db_mgmt_DriverRef>(getter, setter));
    }
    {
      void (db_mgmt_Connection::*setter)(const grt::StringRef &) = &db_mgmt_Connection::hostIdentifier;
      grt::StringRef (db_mgmt_Connection::*getter)() const = &db_mgmt_Connection::hostIdentifier;
      meta->bind_member("hostIdentifier", new grt::MetaClass::Property<db_mgmt_Connection,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Connection::*setter)(const grt::IntegerRef &) = &db_mgmt_Connection::isDefault;
      grt::IntegerRef (db_mgmt_Connection::*getter)() const = &db_mgmt_Connection::isDefault;
      meta->bind_member("isDefault", new grt::MetaClass::Property<db_mgmt_Connection,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mgmt_Connection::*setter)(const grt::DictRef &) = &db_mgmt_Connection::modules;
      grt::DictRef (db_mgmt_Connection::*getter)() const = &db_mgmt_Connection::modules;
      meta->bind_member("modules", new grt::MetaClass::Property<db_mgmt_Connection,grt::DictRef>(getter, setter));
    }
    {
      void (db_mgmt_Connection::*setter)(const grt::DictRef &) = &db_mgmt_Connection::parameterValues;
      grt::DictRef (db_mgmt_Connection::*getter)() const = &db_mgmt_Connection::parameterValues;
      meta->bind_member("parameterValues", new grt::MetaClass::Property<db_mgmt_Connection,grt::DictRef>(getter, setter));
    }
  }
};

/** a list of all parameters the Jdbc driver supports */
class  db_mgmt_DriverParameter : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_DriverParameter(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _accessibilityName(""),
      _caption(""),
      _defaultValue(""),
      _description(""),
      _layoutAdvanced(0),
      _layoutRow(0),
      _layoutWidth(0),
      _lookupValueMethod(""),
      _lookupValueModule(""),
      _paramType(""),
      _paramTypeDetails(this, false),
      _required(0) {
  }

  static std::string static_class_name() {
    return "db.mgmt.DriverParameter";
  }

  /**
   * Getter for attribute accessibilityName
   *
   * accessibility name
   * \par In Python:
   *    value = obj.accessibilityName
   */
  grt::StringRef accessibilityName() const { return _accessibilityName; }

  /**
   * Setter for attribute accessibilityName
   * 
   * accessibility name
   * \par In Python:
   *   obj.accessibilityName = value
   */
  virtual void accessibilityName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_accessibilityName);
    _accessibilityName = value;
    member_changed("accessibilityName", ovalue, value);
  }

  /**
   * Getter for attribute caption
   *
   * the caption displayed in the connection dialog
   * \par In Python:
   *    value = obj.caption
   */
  grt::StringRef caption() const { return _caption; }

  /**
   * Setter for attribute caption
   * 
   * the caption displayed in the connection dialog
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
   * the default value of the parameter
   * \par In Python:
   *    value = obj.defaultValue
   */
  grt::StringRef defaultValue() const { return _defaultValue; }

  /**
   * Setter for attribute defaultValue
   * 
   * the default value of the parameter
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
   * the description displayed in the connection dialog
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * the description displayed in the connection dialog
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /**
   * Getter for attribute layoutAdvanced
   *
   * when set to 1 this is paramter is only displayed in the advanced parameter section
   * \par In Python:
   *    value = obj.layoutAdvanced
   */
  grt::IntegerRef layoutAdvanced() const { return _layoutAdvanced; }

  /**
   * Setter for attribute layoutAdvanced
   * 
   * when set to 1 this is paramter is only displayed in the advanced parameter section
   * \par In Python:
   *   obj.layoutAdvanced = value
   */
  virtual void layoutAdvanced(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_layoutAdvanced);
    _layoutAdvanced = value;
    member_changed("layoutAdvanced", ovalue, value);
  }

  /**
   * Getter for attribute layoutRow
   *
   * the row the parameter is displayed. There can be more than one parameters on the same row. When set to -1 the parameter is appended at the end of the parameter list
   * \par In Python:
   *    value = obj.layoutRow
   */
  grt::IntegerRef layoutRow() const { return _layoutRow; }

  /**
   * Setter for attribute layoutRow
   * 
   * the row the parameter is displayed. There can be more than one parameters on the same row. When set to -1 the parameter is appended at the end of the parameter list
   * \par In Python:
   *   obj.layoutRow = value
   */
  virtual void layoutRow(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_layoutRow);
    _layoutRow = value;
    member_changed("layoutRow", ovalue, value);
  }

  /**
   * Getter for attribute layoutWidth
   *
   * the width of the edit
   * \par In Python:
   *    value = obj.layoutWidth
   */
  grt::IntegerRef layoutWidth() const { return _layoutWidth; }

  /**
   * Setter for attribute layoutWidth
   * 
   * the width of the edit
   * \par In Python:
   *   obj.layoutWidth = value
   */
  virtual void layoutWidth(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_layoutWidth);
    _layoutWidth = value;
    member_changed("layoutWidth", ovalue, value);
  }

  /**
   * Getter for attribute lookupValueMethod
   *
   * the method to call to get the list of possible values
   * \par In Python:
   *    value = obj.lookupValueMethod
   */
  grt::StringRef lookupValueMethod() const { return _lookupValueMethod; }

  /**
   * Setter for attribute lookupValueMethod
   * 
   * the method to call to get the list of possible values
   * \par In Python:
   *   obj.lookupValueMethod = value
   */
  virtual void lookupValueMethod(const grt::StringRef &value) {
    grt::ValueRef ovalue(_lookupValueMethod);
    _lookupValueMethod = value;
    member_changed("lookupValueMethod", ovalue, value);
  }

  /**
   * Getter for attribute lookupValueModule
   *
   * the module that contains the method to call to get the list of possible values
   * \par In Python:
   *    value = obj.lookupValueModule
   */
  grt::StringRef lookupValueModule() const { return _lookupValueModule; }

  /**
   * Setter for attribute lookupValueModule
   * 
   * the module that contains the method to call to get the list of possible values
   * \par In Python:
   *   obj.lookupValueModule = value
   */
  virtual void lookupValueModule(const grt::StringRef &value) {
    grt::ValueRef ovalue(_lookupValueModule);
    _lookupValueModule = value;
    member_changed("lookupValueModule", ovalue, value);
  }

  /**
   * Getter for attribute paramType
   *
   * can be string, int, boolean, tristate, file, dir
   * \par In Python:
   *    value = obj.paramType
   */
  grt::StringRef paramType() const { return _paramType; }

  /**
   * Setter for attribute paramType
   * 
   * can be string, int, boolean, tristate, file, dir
   * \par In Python:
   *   obj.paramType = value
   */
  virtual void paramType(const grt::StringRef &value) {
    grt::ValueRef ovalue(_paramType);
    _paramType = value;
    member_changed("paramType", ovalue, value);
  }

  /**
   * Getter for attribute paramTypeDetails (read-only)
   *
   * additional information e.g. like file extension
   * \par In Python:
   *    value = obj.paramTypeDetails
   */
  grt::DictRef paramTypeDetails() const { return _paramTypeDetails; }


private: // The next attribute is read-only.
  virtual void paramTypeDetails(const grt::DictRef &value) {
    grt::ValueRef ovalue(_paramTypeDetails);
    _paramTypeDetails = value;
    member_changed("paramTypeDetails", ovalue, value);
  }
public:

  /**
   * Getter for attribute required
   *
   * if set to 1 this parameter is a required parameter
   * \par In Python:
   *    value = obj.required
   */
  grt::IntegerRef required() const { return _required; }

  /**
   * Setter for attribute required
   * 
   * if set to 1 this parameter is a required parameter
   * \par In Python:
   *   obj.required = value
   */
  virtual void required(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_required);
    _required = value;
    member_changed("required", ovalue, value);
  }

protected:

  grt::StringRef _accessibilityName;
  grt::StringRef _caption;
  grt::StringRef _defaultValue;
  grt::StringRef _description;
  grt::IntegerRef _layoutAdvanced;
  grt::IntegerRef _layoutRow;
  grt::IntegerRef _layoutWidth;
  grt::StringRef _lookupValueMethod;
  grt::StringRef _lookupValueModule;
  grt::StringRef _paramType;
  grt::DictRef _paramTypeDetails;
  grt::IntegerRef _required;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_DriverParameter());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_DriverParameter::create);
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::StringRef &) = &db_mgmt_DriverParameter::accessibilityName;
      grt::StringRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::accessibilityName;
      meta->bind_member("accessibilityName", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::StringRef &) = &db_mgmt_DriverParameter::caption;
      grt::StringRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::StringRef &) = &db_mgmt_DriverParameter::defaultValue;
      grt::StringRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::defaultValue;
      meta->bind_member("defaultValue", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::StringRef &) = &db_mgmt_DriverParameter::description;
      grt::StringRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::description;
      meta->bind_member("description", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::IntegerRef &) = &db_mgmt_DriverParameter::layoutAdvanced;
      grt::IntegerRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::layoutAdvanced;
      meta->bind_member("layoutAdvanced", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::IntegerRef &) = &db_mgmt_DriverParameter::layoutRow;
      grt::IntegerRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::layoutRow;
      meta->bind_member("layoutRow", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::IntegerRef &) = &db_mgmt_DriverParameter::layoutWidth;
      grt::IntegerRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::layoutWidth;
      meta->bind_member("layoutWidth", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::StringRef &) = &db_mgmt_DriverParameter::lookupValueMethod;
      grt::StringRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::lookupValueMethod;
      meta->bind_member("lookupValueMethod", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::StringRef &) = &db_mgmt_DriverParameter::lookupValueModule;
      grt::StringRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::lookupValueModule;
      meta->bind_member("lookupValueModule", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::StringRef &) = &db_mgmt_DriverParameter::paramType;
      grt::StringRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::paramType;
      meta->bind_member("paramType", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::DictRef &) = &db_mgmt_DriverParameter::paramTypeDetails;
      grt::DictRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::paramTypeDetails;
      meta->bind_member("paramTypeDetails", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::DictRef>(getter, setter));
    }
    {
      void (db_mgmt_DriverParameter::*setter)(const grt::IntegerRef &) = &db_mgmt_DriverParameter::required;
      grt::IntegerRef (db_mgmt_DriverParameter::*getter)() const = &db_mgmt_DriverParameter::required;
      meta->bind_member("required", new grt::MetaClass::Property<db_mgmt_DriverParameter,grt::IntegerRef>(getter, setter));
    }
  }
};

/** information about a database driver */
class  db_mgmt_Driver : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_Driver(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _description(""),
      _driverLibraryName(""),
      _files(this, false),
      _filesTarget(""),
      _hostIdentifierTemplate(""),
      _parameters(this, false) {
  }

  static std::string static_class_name() {
    return "db.mgmt.Driver";
  }

  /**
   * Getter for attribute caption
   *
   * the caption that is displayed in the UI
   * \par In Python:
   *    value = obj.caption
   */
  grt::StringRef caption() const { return _caption; }

  /**
   * Setter for attribute caption
   * 
   * the caption that is displayed in the UI
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
   * a short description of the driver
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * a short description of the driver
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /**
   * Getter for attribute driverLibraryName
   *
   * location of the driver library
   * \par In Python:
   *    value = obj.driverLibraryName
   */
  grt::StringRef driverLibraryName() const { return _driverLibraryName; }

  /**
   * Setter for attribute driverLibraryName
   * 
   * location of the driver library
   * \par In Python:
   *   obj.driverLibraryName = value
   */
  virtual void driverLibraryName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_driverLibraryName);
    _driverLibraryName = value;
    member_changed("driverLibraryName", ovalue, value);
  }

  /**
   * Getter for attribute files (read-only)
   *
   * filename(s) of the driver
   * \par In Python:
   *    value = obj.files
   */
  grt::StringListRef files() const { return _files; }


private: // The next attribute is read-only.
  virtual void files(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_files);
    _files = value;
    member_changed("files", ovalue, value);
  }
public:

  /**
   * Getter for attribute filesTarget
   *
   * location where the driver files are installed
   * \par In Python:
   *    value = obj.filesTarget
   */
  grt::StringRef filesTarget() const { return _filesTarget; }

  /**
   * Setter for attribute filesTarget
   * 
   * location where the driver files are installed
   * \par In Python:
   *   obj.filesTarget = value
   */
  virtual void filesTarget(const grt::StringRef &value) {
    grt::ValueRef ovalue(_filesTarget);
    _filesTarget = value;
    member_changed("filesTarget", ovalue, value);
  }

  /**
   * Getter for attribute hostIdentifierTemplate
   *
   * 
   * \par In Python:
   *    value = obj.hostIdentifierTemplate
   */
  grt::StringRef hostIdentifierTemplate() const { return _hostIdentifierTemplate; }

  /**
   * Setter for attribute hostIdentifierTemplate
   * 
   * 
   * \par In Python:
   *   obj.hostIdentifierTemplate = value
   */
  virtual void hostIdentifierTemplate(const grt::StringRef &value) {
    grt::ValueRef ovalue(_hostIdentifierTemplate);
    _hostIdentifierTemplate = value;
    member_changed("hostIdentifierTemplate", ovalue, value);
  }

  // parameters is owned by db_mgmt_Driver
  /**
   * Getter for attribute parameters (read-only)
   *
   * the parameters the driver supports
   * \par In Python:
   *    value = obj.parameters
   */
  grt::ListRef<db_mgmt_DriverParameter> parameters() const { return _parameters; }


private: // The next attribute is read-only.
  virtual void parameters(const grt::ListRef<db_mgmt_DriverParameter> &value) {
    grt::ValueRef ovalue(_parameters);

    _parameters = value;
    owned_member_changed("parameters", ovalue, value);
  }
public:

protected:

  grt::StringRef _caption;
  grt::StringRef _description;
  grt::StringRef _driverLibraryName;
  grt::StringListRef _files;
  grt::StringRef _filesTarget;
  grt::StringRef _hostIdentifierTemplate;
  grt::ListRef<db_mgmt_DriverParameter> _parameters;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_Driver());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_Driver::create);
    {
      void (db_mgmt_Driver::*setter)(const grt::StringRef &) = &db_mgmt_Driver::caption;
      grt::StringRef (db_mgmt_Driver::*getter)() const = &db_mgmt_Driver::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<db_mgmt_Driver,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Driver::*setter)(const grt::StringRef &) = &db_mgmt_Driver::description;
      grt::StringRef (db_mgmt_Driver::*getter)() const = &db_mgmt_Driver::description;
      meta->bind_member("description", new grt::MetaClass::Property<db_mgmt_Driver,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Driver::*setter)(const grt::StringRef &) = &db_mgmt_Driver::driverLibraryName;
      grt::StringRef (db_mgmt_Driver::*getter)() const = &db_mgmt_Driver::driverLibraryName;
      meta->bind_member("driverLibraryName", new grt::MetaClass::Property<db_mgmt_Driver,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Driver::*setter)(const grt::StringListRef &) = &db_mgmt_Driver::files;
      grt::StringListRef (db_mgmt_Driver::*getter)() const = &db_mgmt_Driver::files;
      meta->bind_member("files", new grt::MetaClass::Property<db_mgmt_Driver,grt::StringListRef>(getter, setter));
    }
    {
      void (db_mgmt_Driver::*setter)(const grt::StringRef &) = &db_mgmt_Driver::filesTarget;
      grt::StringRef (db_mgmt_Driver::*getter)() const = &db_mgmt_Driver::filesTarget;
      meta->bind_member("filesTarget", new grt::MetaClass::Property<db_mgmt_Driver,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Driver::*setter)(const grt::StringRef &) = &db_mgmt_Driver::hostIdentifierTemplate;
      grt::StringRef (db_mgmt_Driver::*getter)() const = &db_mgmt_Driver::hostIdentifierTemplate;
      meta->bind_member("hostIdentifierTemplate", new grt::MetaClass::Property<db_mgmt_Driver,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Driver::*setter)(const grt::ListRef<db_mgmt_DriverParameter> &) = &db_mgmt_Driver::parameters;
      grt::ListRef<db_mgmt_DriverParameter> (db_mgmt_Driver::*getter)() const = &db_mgmt_Driver::parameters;
      meta->bind_member("parameters", new grt::MetaClass::Property<db_mgmt_Driver,grt::ListRef<db_mgmt_DriverParameter>>(getter, setter));
    }
  }
};

/** information about a Python DB 2.0 API compliant driver */
class  db_mgmt_PythonDBAPIDriver : public db_mgmt_Driver {
  typedef db_mgmt_Driver super;

public:
  db_mgmt_PythonDBAPIDriver(grt::MetaClass *meta = nullptr)
    : db_mgmt_Driver(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _connectionStringTemplate("") {
  }

  static std::string static_class_name() {
    return "db.mgmt.PythonDBAPIDriver";
  }

  /**
   * Getter for attribute connectionStringTemplate
   *
   * the template used to build the connection parameter
   * \par In Python:
   *    value = obj.connectionStringTemplate
   */
  grt::StringRef connectionStringTemplate() const { return _connectionStringTemplate; }

  /**
   * Setter for attribute connectionStringTemplate
   * 
   * the template used to build the connection parameter
   * \par In Python:
   *   obj.connectionStringTemplate = value
   */
  virtual void connectionStringTemplate(const grt::StringRef &value) {
    grt::ValueRef ovalue(_connectionStringTemplate);
    _connectionStringTemplate = value;
    member_changed("connectionStringTemplate", ovalue, value);
  }

protected:

  grt::StringRef _connectionStringTemplate;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_PythonDBAPIDriver());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_PythonDBAPIDriver::create);
    {
      void (db_mgmt_PythonDBAPIDriver::*setter)(const grt::StringRef &) = &db_mgmt_PythonDBAPIDriver::connectionStringTemplate;
      grt::StringRef (db_mgmt_PythonDBAPIDriver::*getter)() const = &db_mgmt_PythonDBAPIDriver::connectionStringTemplate;
      meta->bind_member("connectionStringTemplate", new grt::MetaClass::Property<db_mgmt_PythonDBAPIDriver,grt::StringRef>(getter, setter));
    }
  }
};

/** specifies which privileges are available for this object type */
class  db_mgmt_PrivilegeMapping : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_PrivilegeMapping(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _privileges(this, false),
      _structName("") {
  }

  static std::string static_class_name() {
    return "db.mgmt.PrivilegeMapping";
  }

  /**
   * Getter for attribute privileges (read-only)
   *
   * the list of available privileges for this object type
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

  /**
   * Getter for attribute structName
   *
   * the struct of the database object
   * \par In Python:
   *    value = obj.structName
   */
  grt::StringRef structName() const { return _structName; }

  /**
   * Setter for attribute structName
   * 
   * the struct of the database object
   * \par In Python:
   *   obj.structName = value
   */
  virtual void structName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_structName);
    _structName = value;
    member_changed("structName", ovalue, value);
  }

protected:

  grt::StringListRef _privileges;
  grt::StringRef _structName;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_PrivilegeMapping());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_PrivilegeMapping::create);
    {
      void (db_mgmt_PrivilegeMapping::*setter)(const grt::StringListRef &) = &db_mgmt_PrivilegeMapping::privileges;
      grt::StringListRef (db_mgmt_PrivilegeMapping::*getter)() const = &db_mgmt_PrivilegeMapping::privileges;
      meta->bind_member("privileges", new grt::MetaClass::Property<db_mgmt_PrivilegeMapping,grt::StringListRef>(getter, setter));
    }
    {
      void (db_mgmt_PrivilegeMapping::*setter)(const grt::StringRef &) = &db_mgmt_PrivilegeMapping::structName;
      grt::StringRef (db_mgmt_PrivilegeMapping::*getter)() const = &db_mgmt_PrivilegeMapping::structName;
      meta->bind_member("structName", new grt::MetaClass::Property<db_mgmt_PrivilegeMapping,grt::StringRef>(getter, setter));
    }
  }
};

/** Relational Database Management System */
class  db_mgmt_Rdbms : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_Rdbms(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _characterSets(this, false),
      _databaseObjectPackage(""),
      _doesSupportCatalogs(0),
      _drivers(this, false),
      _maximumIdentifierLength(0),
      _privilegeNames(this, false),
      _simpleDatatypes(this, false) {
  }

  static std::string static_class_name() {
    return "db.mgmt.Rdbms";
  }

  /**
   * Getter for attribute caption
   *
   * the caption that is displayed in the UI
   * \par In Python:
   *    value = obj.caption
   */
  grt::StringRef caption() const { return _caption; }

  /**
   * Setter for attribute caption
   * 
   * the caption that is displayed in the UI
   * \par In Python:
   *   obj.caption = value
   */
  virtual void caption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_caption);
    _caption = value;
    member_changed("caption", ovalue, value);
  }

  // characterSets is owned by db_mgmt_Rdbms
  /**
   * Getter for attribute characterSets (read-only)
   *
   * the list of character sets the RDBMS offers
   * \par In Python:
   *    value = obj.characterSets
   */
  grt::ListRef<db_CharacterSet> characterSets() const { return _characterSets; }


private: // The next attribute is read-only.
  virtual void characterSets(const grt::ListRef<db_CharacterSet> &value) {
    grt::ValueRef ovalue(_characterSets);

    _characterSets = value;
    owned_member_changed("characterSets", ovalue, value);
  }
public:

  /**
   * Getter for attribute databaseObjectPackage
   *
   * specifies the schema structs to use, e.g. db.mysql
   * \par In Python:
   *    value = obj.databaseObjectPackage
   */
  grt::StringRef databaseObjectPackage() const { return _databaseObjectPackage; }

  /**
   * Setter for attribute databaseObjectPackage
   * 
   * specifies the schema structs to use, e.g. db.mysql
   * \par In Python:
   *   obj.databaseObjectPackage = value
   */
  virtual void databaseObjectPackage(const grt::StringRef &value) {
    grt::ValueRef ovalue(_databaseObjectPackage);
    _databaseObjectPackage = value;
    member_changed("databaseObjectPackage", ovalue, value);
  }

  /**
   * Getter for attribute defaultDriver
   *
   * the default driver to use
   * \par In Python:
   *    value = obj.defaultDriver
   */
  db_mgmt_DriverRef defaultDriver() const { return _defaultDriver; }

  /**
   * Setter for attribute defaultDriver
   * 
   * the default driver to use
   * \par In Python:
   *   obj.defaultDriver = value
   */
  virtual void defaultDriver(const db_mgmt_DriverRef &value) {
    grt::ValueRef ovalue(_defaultDriver);
    _defaultDriver = value;
    member_changed("defaultDriver", ovalue, value);
  }

  /**
   * Getter for attribute doesSupportCatalogs
   *
   * Whether the RDBMS supports the notion of a database catalog
   * \par In Python:
   *    value = obj.doesSupportCatalogs
   */
  grt::IntegerRef doesSupportCatalogs() const { return _doesSupportCatalogs; }

  /**
   * Setter for attribute doesSupportCatalogs
   * 
   * Whether the RDBMS supports the notion of a database catalog
   * \par In Python:
   *   obj.doesSupportCatalogs = value
   */
  virtual void doesSupportCatalogs(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_doesSupportCatalogs);
    _doesSupportCatalogs = value;
    member_changed("doesSupportCatalogs", ovalue, value);
  }

  // drivers is owned by db_mgmt_Rdbms
  /**
   * Getter for attribute drivers (read-only)
   *
   * a list of drivers that can be used to connect to the database system
   * \par In Python:
   *    value = obj.drivers
   */
  grt::ListRef<db_mgmt_Driver> drivers() const { return _drivers; }


private: // The next attribute is read-only.
  virtual void drivers(const grt::ListRef<db_mgmt_Driver> &value) {
    grt::ValueRef ovalue(_drivers);

    _drivers = value;
    owned_member_changed("drivers", ovalue, value);
  }
public:

  /**
   * Getter for attribute maximumIdentifierLength
   *
   * maximum length for identifiers (schema, table, column, index etc)
   * \par In Python:
   *    value = obj.maximumIdentifierLength
   */
  grt::IntegerRef maximumIdentifierLength() const { return _maximumIdentifierLength; }

  /**
   * Setter for attribute maximumIdentifierLength
   * 
   * maximum length for identifiers (schema, table, column, index etc)
   * \par In Python:
   *   obj.maximumIdentifierLength = value
   */
  virtual void maximumIdentifierLength(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_maximumIdentifierLength);
    _maximumIdentifierLength = value;
    member_changed("maximumIdentifierLength", ovalue, value);
  }

  // privilegeNames is owned by db_mgmt_Rdbms
  /**
   * Getter for attribute privilegeNames (read-only)
   *
   * list of privilege names that are available in this RDBMS
   * \par In Python:
   *    value = obj.privilegeNames
   */
  grt::ListRef<db_mgmt_PrivilegeMapping> privilegeNames() const { return _privilegeNames; }


private: // The next attribute is read-only.
  virtual void privilegeNames(const grt::ListRef<db_mgmt_PrivilegeMapping> &value) {
    grt::ValueRef ovalue(_privilegeNames);

    _privilegeNames = value;
    owned_member_changed("privilegeNames", ovalue, value);
  }
public:

  // simpleDatatypes is owned by db_mgmt_Rdbms
  /**
   * Getter for attribute simpleDatatypes (read-only)
   *
   * the list of simple datatypes the RDBMS offers
   * \par In Python:
   *    value = obj.simpleDatatypes
   */
  grt::ListRef<db_SimpleDatatype> simpleDatatypes() const { return _simpleDatatypes; }


private: // The next attribute is read-only.
  virtual void simpleDatatypes(const grt::ListRef<db_SimpleDatatype> &value) {
    grt::ValueRef ovalue(_simpleDatatypes);

    _simpleDatatypes = value;
    owned_member_changed("simpleDatatypes", ovalue, value);
  }
public:

  // version is owned by db_mgmt_Rdbms
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

  grt::StringRef _caption;
  grt::ListRef<db_CharacterSet> _characterSets;// owned
  grt::StringRef _databaseObjectPackage;
  db_mgmt_DriverRef _defaultDriver;
  grt::IntegerRef _doesSupportCatalogs;
  grt::ListRef<db_mgmt_Driver> _drivers;// owned
  grt::IntegerRef _maximumIdentifierLength;
  grt::ListRef<db_mgmt_PrivilegeMapping> _privilegeNames;// owned
  grt::ListRef<db_SimpleDatatype> _simpleDatatypes;// owned
  GrtVersionRef _version;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_Rdbms());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_Rdbms::create);
    {
      void (db_mgmt_Rdbms::*setter)(const grt::StringRef &) = &db_mgmt_Rdbms::caption;
      grt::StringRef (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const grt::ListRef<db_CharacterSet> &) = &db_mgmt_Rdbms::characterSets;
      grt::ListRef<db_CharacterSet> (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::characterSets;
      meta->bind_member("characterSets", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::ListRef<db_CharacterSet>>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const grt::StringRef &) = &db_mgmt_Rdbms::databaseObjectPackage;
      grt::StringRef (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::databaseObjectPackage;
      meta->bind_member("databaseObjectPackage", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::StringRef>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const db_mgmt_DriverRef &) = &db_mgmt_Rdbms::defaultDriver;
      db_mgmt_DriverRef (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::defaultDriver;
      meta->bind_member("defaultDriver", new grt::MetaClass::Property<db_mgmt_Rdbms,db_mgmt_DriverRef>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const grt::IntegerRef &) = &db_mgmt_Rdbms::doesSupportCatalogs;
      grt::IntegerRef (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::doesSupportCatalogs;
      meta->bind_member("doesSupportCatalogs", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const grt::ListRef<db_mgmt_Driver> &) = &db_mgmt_Rdbms::drivers;
      grt::ListRef<db_mgmt_Driver> (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::drivers;
      meta->bind_member("drivers", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::ListRef<db_mgmt_Driver>>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const grt::IntegerRef &) = &db_mgmt_Rdbms::maximumIdentifierLength;
      grt::IntegerRef (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::maximumIdentifierLength;
      meta->bind_member("maximumIdentifierLength", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::IntegerRef>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const grt::ListRef<db_mgmt_PrivilegeMapping> &) = &db_mgmt_Rdbms::privilegeNames;
      grt::ListRef<db_mgmt_PrivilegeMapping> (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::privilegeNames;
      meta->bind_member("privilegeNames", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::ListRef<db_mgmt_PrivilegeMapping>>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const grt::ListRef<db_SimpleDatatype> &) = &db_mgmt_Rdbms::simpleDatatypes;
      grt::ListRef<db_SimpleDatatype> (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::simpleDatatypes;
      meta->bind_member("simpleDatatypes", new grt::MetaClass::Property<db_mgmt_Rdbms,grt::ListRef<db_SimpleDatatype>>(getter, setter));
    }
    {
      void (db_mgmt_Rdbms::*setter)(const GrtVersionRef &) = &db_mgmt_Rdbms::version;
      GrtVersionRef (db_mgmt_Rdbms::*getter)() const = &db_mgmt_Rdbms::version;
      meta->bind_member("version", new grt::MetaClass::Property<db_mgmt_Rdbms,GrtVersionRef>(getter, setter));
    }
  }
};

/** Management for RDBMS drivers */
class  db_mgmt_Management : public GrtObject {
  typedef GrtObject super;

public:
  db_mgmt_Management(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _datatypeGroups(this, false),
      _otherStoredConns(this, false),
      _rdbms(this, false),
      _storedConns(this, false),
      _storedInstances(this, false) {
  }

  static std::string static_class_name() {
    return "db.mgmt.Management";
  }

  // datatypeGroups is owned by db_mgmt_Management
  /**
   * Getter for attribute datatypeGroups (read-only)
   *
   * list of datatypegroups
   * \par In Python:
   *    value = obj.datatypeGroups
   */
  grt::ListRef<db_DatatypeGroup> datatypeGroups() const { return _datatypeGroups; }


private: // The next attribute is read-only.
  virtual void datatypeGroups(const grt::ListRef<db_DatatypeGroup> &value) {
    grt::ValueRef ovalue(_datatypeGroups);

    _datatypeGroups = value;
    owned_member_changed("datatypeGroups", ovalue, value);
  }
public:

  // otherStoredConns is owned by db_mgmt_Management
  /**
   * Getter for attribute otherStoredConns (read-only)
   *
   * a list of stored non-MySQL connections
   * \par In Python:
   *    value = obj.otherStoredConns
   */
  grt::ListRef<db_mgmt_Connection> otherStoredConns() const { return _otherStoredConns; }


private: // The next attribute is read-only.
  virtual void otherStoredConns(const grt::ListRef<db_mgmt_Connection> &value) {
    grt::ValueRef ovalue(_otherStoredConns);

    _otherStoredConns = value;
    owned_member_changed("otherStoredConns", ovalue, value);
  }
public:

  // rdbms is owned by db_mgmt_Management
  /**
   * Getter for attribute rdbms (read-only)
   *
   * a list of Rdbms with available drivers
   * \par In Python:
   *    value = obj.rdbms
   */
  grt::ListRef<db_mgmt_Rdbms> rdbms() const { return _rdbms; }


private: // The next attribute is read-only.
  virtual void rdbms(const grt::ListRef<db_mgmt_Rdbms> &value) {
    grt::ValueRef ovalue(_rdbms);

    _rdbms = value;
    owned_member_changed("rdbms", ovalue, value);
  }
public:

  // storedConns is owned by db_mgmt_Management
  /**
   * Getter for attribute storedConns (read-only)
   *
   * a list of stored connections
   * \par In Python:
   *    value = obj.storedConns
   */
  grt::ListRef<db_mgmt_Connection> storedConns() const { return _storedConns; }


private: // The next attribute is read-only.
  virtual void storedConns(const grt::ListRef<db_mgmt_Connection> &value) {
    grt::ValueRef ovalue(_storedConns);

    _storedConns = value;
    owned_member_changed("storedConns", ovalue, value);
  }
public:

  // storedInstances is owned by db_mgmt_Management
  /**
   * Getter for attribute storedInstances (read-only)
   *
   * a list of stored DB server instances
   * \par In Python:
   *    value = obj.storedInstances
   */
  grt::ListRef<db_mgmt_ServerInstance> storedInstances() const { return _storedInstances; }


private: // The next attribute is read-only.
  virtual void storedInstances(const grt::ListRef<db_mgmt_ServerInstance> &value) {
    grt::ValueRef ovalue(_storedInstances);

    _storedInstances = value;
    owned_member_changed("storedInstances", ovalue, value);
  }
public:

protected:

  grt::ListRef<db_DatatypeGroup> _datatypeGroups;// owned
  grt::ListRef<db_mgmt_Connection> _otherStoredConns;// owned
  grt::ListRef<db_mgmt_Rdbms> _rdbms;// owned
  grt::ListRef<db_mgmt_Connection> _storedConns;// owned
  grt::ListRef<db_mgmt_ServerInstance> _storedInstances;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new db_mgmt_Management());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_mgmt_Management::create);
    {
      void (db_mgmt_Management::*setter)(const grt::ListRef<db_DatatypeGroup> &) = &db_mgmt_Management::datatypeGroups;
      grt::ListRef<db_DatatypeGroup> (db_mgmt_Management::*getter)() const = &db_mgmt_Management::datatypeGroups;
      meta->bind_member("datatypeGroups", new grt::MetaClass::Property<db_mgmt_Management,grt::ListRef<db_DatatypeGroup>>(getter, setter));
    }
    {
      void (db_mgmt_Management::*setter)(const grt::ListRef<db_mgmt_Connection> &) = &db_mgmt_Management::otherStoredConns;
      grt::ListRef<db_mgmt_Connection> (db_mgmt_Management::*getter)() const = &db_mgmt_Management::otherStoredConns;
      meta->bind_member("otherStoredConns", new grt::MetaClass::Property<db_mgmt_Management,grt::ListRef<db_mgmt_Connection>>(getter, setter));
    }
    {
      void (db_mgmt_Management::*setter)(const grt::ListRef<db_mgmt_Rdbms> &) = &db_mgmt_Management::rdbms;
      grt::ListRef<db_mgmt_Rdbms> (db_mgmt_Management::*getter)() const = &db_mgmt_Management::rdbms;
      meta->bind_member("rdbms", new grt::MetaClass::Property<db_mgmt_Management,grt::ListRef<db_mgmt_Rdbms>>(getter, setter));
    }
    {
      void (db_mgmt_Management::*setter)(const grt::ListRef<db_mgmt_Connection> &) = &db_mgmt_Management::storedConns;
      grt::ListRef<db_mgmt_Connection> (db_mgmt_Management::*getter)() const = &db_mgmt_Management::storedConns;
      meta->bind_member("storedConns", new grt::MetaClass::Property<db_mgmt_Management,grt::ListRef<db_mgmt_Connection>>(getter, setter));
    }
    {
      void (db_mgmt_Management::*setter)(const grt::ListRef<db_mgmt_ServerInstance> &) = &db_mgmt_Management::storedInstances;
      grt::ListRef<db_mgmt_ServerInstance> (db_mgmt_Management::*getter)() const = &db_mgmt_Management::storedInstances;
      meta->bind_member("storedInstances", new grt::MetaClass::Property<db_mgmt_Management,grt::ListRef<db_mgmt_ServerInstance>>(getter, setter));
    }
  }
};



inline void register_structs_db_mgmt_xml() {
  grt::internal::ClassRegistry::register_class<db_mgmt_SyncProfile>();
  grt::internal::ClassRegistry::register_class<db_mgmt_ServerInstance>();
  grt::internal::ClassRegistry::register_class<db_mgmt_SSHFile>();
  grt::internal::ClassRegistry::register_class<db_mgmt_SSHConnection>();
  grt::internal::ClassRegistry::register_class<db_mgmt_Connection>();
  grt::internal::ClassRegistry::register_class<db_mgmt_DriverParameter>();
  grt::internal::ClassRegistry::register_class<db_mgmt_Driver>();
  grt::internal::ClassRegistry::register_class<db_mgmt_PythonDBAPIDriver>();
  grt::internal::ClassRegistry::register_class<db_mgmt_PrivilegeMapping>();
  grt::internal::ClassRegistry::register_class<db_mgmt_Rdbms>();
  grt::internal::ClassRegistry::register_class<db_mgmt_Management>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_db_mgmt_xml {
  _autoreg__structs_db_mgmt_xml() {
    register_structs_db_mgmt_xml();
  }
} __autoreg__structs_db_mgmt_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

