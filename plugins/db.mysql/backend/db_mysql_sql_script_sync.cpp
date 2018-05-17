/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include <algorithm>
#include "grtpp_undo_manager.h"

#include "grtdb/db_object_helpers.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.h"
#include "grts/structs.workbench.physical.h"

#include "grt.h"
#include "db_mysql_sql_export.h"
#include "base/string_utilities.h"

using namespace grt;

#include "diff/diffchange.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "grtdb/sync_profile.h"
#include "grtsqlparser/sql_facade.h"
#include "db.mysql/src/module_db_mysql.h"
#include "interfaces/sqlgenerator.h"

#include "db_mysql_sql_script_sync.h"
//#include "cpp/catalog_templates.h"

#include "db.mysql/src/module_db_mysql_shared_code.h"

#include "base/log.h"

DEFAULT_LOG_DOMAIN(DOMAIN_GRT_DIFF);

template <typename TPred>
void iterate_object(GrtObjectRef& obj, TPred pred) {
  pred(obj);
  MetaClass* meta = obj.get_metaclass();
  while (meta != 0) {
    for (MetaClass::MemberList::const_iterator iter = meta->get_members_partial().begin();
         iter != meta->get_members_partial().end(); ++iter) {
      if (iter->second.overrides)
        continue;

      std::string name = iter->second.name;
      if (name == "owner")
        continue;

      std::string attr = meta->get_member_attribute(name, "dontdiff");
      const int dontdiff = attr.size() && (base::atoi<int>(attr, 0) & 1);

      if (dontdiff)
        continue;

      const bool dontfollow =
        !iter->second.owned_object && (name != "flags") && (name != "columns") && (name != "foreignKeys");

      ValueRef v = obj.get_member(name);

      if (!v.is_valid())
        continue;
      Type type = v.type();
      switch (type) {
        case IntegerType:
        case DoubleType:
        case StringType:
          break;
        case ListType: {
          BaseListRef list = BaseListRef::cast_from(v);
          for (size_t i = 0; i < list.count(); ++i) {
            if (!ObjectRef::can_wrap(list[i]))
              continue;
            GrtObjectRef inner_obj = GrtObjectRef::cast_from(list[i]);
            if (dontfollow)
              pred(inner_obj);
            else
              iterate_object(inner_obj, pred);
          }
        } break;
        case DictType: {
          DictRef dict = DictRef::cast_from(v);
          for (internal::Dict::const_iterator iter = dict.begin(); iter != dict.end(); ++iter) {
            if (!GrtObjectRef::can_wrap(iter->second))
              continue;
            GrtObjectRef inner_obj = GrtObjectRef::cast_from(iter->second);
            if (dontfollow)
              pred(inner_obj);
            else
              iterate_object(inner_obj, pred);
          }
        } break;
        case ObjectType: {
          GrtObjectRef inner_obj = GrtObjectRef::cast_from(v);
          if (dontfollow)
            pred(inner_obj);
          else
            iterate_object(inner_obj, pred);
          break;
        }
        default:
          break;
      }
    }
    meta = meta->parent();
  }
}

/*
template<typename T>
void replace_list_objects(grt::ListRef<T> list, CatalogMap& obj_map)
{
  CatalogMap::const_iterator end= obj_map.end();
  for(size_t i= 0, count= list.count(); i < count; i++)
  {
    Ref<T> t= list.get(i);
    if(!t.is_valid())
    {
      list.remove(i);
      count--;
      i--;
      continue;
    }
    CatalogMap::const_iterator it= obj_map.find(get_catalog_map_key(t));
    if(it == end)
      continue;
    list.remove(i);
    list.insert(Ref<T>::cast_from(it->second), (long)i);
  }
}

template<>
void replace_list_objects(grt::ListRef<db_mysql_IndexColumn> list, CatalogMap& obj_map)
{
  CatalogMap::const_iterator end= obj_map.end();
  for(size_t i= 0, count= list.count(); i < count; i++)
  {
    db_mysql_IndexColumnRef index_column= list.get(i);
    db_ColumnRef column= index_column->referencedColumn();
    CatalogMap::const_iterator it=
      obj_map.find(get_catalog_map_key<db_Column>(column));
    if(it == end)
      continue;
    index_column->referencedColumn(db_ColumnRef::cast_from(it->second));
  }
}*/

static inline void update_old_name(GrtNamedObjectRef obj, bool update_only_empty) {
  if (!update_only_empty || (strlen(obj->oldName().c_str()) == 0))
    obj->oldName(obj->name());
}

template <class _Parent, class _Object>
class ObjectAction {
protected:
  _Parent owner;
  bool update_only_empty;

public:
  ObjectAction(_Parent ow, bool update_empty) : owner(ow), update_only_empty(update_empty) {
  }
  virtual ~ObjectAction() {
  }
  virtual void operator()(_Object object) {
    //    object->owner(owner);
    update_old_name(object, update_only_empty);
  }
};

namespace {
  struct FKAction : public ObjectAction<db_mysql_TableRef, db_mysql_ForeignKeyRef> {
    CatalogMap& map;
    FKAction(db_mysql_TableRef ow, bool update_only_empty, CatalogMap& m)
      : ObjectAction<db_mysql_TableRef, db_mysql_ForeignKeyRef>(ow, update_only_empty), map(m) {
    }

    void operator()(db_mysql_ForeignKeyRef fk) {
      ObjectAction<db_mysql_TableRef, db_mysql_ForeignKeyRef>::operator()(fk);
      //   replace_list_objects(fk->columns(), map);
      //   replace_list_objects(fk->referencedColumns(), map);
    }
  };

  struct IndexAction : public ObjectAction<db_mysql_TableRef, db_mysql_IndexRef> {
    CatalogMap& map;
    IndexAction(db_mysql_TableRef ow, bool update_only_empty, CatalogMap& m)
      : ObjectAction<db_mysql_TableRef, db_mysql_IndexRef>(ow, update_only_empty), map(m) {
    }

    void operator()(db_mysql_IndexRef index) {
      ObjectAction<db_mysql_TableRef, db_mysql_IndexRef>::operator()(index);
      // replace_list_objects(index->columns(), map);
    }
  };

