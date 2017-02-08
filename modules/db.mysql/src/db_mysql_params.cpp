/*
* Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "db_mysql_params.h"
#include "grtpp_util.h"
#include "grt/grt_manager.h"
#include "base/file_utilities.h"

namespace dbmysql {

  typedef std::map<EngineId, std::string> EngineIdByNameMap;

  EngineIdByNameMap& get_map() {
    static EngineIdByNameMap map;
    if (map.empty()) {
      map.insert(std::make_pair(eetMySAM, std::string("MyISAM")));
      map.insert(std::make_pair(eetInnoDB, std::string("InnoDB")));
      map.insert(std::make_pair(eetFalcon, std::string("Falcon")));
      map.insert(std::make_pair(eetMerge, std::string("Merge")));
      map.insert(std::make_pair(eetMemory, std::string("Memory")));
      map.insert(std::make_pair(eetExample, std::string("Example")));
      map.insert(std::make_pair(eetFederated, std::string("Federated")));
      map.insert(std::make_pair(eetArchive, std::string("Archive")));
      map.insert(std::make_pair(eetCsv, std::string("Csv")));
      map.insert(std::make_pair(eetBlackhole, std::string("Blackhole")));
    }

    return map;
  }

  EngineId engine_id_by_name(const char* name) {
    EngineIdByNameMap::iterator iter = get_map().begin();
    for (; iter != get_map().end(); ++iter) {
      if (!strcasecmp(name, iter->second.c_str()))
        return iter->first;
    }

    return eetOTHER;
  }

  std::string engine_name_by_id(EngineId id) {
    EngineIdByNameMap::iterator iter = get_map().find(id);
    if (iter != get_map().end())
      return iter->second;

    return "";
  }

  db_mysql_StorageEngineRef engine_by_name(const char* engineName) {
    if (engineName && *engineName) {
      grt::ListRef<db_mysql_StorageEngine> engines = get_known_engines();

      for (size_t i = 0, count = engines.count(); i < count; i++) {
        db_mysql_StorageEngineRef engine = engines.get(i);
        if (!strcasecmp(engine->name().c_str(), engineName))
          return engine;
      }
    }
    return db_mysql_StorageEngineRef();
  }

  db_mysql_StorageEngineRef engine_by_id(EngineId id) {
    std::string engineName = engine_name_by_id(id);

    return engine_by_name(engineName.c_str());
  }

  grt::ListRef<db_mysql_StorageEngine> get_known_engines() {
    return grt::ListRef<db_mysql_StorageEngine>::cast_from(grt::GRT::get()->unserialize(
      base::makePath(bec::GRTManager::get()->get_basedir(), "modules/data/mysql_engines.xml")));
  }

  bool check_valid_characters(const char* str) {
    while (*str) {
      if (!g_unichar_isalnum(*str) && *str != L'_')
        return false;
      str = g_utf8_next_char(str);
    }

    return true;
  }
}
