#ifndef _DB_MYSQL_PARAMS_DEFINED_H_
#define _DB_MYSQL_PARAMS_DEFINED_H_

#include "db_mysql_public_interface.h"

#include <string>
#include "grts/structs.db.mysql.h"

namespace dbmysql
{

template<class P, class T>
bool get_parent(P &parent, const T &object)
{
  GrtObjectRef owner= object->owner();
  if (!owner.is_valid())
    return false;

  if (!P::can_wrap(owner))
    return get_parent(parent, owner);

  parent= P::cast_from(owner);
  return true;
}


inline std::string full_name(db_DatabaseObjectRef obj, db_SchemaRef schema= db_SchemaRef())
{
  std::string res= '`'+ *obj->name() +'`';
  if (get_parent(schema, obj))
    return '`'+ *schema->name() +"`."+ res;

  return res;
}


enum EngineId
{
  eetMySAM= 0,
  eetInnoDB,
  eetFalcon,
  eetMerge,
  eetMemory,
  eetExample,
  eetFederated,
  eetArchive,
  eetCsv,
  eetBlackhole,
  eetOTHER
};


EngineId MYSQLMODULEDBMYSQL_PUBLIC_FUNC 
engine_id_by_name(const char* name);


std::string MYSQLMODULEDBMYSQL_PUBLIC_FUNC 
engine_name_by_id(EngineId id, grt::GRT* grt);


db_mysql_StorageEngineRef MYSQLMODULEDBMYSQL_PUBLIC_FUNC 
engine_by_name(const char* name, grt::GRT* grt);


db_mysql_StorageEngineRef MYSQLMODULEDBMYSQL_PUBLIC_FUNC 
engine_by_id(EngineId id);


grt::ListRef<db_mysql_StorageEngine> MYSQLMODULEDBMYSQL_PUBLIC_FUNC
get_known_engines(grt::GRT*);


bool MYSQLMODULEDBMYSQL_PUBLIC_FUNC
is_word_reserved(const char* str, grt::GRT*);

bool MYSQLMODULEDBMYSQL_PUBLIC_FUNC
check_valid_characters(const char* str);

}

#endif