  struct TableAction : public ObjectAction<db_mysql_SchemaRef, db_mysql_TableRef> {
    CatalogMap& map;
    TableAction(db_mysql_SchemaRef ow, bool update_only_empty, CatalogMap& m)
      : ObjectAction<db_mysql_SchemaRef, db_mysql_TableRef>(ow, update_only_empty), map(m) {
    }

    void operator()(db_mysql_TableRef table) {
      ObjectAction<db_mysql_SchemaRef, db_mysql_TableRef>::operator()(table);

      ObjectAction<db_mysql_TableRef, db_mysql_ColumnRef> oa_column(table, update_only_empty);
      ct::for_each<ct::Columns>(table, oa_column);

      ObjectAction<db_mysql_TableRef, db_mysql_TriggerRef> oa_trigger(table, update_only_empty);
      ct::for_each<ct::Triggers>(table, oa_trigger);

      IndexAction ia(table, update_only_empty, map);
      // std::for_each(table->indices().begin(), table->indices().end(), ia);
      ct::for_each<ct::Indices>(table, ia);

      FKAction fk_action(table, update_only_empty, map);
      ct::for_each<ct::ForeignKeys>(table, fk_action);
    }
  };

  struct SchemaAction : public ObjectAction<db_mysql_CatalogRef, db_mysql_SchemaRef> {
    CatalogMap& map;
    SchemaAction(db_mysql_CatalogRef ow, bool update_only_empty, CatalogMap& m)
      : ObjectAction<db_mysql_CatalogRef, db_mysql_SchemaRef>(ow, update_only_empty), map(m) {
    }

    void operator()(db_mysql_SchemaRef schema) {
      ObjectAction<db_mysql_CatalogRef, db_mysql_SchemaRef>::operator()(schema);

      TableAction table_action(schema, update_only_empty, map);
      ct::for_each<ct::Tables>(schema, table_action);

      ObjectAction<db_mysql_SchemaRef, db_mysql_ViewRef> oa_view(schema, update_only_empty);
      ct::for_each<ct::Views>(schema, oa_view);

      ObjectAction<db_mysql_SchemaRef, db_mysql_RoutineRef> oa_routine(schema, update_only_empty);
      ct::for_each<ct::Routines>(schema, oa_routine);
    }
  };
}

WBPLUGINDBMYSQLBE_PUBLIC_FUNC
void update_all_old_names(db_mysql_CatalogRef cat, bool update_only_empty, CatalogMap& map) {
  update_old_name(cat, update_only_empty);

  SchemaAction sa(cat, update_only_empty, map);
  ct::for_each<ct::Schemata>(cat, sa);
}

DbMySQLScriptSync::DbMySQLScriptSync()
  : DbMySQLValidationPage(), _alter_list(grt::Initialized), _alter_object_list(true) {
}

DbMySQLScriptSync::~DbMySQLScriptSync() {
  if (_mod_cat_copy.is_valid())
    _mod_cat_copy->reset_references();
}

db_mysql_CatalogRef DbMySQLScriptSync::get_model_catalog() {
  return db_mysql_CatalogRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog"));
}

void DbMySQLScriptSync::get_compared_catalogs(db_CatalogRef& left, db_CatalogRef& right) {
  left = _org_cat;
  right = _mod_cat_copy;
}

void DbMySQLScriptSync::set_option(const std::string& name, const std::string& value) {
  if (name.compare("InputFileName1") == 0)
    _input_filename1 = value;
  else if (name.compare("InputFileName2") == 0)
    _input_filename2 = value;
  else if (name.compare("OutputFileName") == 0)
    _output_filename = value;
}

void DbMySQLScriptSync::start_sync() {
  bec::GRTTask::Ref task = bec::GRTTask::create_task("SQL sync", bec::GRTManager::get()->get_dispatcher(),
                                                     std::bind(&DbMySQLScriptSync::sync_task, this, grt::StringRef()));

  scoped_connect(task->signal_finished(), std::bind(&DbMySQLScriptSync::sync_finished, this, std::placeholders::_1));
  bec::GRTManager::get()->get_dispatcher()->add_task(task);
}

void DbMySQLScriptSync::sync_finished(grt::ValueRef res) {
  logInfo("%s\n", grt::StringRef::cast_from(res).c_str());
}

#if 0
static void dump_alter_map(grt::DictRef alter_map)
{
  for(DictRef::const_iterator iterator= alter_map.begin(); iterator != alter_map.end(); iterator++)
  {
    std::string key= iterator->first;
    grt::ValueRef value= iterator->second;

    if (grt::StringListRef::can_wrap(value))
    {
      grt::StringListRef list= grt::StringListRef::cast_from(value);
      for (size_t listcount= list.count(), j= 0; j < listcount; j++)
      {
        std::cout << list.get(j).c_str() << std::endl;
      }
    }
    else if (grt::StringRef::can_wrap(value))
    {
      std::cout << grt::StringRef::cast_from(value).c_str() << std::endl;
    }
  }
}
#endif

// this function gets catalog from file or (if filename is empty) from the GRT tree
db_mysql_CatalogRef DbMySQLScriptSync::get_cat_from_file_or_tree(std::string filename, std::string& error_msg) {
  db_mysql_CatalogRef ref_cat = get_model_catalog();

  if (filename.empty()) {
    ref_cat->name("default");
    ref_cat->oldName("default");
    return ref_cat;
  }

  DbMySQLImpl* diffsql_module = grt::GRT::get()->find_native_module<DbMySQLImpl>("DbMySQL");

  if (diffsql_module == NULL) {
    error_msg.assign("Internal error. Not able to load 'MySQLModuleDbMySQL' module");
    return db_mysql_CatalogRef();
  }

  if (!ref_cat.is_valid()) {
    error_msg.assign("Internal error. Catalog is invalid");
    return db_mysql_CatalogRef();
  }

  workbench_physical_ModelRef pm = workbench_physical_ModelRef::cast_from(ref_cat->owner());

  db_mysql_CatalogRef cat(grt::Initialized);
  cat->version(pm->rdbms()->version());
  grt::replace_contents(cat->simpleDatatypes(), pm->rdbms()->simpleDatatypes());
  cat->name("default");
  cat->oldName("default");

  GError* file_error = NULL;
  char* sql_input_script = NULL;
  gsize sql_input_script_length = 0;

  if (!g_file_get_contents(filename.c_str(), &sql_input_script, &sql_input_script_length, &file_error)) {
    std::string file_error_msg("Error reading input file: ");
    file_error_msg.append(file_error->message);
    error_msg.assign(file_error_msg.c_str());
    return db_mysql_CatalogRef();
  }

  SqlFacade::Ref sql_parser = SqlFacade::instance_for_rdbms(pm->rdbms());
  sql_parser->parseSqlScriptString(cat, sql_input_script);
  g_free(sql_input_script);

  return cat;
}

