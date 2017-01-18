/*
* Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.db.migration.h>

#include <grtpp_util.h>

class db_migration_Migration::ImplData {
public:
  ImplData(){};
  virtual ~ImplData(){};
  void addSourceObject(const std::string &id, const grt::Ref<GrtObject> &object) {
    source_objects[id] = object;
  }

  grt::Ref<GrtObject> &getSourceObject(const std::string &id) {
    return source_objects[id];
  }

  void addTargetObject(const std::string &id, const grt::Ref<GrtObject> &object) {
    target_objects[id] = object;
  }

  grt::Ref<GrtObject> &getTargetObject(const std::string &id) {
    return target_objects[id];
  }

private:
  std::map<std::string, grt::Ref<GrtObject> > target_objects;
  std::map<std::string, grt::Ref<GrtObject> > source_objects;
};

//================================================================================
// db_migration_Migration

void db_migration_Migration::init() {
  if (!_data)
    _data = new db_migration_Migration::ImplData();
}

db_migration_Migration::~db_migration_Migration() {
  delete _data;
}

grt::Ref<GrtLogObject> db_migration_Migration::addMigrationLogEntry(ssize_t type,
                                                                    const grt::Ref<GrtObject> &sourceObject,
                                                                    const grt::Ref<GrtObject> &targetObject,
                                                                    const std::string &message) {
  GrtLogObjectRef log = findMigrationLogEntry(sourceObject, targetObject);
  if (!log.is_valid()) {
    log = GrtLogObjectRef(grt::Initialized);
    log->owner(this);
    log->logObject(sourceObject);
    log->refObject(targetObject);

    migrationLog().insert(log);
  }

  GrtLogEntryRef entry(grt::Initialized);
  entry->owner(log);
  entry->entryType(type);
  entry->name(grt::StringRef(message));
  log->entries().insert(entry);

  if (0 == type) {
    this->_data->addSourceObject(targetObject->id(), sourceObject);
    this->_data->addTargetObject(sourceObject->id(), targetObject);
  }

  return log;
}

grt::Ref<GrtLogObject> db_migration_Migration::findMigrationLogEntry(const grt::Ref<GrtObject> &sourceObject,
                                                                     const grt::Ref<GrtObject> &targetObject) {
  for (size_t c = migrationLog().count(), i = 0; i < c; i++) {
    GrtLogObjectRef log(migrationLog()[i]);
    if (log->logObject() == sourceObject && log->refObject() == targetObject)
      return log;
  }
  return GrtLogObjectRef();
}

grt::Ref<GrtObject> db_migration_Migration::lookupMigratedObject(const grt::Ref<GrtObject> &sourceObject) {
  return this->_data->getTargetObject(sourceObject->id());
}

grt::Ref<GrtObject> db_migration_Migration::lookupSourceObject(const grt::Ref<GrtObject> &targetObject) {
  return this->_data->getSourceObject(targetObject->id());
}
