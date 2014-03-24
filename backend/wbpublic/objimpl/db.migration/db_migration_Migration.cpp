
#include "stdafx.h"
#include <grts/structs.db.migration.h>

#include <grtpp_util.h>

class db_migration_Migration::ImplData
{
public:
  ImplData(){};
  virtual ~ImplData() {};
  void addSourceObject(const std::string& id, const grt::Ref<GrtObject> &object)
  {
    source_objects[id] = object;
  }

  grt::Ref<GrtObject>& getSourceObject(const std::string& id)
  {
    return source_objects[id];
  }

  void addTargetObject(const std::string& id, const grt::Ref<GrtObject> &object)
  {
    target_objects[id] = object;
  }

  grt::Ref<GrtObject>& getTargetObject(const std::string& id)
  {
    return target_objects[id];
  }
private:
  std::map<std::string, grt::Ref<GrtObject> > target_objects;
  std::map<std::string, grt::Ref<GrtObject> > source_objects;
};

//================================================================================
// db_migration_Migration


void db_migration_Migration::init()
{
  if (!_data) _data= new db_migration_Migration::ImplData();
}

db_migration_Migration::~db_migration_Migration()
{
  delete _data;
}


grt::Ref<GrtLogObject> db_migration_Migration::addMigrationLogEntry(long type, const grt::Ref<GrtObject> &sourceObject, const grt::Ref<GrtObject> &targetObject, const std::string &message)
{
  GrtLogObjectRef log = findMigrationLogEntry(sourceObject, targetObject);
  if (!log.is_valid())
  {
    log = GrtLogObjectRef(get_grt());
    log->owner(this);
    log->logObject(sourceObject);
    log->refObject(targetObject);

    migrationLog().insert(log);
  }

  GrtLogEntryRef entry = GrtLogEntryRef(get_grt());
  
  entry->owner(log);
  entry->entryType(type);
  entry->name(grt::StringRef(message));
  log->entries().insert(entry);

  if(0 == type)
  {
    this->_data->addSourceObject(targetObject->id(), sourceObject);
    this->_data->addTargetObject(sourceObject->id(), targetObject);
  }

  return log;
}


grt::Ref<GrtLogObject> db_migration_Migration::findMigrationLogEntry(const grt::Ref<GrtObject> &sourceObject, const grt::Ref<GrtObject> &targetObject)
{
  for (size_t c = migrationLog().count(), i = 0; i < c; i++)
  {
    GrtLogObjectRef log(migrationLog()[i]);
    if (log->logObject() == sourceObject && log->refObject() == targetObject)
      return log;
  }
  return GrtLogObjectRef();
}


grt::Ref<GrtObject> db_migration_Migration::lookupMigratedObject(const grt::Ref<GrtObject> &sourceObject)
{
  return this->_data->getTargetObject(sourceObject->id());
}


grt::Ref<GrtObject> db_migration_Migration::lookupSourceObject(const grt::Ref<GrtObject> &targetObject)
{
  return this->_data->getSourceObject(targetObject->id());
}