ValueRef DbMySQLScriptSync::sync_task(grt::StringRef) {
  std::string err;

  db_mysql_CatalogRef mod_cat = get_cat_from_file_or_tree(std::string(), err);

  if (!err.empty())
    return StringRef(err);

  db_mysql_CatalogRef org_cat = get_cat_from_file_or_tree(_input_filename1, err);

  if (!err.empty())
    return StringRef(err);

  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt/rdbms/0"));

  db_mysql_CatalogRef org_cat_copy = db_mysql_CatalogRef::cast_from(grt::copy_object(org_cat));
  db_mysql_CatalogRef mod_cat_copy = db_mysql_CatalogRef::cast_from(grt::copy_object(mod_cat));

  apply_user_datatypes(org_cat_copy, rdbms);
  apply_user_datatypes(mod_cat_copy, rdbms);

  try {
    return generate_alter(org_cat, org_cat_copy, mod_cat_copy);
  } catch (std::exception& e) {
    return grt::StringRef(e.what());
  }
}

grt::StringRef DbMySQLScriptSync::generate_alter(db_mysql_CatalogRef org_cat, db_mysql_CatalogRef org_cat_copy,
                                                 db_mysql_CatalogRef mod_cat_copy) {
  DbMySQLImpl* diffsql_module = grt::GRT::get()->find_native_module<DbMySQLImpl>("DbMySQL");

  grt::DbObjectMatchAlterOmf omf;
  omf.dontdiff_mask = 3;
  grt::NormalizedComparer normalizer;
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> alter_change = diff_make(org_cat_copy, mod_cat_copy, &omf);

  // nothing changed
  if (!alter_change)
    return grt::StringRef("");

  grt::DictRef options(true);
  grt::StringListRef alter_list(grt::Initialized);
  options.set("OutputContainer", alter_list);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("KeepOrder", grt::IntegerRef(1));
  grt::ListRef<GrtNamedObject> alter_object_list(true);
  options.set("OutputObjectContainer", alter_object_list);
  options.set("SQL_MODE", bec::GRTManager::get()->get_app_option("SqlGenerator.Mysql:SQL_MODE"));

  diffsql_module->generateSQL(org_cat, options, alter_change);

  ssize_t res = diffsql_module->makeSQLSyncScript(org_cat, options, alter_list, alter_object_list);
  if (res != 0)
    throw std::runtime_error("SQL Script Export Module Returned Error");

  return grt::StringRef::cast_from(options.get("OutputScript"));
}

// Called once sync is finished, if there were no errors
// This saves oldNames and sqlDefinition for views in syncProfile
void DbMySQLScriptSync::save_sync_profile() {
  db_mysql_CatalogRef mod_cat = get_model_catalog();
  GrtObjectRef model_obj = mod_cat->owner();
  if (_sync_profile_name.is_valid() && model_obj.is_valid() && workbench_physical_ModelRef::can_wrap(model_obj)) {
    for (size_t i = 0; i < mod_cat->schemata().count(); i++) {
      db_SchemaRef schema(mod_cat->schemata()[i]);
      logInfo("Saving oldNames and other sync state info for %s::%s (catalog %s)\n", _sync_profile_name.c_str(),
              schema->name().c_str(), mod_cat.id().c_str());
      db_mgmt_SyncProfileRef profile =
        bec::get_sync_profile(workbench_physical_ModelRef::cast_from(model_obj), _sync_profile_name, schema->name());
      if (!profile.is_valid())
        profile = bec::create_sync_profile(workbench_physical_ModelRef::cast_from(model_obj), _sync_profile_name,
                                           schema->name());
      bec::update_sync_profile_from_schema(profile, schema);
    }
  }
}

// Called when starting sync, to load previously saved oldNames and other info
// for this specific connection
void DbMySQLScriptSync::restore_sync_profile(db_CatalogRef catalog) {
  GrtObjectRef model_obj = catalog->owner();
  if (_sync_profile_name.is_valid() && model_obj.is_valid() && workbench_physical_ModelRef::can_wrap(model_obj)) {
    for (size_t i = 0; i < catalog->schemata().count(); i++) {
      db_SchemaRef schema(catalog->schemata()[i]);
      db_mgmt_SyncProfileRef profile =
        bec::get_sync_profile(workbench_physical_ModelRef::cast_from(model_obj), _sync_profile_name, schema->name());
      if (profile.is_valid()) {
        logInfo("Restoring oldNames and other sync state info for %s::%s (catalog %s)\n", _sync_profile_name.c_str(),
                schema->name().c_str(), catalog.id().c_str());
        bec::update_schema_from_sync_profile(schema, profile);
      } else
        logInfo("No sync profile found for %s::%s\n", _sync_profile_name.c_str(), schema->name().c_str());
    }
  }
}

void DbMySQLScriptSync::restore_overriden_names() {
  db_mysql_CatalogRef mod_cat = get_model_catalog();
  for (size_t i = 0; i < mod_cat->schemata().count(); i++) {
    db_SchemaRef schema(mod_cat->schemata()[i]);
    std::string original_name = schema->customData().get_string("db.mysql.synchronize:originalName", schema->name());
    std::string original_old_name =
      schema->customData().get_string("db.mysql.synchronize:originalOldName", schema->oldName());
    schema->customData().remove("db.mysql.synchronize:originalName");
    schema->customData().remove("db.mysql.synchronize:originalOldName");
    schema->name(original_name);
    schema->oldName(original_old_name);
  }
}

