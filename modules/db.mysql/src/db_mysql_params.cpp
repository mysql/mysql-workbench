#include "stdafx.h"
#include "db_mysql_params.h"
#include "grtpp_util.h"
#include "grt/grt_manager.h"

namespace dbmysql
{

typedef std::map<EngineId, std::string> EngineIdByNameMap;

EngineIdByNameMap& get_map()
{
  static EngineIdByNameMap map;
  if (map.empty())
  {
    map.insert(std::make_pair(eetMySAM,     std::string("MyISAM")));
    map.insert(std::make_pair(eetInnoDB,    std::string("InnoDB")));
    map.insert(std::make_pair(eetFalcon,    std::string("Falcon")));
    map.insert(std::make_pair(eetMerge,     std::string("Merge")));
    map.insert(std::make_pair(eetMemory,    std::string("Memory")));
    map.insert(std::make_pair(eetExample,   std::string("Example")));
    map.insert(std::make_pair(eetFederated, std::string("Federated")));
    map.insert(std::make_pair(eetArchive,   std::string("Archive")));
    map.insert(std::make_pair(eetCsv,       std::string("Csv")));
    map.insert(std::make_pair(eetBlackhole, std::string("Blackhole")));
  }

  return map;
}


EngineId engine_id_by_name(const char* name)
{
  EngineIdByNameMap::iterator iter= get_map().begin();
  for (; iter != get_map().end(); ++iter) 
  {
    if (!stricmp(name, iter->second.c_str()))
      return iter->first;
  }

  return eetOTHER;
}

std::string engine_name_by_id(EngineId id)
{
  EngineIdByNameMap::iterator iter= get_map().find(id);
  if (iter != get_map().end())
    return iter->second;

  return "";
}


db_mysql_StorageEngineRef engine_by_name(const char* engineName, grt::GRT* grt)
{
  if (engineName && *engineName)
  {
    grt::ListRef<db_mysql_StorageEngine> engines= get_known_engines(grt);

    for (unsigned int i= 0, count= engines.count(); i < count; i++)
    {
      db_mysql_StorageEngineRef engine= engines.get(i);
      if (!stricmp(engine->name().c_str(), engineName))
        return engine;
    }

  }
  return db_mysql_StorageEngineRef();
}


db_mysql_StorageEngineRef engine_by_id(EngineId id, grt::GRT* grt)
{
  std::string engineName= engine_name_by_id(id);

  return engine_by_name(engineName.c_str(), grt);
}


grt::ListRef<db_mysql_StorageEngine> get_known_engines(grt::GRT *grt)
{
  return grt::ListRef<db_mysql_StorageEngine>::cast_from(
    grt->unserialize(bec::make_path(bec::GRTManager::get_instance_for(grt)->get_basedir(), "modules/data/mysql_engines.xml")));
}

// TODO: remove this and convert callers to use mforms::CodeEditor static call.
bool is_word_reserved(const char* str, grt::GRT* grt)
{
  bool ret = false;
  static grt::StringListRef reserved_words;
  static std::vector<int> lengths;
  if (!reserved_words.is_valid())
  {
    reserved_words= grt::StringListRef::cast_from(grt->unserialize(bec::make_path(bec::GRTManager::get_instance_for(grt)->get_basedir(), "modules/data/mysql_reserved.xml")));
    for (unsigned int i= 0, count= reserved_words.count(); i < count; i++)
      lengths.push_back(strlen(reserved_words.get(i).c_str()));
  }

  if ( str )
  {
    const int str_len = strlen(str);
    static const int count = reserved_words.count();
    for (int i = 0; i < count; i++)
    {
      // TODO create and use hash here
      if (!stricmp(reserved_words.get(i).c_str(), str))
      {
        if ( str_len == lengths[i] ) 
          ret = true;
      }
    }
  }
  
  return ret;
}

bool check_valid_characters(const char* str)
{
  while (*str)
  {
    if (!g_unichar_isalnum(*str) && *str!=L'_')
      return false;
    str= g_utf8_next_char(str);
  }

  return true;
}

}