std::shared_ptr<DiffTreeBE> DbMySQLScriptSync::init_diff_tree(const std::vector<std::string>& schemata,
                                                              const ValueRef& left, const ValueRef& right,
                                                              StringListRef SchemaSkipList, grt::DictRef options) {
  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt/rdbms/0"));
  std::string default_engine_name;
  grt::ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
  if (grt::StringRef::can_wrap(default_engine))
    default_engine_name = grt::StringRef::cast_from(default_engine);
  std::string err;

  schemata_list.assign(schemata.begin(), schemata.end());

  // 1. load db_Catalog-s
  db_mysql_CatalogRef left_catalog(db_mysql_CatalogRef::cast_from(left));
  restore_sync_profile(left_catalog);

  _mod_cat_copy = db_mysql_CatalogRef::cast_from(grt::copy_object(left_catalog));
  apply_user_datatypes(_mod_cat_copy, rdbms);
  bec::CatalogHelper::apply_defaults(_mod_cat_copy, default_engine_name);

  CatalogMap left_catalog_map;
  build_catalog_map(_mod_cat_copy, left_catalog_map);
  update_all_old_names(_mod_cat_copy, true, left_catalog_map);

  _org_cat = db_mysql_CatalogRef::cast_from(right);
  if (_org_cat.is_valid()) {
    apply_user_datatypes(_org_cat, rdbms);
    bec::CatalogHelper::apply_defaults(_org_cat, default_engine_name);

    CatalogMap right_catalog_map;
    build_catalog_map(_org_cat, right_catalog_map);
    update_all_old_names(_org_cat, true, right_catalog_map);
  }

  {
    SqlFacade* parser = SqlFacade::instance_for_rdbms_name("Mysql");
    // if the target schema does not have the same name as the original, make sure that the
    // target objects have references to the old schema name fixed in all code objects (triggers, views, SPs, functions)
    for (size_t i = 0; i < _mod_cat_copy->schemata().count(); i++) {
      db_SchemaRef schema(_mod_cat_copy->schemata()[i]);

      std::string orig_schema_name = schema->customData().get_string("db.mysql.synchronize:originalName", "");
      if (!orig_schema_name.empty() && schema->name() != orig_schema_name) {
        logInfo("Fix schema references of %s (from %s)\n", schema->name().c_str(), orig_schema_name.c_str());
        Sql_schema_rename::Ref renamer = parser->sqlSchemaRenamer();
        renamer->rename_schema_references(_mod_cat_copy, orig_schema_name, schema->name());
      }

      // remove excluded object types from the copy of the model catalog... the right catalog should already come
      // stripped from the source
      if (options.is_valid() && options.get_int("SkipTriggers")) {
        logInfo("Remove triggers from copy of model schema %s\n", schema->name().c_str());
        for (size_t t = 0; t < schema->tables().count(); t++) {
          schema->tables()[t]->triggers().remove_all();
        }
      }
      if (options.is_valid() && options.get_int("SkipRoutines")) {
        logInfo("Remove routines from copy of model schema %s\n", schema->name().c_str());
        schema->routines().remove_all();
        schema->routineGroups().remove_all();
      }
    }
  }

  // 2. diff with model

  for (size_t i = 0; i < SchemaSkipList.count(); i++) {
    StringRef schema_name = SchemaSkipList.get(i);
    if (_org_cat.is_valid())
      for (size_t schema_count = 0; schema_count < _org_cat->schemata().count(); schema_count++)
        while ((schema_count < _org_cat->schemata().count()) &&
               (_org_cat->schemata().get(schema_count)->name() == schema_name))
          _org_cat->schemata().remove(schema_count);

    for (size_t schema_count = 0; schema_count < _mod_cat_copy->schemata().count(); schema_count++)
      while ((schema_count < _mod_cat_copy->schemata().count()) &&
             (_mod_cat_copy->schemata().get(schema_count)->name() == schema_name))
        _mod_cat_copy->schemata().remove(schema_count);
  }

  grt::DbObjectMatchAlterOmf omf;
  omf.dontdiff_mask = 3;
  grt::DictRef db_opts = get_db_options();
  if (options.is_valid())
    db_opts.set("SkipRoutineDefiner", options.get("SkipRoutineDefiner"));
  else
    db_opts.set("SkipRoutineDefiner", grt::IntegerRef(0));

  grt::NormalizedComparer comparer(db_opts);
  comparer.init_omf(&omf);
  _alter_change = diff_make(_org_cat, _mod_cat_copy, &omf);

  DbMySQLImpl* diffsql_module = grt::GRT::get()->find_native_module<DbMySQLImpl>("DbMySQL");

  _alter_list.remove_all();
  _alter_object_list.remove_all();

  grt::DictRef genoptions(true);
  genoptions.set("DBSettings", get_db_options());
  genoptions.set("OutputContainer", _alter_list);
  genoptions.set("OutputObjectContainer", _alter_object_list);
  genoptions.set("UseFilteredLists", grt::IntegerRef(0));
  // enable this once the ALTER script generation code is able to properly generate USE statements
  // options.set("OmitSchemas", grt::IntegerRef(1));
  //  options.set("CaseSensitive", grt::IntegerRef(_case_sensitive));

  if (_alter_change && diffsql_module) {
    //    _alter_change->dump_log(0);
    diffsql_module->generateSQL(_org_cat, genoptions, _alter_change);
    // TODO: use this result in generate_diff_tree_report
  }

  // 3. build the tree
  return _diff_tree = std::shared_ptr<DiffTreeBE>(new ::DiffTreeBE(schemata, _mod_cat_copy, _org_cat, _alter_change));
}

std::string DbMySQLScriptSync::get_sql_for_object(GrtNamedObjectRef obj) {
  std::string result;
  for (size_t i = 0; i < _alter_list.count(); ++i)
    if (_alter_object_list.get(i) == obj) {
      result.append(_alter_list.get(i)).append("\n");
    }
  return result;
};

inline void save_id(const GrtObjectRef& obj, std::set<std::string>& map) {
  map.insert(obj->id());
};

std::string DbMySQLScriptSync::generate_diff_tree_script() {
  DbMySQLImpl* diffsql_module = grt::GRT::get()->find_native_module<DbMySQLImpl>("DbMySQL");
  if (diffsql_module == NULL)
    return NULL;

  std::vector<grt::ValueRef> vec;
  _diff_tree->get_object_list_for_script(vec);
  std::vector<std::string> schemata;
  std::vector<std::string> tables;
  std::vector<std::string> triggers;
  std::vector<std::string> views;
  std::vector<std::string> routines;

  for (std::vector<grt::ValueRef>::const_iterator e = vec.end(), it = vec.begin(); it != e; it++) {
    grt::ValueRef v = *it;
    if (!GrtNamedObjectRef::can_wrap(v))
      continue;

    std::string name(
      get_old_object_name_for_key(GrtNamedObjectRef::cast_from(v), get_db_options().get_int("CaseSensitive") != 0));

    if (db_mysql_SchemaRef::can_wrap(v)) {
      // db_mysql_SchemaRef schema= db_mysql_SchemaRef::cast_from(v);
      schemata.push_back(name);
    } else if (db_mysql_TableRef::can_wrap(v)) {
      // db_mysql_TableRef table= db_mysql_TableRef::cast_from(v);
      tables.push_back(name);
    } else if (db_mysql_ViewRef::can_wrap(v)) {
      // db_mysql_ViewRef view= db_mysql_ViewRef::cast_from(v);
      views.push_back(name);
    } else if (db_mysql_RoutineRef::can_wrap(v)) {
      // db_mysql_RoutineRef routine= db_mysql_RoutineRef::cast_from(v);
      routines.push_back(name);
    } else if (db_mysql_TriggerRef::can_wrap(v)) {
      // db_mysql_TriggerRef trigger= db_mysql_TriggerRef::cast_from(v);
      triggers.push_back(name);
    }
  }

  grt::DictRef options(true);

  merge_contents(options, get_options(), true);

  options.set("DBSettings", get_db_options());
  options.set("SchemaFilterList", convert_string_vector_to_grt_list(schemata));
  options.set("TableFilterList", convert_string_vector_to_grt_list(tables));
  options.set("ViewFilterList", convert_string_vector_to_grt_list(views));
  options.set("RoutineFilterList", convert_string_vector_to_grt_list(routines));
  options.set("TriggerFilterList", convert_string_vector_to_grt_list(triggers));

  options.set("KeepOrder", grt::IntegerRef(1));
  options.set("SQL_MODE", bec::GRTManager::get()->get_app_option("SqlGenerator.Mysql:SQL_MODE"));

  grt::StringListRef alter_list(grt::Initialized);
  grt::ListRef<GrtNamedObject> alter_object_list(true);
  options.set("OutputContainer", alter_list);
  options.set("OutputObjectContainer", alter_object_list);

  if (_alter_change) {
    diffsql_module->generateSQL(_org_cat, options, _alter_change);
  }

  ssize_t res = diffsql_module->makeSQLSyncScript(_mod_cat_copy, options, alter_list, alter_object_list);
  if (res != 0)
    return "";

  grt::StringRef script = grt::StringRef::cast_from(options.get("OutputScript"));

  return std::string(script.c_str());
}

std::string DbMySQLScriptSync::generate_diff_tree_report() {
  DbMySQLImpl* diffsql_module = grt::GRT::get()->find_native_module<DbMySQLImpl>("DbMySQL");

  if (diffsql_module == NULL)
    return NULL;

  std::vector<grt::ValueRef> vec;
  _diff_tree->get_object_list_for_script(vec);

  std::vector<std::string> schemata;
  std::vector<std::string> tables;
  std::vector<std::string> triggers;
  std::vector<std::string> views;
  std::vector<std::string> routines;

  for (std::vector<grt::ValueRef>::const_iterator e = vec.end(), it = vec.begin(); it != e; it++) {
    grt::ValueRef v = *it;
    if (!GrtNamedObjectRef::can_wrap(v))
      continue;

    std::string name(
      get_old_object_name_for_key(GrtNamedObjectRef::cast_from(v), get_db_options().get_int("CaseSensitive") != 0));

    if (db_mysql_SchemaRef::can_wrap(v)) {
      db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(v);
      // name.append(get_old_name_or_name(schema));
      schemata.push_back(name);
    } else if (db_mysql_TableRef::can_wrap(v)) {
      db_mysql_TableRef table = db_mysql_TableRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(table->owner()))).append(".").append(get_old_name_or_name(table));
      tables.push_back(name);
    } else if (db_mysql_ViewRef::can_wrap(v)) {
      db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(view->owner()))).append(".").append(get_old_name_or_name(view));
      views.push_back(name);
    } else if (db_mysql_RoutineRef::can_wrap(v)) {
      db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(routine->owner()))).append(".").append(get_old_name_or_name(routine));
      routines.push_back(name);
    } else if (db_mysql_TriggerRef::can_wrap(v)) {
      db_mysql_TriggerRef trigger = db_mysql_TriggerRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(trigger->owner()->owner()))).append(".").append(get_old_name_or_name(trigger));
      triggers.push_back(name);
    }
  }

  grt::DictRef options(true);
  options.set("SchemaFilterList", convert_string_vector_to_grt_list(schemata));
  options.set("TableFilterList", convert_string_vector_to_grt_list(tables));
  options.set("ViewFilterList", convert_string_vector_to_grt_list(views));
  options.set("RoutineFilterList", convert_string_vector_to_grt_list(routines));
  options.set("TriggerFilterList", convert_string_vector_to_grt_list(triggers));
  options.set("TemplateFile",
              grt::StringRef(bec::GRTManager::get()
                               ->get_data_file_path(
                                 "modules/data/db_mysql_catalog_reporting/Basic_Text.tpl/basic_text_report.txt.tpl")
                               .c_str()));

  grt::StringRef output_string(diffsql_module->generateReport(_org_cat, options, _alter_change));

  CatalogMap ca;
  update_all_old_names(get_model_catalog(), false, ca);

  return std::string(output_string.c_str());
}

class ChangesApplier {
  std::map<std::string, GrtObjectRef> mapping;
  // secondary_mapping is used to collect object mappings for fields/containers/paths that are marked dontfollow
  // these objects should be collected by the other non-dontfollow paths, but in case they're not, they'll be here
  std::map<std::string, GrtObjectRef> secondary_mapping;
  std::set<std::shared_ptr<DiffChange> > processed_changes;
  std::set<std::string> removed_objects;

  bool case_sensitive;

  // for faster checking of the type of an object
  grt::MetaClass* table_mc;
  grt::MetaClass* schema_mc;

public:
  ChangesApplier() : case_sensitive(true) {
    table_mc = grt::GRT::get()->get_metaclass("db.mysql.Table");
    schema_mc = grt::GRT::get()->get_metaclass("db.mysql.Schema");
  }

  void set_case_sensitive(bool flag) {
    case_sensitive = flag;
  }

  // Changes being applied from db side to model so everything goes backward
  // Added values being removed, removed being added and old values are restored

  void apply_change_to_model(const std::shared_ptr<DiffChange> change, GrtNamedObjectRef owner) {
    if (processed_changes.find(change) != processed_changes.end())
      return;
    processed_changes.insert(change);

    switch (change->get_change_type()) {
      case grt::ListItemModified: {
        const grt::ListItemModifiedChange* listchange = static_cast<const grt::ListItemModifiedChange*>(change.get());
        GrtObjectRef new_obj = GrtObjectRef::cast_from(listchange->get_new_value());
        GrtObjectRef obj = mapping[new_obj->id()];
        GrtObjectRef old_obj = GrtObjectRef::cast_from(listchange->get_old_value());
        const ChangeSet* subchanges =
          static_cast<const grt::MultiChange*>(listchange->get_subchange().get())->subchanges();
        for (grt::ChangeSet::const_iterator mce = subchanges->end(), mcit = subchanges->begin(); mcit != mce; ++mcit) {
          const grt::ObjectAttrModifiedChange* attr_change =
            static_cast<const grt::ObjectAttrModifiedChange*>(mcit->get());
          if (attr_change->get_subchange()->get_change_type() == SimpleValue) {
            std::string change_name = attr_change->get_attr_name();
            ValueRef newval =
              static_cast<const grt::SimpleValueChange*>(attr_change->get_subchange().get())->get_old_value();
            obj->set_member(change_name, newval);
          } else {
            const ChangeSet* changeset =
              dynamic_cast<grt::MultiChange*>(attr_change->get_subchange().get())->subchanges();
            std::for_each(changeset->begin(), changeset->end(),
                          std::bind(&ChangesApplier::apply_change_to_model, this, std::placeholders::_1,
                                    GrtNamedObjectRef::cast_from(obj)));
          }
        }
        build_obj_mapping(old_obj, obj);
      } break;

      case grt::ListItemRemoved: {
        const grt::ListItemRemovedChange* listchange = static_cast<const grt::ListItemRemovedChange*>(change.get());
        const grt::ObjectAttrModifiedChange* parent_change =
          dynamic_cast<const grt::ObjectAttrModifiedChange*>(change->parent()->parent());
        std::string attr_name = parent_change->get_attr_name();

        ValueRef val = listchange->get_value();
        if (GrtObjectRef::can_wrap(val)) {
          GrtObjectRef obj = GrtObjectRef::cast_from(val);
          grt::MetaClass* mc = owner->get_metaclass();
          bool is_owned = mc->get_member_info(attr_name)->owned_object;

          ObjectListRef list = ObjectListRef::cast_from(obj->owner()->get_member(attr_name));
          size_t pos = grt::find_object_index_in_list(list, obj->id());
          ObjectListRef owner_list = ObjectListRef::cast_from(owner->get_member(attr_name));
          // only duplicate object if the object is owned, otherwise just add the old reference, which will be updated
          // later in update_catalog()
          GrtObjectRef newobj = is_owned ? GrtObjectRef::cast_from(grt::copy_object(obj)) : obj;
          if (is_owned)
            newobj->owner(owner);
          size_t owner_pos = owner_list.count();
          while (pos < list.count()) {
            size_t check_pos = owner_list.get_index(mapping[list[pos]->id()]);
            if (check_pos != grt::BaseListRef::npos) {
              owner_pos = check_pos;
              break;
            }
            pos++;
          }

          // logInfo("Add new %s (%s / %s -> %s) to %s::%s (%s / %s)\n", newobj->class_name().c_str(),
          // newobj->name().c_str(), obj.id().c_str(), newobj.id().c_str(), owner->class_name().c_str(),
          // attr_name.c_str(), owner->name().c_str(),
          //         owner->id().c_str());
          owner_list.ginsert(newobj, owner_pos);

          if (is_owned)
            build_obj_mapping(obj, newobj, true);
        } else {
          BaseListRef owner_list = BaseListRef::cast_from(owner->get_member(attr_name));
          owner_list.ginsert(val);
        }
      } break;
      case grt::ListItemAdded: {
        const grt::ListItemAddedChange* listchange = static_cast<const grt::ListItemAddedChange*>(change.get());
        const grt::ObjectAttrModifiedChange* parent_change =
          dynamic_cast<const grt::ObjectAttrModifiedChange*>(change->parent()->parent());
        std::string attr_name = parent_change->get_attr_name();
        ValueRef val = listchange->get_value();
        if (GrtObjectRef::can_wrap(val)) {
          GrtObjectRef obj = GrtObjectRef::cast_from(val);
          GrtObjectRef model_obj = mapping[obj->id()];
          if (!model_obj.is_valid())
            break;
          ObjectListRef owner_list = ObjectListRef::cast_from(owner->get_member(attr_name));
          iterate_object(model_obj, std::bind(save_id, std::placeholders::_1, std::ref(removed_objects)));
          // GCC doesn' like this :(
          /*
                            iterate_object(model_obj,
                            std::bind(&std::set<std::string>::insert, boost::ref(removed_objects),
                            std::bind(&GrtObjectRef::RefType::id, std::bind(&GrtObjectRef::content,_1))));
          */
          owner_list.remove_value(model_obj);
        } else {
          BaseListRef owner_list = BaseListRef::cast_from(owner->get_member(attr_name));
          owner_list.remove(owner_list.get_index(val));
        }
      } break;
      case grt::ListItemOrderChanged: {
        const grt::ListItemOrderChange* listchange = static_cast<const grt::ListItemOrderChange*>(change.get());
        const grt::ObjectAttrModifiedChange* parent_change =
          dynamic_cast<const grt::ObjectAttrModifiedChange*>(change->parent()->parent());
        std::string attr_name = parent_change->get_attr_name();
        ValueRef val = listchange->get_old_value();

        if (GrtObjectRef::can_wrap(val)) {
          GrtObjectRef obj = GrtObjectRef::cast_from(val);
          ObjectListRef list = ObjectListRef::cast_from(obj->owner()->get_member(attr_name));
          size_t new_pos = grt::find_object_index_in_list(list, obj->id());
          GrtObjectRef model_obj = mapping[obj->owner()->id()];
          ObjectListRef owner_list = ObjectListRef::cast_from(model_obj->get_member(attr_name));
          size_t old_pos = grt::find_object_index_in_list(owner_list, mapping[obj->id()]->id());
          owner_list.reorder(old_pos, new_pos);
          if (listchange->get_subchange())
            apply_change_to_model(listchange->get_subchange(), owner);
        } else {
          ValueRef prev = listchange->get_prev_item();
          BaseListRef owner_list = BaseListRef::cast_from(owner->get_member(attr_name));
          size_t pos = owner_list.get_index(prev);
          if (pos == grt::BaseListRef::npos)
            pos = 0;
          else
            pos++;
          owner_list.ginsert(val, pos);
        }
      } break;
      default:
        logError("Unhandled change!\n");
        break;
    }
  };

  void consolidate_mapping() {
    for (std::map<std::string, GrtObjectRef>::const_iterator iter = secondary_mapping.begin();
         iter != secondary_mapping.end(); ++iter) {
      if (mapping.find(iter->first) == mapping.end()) {
        logDebug3("%s is not in primary mapping\n", iter->first.c_str());
        mapping[iter->first] = iter->second;
      }
    }
  }

  void apply_node_to_model(const DiffNode* node) {
    GrtNamedObjectRef changeobj =
      node->get_model_part().is_valid_object() ? node->get_model_part().get_object() : node->get_db_part().get_object();
    if (node->get_change() && (node->get_application_direction() == DiffNode::ApplyToModel)) {
      apply_change_to_model(node->get_change(), GrtNamedObjectRef::cast_from(mapping[changeobj->owner()->id()]));
      return; // There is no need go deeper inside nodes as all subchanges will be processed with parent's change
    }

    std::for_each(node->get_children_begin(), node->get_children_end(),
                  std::bind(&ChangesApplier::apply_node_to_model, this, std::placeholders::_1));
  }

  bool compare_names(const GrtObjectRef& obj1, const GrtObjectRef& obj2) {
    // if the DB server is case-insensitive, then tables and schemas should be compared case insensitively
    if (obj1->get_metaclass() == table_mc || obj1->get_metaclass() == schema_mc)
      return base::same_string(obj1->name(), obj2->name(), case_sensitive);

    // everything else is case insensitive str compare
    return base::same_string(obj1->name(), obj2->name(), false);
  }

  bool build_obj_mapping(const GrtObjectRef& obj1, const GrtObjectRef& obj2, bool overwrite = false) {
    // std::cout<<obj1->name().c_str()<<std::endl;
    if (!compare_names(obj1, obj2))
      return false;
    if (!overwrite && mapping.find(obj1->id()) != mapping.end() && mapping[obj1->id()].is_valid())
      return false;
    if (obj1->id() == obj2->id())
      return false;

    mapping[obj1->id()] = obj2;
    MetaClass* meta = obj1.get_metaclass();
    while (meta != 0) {
      for (MetaClass::MemberList::const_iterator iter = meta->get_members_partial().begin();
           iter != meta->get_members_partial().end(); ++iter) {
        if (iter->second.overrides)
          continue;

        std::string name = iter->second.name;
        if (name == "owner")
          continue;

        std::string attr = meta->get_member_attribute(name, "dontdiff");
        const int dontdiff = attr.size() && (base::atoi<int>(attr, 0) & 1);

        if (dontdiff)
          continue;

        const bool dontfollow =
          !iter->second.owned_object; // && (name != "flags") && (name != "columns")&& (name != "foreignKeys");

        ValueRef v1 = obj1.get_member(name);
        ValueRef v2 = obj2.get_member(name);

        if (!v1.is_valid() || !v2.is_valid())
          continue;

        Type type = v1.type();
        switch (type) {
          case IntegerType:
          case DoubleType:
          case StringType:
            break;
          case ListType: {
            if (!grt::BaseListRef::can_wrap(v1) || !grt::BaseListRef::can_wrap(v2))
              break;
            grt::BaseListRef list1 = grt::BaseListRef::cast_from(v1);
            grt::BaseListRef list2 = grt::BaseListRef::cast_from(v2);
            if ((list1.content_type() != ObjectType) || (list2.content_type() != ObjectType))
              break;
            for (size_t i = 0; i < list1.count(); ++i) {
              if (!GrtObjectRef::can_wrap(list1[i]))
                continue;

              GrtObjectRef inner_obj1 = GrtObjectRef::cast_from(list1[i]);
              for (size_t list2_idx = 0; list2_idx < list2.count(); ++list2_idx) {
                GrtObjectRef inner_obj2 = GrtObjectRef::cast_from(list2[list2_idx]);
                if (compare_names(inner_obj2, inner_obj1)) {
                  if (dontfollow) {
                    if (inner_obj1 != inner_obj2)
                      secondary_mapping[inner_obj1.id()] = inner_obj2;
                  } else
                    build_obj_mapping(inner_obj1, inner_obj2);
                  break;
                }
              }
            }
          } break;
          case DictType: {
            DictRef dict1 = DictRef::cast_from(v1);
            DictRef dict2 = DictRef::cast_from(v2);
            for (internal::Dict::const_iterator iter = dict1.begin(); iter != dict1.end(); ++iter) {
              if (!GrtObjectRef::can_wrap(iter->second))
                continue;
              GrtObjectRef inner_obj1 = GrtObjectRef::cast_from(iter->second);
              GrtObjectRef inner_obj2;
              for (internal::Dict::const_iterator iter2 = dict2.begin(); iter != dict2.end(); ++iter) {
                if (!GrtObjectRef::can_wrap(iter2->second))
                  continue;
                GrtObjectRef dictobj = GrtObjectRef::cast_from(iter2->second);
                if (compare_names(dictobj, inner_obj1)) {
                  inner_obj2 = dictobj;
                  break;
                }
              }
              if (!inner_obj2.is_valid())
                break;
              if (dontfollow) {
                if (inner_obj1 != inner_obj2)
                  secondary_mapping[inner_obj1.id()] = inner_obj2;
              } else
                build_obj_mapping(inner_obj1, inner_obj2);
            }
          } break;
          case ObjectType: {
            GrtObjectRef inner_obj1 = GrtObjectRef::cast_from(v1);
            GrtObjectRef inner_obj2 = GrtObjectRef::cast_from(v2);
            if (dontfollow) {
              if (inner_obj1 != inner_obj2)
                secondary_mapping[inner_obj1.id()] = inner_obj2;
            } else
              build_obj_mapping(inner_obj1, inner_obj2);
            break;
          }
          default:
            break;
        }
      }
      meta = meta->parent();
    }
    return true;
  }

  void update_catalog(db_mysql_CatalogRef cat) {
    for (size_t i = 0; i < cat->schemata().count(); i++) {
      db_mysql_SchemaRef schema = cat->schemata().get(i);
      for (size_t j = 0; j < schema->tables().count(); j++) {
        db_mysql_TableRef table = schema->tables().get(j);
        std::list<size_t> dead_keys;

        // go through all foreign keys and update references that point to the temporary DB catalog
        // to point to the model catalog
        for (size_t k = 0; k < table->foreignKeys().count(); k++) {
          db_mysql_ForeignKeyRef fk = table->foreignKeys().get(k);

          if (!fk->referencedTable().is_valid()) {
            logError("FK %s from table %s is invalid and has no referencedTable set\n", fk->name().c_str(),
                     table->name().c_str());
            grt::GRT::get()->send_error(
              base::strfmt("ForeignKey %s from table %s is invalid and has no referencedTable set", fk->name().c_str(),
                           table->name().c_str()));
            continue;
          }

          if (removed_objects.find(fk->referencedTable()->id()) != removed_objects.end()) {
            // push_front, so that forward iteration will delete objects with biggest indexes first
            dead_keys.push_front(k);
            continue;
          }

          db_mysql_TableRef new_table = db_mysql_TableRef::cast_from(mapping[fk->referencedTable()->id()]);
          if (new_table.is_valid())
            fk->referencedTable(new_table);

          /* validation
          if (fk->referencedTable().is_valid())
          {
            bool ok = false;
            for (size_t xx = 0; xx < schema->tables().count(); xx++)
            {
              if (schema->tables()[xx]->id() == fk->referencedTable()->id())
                ok = true;
            }
            if (!ok)
              logError("The table %s (%s) referenced from %s.%s doesn't exist (newtable = %s)!\n",
          fk->referencedTable()->id().c_str(),
                        fk->referencedTable()->name().c_str(), fk->owner()->name().c_str(), fk->name().c_str(),
                        new_table.is_valid() ? new_table->id().c_str() : "???");
          }*/

          for (size_t l = 0; l < fk->columns().count(); l++) {
            db_ColumnRef old_col = fk->columns().get(l);
            if (removed_objects.find(old_col->id()) != removed_objects.end()) {
              // push_front, so that forward iteration will delete objects with biggest indexes first
              dead_keys.push_front(k);
              continue;
            }

            db_ColumnRef new_col = db_ColumnRef::cast_from(mapping[old_col->id()]);
            if (new_col.is_valid())
              fk->columns().set(l, new_col);
          }

          for (size_t l = 0; l < fk->referencedColumns().count(); l++) {
            db_ColumnRef old_col = fk->referencedColumns().get(l);
            if (removed_objects.find(old_col->id()) != removed_objects.end()) {
              // push_front, so that forward iteration will delete objects with biggest indexes first
              dead_keys.push_front(k);
              continue;
            }

            db_ColumnRef new_col = db_ColumnRef::cast_from(mapping[old_col->id()]);
            if (new_col.is_valid())
              fk->referencedColumns().set(l, new_col);
          }
        } // handle FKs
        for_each(dead_keys.begin(), dead_keys.end(),
                 std::bind(&grt::ListRef<db_ForeignKey>::remove, table->foreignKeys(), std::placeholders::_1));

        // same thing for indexes
        for (size_t k = 0; k < table->indices().count(); k++) {
          db_mysql_IndexRef idx = table->indices().get(k);

          for (size_t l = 0; l < idx->columns().count(); l++) {
            db_ColumnRef old_col = idx->columns()[l]->referencedColumn();

            db_ColumnRef new_col = db_ColumnRef::cast_from(mapping[old_col->id()]);
            if (new_col.is_valid())
              idx->columns()[l]->referencedColumn(new_col);
          }
        } // handle indexes
      }
    }
  }
};

void DbMySQLScriptSync::apply_changes_to_model() {
  grt::AutoUndo undo;
  NodeId rootnodeid = _diff_tree->get_root();
  DiffNode* rootnode = _diff_tree->get_node_with_id(rootnodeid);
  db_mysql_CatalogRef mod_cat = get_model_catalog();
  db_mysql_CatalogRef diff_mod_cat = db_mysql_CatalogRef::cast_from(rootnode->get_model_part().get_object());
  db_mysql_CatalogRef diff_db_cat = db_mysql_CatalogRef::cast_from(rootnode->get_db_part().get_object());

  ChangesApplier applier;

  applier.set_case_sensitive(get_db_options().get_int("CaseSensitive", 1) != 1);

  applier.build_obj_mapping(diff_mod_cat, mod_cat);
  if (diff_db_cat.is_valid())
    applier.build_obj_mapping(diff_db_cat, mod_cat);
  applier.consolidate_mapping();
  // Update changes to the model
  applier.apply_node_to_model(rootnode);
  // Scan model updating references from old objects to the newly created objects
  applier.update_catalog(mod_cat);

  // validate_tree_structure(mod_cat);

  undo.end(_("Apply Changes from DB to Model"));
}

std::string DbMySQLScriptSync::get_col_name(const size_t col_id) {
  switch (col_id) {
    case 0:
      return "Model";
      break;
    case 1:
      return "Update";
      break;
    case 2:
      return "Source";
      break;
  };
  return "No Column Name Defined";
};
